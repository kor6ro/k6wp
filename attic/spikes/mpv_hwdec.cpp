// spikes/mpv_hwdec.cpp
// Spike (throwaway, NOT shipped): libmpv render into a Win32 window with hwdec chain.
// Usage: mpv_hwdec.exe <video> [--hwdec=d3d11va|dxva2|no] [--hold-ms=N]
// Exit 0 if playback started AND (hwdec mode active OR hwdec=no requested).
// Prints: log lines containing "hardware decoding", hwdec-current property, CPU cycles/sec.

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0A00
#include <windows.h>
#include <mpv/client.h>
#include <cstdio>
#include <cstdlib>
#include <string>

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_DESTROY: PostQuitMessage(0); return 0;
    default: return DefWindowProcW(hwnd, msg, wp, lp);
    }
}

static HWND CreateProbeWindow(int w, int h) {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"K6WPMpvHwdecWnd";
    wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(IDC_ARROW));
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowExW(0, L"K6WPMpvHwdecWnd", L"K6WP mpv hwdec spike",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, w, h,
        nullptr, nullptr, wc.hInstance, nullptr);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    return hwnd;
}

int main(int argc, char** argv) {
    std::string video;
    std::string hwdec = "d3d11va";
    int hold_ms = 5000;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a.rfind("--hwdec=", 0) == 0) hwdec = a.substr(8);
        else if (a.rfind("--hold-ms=", 0) == 0) hold_ms = atoi(a.c_str() + 10);
        else video = a;
    }
    if (video.empty()) {
        fprintf(stderr, "usage: mpv_hwdec.exe <video> [--hwdec=d3d11va|dxva2|no] [--hold-ms=N]\n");
        return 2;
    }

    HWND hwnd = CreateProbeWindow(1280, 720);
    if (!hwnd) { fprintf(stderr, "CreateWindowExW failed\n"); return 1; }

    mpv_handle* mpv = mpv_create();
    if (!mpv) { fprintf(stderr, "mpv_create failed\n"); return 1; }

    mpv_set_option_string(mpv, "wid", std::to_string((long long)(intptr_t)hwnd).c_str());
    mpv_set_option_string(mpv, "hwdec", hwdec.c_str());
    mpv_set_option_string(mpv, "loop-file", "inf");
    mpv_set_option_string(mpv, "audio", "no");
    mpv_set_option_string(mpv, "vo", "gpu");
    mpv_set_option_string(mpv, "keep-open", "yes");

    if (mpv_initialize(mpv) < 0) { fprintf(stderr, "mpv_initialize failed\n"); return 1; }

    mpv_request_log_messages(mpv, "info");
    mpv_observe_property(mpv, 0, "hwdec-current", MPV_FORMAT_STRING);

    unsigned long long c0 = 0, c1 = 0;
    QueryProcessCycleTime(GetCurrentProcess(), &c0);

    const char* cmd[] = {"loadfile", video.c_str(), nullptr};
    mpv_command(mpv, cmd);

    bool hwdec_active = false;
    bool saw_hwdec_log = false;
    bool playback_started = false;
    DWORD t0 = GetTickCount();
    while (GetTickCount() - t0 < (DWORD)hold_ms) {
        mpv_event* ev = mpv_wait_event(mpv, 100);
        if (ev->event_id == MPV_EVENT_NONE) continue;
        switch (ev->event_id) {
        case MPV_EVENT_LOG_MESSAGE: {
            mpv_event_log_message* lm = (mpv_event_log_message*)ev->data;
            if (strstr(lm->text, "hardware decoding")) {
                saw_hwdec_log = true;
                printf("[mpv-log] %s", lm->text);
            }
            break;
        }
        case MPV_EVENT_PROPERTY_CHANGE: {
            mpv_event_property* p = (mpv_event_property*)ev->data;
            if (p->format == MPV_FORMAT_STRING && p->data) {
                const char* val = *(const char**)p->data;
                printf("[prop] hwdec-current = %s\n", val);
                if (strcmp(val, "no") != 0) hwdec_active = true;
            }
            break;
        }
        case MPV_EVENT_START_FILE: playback_started = true; printf("[event] START_FILE\n"); break;
        case MPV_EVENT_END_FILE: printf("[event] END_FILE\n"); break;
        case MPV_EVENT_SHUTDOWN: printf("[event] SHUTDOWN\n"); break;
        default: break;
        }
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    QueryProcessCycleTime(GetCurrentProcess(), &c1);
    double cycles_per_sec = (double)(c1 - c0) / ((double)hold_ms / 1000.0);

    char* cur = nullptr;
    if (mpv_get_property(mpv, "hwdec-current", MPV_FORMAT_STRING, &cur) >= 0 && cur) {
        printf("[final] hwdec-current = %s\n", cur);
        if (strcmp(cur, "no") != 0) hwdec_active = true;
        mpv_free(cur);
    }

    printf("[cpu] cycles/sec = %.0f\n", cycles_per_sec);
    printf("[result] playback_started=%d hwdec_log=%d hwdec_active=%d\n",
           playback_started ? 1 : 0, saw_hwdec_log ? 1 : 0, hwdec_active ? 1 : 0);

    mpv_terminate_destroy(mpv);
    DestroyWindow(hwnd);

    bool ok = playback_started;
    if (hwdec != "no") ok = ok && hwdec_active;
    return ok ? 0 : 1;
}