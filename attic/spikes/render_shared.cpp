// spikes/render_shared.cpp — P3.1 spike (SESI A, standalone, NOT shipped).
// Proof of concept: 1 mpv decode (vo=libmpv + mpv_render_context, OpenGL)
// fan-out to 2 windows via shared GL textures + per-consumer present threads.
// PLUS baseline mode: 2 mpv instances (vo=gpu + wid) for CPU/thread/RAM delta.
//
// Build (manual, NOT in CMake — Sesi A dilarang sentuh engine/):
//   "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
//   cl.exe /EHsc /std:c++17 /W4 spikes\render_shared.cpp /I vendor\libmpv\include ^
//     vendor\libmpv\lib\mpv.lib opengl32.lib gdi32.lib user32.lib psapi.lib dxgi.lib ^
//     /Fe:build\spikes\render_shared.exe
//   copy vendor\libmpv\bin\libmpv-2.dll build\spikes\ (already there)
//
// Usage:
//   render_shared.exe <video> [--mode=shared|dual] [--hold-ms=N]
//     [--fit=contain|cover|stretch|center] [--win-w=N --win-h=N]
//     [--tex-w=N --tex-h=N] [--verify]
//   Defaults: mode=shared hold-ms=8000 fit=cover win 640x360 tex 1920x1080.
//   Exit 0 = both windows presented frames (shared: decode=1).
//
// Architecture (shared mode):
//   master hidden window + GL ctx renders via mpv_render_context_render()
//   into FBO->texture. Two visible windows each own a GL ctx created from
//   THE SAME pixel format, shared via wglShareLists(master, consumer).
//   Texture is shareable; FBO is NOT — consumers only sample the texture
//   and draw a textured quad to their backbuffer. Sync: glFenceSync in
//   master after render, glClientWaitSync in consumers before sampling.
//   One present thread per consumer (a GL ctx is current in exactly one
//   thread), wglSwapIntervalEXT(1).

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00
#include <windows.h>
#include <GL/gl.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <dxgi.h>

#include <mpv/client.h>
#include <mpv/render.h>
#include <mpv/render_gl.h>

#include <atomic>
#include <chrono>
#include <clocale>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// ---- GL enums missing from old gl.h (values per GL spec) ----
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif
#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_SYNC_GPU_COMMANDS_COMPLETE
#define GL_SYNC_GPU_COMMANDS_COMPLETE 0x9117
#endif
#ifndef GL_SYNC_FLUSH_COMMANDS_BIT
#define GL_SYNC_FLUSH_COMMANDS_BIT 0x00000001
#endif
#ifndef GL_ALREADY_SIGNALED
#define GL_ALREADY_SIGNALED 0x911A
#endif
#ifndef GL_TIMEOUT_EXPIRED
#define GL_TIMEOUT_EXPIRED 0x911B
#endif
#ifndef GL_CONDITION_SATISFIED
#define GL_CONDITION_SATISFIED 0x911C
#endif
#ifndef GL_WAIT_FAILED
#define GL_WAIT_FAILED 0x911D
#endif
#ifndef GL_TIMEOUT_IGNORED
#define GL_TIMEOUT_IGNORED 0xFFFFFFFFFFFFFFFFull
#endif
typedef void* GLsync_p;
typedef uint64_t GLuint64_p;

// ---- WGL/GL function pointers ----
typedef HGLRC(WINAPI* PFN_wglCreateContext)(HDC);
typedef BOOL(WINAPI* PFN_wglMakeCurrent)(HDC, HGLRC);
typedef BOOL(WINAPI* PFN_wglShareLists)(HGLRC, HGLRC);
typedef BOOL(WINAPI* PFN_wglDeleteContext)(HGLRC);
typedef PROC(WINAPI* PFN_wglGetProcAddress)(LPCSTR);
typedef BOOL(WINAPI* PFN_wglSwapIntervalEXT)(int);

typedef void(APIENTRY* PFN_glGenTextures)(GLsizei, GLuint*);
typedef void(APIENTRY* PFN_glBindTexture)(GLenum, GLuint);
typedef void(APIENTRY* PFN_glTexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei,
                                         GLint, GLenum, GLenum, const void*);
typedef void(APIENTRY* PFN_glTexParameteri)(GLenum, GLenum, GLint);
typedef void(APIENTRY* PFN_glDeleteTextures)(GLsizei, const GLuint*);
typedef void(APIENTRY* PFN_glGenFramebuffers)(GLsizei, GLuint*);
typedef void(APIENTRY* PFN_glBindFramebuffer)(GLenum, GLuint);
typedef void(APIENTRY* PFN_glFramebufferTexture2D)(GLenum, GLenum, GLenum,
                                                   GLuint, GLint);
typedef GLenum(APIENTRY* PFN_glCheckFramebufferStatus)(GLenum);
typedef void(APIENTRY* PFN_glDeleteFramebuffers)(GLsizei, const GLuint*);
typedef GLsync_p(APIENTRY* PFN_glFenceSync)(GLenum, GLbitfield);
typedef void(APIENTRY* PFN_glDeleteSync)(GLsync_p);
typedef GLenum(APIENTRY* PFN_glClientWaitSync)(GLsync_p, GLbitfield, GLuint64_p);

struct GLProcs {
    PFN_glGenTextures GenTextures = nullptr;
    PFN_glBindTexture BindTexture = nullptr;
    PFN_glTexImage2D TexImage2D = nullptr;
    PFN_glTexParameteri TexParameteri = nullptr;
    PFN_glDeleteTextures DeleteTextures = nullptr;
    PFN_glGenFramebuffers GenFramebuffers = nullptr;
    PFN_glBindFramebuffer BindFramebuffer = nullptr;
    PFN_glFramebufferTexture2D FramebufferTexture2D = nullptr;
    PFN_glCheckFramebufferStatus CheckFramebufferStatus = nullptr;
    PFN_glDeleteFramebuffers DeleteFramebuffers = nullptr;
    PFN_glFenceSync FenceSync = nullptr;
    PFN_glDeleteSync DeleteSync = nullptr;
    PFN_glClientWaitSync ClientWaitSync = nullptr;
    PFN_wglSwapIntervalEXT SwapIntervalEXT = nullptr;
    bool have_fbo = false;
    bool have_sync = false;
};

static HMODULE g_opengl32 = nullptr;
static void* SpikeGetProc(const char* name) {
    // Spec: wglGetProcAddress first, fallback GetProcAddress(opengl32).
    PROC p = ::wglGetProcAddress(name);
    if (p) return reinterpret_cast<void*>(p);
    if (g_opengl32) {
        FARPROC q = ::GetProcAddress(g_opengl32, name);
        if (q) return reinterpret_cast<void*>(q);
    }
    return nullptr;
}

static void* MpvGetProc(void* /*ctx*/, const char* name) {
    return SpikeGetProc(name);
}

static bool LoadGLProcs(GLProcs* out) {
    g_opengl32 = ::GetModuleHandleW(L"opengl32.dll");
    if (!g_opengl32) g_opengl32 = ::LoadLibraryW(L"opengl32.dll");
#define LOAD(field, type, name)                                   \
    do {                                                          \
        out->field = reinterpret_cast<type>(SpikeGetProc(name));  \
    } while (0)
    LOAD(GenTextures, PFN_glGenTextures, "glGenTextures");
    LOAD(BindTexture, PFN_glBindTexture, "glBindTexture");
    LOAD(TexImage2D, PFN_glTexImage2D, "glTexImage2D");
    LOAD(TexParameteri, PFN_glTexParameteri, "glTexParameteri");
    LOAD(DeleteTextures, PFN_glDeleteTextures, "glDeleteTextures");
    LOAD(GenFramebuffers, PFN_glGenFramebuffers, "glGenFramebuffers");
    LOAD(BindFramebuffer, PFN_glBindFramebuffer, "glBindFramebuffer");
    LOAD(FramebufferTexture2D, PFN_glFramebufferTexture2D,
         "glFramebufferTexture2D");
    LOAD(CheckFramebufferStatus, PFN_glCheckFramebufferStatus,
         "glCheckFramebufferStatus");
    LOAD(DeleteFramebuffers, PFN_glDeleteFramebuffers, "glDeleteFramebuffers");
    LOAD(FenceSync, PFN_glFenceSync, "glFenceSync");
    LOAD(DeleteSync, PFN_glDeleteSync, "glDeleteSync");
    LOAD(ClientWaitSync, PFN_glClientWaitSync, "glClientWaitSync");
    LOAD(SwapIntervalEXT, PFN_wglSwapIntervalEXT, "wglSwapIntervalEXT");
#undef LOAD
    out->have_fbo = out->GenFramebuffers && out->BindFramebuffer &&
                    out->FramebufferTexture2D && out->CheckFramebufferStatus;
    out->have_sync = out->FenceSync && out->ClientWaitSync && out->DeleteSync;
    return out->GenTextures && out->BindTexture && out->TexImage2D;
}

// ---- small helpers ----
static void Log(const char* fmt, ...) {
    char buf[2048];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    printf("%s\n", buf);
    fflush(stdout);
}

static unsigned long long ThreadCountNow() {
    const DWORD pid = GetCurrentProcessId();
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    unsigned long long n = 0;
    THREADENTRY32 te{};
    te.dwSize = sizeof(te);
    if (Thread32First(snap, &te)) {
        do {
            if (te.th32OwnerProcessID == pid) ++n;
        } while (Thread32Next(snap, &te));
    }
    CloseHandle(snap);
    return n;
}

static void MemSample(unsigned long long* ws_mb, unsigned long long* priv_mb) {
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    pmc.cb = sizeof(pmc);
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                             sizeof(pmc))) {
        *ws_mb = pmc.WorkingSetSize / (1024 * 1024);
        *priv_mb = pmc.PrivateUsage / (1024 * 1024);
    } else {
        *ws_mb = 0;
        *priv_mb = 0;
    }
}

static void LogAdapters() {
    // DXGI adapter census: single vs hybrid rig classification for P3.6 scope.
    IDXGIFactory1* factory = nullptr;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1),
                                    reinterpret_cast<void**>(&factory));
    if (FAILED(hr) || !factory) {
        Log("[gpu] DXGI factory failed hr=0x%08lx", (unsigned long)hr);
        return;
    }
    int n = 0;
    for (int i = 0;; ++i) {
        IDXGIAdapter1* ad = nullptr;
        if (factory->EnumAdapters1(i, &ad) == DXGI_ERROR_NOT_FOUND) break;
        DXGI_ADAPTER_DESC1 d{};
        if (ad && SUCCEEDED(ad->GetDesc1(&d))) {
            char desc[128] = {};
            WideCharToMultiByte(CP_UTF8, 0, d.Description, -1, desc,
                                (int)sizeof(desc), nullptr, nullptr);
            const bool integrated =
                d.DedicatedVideoMemory < d.SharedSystemMemory;
            Log("[gpu] adapter%d desc='%s' dedicated=%lluMB shared=%lluMB %s",
                i, desc, (unsigned long long)d.DedicatedVideoMemory /
                             (1024 * 1024),
                (unsigned long long)d.SharedSystemMemory / (1024 * 1024),
                integrated ? "integrated" : "discrete");
            ++n;
        }
        if (ad) ad->Release();
    }
    Log("[gpu] adapter_count=%d class=%s", n,
        n >= 2 ? "HYBRID" : "SINGLE");
    factory->Release();
}

static void LogAffinityPriority() {
    DWORD_PTR proc_mask = 0, sys_mask = 0;
    if (GetProcessAffinityMask(GetCurrentProcess(), &proc_mask, &sys_mask)) {
        Log("[cpu] affinity proc_mask=0x%llx sys_mask=0x%llx",
            (unsigned long long)proc_mask, (unsigned long long)sys_mask);
    }
    Log("[cpu] priority_class=%lu", (unsigned long)GetPriorityClass(GetCurrentProcess()));
}

// ---- windows ----
static const wchar_t kCls[] = L"K6WPSharedSpike";
static LRESULT CALLBACK SpikeWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_DESTROY) return 0;
    return DefWindowProcW(h, m, w, l);
}

static bool EnsureClass() {
    static bool done = false;
    if (done) return true;
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = SpikeWndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    wc.lpszClassName = kCls;
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return false;
    done = true;
    return true;
}

static HWND MakeWin(const wchar_t* title, int x, int y, int w, int h,
                    bool show) {
    HWND hwnd = CreateWindowExW(0, kCls, title, WS_OVERLAPPEDWINDOW, x, y, w,
                                h, nullptr, nullptr, GetModuleHandleW(nullptr),
                                nullptr);
    if (hwnd && show) {
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
    }
    return hwnd;
}

static void PumpMessages() {
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

// ---- fit-mode UV/vertex computation (spike parity with MpvRenderer) ----
// Modes: contain=letterbox full UV + letterboxed verts; cover=fullscreen
// verts + cropped UV; stretch=fullscreen + full UV; center=1:1 centered.
struct Quad {
    float x0, y0, x1, y1;  // NDC verts
    float u0, v0, u1, v1;  // UV
};

static Quad ComputeQuad(const std::string& fit, int vw, int vh, int ww,
                        int wh) {
    Quad q{-1, -1, 1, 1, 0, 0, 1, 1};
    if (vw <= 0 || vh <= 0 || ww <= 0 || wh <= 0) return q;
    const double va = (double)vw / (double)vh;
    const double wa = (double)ww / (double)wh;
    if (fit == "stretch") {
        return q;
    } else if (fit == "contain" || fit == "fit") {
        // Letterbox: shrink verts to preserve aspect.
        if (va > wa) {
            // Video wider: full width, centered height band.
            const float hfrac = (float)(wa / va);
            q.y0 = -hfrac;
            q.y1 = hfrac;
        } else {
            const float wfrac = (float)(va / wa);
            q.x0 = -wfrac;
            q.x1 = wfrac;
        }
        return q;
    } else if (fit == "cover" || fit == "fill") {
        // Fullscreen verts, crop UV centered.
        if (va > wa) {
            // Video wider than window: crop left/right.
            const float ufrac = (float)(wa / va);
            q.u0 = (1.0f - ufrac) * 0.5f;
            q.u1 = 1.0f - q.u0;
        } else {
            const float vfrac = (float)(va / wa);
            q.v0 = (1.0f - vfrac) * 0.5f;
            q.v1 = 1.0f - q.v0;
        }
        return q;
    } else if (fit == "center") {
        // 1:1 pixels centered; crop if video larger than window.
        const double dw = vw < ww ? vw : ww;
        const double dh = vh < wh ? vh : wh;
        q.x0 = (float)(-dw / ww);
        q.x1 = (float)(dw / ww);
        q.y0 = (float)(-dh / wh);
        q.y1 = (float)(dh / wh);
        const float ufrac = (float)(dw / vw);
        const float vfrac = (float)(dh / vh);
        q.u0 = (1.0f - ufrac) * 0.5f;
        q.u1 = 1.0f - q.u0;
        q.v0 = (1.0f - vfrac) * 0.5f;
        q.v1 = 1.0f - q.v0;
        return q;
    }
    return q;  // unknown -> stretch (safe fullscreen)
}

static void DrawQuadImmediate(const Quad& q) {
    ::glBegin(GL_QUADS);
    ::glTexCoord2f(q.u0, q.v0);
    ::glVertex2f(q.x0, q.y0);
    ::glTexCoord2f(q.u1, q.v0);
    ::glVertex2f(q.x1, q.y0);
    ::glTexCoord2f(q.u1, q.v1);
    ::glVertex2f(q.x1, q.y1);
    ::glTexCoord2f(q.u0, q.v1);
    ::glVertex2f(q.x0, q.y1);
    ::glEnd();
}

// ---- shared state between master render thread and present threads ----
struct SharedFrame {
    std::atomic<unsigned long long> frame_id{0};
    std::mutex fence_mutex;
    GLsync_p fence = nullptr;  // latest master fence (leaked per-frame, freed at teardown)
    std::vector<GLsync_p> all_fences;
    GLuint texture = 0;
    std::atomic<bool> running{true};
    std::atomic<bool> failed{false};
    std::string fail_reason;
};

// Update-callback flag (mpv render thread -> master loop thread).
static std::atomic<bool> g_mpv_update{false};
static void MpvUpdateCb(void* /*ctx*/) { g_mpv_update.store(true); }

struct ConsumerArgs {
    HWND hwnd = nullptr;
    HDC hdc = nullptr;
    HGLRC rc = nullptr;
    GLProcs* procs = nullptr;
    SharedFrame* shared = nullptr;
    std::string fit;
    int video_w = 1920;
    int video_h = 1080;
    int index = 0;
    std::atomic<unsigned long long> presented{0};
    std::atomic<bool> saw_frame{false};
    double mean_brightness = 0.0;  // set once via readback (main thread joins first)
};

static void ConsumerThread(ConsumerArgs* a) {
    if (!::wglMakeCurrent(a->hdc, a->rc)) {
        a->shared->failed.store(true);
        return;
    }
    if (a->procs->SwapIntervalEXT) a->procs->SwapIntervalEXT(1);
    ::glEnable(GL_TEXTURE_2D);
    ::glDisable(GL_DEPTH_TEST);
    unsigned long long last = 0;
    while (a->shared->running.load()) {
        const unsigned long long fid = a->shared->frame_id.load();
        if (fid == last) {
            ::Sleep(1);
            continue;
        }
        last = fid;
        // GPU sync: wait for the master fence before sampling the texture.
        GLsync_p fence = nullptr;
        {
            std::lock_guard<std::mutex> lk(a->shared->fence_mutex);
            fence = a->shared->fence;
        }
        if (fence && a->procs->ClientWaitSync) {
            const GLenum r = a->procs->ClientWaitSync(
                fence, GL_SYNC_FLUSH_COMMANDS_BIT, 100 * 1000 * 1000ull);
            (void)r;  // TIMEOUT_EXPIRED also proceeds (avoid stall); spike logs counts
        }
        RECT cr{};
        GetClientRect(a->hwnd, &cr);
        const int ww = cr.right - cr.left > 0 ? cr.right - cr.left : 1;
        const int wh = cr.bottom - cr.top > 0 ? cr.bottom - cr.top : 1;
        ::glViewport(0, 0, ww, wh);
        ::glMatrixMode(GL_PROJECTION);
        ::glLoadIdentity();
        ::glMatrixMode(GL_MODELVIEW);
        ::glLoadIdentity();
        ::glClearColor(0, 0, 0, 1);
        ::glClear(GL_COLOR_BUFFER_BIT);
        a->procs->BindTexture(GL_TEXTURE_2D, a->shared->texture);
        const Quad q = ComputeQuad(a->fit, a->video_w, a->video_h, ww, wh);
        DrawQuadImmediate(q);
        ::glFlush();
        if (!::SwapBuffers(a->hdc)) {
            a->shared->failed.store(true);
            break;
        }
        a->presented.fetch_add(1);
        a->saw_frame.store(true);
    }
    ::wglMakeCurrent(nullptr, nullptr);
}

// Read back the shared texture in the master context (objective non-black proof).
static double MasterTextureBrightness(GLProcs* p, GLuint tex, int w, int h) {
    if (!p->BindTexture) return 0.0;
    std::vector<unsigned char> px((size_t)w * (size_t)h * 4);
    p->BindTexture(GL_TEXTURE_2D, tex);
    ::glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    unsigned long long sum = 0;
    const size_t step = 4 * 7;  // sample every 7th pixel for speed
    size_t n = 0;
    for (size_t i = 0; i + 2 < px.size(); i += step) {
        sum += px[i] + px[i + 1] + px[i + 2];
        ++n;
    }
    if (!n) return 0.0;
    return (double)sum / (double)n / 3.0;
}

// ---- mpv helpers ----
static void SetOpt(mpv_handle* mpv, const char* k, const char* v) {
    const int rc = mpv_set_option_string(mpv, k, v);
    if (rc < 0) Log("[mpv] option '%s=%s' refused (%s)", k, v, mpv_error_string(rc));
}

static std::string HwdecCurrent(mpv_handle* mpv) {
    char* s = nullptr;
    if (mpv_get_property(mpv, "hwdec-current", MPV_FORMAT_STRING, &s) >= 0 && s) {
        std::string out = s;
        mpv_free(s);
        return out;
    }
    return "unknown";
}

// ================= SHARED MODE =================
static int RunShared(const std::string& video, int hold_ms,
                     const std::string& fit, int win_w, int win_h, int tex_w,
                     int tex_h, bool verify) {
    Log("[spike] mode=shared video='%s' hold=%d fit=%s win=%dx%d tex=%dx%d",
        video.c_str(), hold_ms, fit.c_str(), win_w, win_h, tex_w, tex_h);
    LogAdapters();
    LogAffinityPriority();
    if (!EnsureClass()) {
        Log("[fail] RegisterClass failed");
        return 1;
    }

    // Windows: 1 hidden master + 2 visible side-by-side (1-monitor QA).
    HWND master_wnd = MakeWin(L"spike-master (hidden)", 0, 0, 64, 64, false);
    HWND win1 =
        MakeWin(L"spike-shared-A", 60, 60, win_w, win_h, true);
    HWND win2 =
        MakeWin(L"spike-shared-B", 60 + win_w + 40, 60, win_w, win_h, true);
    if (!master_wnd || !win1 || !win2) {
        Log("[fail] CreateWindow failed");
        return 1;
    }
    HDC mdc = GetDC(master_wnd);
    HDC d1 = GetDC(win1);
    HDC d2 = GetDC(win2);

    // ONE pixel format for all DCs (sharing requirement).
    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;
    const int pf = ChoosePixelFormat(mdc, &pfd);
    if (!pf) {
        Log("[fail] ChoosePixelFormat failed");
        return 1;
    }
    Log("[gl] shared pixelformat index=%d", pf);
    if (!SetPixelFormat(mdc, pf, &pfd) || !SetPixelFormat(d1, pf, &pfd) ||
        !SetPixelFormat(d2, pf, &pfd)) {
        Log("[fail] SetPixelFormat (same index) failed err=%lu", GetLastError());
        return 1;
    }

    // Contexts: master first, consumers shared from it. Share BEFORE any
    // consumer is current; master not current during wglShareLists.
    ::wglMakeCurrent(nullptr, nullptr);
    HGLRC mrc = ::wglCreateContext(mdc);
    HGLRC c1 = ::wglCreateContext(d1);
    HGLRC c2 = ::wglCreateContext(d2);
    if (!mrc || !c1 || !c2) {
        Log("[fail] wglCreateContext failed");
        return 1;
    }
    if (!::wglShareLists(mrc, c1)) {
        Log("[fail] wglShareLists(master->c1) failed err=%lu. "
            "OPT-B candidate: D3D11 shared texture + interop.",
            GetLastError());
        return 1;
    }
    if (!::wglShareLists(mrc, c2)) {
        Log("[fail] wglShareLists(master->c2) failed err=%lu. "
            "OPT-B candidate: D3D11 shared texture + interop.",
            GetLastError());
        return 1;
    }
    Log("[gl] wglShareLists ok (texture shareable; FBO intentionally NOT shared)");

    if (!::wglMakeCurrent(mdc, mrc)) {
        Log("[fail] master wglMakeCurrent failed");
        return 1;
    }
    GLProcs gl{};
    if (!LoadGLProcs(&gl)) {
        Log("[fail] LoadGLProcs failed (core tex funcs missing)");
        return 1;
    }
    Log("[gl] procs fbo=%d sync=%d swapinterval=%d (get_proc=wglGetProcAddress->opengl32)",
        gl.have_fbo ? 1 : 0, gl.have_sync ? 1 : 0,
        gl.SwapIntervalEXT ? 1 : 0);
    if (!gl.have_fbo) {
        Log("[fail] FBO procs missing — cannot render offscreen. OPT-B candidate.");
        return 1;
    }

    // Shared texture + master-private FBO (FBO names are NOT shared).
    GLuint tex = 0, fbo = 0;
    gl.GenTextures(1, &tex);
    gl.BindTexture(GL_TEXTURE_2D, tex);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl.TexParameteri(GL_TEXTURE_2D, 0x2802 /*WRAP_S*/, 0x812F /*CLAMP_TO_EDGE*/);
    gl.TexParameteri(GL_TEXTURE_2D, 0x2803 /*WRAP_T*/, 0x812F /*CLAMP_TO_EDGE*/);
    gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex_w, tex_h, 0, GL_RGBA,
                  GL_UNSIGNED_BYTE, nullptr);
    gl.GenFramebuffers(1, &fbo);
    gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_TEXTURE_2D, tex, 0);
    const GLenum fbo_status = gl.CheckFramebufferStatus(GL_FRAMEBUFFER);
    Log("[gl] tex=%u fbo=%u fbo_status=0x%x (complete=0x%x)", tex, fbo,
        fbo_status, GL_FRAMEBUFFER_COMPLETE);
    if (fbo_status != GL_FRAMEBUFFER_COMPLETE) {
        Log("[fail] FBO incomplete. OPT-B candidate.");
        return 1;
    }

    // mpv with vo=libmpv, render ctx on the master context (current here).
    setlocale(LC_NUMERIC, "C");
    mpv_handle* mpv = mpv_create();
    if (!mpv) {
        Log("[fail] mpv_create null");
        return 1;
    }
    SetOpt(mpv, "vo", "libmpv");
    SetOpt(mpv, "hwdec", "d3d11va");
    SetOpt(mpv, "audio", "no");
    SetOpt(mpv, "loop-file", "inf");
    SetOpt(mpv, "keep-open", "yes");
    SetOpt(mpv, "idle", "yes");
    SetOpt(mpv, "osc", "no");
    SetOpt(mpv, "video-timing-offset", "0");
    if (mpv_initialize(mpv) < 0) {
        Log("[fail] mpv_initialize failed");
        mpv_terminate_destroy(mpv);
        return 1;
    }
    mpv_request_log_messages(mpv, "info");
    mpv_observe_property(mpv, 0, "hwdec-current", MPV_FORMAT_STRING);

    mpv_opengl_init_params gl_params{};
    gl_params.get_proc_address = MpvGetProc;
    gl_params.get_proc_address_ctx = nullptr;
    int adv = 1;
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, (void*)MPV_RENDER_API_TYPE_OPENGL},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_params},
        {MPV_RENDER_PARAM_ADVANCED_CONTROL, &adv},
        {(mpv_render_param_type)0, nullptr},
    };
    mpv_render_context* mctx = nullptr;
    const int rc_create = mpv_render_context_create(&mctx, mpv, params);
    if (rc_create < 0 || !mctx) {
        Log("[fail] mpv_render_context_create rc=%d (%s). "
            "OPT-B candidate: shared D3D11 texture + blit via interop.",
            rc_create, mpv_error_string(rc_create));
        mpv_terminate_destroy(mpv);
        return 1;
    }
    Log("[mpv] render_context_create ok (OPENGL, advctl=1)");
    mpv_render_context_set_update_callback(mctx, MpvUpdateCb, nullptr);

    const char* cmd[] = {"loadfile", video.c_str(), nullptr};
    if (mpv_command(mpv, cmd) < 0) {
        Log("[fail] loadfile refused");
        mpv_render_context_free(mctx);
        mpv_terminate_destroy(mpv);
        return 1;
    }

    // Video dims for UV quads (fallback tex size).
    int vw = tex_w, vh = tex_h;
    // Consumers start now (each makes its own ctx current in its thread).
    SharedFrame shared;
    shared.texture = tex;
    ConsumerArgs a1{}, a2{};
    a1.hwnd = win1;
    a1.hdc = d1;
    a1.rc = c1;
    a1.procs = &gl;
    a1.shared = &shared;
    a1.fit = fit;
    a1.video_w = vw;
    a1.video_h = vh;
    a1.index = 0;
    a2.hwnd = win2;
    a2.hdc = d2;
    a2.rc = c2;
    a2.procs = &gl;
    a2.shared = &shared;
    a2.fit = fit;
    a2.video_w = vw;
    a2.video_h = vh;
    a2.index = 1;
    std::thread t1(ConsumerThread, &a1), t2(ConsumerThread, &a2);

    // Metrics baseline.
    const unsigned long long thr0 = ThreadCountNow();
    unsigned long long ws0 = 0, priv0 = 0;
    MemSample(&ws0, &priv0);
    unsigned long long cyc0 = 0;
    QueryProcessCycleTime(GetCurrentProcess(), &cyc0);
    auto t0 = std::chrono::steady_clock::now();

    unsigned long long rendered = 0;
    std::string hwdec_seen = "unknown";
    bool saw_frame = false;
    bool playback_started = false;
    unsigned long long thr_peak = thr0;
    const auto deadline =
        t0 + std::chrono::milliseconds(hold_ms);
    while (std::chrono::steady_clock::now() < deadline) {
        // Drain mpv events (same thread owns render ctx; update cb only flags).
        for (int i = 0; i < 16; ++i) {
            mpv_event* ev = mpv_wait_event(mpv, 0);
            if (!ev || ev->event_id == MPV_EVENT_NONE) break;
            if (ev->event_id == MPV_EVENT_START_FILE ||
                ev->event_id == MPV_EVENT_FILE_LOADED)
                playback_started = true;
            else if (ev->event_id == MPV_EVENT_LOG_MESSAGE) {
                auto* lm = (mpv_event_log_message*)ev->data;
                if (lm && lm->text && strstr(lm->text, "hardware decoding"))
                    Log("[mpv-log] %s", lm->text);
            } else if (ev->event_id == MPV_EVENT_PROPERTY_CHANGE) {
                auto* p = (mpv_event_property*)ev->data;
                if (p && p->name && strcmp(p->name, "hwdec-current") == 0 &&
                    p->format == MPV_FORMAT_STRING && p->data) {
                    const char* v = *(const char**)p->data;
                    hwdec_seen = v ? v : "no";
                    Log("[prop] hwdec-current=%s", hwdec_seen.c_str());
                }
            }
        }
        PumpMessages();
        const unsigned long long thr = ThreadCountNow();
        if (thr > thr_peak) thr_peak = thr;

        bool want = g_mpv_update.exchange(false);
        uint64_t upd = 0;
        // With ADVANCED_CONTROL, update() must be consulted; also poll
        // periodically so first frame isn't missed if the callback races.
        upd = mpv_render_context_update(mctx);
        if ((upd & MPV_RENDER_UPDATE_FRAME) || want) {
            mpv_opengl_fbo fbo_p{};
            fbo_p.fbo = (int)fbo;
            fbo_p.w = tex_w;
            fbo_p.h = tex_h;
            fbo_p.internal_format = 0;
            int flip = 0;
            mpv_render_param rp[] = {
                {MPV_RENDER_PARAM_OPENGL_FBO, &fbo_p},
                {MPV_RENDER_PARAM_FLIP_Y, &flip},
                {(mpv_render_param_type)0, nullptr},
            };
            // Master ctx is current in THIS thread (render API requirement).
            const int rr = mpv_render_context_render(mctx, rp);
            if (rr < 0) {
                Log("[warn] render rc=%d (%s)", rr, mpv_error_string(rr));
            } else {
                ::glFlush();
                GLsync_p fence = nullptr;
                if (gl.FenceSync)
                    fence = gl.FenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
                {
                    std::lock_guard<std::mutex> lk(shared.fence_mutex);
                    shared.fence = fence;
                    if (fence) shared.all_fences.push_back(fence);
                }
                shared.frame_id.fetch_add(1);
                ++rendered;
                saw_frame = true;
                // Query real video size once for honest UV quads.
                if (rendered == 5) {
                    int64_t w = 0, h = 0;
                    if (mpv_get_property(mpv, "width", MPV_FORMAT_INT64, &w) >=
                            0 &&
                        mpv_get_property(mpv, "height", MPV_FORMAT_INT64, &h) >=
                            0 &&
                        w > 0 && h > 0) {
                        vw = (int)w;
                        vh = (int)h;
                        a1.video_w = vw;
                        a1.video_h = vh;
                        a2.video_w = vw;
                        a2.video_h = vh;
                        Log("[mpv] video size %dx%d", vw, vh);
                    }
                }
            }
            mpv_render_context_report_swap(mctx);
        } else {
            ::Sleep(2);
        }
    }

    unsigned long long cyc1 = 0;
    QueryProcessCycleTime(GetCurrentProcess(), &cyc1);
    unsigned long long ws1 = 0, priv1 = 0;
    MemSample(&ws1, &priv1);
    const double secs = hold_ms / 1000.0;
    const double cps = (cyc1 >= cyc0) ? (double)(cyc1 - cyc0) / secs : 0.0;
    hwdec_seen = HwdecCurrent(mpv);
    double bright = 0.0;
    if (verify && saw_frame) {
        gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
        bright = MasterTextureBrightness(&gl, tex, tex_w, tex_h);
    }

    shared.running.store(false);
    t1.join();
    t2.join();

    Log("[result] decode_instances=1 rendered=%llu presentedA=%llu "
        "presentedB=%llu",
        rendered, a1.presented.load(), a2.presented.load());
    Log("[result] threads start=%llu peak=%llu", thr0, thr_peak);
    Log("[result] cycles/sec=%.0f ws=%lluMB priv=%lluMB (start ws=%llu priv=%llu)",
        cps, ws1, priv1, ws0, priv0);
    Log("[result] vo=libmpv hwdec-current=%s playback=%d saw_frame=%d "
        "verify_brightness=%.1f",
        hwdec_seen.c_str(), playback_started ? 1 : 0, saw_frame ? 1 : 0,
        bright);

    // Teardown order (mirrors P3.3 spec): stop present threads (joined) ->
    // render_context_free with NO concurrent render -> mpv terminate.
    for (GLsync_p s : shared.all_fences) {
        if (s && gl.DeleteSync) gl.DeleteSync(s);
    }
    if (gl.BindFramebuffer) gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
    if (gl.DeleteFramebuffers) gl.DeleteFramebuffers(1, &fbo);
    if (gl.DeleteTextures) gl.DeleteTextures(1, &tex);
    mpv_render_context_free(mctx);
    mpv_terminate_destroy(mpv);
    ::wglMakeCurrent(nullptr, nullptr);
    ::wglDeleteContext(c1);
    ::wglDeleteContext(c2);
    ::wglDeleteContext(mrc);
    ReleaseDC(win1, d1);
    ReleaseDC(win2, d2);
    ReleaseDC(master_wnd, mdc);
    DestroyWindow(win1);
    DestroyWindow(win2);
    DestroyWindow(master_wnd);

    const bool ok = saw_frame && rendered > 10 && a1.saw_frame.load() &&
                    a2.saw_frame.load() && !shared.failed.load();
    Log("[verdict] %s", ok ? "PASS shared fan-out (decode=1, 2 windows fed)"
                           : "FAIL — see lines above; evaluate OPT-B, do not integrate");
    return ok ? 0 : 1;
}

// ================= DUAL BASELINE (2x vo=gpu) =================
static int RunDual(const std::string& video, int hold_ms, int win_w,
                   int win_h, const std::string& hwdec) {
    Log("[spike] mode=dual video='%s' hold=%d win=%dx%d", video.c_str(),
        hold_ms, win_w, win_h);
    LogAdapters();
    LogAffinityPriority();
    if (!EnsureClass()) return 1;
    HWND win1 = MakeWin(L"spike-dual-A", 60, 60, win_w, win_h, true);
    HWND win2 =
        MakeWin(L"spike-dual-B", 60 + win_w + 40, 60, win_w, win_h, true);
    if (!win1 || !win2) {
        Log("[fail] CreateWindow failed");
        return 1;
    }
    setlocale(LC_NUMERIC, "C");
    mpv_handle* m1 = mpv_create();
    mpv_handle* m2 = mpv_create();
    if (!m1 || !m2) {
        Log("[fail] mpv_create null");
        return 1;
    }
    for (int k = 0; k < 2; ++k) {
        mpv_handle* m = k ? m2 : m1;
        HWND w = k ? win2 : win1;
        char wid[32];
        snprintf(wid, sizeof(wid), "%lld", (long long)(intptr_t)w);
        SetOpt(m, "wid", wid);
        SetOpt(m, "vo", "gpu");
        SetOpt(m, "hwdec", hwdec.c_str());
        SetOpt(m, "audio", "no");
        SetOpt(m, "loop-file", "inf");
        SetOpt(m, "keep-open", "yes");
        if (mpv_initialize(m) < 0) {
            Log("[fail] mpv_initialize instance %d", k);
            return 1;
        }
        mpv_request_log_messages(m, "info");
        mpv_observe_property(m, 0, "hwdec-current", MPV_FORMAT_STRING);
        const char* cmd[] = {"loadfile", video.c_str(), nullptr};
        mpv_command(m, cmd);
    }
    const unsigned long long thr0 = ThreadCountNow();
    unsigned long long ws0 = 0, priv0 = 0;
    MemSample(&ws0, &priv0);
    unsigned long long cyc0 = 0;
    QueryProcessCycleTime(GetCurrentProcess(), &cyc0);
    auto t0 = std::chrono::steady_clock::now();
    unsigned long long thr_peak = thr0;
    std::string hw1 = "unknown", hw2 = "unknown";
    bool play = false;
    const auto deadline = t0 + std::chrono::milliseconds(hold_ms);
    while (std::chrono::steady_clock::now() < deadline) {
        for (int k = 0; k < 2; ++k) {
            mpv_handle* m = k ? m2 : m1;
            for (int i = 0; i < 8; ++i) {
                mpv_event* ev = mpv_wait_event(m, 0);
                if (!ev || ev->event_id == MPV_EVENT_NONE) break;
                if (ev->event_id == MPV_EVENT_START_FILE) play = true;
                if (ev->event_id == MPV_EVENT_PROPERTY_CHANGE) {
                    auto* p = (mpv_event_property*)ev->data;
                    if (p && p->name &&
                        strcmp(p->name, "hwdec-current") == 0 &&
                        p->format == MPV_FORMAT_STRING && p->data) {
                        const char* v = *(const char**)p->data;
                        if (k)
                            hw2 = v ? v : "no";
                        else
                            hw1 = v ? v : "no";
                    }
                }
            }
        }
        PumpMessages();
        const unsigned long long thr = ThreadCountNow();
        if (thr > thr_peak) thr_peak = thr;
        ::Sleep(5);
    }
    unsigned long long cyc1 = 0;
    QueryProcessCycleTime(GetCurrentProcess(), &cyc1);
    unsigned long long ws1 = 0, priv1 = 0;
    MemSample(&ws1, &priv1);
    const double secs = hold_ms / 1000.0;
    const double cps = (cyc1 >= cyc0) ? (double)(cyc1 - cyc0) / secs : 0.0;
    hw1 = HwdecCurrent(m1);
    hw2 = HwdecCurrent(m2);
    Log("[result] decode_instances=2 playback=%d", play ? 1 : 0);
    Log("[result] threads start=%llu peak=%llu", thr0, thr_peak);
    Log("[result] cycles/sec=%.0f ws=%lluMB priv=%lluMB (start ws=%llu priv=%llu)",
        cps, ws1, priv1, ws0, priv0);
    Log("[result] vo=gpu hwdec1=%s hwdec2=%s", hw1.c_str(), hw2.c_str());
    mpv_terminate_destroy(m1);
    mpv_terminate_destroy(m2);
    DestroyWindow(win1);
    DestroyWindow(win2);
    Log("[verdict] %s", play ? "PASS dual baseline" : "FAIL dual baseline");
    return play ? 0 : 1;
}

int main(int argc, char** argv) {
    std::string video;
    std::string mode = "shared";
    std::string fit = "cover";
    std::string hwdec = "d3d11va";
    int hold_ms = 8000;
    int win_w = 640, win_h = 360, tex_w = 1920, tex_h = 1080;
    bool verify = false;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind("--mode=", 0) == 0)
            mode = a.substr(7);
        else if (a.rfind("--hold-ms=", 0) == 0)
            hold_ms = atoi(a.c_str() + 10);
        else if (a.rfind("--fit=", 0) == 0)
            fit = a.substr(6);
        else if (a.rfind("--hwdec=", 0) == 0)
            hwdec = a.substr(8);
        else if (a.rfind("--win-w=", 0) == 0)
            win_w = atoi(a.c_str() + 8);
        else if (a.rfind("--win-h=", 0) == 0)
            win_h = atoi(a.c_str() + 8);
        else if (a.rfind("--tex-w=", 0) == 0)
            tex_w = atoi(a.c_str() + 8);
        else if (a.rfind("--tex-h=", 0) == 0)
            tex_h = atoi(a.c_str() + 8);
        else if (a == "--verify")
            verify = true;
        else if (!a.empty() && a[0] != '-')
            video = a;
    }
    if (video.empty()) {
        fprintf(stderr,
                "usage: render_shared.exe <video> [--mode=shared|dual] "
                "[--hold-ms=N] [--fit=contain|cover|stretch|center] "
                "[--win-w=N --win-h=N] [--tex-w=N --tex-h=N] [--hwdec=X] [--verify]\n");
        return 2;
    }
    if (mode == "dual") return RunDual(video, hold_ms, win_w, win_h, hwdec);
    return RunShared(video, hold_ms, fit, win_w, win_h, tex_w, tex_h, verify);
}
