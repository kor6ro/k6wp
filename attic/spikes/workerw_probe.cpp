// spikes/workerw_probe.cpp
// Throwaway Win32 probe for k6wp-v1 Todo 3 (NOT part of the CMake build).
// Validates WorkerW desktop-injection strategies on THIS machine.
//
// Strategies:
//   A (classic, Win10-era): FindWindowW("Progman") -> SendMessageTimeoutW(0x052C)
//     -> EnumWindows -> top-level window whose direct child is SHELLDLL_DefView
//     (on Win10 that window IS the WorkerW hosting the desktop icons).
//   B (Win11 24H2): SHELLDLL_DefView now lives INSIDE Progman. After 0x052C a new
//     EMPTY WorkerW is spawned as a sibling of Progman -> attach there.
//   Fallback: SetParent(testHwnd, hProgman) directly.
//
// Usage:
//   workerw_probe.exe [--wallpaper-mode=auto|workerw|progman] [--hold-ms=N]
//                      [--probe-all-workerws]
//     auto    (default) try A, then B, then fallback; exit 0 if any succeeded
//     workerw force WorkerW strategies (A then B); exit 0 if either succeeded
//     progman force Progman fallback; exit 0 if it succeeded
//     --hold-ms=N          keep the test window visible for N ms (default 500)
//     --probe-all-workerws additionally attach to every empty WorkerW found
//
// The probe creates a 200x150 solid-color WS_POPUP test window, SetParent's it to
// the target (cross-process SetParent requires the child to be created as a
// top-level WS_POPUP first, else error 5), reports the result, then destroys the
// window before exiting. Nothing is left running.

#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00  // Windows 10+ (SPI_GETCLIENTANIMATIONS needs >= 0x0600)
#endif
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#ifndef SPI_GETCLIENTANIMATIONS
#define SPI_GETCLIENTANIMATIONS 0x104E  // "Animate controls and elements inside windows"
#endif

namespace {

constexpr UINT WM_SPAWN_WORKERW = 0x052C;
constexpr wchar_t kProgmanClass[] = L"Progman";
constexpr wchar_t kDefViewClass[] = L"SHELLDLL_DefView";
constexpr wchar_t kWorkerWClass[] = L"WorkerW";
constexpr wchar_t kProbeClass[] = L"K6WPProbeWnd";

struct WorkerWInfo {
    HWND hwnd = nullptr;
    bool hasChildren = false;
    HWND above = nullptr;  // GW_HWNDPREV (window in front)
    HWND below = nullptr;  // GW_HWNDNEXT (window behind)
};

struct DesktopInfo {
    HWND progman = nullptr;
    HWND defView = nullptr;
    HWND defViewHost = nullptr;  // top-level window whose direct child is defView
    std::vector<WorkerWInfo> workerWs;
    size_t workerWCountBefore = 0;
    size_t progmanWorkerWChildrenBefore = 0;
    DWORD_PTR spawnResult = 0;
    DWORD spawnError = 0;
    bool clientAnimations = false;
};

bool IsClass(HWND hwnd, const wchar_t* cls);

size_t CountProgmanWorkerWChildren(HWND progman) {
    if (!progman) return 0;
    size_t n = 0;
    HWND child = GetWindow(progman, GW_CHILD);
    while (child) {
        if (IsClass(child, kWorkerWClass)) ++n;
        child = GetWindow(child, GW_HWNDNEXT);
    }
    return n;
}

std::wstring ClassOf(HWND hwnd) {
    if (!hwnd) return L"(null)";
    wchar_t buf[256];
    if (GetClassNameW(hwnd, buf, 256) == 0) return L"(?)";
    return buf;
}

std::wstring TitleOf(HWND hwnd) {
    if (!hwnd) return L"(null)";
    wchar_t buf[256];
    if (GetWindowTextW(hwnd, buf, 256) == 0) return L"";
    return buf;
}

bool IsClass(HWND hwnd, const wchar_t* cls) {
    return ClassOf(hwnd) == cls;
}

struct EnumCtx {
    DesktopInfo* info;
    bool collectWorkerWs;
};

BOOL CALLBACK EnumTopLevelProc(HWND hwnd, LPARAM lParam) {
    auto* ctx = reinterpret_cast<EnumCtx*>(lParam);
    DesktopInfo& info = *ctx->info;

    if (ctx->collectWorkerWs && IsClass(hwnd, kWorkerWClass)) {
        WorkerWInfo w;
        w.hwnd = hwnd;
        w.hasChildren = (GetWindow(hwnd, GW_CHILD) != nullptr);
        w.above = GetWindow(hwnd, GW_HWNDPREV);
        w.below = GetWindow(hwnd, GW_HWNDNEXT);
        info.workerWs.push_back(w);
    }

    HWND child = FindWindowExW(hwnd, nullptr, kDefViewClass, nullptr);
    if (child != nullptr) {
        info.defView = child;
        info.defViewHost = hwnd;
    }
    return TRUE;
}

DesktopInfo FindDesktopWindows() {
    DesktopInfo info;

    // "Animate controls and elements inside windows" -> SPI_GETCLIENTANIMATIONS.
    // When OFF, 0x052C may not spawn a WorkerW at all.
    BOOL anim = FALSE;
    if (SystemParametersInfoW(SPI_GETCLIENTANIMATIONS, 0, &anim, 0)) {
        info.clientAnimations = (anim != FALSE);
    }

    info.progman = FindWindowW(kProgmanClass, nullptr);

    // Count WorkerW windows BEFORE 0x052C.
    {
        EnumCtx ctx{&info, true};
        info.workerWs.clear();
        EnumWindows(EnumTopLevelProc, reinterpret_cast<LPARAM>(&ctx));
        info.workerWCountBefore = info.workerWs.size();
        info.workerWs.clear();
    }
    info.progmanWorkerWChildrenBefore = CountProgmanWorkerWChildren(info.progman);

    if (info.progman) {
        info.spawnResult = 0;
        info.spawnError = 0;
        if (!SendMessageTimeoutW(info.progman, WM_SPAWN_WORKERW, 0, 0,
                                 SMTO_NORMAL, 1000, &info.spawnResult)) {
            info.spawnError = GetLastError();
        }
    }

    // Enumerate AFTER 0x052C (collects WorkerW list + DefView host).
    {
        EnumCtx ctx{&info, true};
        EnumWindows(EnumTopLevelProc, reinterpret_cast<LPARAM>(&ctx));
    }

    return info;
}

// Strategy B: find the empty WorkerW spawned by 0x052C.
// Prefer the WorkerW directly behind Progman in z-order (GW_HWNDNEXT); else any
// empty WorkerW.
HWND FindEmptyWorkerW(const DesktopInfo& info) {
    if (!info.progman) return nullptr;
    HWND below = GetWindow(info.progman, GW_HWNDNEXT);
    if (below && IsClass(below, kWorkerWClass) && GetWindow(below, GW_CHILD) == nullptr) {
        return below;
    }
    HWND above = GetWindow(info.progman, GW_HWNDPREV);
    if (above && IsClass(above, kWorkerWClass) && GetWindow(above, GW_CHILD) == nullptr) {
        return above;
    }
    for (const auto& w : info.workerWs) {
        if (!w.hasChildren) return w.hwnd;
    }
    return nullptr;
}

// --- test window -----------------------------------------------------------

// Paint diagnostics: how many paint/erase messages the test window actually
// receives. If these stay 0 while attached to Progman, the shell never asks
// the window to draw (rendering is suppressed, not occluded).
static volatile LONG g_paintCount = 0;
static volatile LONG g_eraseCount = 0;

LRESULT CALLBACK ProbeWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_PAINT) {
        InterlockedIncrement(&g_paintCount);
    }
    if (msg == WM_ERASEBKGND) {
        InterlockedIncrement(&g_eraseCount);
    }
    // DefWindowProc paints the class background brush on WM_ERASEBKGND and
    // handles WM_PRINTCLIENT (used by PrintWindow) with the same brush.
    return DefWindowProcW(hwnd, msg, wp, lp);
}

bool RegisterProbeClass(HINSTANCE hInstance) {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = ProbeWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = kProbeClass;
    wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);  // visible solid color
    return RegisterClassW(&wc) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

struct AttachResult {
    bool ok = false;
    DWORD error = 0;
    HWND parent = nullptr;
    HWND above = nullptr;  // window in front of the test window after attach
    bool visible = false;
};

// Print the children of `parent` in z-order (front to back), marking `mark`.
void DumpChildZOrder(HWND parent, HWND mark) {
    if (!parent) return;
    printf("    child z-order of 0x%p (front -> back):\n", parent);
    HWND child = GetWindow(parent, GW_CHILD);
    int idx = 0;
    while (child) {
        printf("      [%d] 0x%p class=%-16S%s\n", idx++, child, ClassOf(child).c_str(),
               child == mark ? "  <-- test window" : "");
        child = GetWindow(child, GW_HWNDNEXT);
    }
    if (idx == 0) printf("      (no children)\n");
}

AttachResult AttachTestWindow(HWND target, HINSTANCE hInstance, DWORD holdMs,
                              HWND insertAfter = HWND_BOTTOM, bool childStyle = false,
                              bool layered = false) {
    AttachResult r;
    if (!target) {
        r.error = ERROR_INVALID_HANDLE;
        return r;
    }
    InterlockedExchange(&g_paintCount, 0);
    InterlockedExchange(&g_eraseCount, 0);
    // Create as top-level WS_POPUP first. Cross-process SetParent requires this;
    // creating directly with a foreign parent fails with error 5 (access denied).
    // On Win11 24H2+ the injected window MUST be WS_EX_LAYERED + fully opaque
    // (Progman is WS_EX_NOREDIRECTIONBITMAP; DefView is a layered child), else
    // DWM never composites it into the desktop.
    const DWORD exStyle = layered ? (WS_EX_LAYERED | WS_EX_NOACTIVATE) : 0;
    HWND wnd = CreateWindowExW(exStyle, kProbeClass, L"K6WP Probe", WS_POPUP,
                               0, 0, 200, 150, nullptr, nullptr, hInstance, nullptr);
    if (!wnd) {
        r.error = GetLastError();
        return r;
    }
    if (layered) {
        SetLayeredWindowAttributes(wnd, 0, 255, LWA_ALPHA);
    }
    if (SetParent(wnd, target) == nullptr) {
        r.error = GetLastError();
        DestroyWindow(wnd);
        return r;
    }
    if (layered) {
        // SetParent can strip WS_EX_LAYERED from the window; re-apply it and
        // the alpha so DWM composites the child into the desktop.
        LONG_PTR ex = GetWindowLongPtrW(wnd, GWL_EXSTYLE);
        SetWindowLongPtrW(wnd, GWL_EXSTYLE, ex | WS_EX_LAYERED);
        SetLayeredWindowAttributes(wnd, 0, 255, LWA_ALPHA);
    }
    if (childStyle) {
        LONG_PTR style = GetWindowLongPtrW(wnd, GWL_STYLE);
        style = (style & ~WS_POPUP) | WS_CHILD;
        SetWindowLongPtrW(wnd, GWL_STYLE, style);
    }
    r.parent = target;
    ShowWindow(wnd, SW_SHOW);
    // Pump messages briefly so the window paints (and the shell settles).
    MSG msg;
    const DWORD start = GetTickCount();
    while (GetTickCount() - start < holdMs) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        Sleep(10);
    }
    r.above = GetWindow(wnd, GW_HWNDPREV);
    r.visible = IsWindowVisible(wnd) != FALSE;
    r.ok = true;
    printf("    paint messages: WM_PAINT=%ld WM_ERASEBKGND=%ld\n",
           (long)g_paintCount, (long)g_eraseCount);
    const LONG_PTR exAfter = GetWindowLongPtrW(wnd, GWL_EXSTYLE);
    printf("    ex-style after attach: 0x%llX (WS_EX_LAYERED=%s)\n",
           (unsigned long long)exAfter,
           (exAfter & WS_EX_LAYERED) ? "YES" : "NO");
    RECT rc = {};
    if (GetWindowRect(wnd, &rc)) {
        printf("    test window screen rect: (%ld,%ld)-(%ld,%ld) %ldx%ld\n", rc.left,
               rc.top, rc.right, rc.bottom, rc.right - rc.left, rc.bottom - rc.top);
        const POINT center = {(rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2};
        HWND hit = WindowFromPoint(center);
        printf("    WindowFromPoint(center) -> 0x%p (%S) %s\n", hit,
               ClassOf(hit).c_str(), hit == wnd ? "<-- test window" : "");
        if (hit && hit != wnd) {
            RECT hrc = {};
            if (GetWindowRect(hit, &hrc)) {
                printf("    hit window rect: (%ld,%ld)-(%ld,%ld) %ldx%ld\n", hrc.left,
                       hrc.top, hrc.right, hrc.bottom, hrc.right - hrc.left,
                       hrc.bottom - hrc.top);
            }
        }
        HDC dc = GetDC(wnd);
        if (dc) {
            HDC mem = CreateCompatibleDC(dc);
            HBITMAP bmp = CreateCompatibleBitmap(dc, rc.right - rc.left, rc.bottom - rc.top);
            HGDIOBJ old = SelectObject(mem, bmp);
            const BOOL pw = PrintWindow(wnd, mem, PW_CLIENTONLY);
            COLORREF px = GetPixel(mem, 10, 10);
            printf("    PrintWindow=%s pixel(10,10)=RGB(%lu,%lu,%lu)\n",
                   pw ? "OK" : "FAIL", GetRValue(px), GetGValue(px), GetBValue(px));
            SelectObject(mem, old);
            DeleteObject(bmp);
            DeleteDC(mem);
            ReleaseDC(wnd, dc);
        }
    }
    RECT prc = {};
    if (GetWindowRect(target, &prc)) {
        printf("    target screen rect: (%ld,%ld)-(%ld,%ld) %ldx%ld\n", prc.left, prc.top,
               prc.right, prc.bottom, prc.right - prc.left, prc.bottom - prc.top);
    }
    printf("    z-order before push:\n");
    DumpChildZOrder(target, wnd);
    // Push the window down in the parent's z-order so it sits BEHIND the desktop
    // icons (DefView). insertAfter=HWND_BOTTOM goes below everything (may end up
    // behind the wallpaper layer on 24H2); insertAfter=defView goes directly
    // below the icons, above the wallpaper layer.
    SetWindowPos(wnd, insertAfter, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    printf("    z-order after push:\n");
    DumpChildZOrder(target, wnd);
    r.above = GetWindow(wnd, GW_HWNDPREV);
    DestroyWindow(wnd);
    return r;
}

// --- OS version ------------------------------------------------------------

std::wstring OsBuildString() {
    // RtlGetVersion returns the real build number even without a manifest.
    typedef LONG(WINAPI* RtlGetVersionFn)(PRTL_OSVERSIONINFOW);
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    std::wstring result = L"unknown";
    if (ntdll) {
        auto fn = reinterpret_cast<RtlGetVersionFn>(GetProcAddress(ntdll, "RtlGetVersion"));
        if (fn) {
            RTL_OSVERSIONINFOW vi = {};
            vi.dwOSVersionInfoSize = sizeof(vi);
            if (fn(&vi) == 0) {
                wchar_t buf[128];
                swprintf_s(buf, L"Windows %lu.%lu (build %lu)",
                           vi.dwMajorVersion, vi.dwMinorVersion, vi.dwBuildNumber);
                result = buf;
            }
        }
    }
    return result;
}

// --- mode ------------------------------------------------------------------

enum class Mode { Auto, WorkerW, Progman };

struct Options {
    Mode mode = Mode::Auto;
    DWORD holdMs = 500;
    bool probeAllWorkerWs = false;
    bool targetProgmanChild = false;
    bool insertBelowDefView = false;
    bool targetTopLevel = false;
    bool childStyle = false;
    bool targetDefView = false;
    bool layered = false;
};

Options ParseOptions(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--wallpaper-mode=", 17) == 0) {
            const char* v = argv[i] + 17;
            if (strcmp(v, "workerw") == 0) o.mode = Mode::WorkerW;
            else if (strcmp(v, "progman") == 0) o.mode = Mode::Progman;
            else if (strcmp(v, "auto") == 0) o.mode = Mode::Auto;
            else fprintf(stderr, "unknown --wallpaper-mode '%s' (auto|workerw|progman)\n", v);
        } else if (strncmp(argv[i], "--hold-ms=", 10) == 0) {
            o.holdMs = (DWORD)atol(argv[i] + 10);
        } else if (strcmp(argv[i], "--probe-all-workerws") == 0) {
            o.probeAllWorkerWs = true;
        } else if (strcmp(argv[i], "--target=progman-child") == 0) {
            o.targetProgmanChild = true;
        } else if (strcmp(argv[i], "--insert-below-defview") == 0) {
            o.insertBelowDefView = true;
        } else if (strcmp(argv[i], "--target=top-level") == 0) {
            o.targetTopLevel = true;
        } else if (strcmp(argv[i], "--child-style") == 0) {
            o.childStyle = true;
        } else if (strcmp(argv[i], "--target=defview") == 0) {
            o.targetDefView = true;
        } else if (strcmp(argv[i], "--layered") == 0) {
            o.layered = true;
        }
    }
    return o;
}

void PrintHwndLine(const char* label, HWND hwnd) {
    if (hwnd) {
        printf("%-28s 0x%p  class=%-16S title=\"%S\"\n", label, hwnd,
               ClassOf(hwnd).c_str(), TitleOf(hwnd).c_str());
    } else {
        printf("%-28s (null)\n", label);
    }
}

}  // namespace

int main(int argc, char** argv) {
    const Options opt = ParseOptions(argc, argv);
    HINSTANCE hInstance = GetModuleHandleW(nullptr);
    if (!RegisterProbeClass(hInstance)) {
        fprintf(stderr, "FATAL: RegisterClassW failed, error=%lu\n", GetLastError());
        return 2;
    }

    printf("=== K6WP WorkerW Probe ===\n");
    printf("OS: %S\n", OsBuildString().c_str());
    printf("Mode: %s\n",
           opt.mode == Mode::Auto ? "auto" : (opt.mode == Mode::WorkerW ? "workerw" : "progman"));
    printf("Hold: %lu ms\n", opt.holdMs);

    const DesktopInfo info = FindDesktopWindows();

    printf("Client animations (\"Animate controls...\"): %s\n",
           info.clientAnimations ? "ON" : "OFF");
    printf("\n--- Desktop window detection ---\n");
    PrintHwndLine("Progman", info.progman);
    PrintHwndLine("SHELLDLL_DefView", info.defView);
    PrintHwndLine("DefView host", info.defViewHost);
    printf("Progman visible: %s, DefView visible: %s\n",
           (info.progman && IsWindowVisible(info.progman)) ? "YES" : "NO",
           (info.defView && IsWindowVisible(info.defView)) ? "YES" : "NO");
    printf("DefView inside Progman (Win11 24H2 layout): %s\n",
           (info.defViewHost && info.defViewHost == info.progman) ? "YES" : "NO");
    printf("0x052C send: %s (result=0x%llX, error=%lu)\n",
           info.spawnError == 0 ? "OK" : "FAIL", (unsigned long long)info.spawnResult,
           info.spawnError);
    printf("WorkerW count: before 0x052C=%zu, after=%zu (spawned=%lld)\n",
           info.workerWCountBefore, info.workerWs.size(),
           (long long)info.workerWs.size() - (long long)info.workerWCountBefore);
    printf("Progman WorkerW children: before 0x052C=%zu, after=%zu (spawned=%lld)\n",
           info.progmanWorkerWChildrenBefore, CountProgmanWorkerWChildren(info.progman),
           (long long)CountProgmanWorkerWChildren(info.progman) -
               (long long)info.progmanWorkerWChildrenBefore);

    printf("\nWorkerW windows (after 0x052C):\n");
    for (size_t i = 0; i < info.workerWs.size(); ++i) {
        const auto& w = info.workerWs[i];
        printf("  [%zu] 0x%p children=%s above=0x%p(%S) below=0x%p(%S)%s\n", i, w.hwnd,
               w.hasChildren ? "YES" : "NO", w.above, ClassOf(w.above).c_str(), w.below,
               ClassOf(w.below).c_str(),
               (!w.hasChildren && info.progman &&
                (w.hwnd == GetWindow(info.progman, GW_HWNDNEXT) ||
                 w.hwnd == GetWindow(info.progman, GW_HWNDPREV)))
                   ? "  <-- empty, z-order neighbor of Progman (Strategy B candidate)"
                   : "");
    }

    // Strategy targets.
    HWND strategyATarget = nullptr;
    if (info.defViewHost && info.defViewHost != info.progman) {
        strategyATarget = info.defViewHost;  // classic: WorkerW containing DefView
    }
    HWND strategyBTarget = FindEmptyWorkerW(info);
    HWND fallbackTarget = info.progman;

    printf("\nStrategy targets:\n");
    PrintHwndLine("A (WorkerW w/ DefView)", strategyATarget);
    PrintHwndLine("B (empty WorkerW)", strategyBTarget);
    PrintHwndLine("Fallback (Progman)", fallbackTarget);

    // Diagnostic: Progman may itself contain a WorkerW child (24H2 wallpaper
    // layer). Dump it so Todo 9 knows whether it is a usable target.
    if (info.progman) {
        HWND child = GetWindow(info.progman, GW_CHILD);
        while (child) {
            if (IsClass(child, kWorkerWClass)) {
                printf("\nProgman WorkerW child: 0x%p children=%s visible=%s\n", child,
                       GetWindow(child, GW_CHILD) ? "YES" : "NO",
                       IsWindowVisible(child) ? "YES" : "NO");
                DumpChildZOrder(child, nullptr);
            }
            child = GetWindow(child, GW_HWNDNEXT);
        }
    }

    if (opt.targetTopLevel) {
        printf("\n--- Plain top-level window (sanity check) ---\n");
        HWND wnd = CreateWindowExW(0, kProbeClass, L"K6WP Probe", WS_POPUP,
                                   0, 0, 200, 150, nullptr, nullptr, hInstance, nullptr);
        if (!wnd) {
            printf("  CreateWindowExW failed, error=%lu\n", GetLastError());
            return 1;
        }
        ShowWindow(wnd, SW_SHOW);
        MSG msg;
        const DWORD start = GetTickCount();
        while (GetTickCount() - start < opt.holdMs) {
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            Sleep(10);
        }
        RECT rc = {};
        GetWindowRect(wnd, &rc);
        printf("  top-level window rect: (%ld,%ld)-(%ld,%ld), visible=%s\n", rc.left,
               rc.top, rc.right, rc.bottom, IsWindowVisible(wnd) ? "YES" : "NO");
        DestroyWindow(wnd);
        return 0;
    }

    if (opt.targetProgmanChild) {
        printf("\n--- Progman WorkerW child only ---\n");
        HWND child = nullptr;
        if (info.progman) {
            HWND c = GetWindow(info.progman, GW_CHILD);
            while (c) {
                if (IsClass(c, kWorkerWClass)) { child = c; break; }
                c = GetWindow(c, GW_HWNDNEXT);
            }
        }
        if (!child) {
            printf("  no WorkerW child of Progman found\n");
            return 1;
        }
        const AttachResult r = AttachTestWindow(child, hInstance, opt.holdMs, HWND_BOTTOM,
                                               opt.childStyle, opt.layered);
        printf("  Progman-WorkerW-child 0x%p -> SetParent %s (error=%lu), visible=%s, above=0x%p (%S)\n",
               child, r.ok ? "OK" : "FAIL", r.error, r.visible ? "YES" : "NO",
               r.above, ClassOf(r.above).c_str());
        return r.ok ? 0 : 1;
    }

    if (opt.targetDefView) {
        printf("\n--- DefView attach (below icons) ---\n");
        if (!info.defView) {
            printf("  no DefView found\n");
            return 1;
        }
        const AttachResult r =
            AttachTestWindow(info.defView, hInstance, opt.holdMs, HWND_BOTTOM, opt.childStyle,
                             opt.layered);
        printf("  DefView 0x%p -> SetParent %s (error=%lu), visible=%s, above=0x%p (%S)\n",
               info.defView, r.ok ? "OK" : "FAIL", r.error, r.visible ? "YES" : "NO",
               r.above, ClassOf(r.above).c_str());
        return r.ok ? 0 : 1;
    }

    // Attach test window per mode.
    bool anyOk = false;
    auto runAttach = [&](const char* name, HWND target) -> bool {
        if (!target) {
            printf("  %-12s target=(null) -> SKIPPED\n", name);
            return false;
        }
        HWND insertAfter = HWND_BOTTOM;
        if (opt.insertBelowDefView && target == info.progman && info.defView) {
            insertAfter = info.defView;
        }
        const AttachResult r =
            AttachTestWindow(target, hInstance, opt.holdMs, insertAfter, opt.childStyle,
                             opt.layered);
        printf("  %-12s target=0x%p -> SetParent %s (error=%lu), visible=%s, above=0x%p (%S)\n",
               name, target, r.ok ? "OK" : "FAIL", r.error, r.visible ? "YES" : "NO",
               r.above, ClassOf(r.above).c_str());
        return r.ok;
    };

    if (opt.mode == Mode::WorkerW || opt.mode == Mode::Auto) {
        printf("\n--- WorkerW strategies ---\n");
        const bool a = runAttach("Strategy A", strategyATarget);
        const bool b = runAttach("Strategy B", strategyBTarget);
        anyOk = a || b;
    }
    if (opt.mode == Mode::Progman || opt.mode == Mode::Auto) {
        printf("\n--- Progman fallback ---\n");
        const bool f = runAttach("Fallback", fallbackTarget);
        anyOk = anyOk || f;
    }

    if (opt.probeAllWorkerWs) {
        printf("\n--- Probe every empty WorkerW ---\n");
        for (const auto& w : info.workerWs) {
            if (w.hasChildren) continue;
            const AttachResult r = AttachTestWindow(w.hwnd, hInstance, opt.holdMs, HWND_BOTTOM,
                                                    opt.childStyle, opt.layered);
            printf("  WorkerW 0x%p -> SetParent %s (error=%lu), visible=%s, above=0x%p (%S)\n",
                   w.hwnd, r.ok ? "OK" : "FAIL", r.error, r.visible ? "YES" : "NO",
                   r.above, ClassOf(r.above).c_str());
            anyOk = anyOk || r.ok;
        }
        // Also probe the WorkerW child of Progman (native 24H2 wallpaper layer).
        if (info.progman) {
            HWND child = GetWindow(info.progman, GW_CHILD);
            while (child) {
                if (IsClass(child, kWorkerWClass)) {
                    const AttachResult r = AttachTestWindow(child, hInstance, opt.holdMs,
                                                            HWND_BOTTOM, opt.childStyle,
                                                            opt.layered);
                    printf("  Progman-WorkerW-child 0x%p -> SetParent %s (error=%lu), visible=%s, above=0x%p (%S)\n",
                           child, r.ok ? "OK" : "FAIL", r.error, r.visible ? "YES" : "NO",
                           r.above, ClassOf(r.above).c_str());
                    anyOk = anyOk || r.ok;
                }
                child = GetWindow(child, GW_HWNDNEXT);
            }
        }
    }

    printf("\nRESULT: %s\n",
           anyOk ? "SUCCESS (at least one attach OK)" : "FAILURE (no attach succeeded)");
    return anyOk ? 0 : 1;
}