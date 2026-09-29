// spikes/qt_embed.cpp
// Spike (throwaway, NOT shipped): Qt 6.8 QWidget with WA_NativeWindow, winId -> mpv wid, mpv in QThread.
// Usage: qt_embed.exe <video> [--hold-ms=N] [--out=<prefix>]
// Saves two screen-region BMP captures of the widget (at start+1.5s and start+2.5s).
// Exit 0 if playback started (captures analyzed externally via PowerShell).

#include <QApplication>
#include <QWidget>
#include <QThread>
#include <QTimer>
#include <mpv/client.h>
#include <windows.h>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

// Capture the screen region covered by hwnd (D3D content is NOT readable via GDI BitBlt
// from the window DC, but IS visible in the screen DC after DWM composition).
static bool SaveScreenRegionBmp(HWND hwnd, const char* path) {
    RECT rc;
    GetWindowRect(hwnd, &rc);
    int w = rc.right - rc.left, h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0) return false;
    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hbm = CreateCompatibleBitmap(hdcScreen, w, h);
    HGDIOBJ old = SelectObject(hdcMem, hbm);
    BitBlt(hdcMem, 0, 0, w, h, hdcScreen, rc.left, rc.top, SRCCOPY);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    std::vector<unsigned char> pixels((size_t)w * h * 4);
    GetDIBits(hdcMem, hbm, 0, h, pixels.data(), &bmi, DIB_RGB_COLORS);

    FILE* f = nullptr;
    fopen_s(&f, path, "wb");
    if (!f) { SelectObject(hdcMem, old); DeleteObject(hbm); DeleteDC(hdcMem); ReleaseDC(nullptr, hdcScreen); return false; }
    BITMAPFILEHEADER bfh = {};
    bfh.bfType = 0x4D42;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = bfh.bfOffBits + (DWORD)pixels.size();
    fwrite(&bfh, 1, sizeof(bfh), f);
    fwrite(&bmi.bmiHeader, 1, sizeof(BITMAPINFOHEADER), f);
    fwrite(pixels.data(), 1, pixels.size(), f);
    fclose(f);

    SelectObject(hdcMem, old);
    DeleteObject(hbm);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
    return true;
}

class MpvWorker : public QThread {
public:
    MpvWorker(WId wid, std::string video, std::atomic<bool>& started, std::atomic<bool>& stop)
        : wid_(wid), video_(std::move(video)), started_(started), stop_(stop) {}

protected:
    void run() override {
        mpv_handle* mpv = mpv_create();
        if (!mpv) { fprintf(stderr, "[qt] mpv_create failed\n"); return; }
        mpv_set_option_string(mpv, "wid", std::to_string((long long)wid_).c_str());
        mpv_set_option_string(mpv, "hwdec", "d3d11va");
        mpv_set_option_string(mpv, "loop-file", "inf");
        mpv_set_option_string(mpv, "audio", "no");
        mpv_set_option_string(mpv, "vo", "gpu");
        mpv_set_option_string(mpv, "keep-open", "yes");
        if (mpv_initialize(mpv) < 0) {
            fprintf(stderr, "[qt] mpv_initialize failed\n");
            mpv_terminate_destroy(mpv);
            return;
        }
        mpv_request_log_messages(mpv, "info");
        const char* cmd[] = {"loadfile", video_.c_str(), nullptr};
        mpv_command(mpv, cmd);
        started_.store(true);
        while (!stop_.load()) {
            mpv_event* ev = mpv_wait_event(mpv, 100);
            if (ev->event_id == MPV_EVENT_LOG_MESSAGE) {
                mpv_event_log_message* lm = (mpv_event_log_message*)ev->data;
                if (strstr(lm->text, "hardware decoding")) printf("[qt-mpv] %s", lm->text);
            }
            if (ev->event_id == MPV_EVENT_SHUTDOWN) break;
        }
        mpv_terminate_destroy(mpv);
    }

private:
    WId wid_;
    std::string video_;
    std::atomic<bool>& started_;
    std::atomic<bool>& stop_;
};

int main(int argc, char** argv) {
    std::string video;
    std::string out_prefix = "qt_embed_cap";
    int hold_ms = 5000;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a.rfind("--hold-ms=", 0) == 0) hold_ms = atoi(a.c_str() + 10);
        else if (a.rfind("--out=", 0) == 0) out_prefix = a.substr(6);
        else video = a;
    }
    if (video.empty()) {
        fprintf(stderr, "usage: qt_embed.exe <video> [--hold-ms=N] [--out=<prefix>]\n");
        return 2;
    }

    QApplication app(argc, argv);
    QWidget w;
    w.setAttribute(Qt::WA_NativeWindow);
    w.setWindowTitle("K6WP qt_embed spike");
    w.resize(1280, 720);
    w.show();
    w.raise();
    w.activateWindow();
    WId wid = w.winId(); // forces native window creation

    std::atomic<bool> started(false), stop(false);
    MpvWorker worker(wid, video, started, stop);
    worker.start();

    QTimer poll;
    bool scheduled = false;
    QObject::connect(&poll, &QTimer::timeout, [&]() {
        if (started.load() && !scheduled) {
            scheduled = true;
            poll.stop();
            fprintf(stderr, "[qt] playback started\n");
            QTimer::singleShot(1500, [&]() {
                SaveScreenRegionBmp((HWND)wid, (out_prefix + "1.bmp").c_str());
                fprintf(stderr, "[qt] capture 1 saved\n");
            });
            QTimer::singleShot(2500, [&]() {
                SaveScreenRegionBmp((HWND)wid, (out_prefix + "2.bmp").c_str());
                fprintf(stderr, "[qt] capture 2 saved\n");
            });
        }
    });
    poll.start(50);

    QTimer::singleShot(hold_ms, [&]() {
        stop.store(true);
        worker.wait(2000);
        app.quit();
    });

    int rc = app.exec();
    fprintf(stderr, "[qt] exit rc=%d started=%d\n", rc, started.load() ? 1 : 0);
    return (rc == 0 && started.load()) ? 0 : 1;
}