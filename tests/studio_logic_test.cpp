// studio_logic_test.cpp — MED-2 slice B: Studio non-GUI logic without the
// full main_window (no widgets, no event-loop pumping).
//
// QCoreApplication, not QApplication: this test creates no widgets and must
// not load a QPA platform plugin (only qwindows is deployed; the offscreen
// plugin is absent and its load failure hangs init — HIGH-3 slice B GOTCHA).
// Compiles the REAL studio/src/ipc_client.cpp + src/apply_manager.cpp
// (AUTOMOC covers ApplyManager's Q_OBJECT, studio_async_test pattern).
// Covers:
//  1. Unwired ApplyManager refuses loudly (SyncMonitor/Apply without
//     SetIpcClient -> false + "SetIpcClient" in the error).
//  2. WriteConfig round-trip to an explicit temp path (LoadConfig reads back
//     the same video_path + monitor_id).
//  3. ResolveEnginePath resolves to engine.exe.
//  4. No server -> kNotRunning (before the fake server starts).
//  5. SyncMonitor dedup over an in-process fake pipe server: first push of
//     monitor 0 sends set_monitor, repeat of 0 sends nothing, push of 1
//     sends again — all on the ONE kept connection (LOW-6: IpcClient does
//     not Disconnect per Send, so the server sees 1 accept + 2 requests,
//     never 2 accepts + drain-to-disconnect).
//  6. Engine-read settings contract (LOW-16): only lockscreen_sync +
//     lockscreen_offset_sec are Engine-visible (LOCALAPPDATA redirected at
//     a scratch dir; forbidden-field flips must not move the behavior).
//  7. Dev-fallback gate (LOW-16): IsDevEngineFallbackEnabled() needs _DEBUG
//     AND K6WP_DEV=1; `--probe-resolve` prints gate + path for QA evidence.
//  16. Studio import path (LibraryGridModel): the ffmpeg thumbnail spawn and
//     the ffprobe metadata probe are OFF the calling thread, proven against
//     real blocking child processes (see the fake-media-tools harness above).
//  17. First-run wizard: an over-threshold pick still yields a completable
//     wizard, and "Nanti saja" is as permanent as Finish.
//  20. User-facing error wording: the engine / IPC / preview mappers never
//     echo a technical string, never leak a path, a .exe name, an env var or
//     a log prefix, and always say what to do next; the 6 pre-existing
//     compress sentences are byte-for-byte unchanged.
//
// The fake server listens on the REAL session pipe name, so section 4-5
// SKIP when a live engine answers (never steal the real endpoint).

#include "apply_manager.hpp"
#include "compress_bridge.hpp"
#include "compress_controller.hpp"
#include "compress_errors.hpp"
#include "compress_first_offer.hpp"
#include "displays_schema.hpp"
#include "engine_status_controller.hpp"
#include "ffmpeg_path.hpp"
#include "first_run_wizard.hpp"
#include "ipc_client.hpp"
#include "ipc_protocol.hpp"
#include "library_format.hpp"
#include "library_grid_model.hpp"
#include "lockscreen.hpp"
#include "monitor_util.hpp"
#include "qml_shell.hpp"
#include "settings_bridge.hpp"
#include "studio_bridge.hpp"
#include "studio_settings.hpp"
#include "user_errors.hpp"

#include <atomic>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>

namespace {

int g_checks = 0;
int g_failures = 0;

void Check(bool cond, const std::string& name) {
  ++g_checks;
  if (cond) {
    std::printf("PASS %s\n", name.c_str());
  } else {
    ++g_failures;
    std::printf("FAIL %s\n", name.c_str());
  }
}

bool HasSubstr(const std::string& hay, const std::string& needle) {
  return hay.find(needle) != std::string::npos;
}

bool HasSubstrQ(const QString& hay, const char* needle) {
  return hay.toStdString().find(needle) != std::string::npos;
}

bool EngineAlive() {
  // Retry: a resident engine's single pipe instance is transiently BUSY
  // while another client holds it — one probe then looks "dead" and the
  // fake-server sections below race the real engine for the name. A short
  // retry makes the skip decision stable in a shared session.
  for (int i = 0; i < 5; ++i) {
    const std::wstring name = k6wp::CurrentSessionPipeName();
    HANDLE h =
        CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                    OPEN_EXISTING, 0, nullptr);
    if (h != INVALID_HANDLE_VALUE) {
      CloseHandle(h);
      return true;
    }
    if (GetLastError() == ERROR_PIPE_BUSY) {
      return true;  // someone holds it -> an engine is serving
    }
    Sleep(100);
  }
  return false;
}

HANDLE MakePipeInstance() {
  const std::wstring name = k6wp::CurrentSessionPipeName();
  return CreateNamedPipeW(name.c_str(), PIPE_ACCESS_DUPLEX,
                          PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                          1, 65536, 65536, 0, nullptr);
}

bool ReadOne(HANDLE pipe, std::string* out) {
  char buf[65536];
  DWORD got = 0;
  if (ReadFile(pipe, buf, sizeof(buf), &got, nullptr) == 0 || got == 0) {
    return false;
  }
  out->assign(buf, got);
  return true;
}

bool WriteOne(HANDLE pipe, const std::string& msg) {
  DWORD written = 0;
  return WriteFile(pipe, msg.data(), static_cast<DWORD>(msg.size()), &written,
                   nullptr) != 0 &&
         written == msg.size();
}

std::filesystem::path TempConfigPath(const char* leaf) {
  char tmp[MAX_PATH];
  const DWORD n = GetTempPathA(MAX_PATH, tmp);
  if (n == 0 || n >= MAX_PATH) {
    return std::filesystem::path();
  }
  return std::filesystem::path(tmp) / leaf;
}

// --- fake ffmpeg / ffprobe (sections 16-18) ----------------------------------
//
// The import path spawns two real subprocesses per file (ffmpeg for the grid
// thumbnail, ffprobe for the metadata) and each one blocks for up to 10s
// (kFfmpegTimeoutMs in thumbnailer.cpp, kProbeTimeoutMs in proc_util.hpp).
// "That work is not on the GUI thread" is only provable against a process that
// really blocks, so K6WP_FFMPEG / K6WP_FFPROBE are pointed at THIS executable:
// when a child sees K6WP_TEST_FAKE_MODE it behaves like the tool it stands in
// for and exits, and never reaches the suite. No second helper binary.
//
// The sleep lives in a CHILD process, so it blocks nobody the UI owns - which
// is exactly the property the parent is being asked to preserve.
constexpr wchar_t kFakeModeEnv[] = L"K6WP_TEST_FAKE_MODE";
constexpr wchar_t kFakeMsEnv[] = L"K6WP_TEST_FAKE_MS";

// Returns true when this process IS the fake child (having done its job), so
// main() can bail out before the suite runs.
bool RunFakeChildIfRequested(int argc, char** argv) {
  wchar_t mode[32] = {};
  const DWORD mode_len = GetEnvironmentVariableW(kFakeModeEnv, mode, 32);
  if (mode_len == 0 || mode_len >= 32) {
    return false;
  }
  wchar_t ms[16] = {};
  const DWORD ms_len = GetEnvironmentVariableW(kFakeMsEnv, ms, 16);
  const long block_ms =
      (ms_len > 0 && ms_len < 16) ? wcstol(ms, nullptr, 10) : 3000;
  std::this_thread::sleep_for(std::chrono::milliseconds(block_ms));

  if (std::wstring(mode) == L"ffmpeg") {
    // Thumbnailer::BuildThumbCommand puts the output JPEG last, so the tail of
    // argv is the file to produce. GetThumb only requires a non-empty file.
    if (argc >= 2) {
      QFile out(QString::fromLocal8Bit(argv[argc - 1]));
      if (out.open(QIODevice::WriteOnly)) {
        out.write("fake-jpeg-bytes", 15);
        out.close();
      }
    }
  } else {
    // The exact shape FfprobeHelper::Probe parses, so the async probe's result
    // is assertable rather than merely "something happened".
    const char* kJson =
        R"({"streams":[{"codec_name":"h264","codec_type":"video",)"
        R"("width":1280,"height":720,"r_frame_rate":"30000/1001"}],)"
        R"("format":{"duration":"12.5"}})";
    std::fputs(kJson, stdout);
    std::fflush(stdout);
  }
  return true;
}

// Watches the event loop of the thread that pumps it, so a GUI-thread block
// shows up as a number instead of a vibe.
//
// beats() counts queued callbacks the loop actually delivered, and maxGapMs() is
// the longest stretch the loop went without one. A worker thread is invisible
// to both; a spawn-and-wait on the pumping thread is a multi-second gap.
class EventLoopWatchdog {
 public:
  explicit EventLoopWatchdog(int interval_ms = 5) {
    clock_.start();
    timer_.setInterval(interval_ms);
    QObject::connect(&timer_, &QTimer::timeout, [this]() {
      ++beats_;
      Observe();
    });
    timer_.start();
  }

  // Called from the watched thread once per pump iteration. This is the half
  // that can SEE a block: the timer callback cannot run while the thread is
  // stuck, so the gap must be sampled from outside the stuck window.
  void Observe() {
    const qint64 now = clock_.elapsed();
    max_gap_ms_ = std::max(max_gap_ms_, now - last_seen_ms_);
    last_seen_ms_ = now;
  }

  void Pump() {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    Observe();
  }

  // Pumps until `done` or `budget_ms`, then returns whether it finished. The
  // 5ms nap keeps the loop from spinning hot for the whole budget.
  bool PumpUntil(const std::function<bool()>& done, qint64 budget_ms) {
    while (!done() && clock_.elapsed() < budget_ms) {
      Pump();
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return done();
  }

  int beats() const { return beats_; }
  qint64 maxGapMs() const { return max_gap_ms_; }
  qint64 elapsedMs() const { return clock_.elapsed(); }

 private:
  QElapsedTimer clock_;
  QTimer timer_;
  int beats_ = 0;
  qint64 max_gap_ms_ = 0;
  qint64 last_seen_ms_ = 0;
};

// Longest the Studio event loop may go unserved while a thumbnail/probe is in
// flight. The real defect is a 10s stall; the fakes below stall for 3s, so this
// budget fails loudly on the regression without being twitchy on a loaded box.
constexpr int kGuiFreezeBudgetMs = 1500;

// Points K6WP_FFMPEG + K6WP_FFPROBE at this executable and arms the fake-child
// protocol. Returns the exe path for diagnostics.
QString ArmFakeMediaTools(const char* mode, int block_ms) {
  const QString self = QFileInfo(QCoreApplication::applicationFilePath())
                           .absoluteFilePath();
  const std::wstring self_w = self.toStdWString();
  (void)SetEnvironmentVariableW(L"K6WP_FFMPEG", self_w.c_str());
  (void)SetEnvironmentVariableW(L"K6WP_FFPROBE", self_w.c_str());
  const std::string ms = std::to_string(block_ms);
  std::wstring ms_w(ms.begin(), ms.end());
  const std::wstring mode_w(mode, mode + std::strlen(mode));
  (void)SetEnvironmentVariableW(kFakeModeEnv, mode_w.c_str());
  (void)SetEnvironmentVariableW(kFakeMsEnv, ms_w.c_str());
  return self;
}

void DisarmFakeMediaTools() {
  (void)SetEnvironmentVariableW(kFakeModeEnv, nullptr);
  (void)SetEnvironmentVariableW(kFakeMsEnv, nullptr);
  (void)SetEnvironmentVariableW(L"K6WP_FFMPEG", nullptr);
  (void)SetEnvironmentVariableW(L"K6WP_FFPROBE", nullptr);
}

// A regular file of exactly `bytes` bytes (resize extends the length without
// writing, so a 20 MiB fixture costs no disk).
QString WriteSizedFile(const QDir& dir, const char* leaf, qint64 bytes) {
  QFile f(dir.filePath(QString::fromLatin1(leaf)));
  if (!f.open(QIODevice::WriteOnly)) {
    return {};
  }
  f.resize(bytes);
  f.close();
  return f.fileName();
}

std::filesystem::path ToPath(const QString& p) {
  return std::filesystem::path(p.toStdWString());
}

// Whole file as bytes, or empty when unreadable - the yardstick for
// "displays.json was (not) rewritten by assign/clear".
std::vector<char> ReadAllBytes(const std::filesystem::path& p) {
  std::ifstream f(p, std::ios::binary);
  if (!f) {
    return {};
  }
  return std::vector<char>((std::istreambuf_iterator<char>(f)),
                           std::istreambuf_iterator<char>());
}

std::wstring LocalAppDataDir() {
  wchar_t buf[MAX_PATH] = {};
  const DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", buf, MAX_PATH);
  return n > 0 && n < MAX_PATH ? std::wstring(buf, n) : std::wstring();
}

bool SetLocalAppDataDir(const QString& dir) {
  const std::wstring w = dir.toStdWString();
  return SetEnvironmentVariableW(L"LOCALAPPDATA", w.c_str()) != 0;
}

std::wstring UserProfileDir() {
  wchar_t buf[MAX_PATH] = {};
  const DWORD n = GetEnvironmentVariableW(L"USERPROFILE", buf, MAX_PATH);
  return n > 0 && n < MAX_PATH ? std::wstring(buf, n) : std::wstring();
}

bool SetUserProfileDir(const std::wstring& dir) {
  return SetEnvironmentVariableW(L"USERPROFILE", dir.c_str()) != 0;
}

// Every regular file under `dir`, recursively - the yardstick for "did
// clearCache() delete anything it should not have".
int CountFiles(const std::filesystem::path& dir) {
  std::error_code ec;
  if (!std::filesystem::exists(dir, ec)) {
    return 0;
  }
  int n = 0;
  for (std::filesystem::recursive_directory_iterator it(dir, ec), end;
       it != end; it.increment(ec)) {
    if (ec) {
      break;
    }
    std::error_code inner;
    if (it->is_regular_file(inner) && !inner) {
      ++n;
    }
  }
  return n;
}

// A drive letter no volume currently uses, or 0 when there is none free.
char FreeDriveLetter() {
  const DWORD used = GetLogicalDrives();
  for (int i = 0; i < 26; ++i) {
    if ((used & (1UL << i)) == 0) {
      return static_cast<char>('A' + i);
    }
  }
  return 0;
}

// Section 19 needs a directory reparse point that costs no privilege: a
// junction (mklink /J) always works, a symlink needs SeCreateSymbolicLink /
// Developer Mode. Returns false when neither could be made, so the caller can
// skip rather than report a bogus failure.
bool MakeJunction(const QString& link, const QString& target) {
  const std::string cmd = "cmd /c mklink /J \"" + link.toStdString() + "\" \"" +
                          target.toStdString() + "\" >nul 2>&1";
  if (std::system(cmd.c_str()) == 0 && QFileInfo(link).isDir()) {
    return true;
  }
  QFile::remove(link);
  return CreateSymbolicLinkW(reinterpret_cast<LPCWSTR>(link.utf16()),
                             reinterpret_cast<LPCWSTR>(target.utf16()),
                             SYMBOLIC_LINK_FLAG_DIRECTORY) != 0;
}

// Counts entries in `json` that carry probed dimensions - i.e. how many async
// metadata probes have landed and been persisted.
int ProbedEntryCount(const std::filesystem::path& json) {
  try {
    k6wp::LibraryManager probe(json);
    probe.Load();
    int n = 0;
    for (const auto& e : probe.ListItems()) {
      if (e.width > 0 && e.height > 0) {
        ++n;
      }
    }
    return n;
  } catch (const std::exception&) {
    return 0;  // a half-written file just means "not landed yet"
  }
}

}  // namespace

// The model's constructor registers itself with the QML window as the
// drag-and-drop import target. QmlShell is the whole QQuickWidget + libmpv
// window and cannot be linked into a QCoreApplication test, so the test
// supplies the members the model actually calls and answers "no shell" -
// which is exactly right for a headless run.
namespace k6wp {
void SetActiveQmlShell(QmlShell*) {}
QmlShell* ActiveQmlShell() { return nullptr; }
void QmlShell::SetImportTarget(LibraryGridModel*) {}
// Row 19: StudioBridge forwards the three preview invokables to the active
// shell; studio_logic_test compiles studio_bridge.cpp but not qml_shell.cpp,
// so the forward targets are stubbed here (headless: ActiveQmlShell() is
// always null, so the stubs are never reached at runtime).
void QmlShell::syncPreviewGeometry(int, int, int, int) {}
void QmlShell::loadPreview(const QString&) {}
void QmlShell::setPreviewPaused(bool) {}
}  // namespace k6wp

int main(int argc, char** argv) {
  setvbuf(stdout, nullptr, _IONBF, 0);
  if (RunFakeChildIfRequested(argc, argv)) {
    return 0;
  }
  QCoreApplication app(argc, argv);

  // QA probe (LOW-16): `--probe-resolve` prints the gate state and the
  // resolved engine path, then exits 0 without running the suite. Used by
  // .omo/qa 24-failure.txt (K6WP_DEV unset -> fallback NOT used).
  if (argc > 1 && std::string(argv[1]) == "--probe-resolve") {
    wchar_t dev[8] = {};
    const DWORD dev_n = GetEnvironmentVariableW(L"K6WP_DEV", dev, 8);
    const QString resolved = k6wp::ApplyManager::ResolveEnginePath();
    // fallback_taken is meaningful only when the primary is missing (here
    // the primary exists side-by-side, so the gate state is the signal and
    // taken stays 0): the dev fallback path always carries "msvc-dev".
    const QString primary = QDir(QCoreApplication::applicationDirPath())
                                .filePath(QStringLiteral("engine.exe"));
    const bool primary_exists = QFile::exists(primary);
    std::printf("probe k6wp_dev=%ls\n",
                dev_n > 0 && dev_n < 8 ? dev : L"(unset)");
    std::printf("probe fallback_enabled=%d\n",
                k6wp::ApplyManager::IsDevEngineFallbackEnabled() ? 1 : 0);
    std::printf("probe resolve=%s\n", resolved.toStdString().c_str());
    std::printf("probe primary_exists=%d\n", primary_exists ? 1 : 0);
    std::printf("probe fallback_taken=%d\n",
                (!primary_exists &&
                 resolved.contains(QStringLiteral("msvc-dev")))
                    ? 1
                    : 0);
    return 0;
  }

  // 1. Unwired manager refuses loudly (never a null deref, never silent).
  {
    k6wp::ApplyManager mgr;
    QString err;
    Check(!mgr.SyncMonitor(0, &err), "unwired SyncMonitor returns false");
    Check(HasSubstrQ(err, "SetIpcClient"), "unwired SyncMonitor names SetIpcClient");
    k6wp::WallpaperConfig cfg;
    cfg.video_path = L"C:\\videos\\wallpaper.mp4";
    QString aerr;
    Check(!static_cast<bool>(mgr.Apply(cfg, &aerr)),
          "unwired Apply returns false");
    Check(HasSubstrQ(aerr, "SetIpcClient"), "unwired Apply names SetIpcClient");
  }

  // 2. WriteConfig round-trip through the real loader.
  {
    k6wp::ApplyManager mgr;
    const std::filesystem::path cfg_path =
        TempConfigPath("k6wp-logic-test-config.json");
    mgr.SetConfigPath(cfg_path);
    k6wp::WallpaperConfig cfg;
    cfg.video_path = L"C:\\videos\\wallpaper.mp4";
    cfg.monitor_id = 2;
    QString err;
    Check(mgr.WriteConfig(cfg, &err), "WriteConfig to temp path succeeds");
    bool loaded = false;
    k6wp::WallpaperConfig back;
    try {
      back = k6wp::LoadConfig(cfg_path);
      loaded = true;
    } catch (...) {
      loaded = false;
    }
    Check(loaded, "LoadConfig reads back the written file");
    Check(back.video_path == cfg.video_path, "round-trip video_path identical");
    Check(back.monitor_id == 2, "round-trip monitor_id identical");
    std::error_code ec;
    std::filesystem::remove(cfg_path, ec);
  }

  // 3. Engine path resolution ends at engine.exe (never empty).
  {
    const QString p = k6wp::ApplyManager::ResolveEnginePath();
    Check(!p.isEmpty(), "ResolveEnginePath non-empty");
    Check(p.endsWith(QStringLiteral("engine.exe"), Qt::CaseInsensitive),
          "ResolveEnginePath ends with engine.exe");
  }

  const bool skip_server = EngineAlive();
  std::printf("[info] engine alive: %d (server tests %s)\n",
              static_cast<int>(skip_server),
              skip_server ? "SKIPPED" : "running");

  // 4. No server yet -> kNotRunning (engine-dead path, UI offers Start).
  if (!skip_server) {
    k6wp::IpcClient client;
    const k6wp::IpcResult r = client.Send(k6wp::Cmd::get_state);
    Check(r.status == k6wp::IpcStatus::kNotRunning,
          "no-server Send maps to kNotRunning");
  }

  // 5. SyncMonitor dedup: push(0), push(0), push(1) ride ONE kept
  // connection (LOW-6: no Disconnect per Send). The server acks {"ok":true}
  // per request and records both set_monitors: exactly 1 accept, 2
  // requests. (A drain-to-disconnect script would block here — the kept
  // client never disconnects until its dtor — so the server reads exactly
  // the scripted requests and closes unilaterally.)
  if (!skip_server) {
    std::atomic<int> accepts{0};
    std::vector<std::string> seen;
    std::thread server([&]() {
      HANDLE pipe = MakePipeInstance();
      if (pipe == INVALID_HANDLE_VALUE) {
        return;
      }
      const BOOL ok = ConnectNamedPipe(pipe, nullptr) != 0 ||
                      GetLastError() == ERROR_PIPE_CONNECTED;
      if (!ok) {
        CloseHandle(pipe);
        return;
      }
      ++accepts;
      for (int i = 0; i < 2; ++i) {
        std::string req;
        if (!ReadOne(pipe, &req)) {
          CloseHandle(pipe);
          return;
        }
        seen.push_back(req);
        if (!WriteOne(pipe, "{\"ok\":true}")) {
          CloseHandle(pipe);
          return;
        }
      }
      CloseHandle(pipe);
    });

    k6wp::IpcClient client;
    k6wp::ApplyManager mgr;
    mgr.SetIpcClient(&client);
    QString err;
    Check(mgr.SyncMonitor(0, &err), "dedup first push(0) true");
    Check(mgr.SyncMonitor(0, &err), "dedup repeat push(0) true");
    Check(mgr.SyncMonitor(1, &err), "dedup changed push(1) true");
    server.join();

    std::printf("[info] dedup accepts=%d\n", accepts.load());
    Check(accepts.load() == 1, "dedup server saw exactly 1 kept connection");
    bool wire = seen.size() == 2;
    for (const auto& req : seen) {
      wire = wire && HasSubstr(req, "\"set_monitor\"");
    }
    Check(wire, "dedup both requests carried set_monitor");
  }

  // 6. Engine-read settings contract (LOW-16, dev-contracts.md §3): the
  // Engine may observe exactly lockscreen_sync + lockscreen_offset_sec
  // (shared/lockscreen.cpp). Point LOCALAPPDATA at a scratch dir so no real
  // profile is touched, then prove every other field is behavior-invisible.
  {
    wchar_t old_local[MAX_PATH] = {};
    const DWORD old_n =
        GetEnvironmentVariableW(L"LOCALAPPDATA", old_local, MAX_PATH);
    char tmp_a[MAX_PATH] = {};
    Check(GetTempPathA(MAX_PATH, tmp_a) != 0, "contract temp dir available");
    const std::filesystem::path scratch =
        std::filesystem::path(tmp_a) / "k6wp-logic-test-settings";
    std::error_code ec;
    std::filesystem::create_directories(scratch / "K6WP", ec);
    Check(!ec, "contract scratch dir created");
    Check(SetEnvironmentVariableW(L"LOCALAPPDATA", scratch.wstring().c_str()) != 0,
          "contract LOCALAPPDATA redirected");
    auto write_settings = [&](bool sync, double offset, int crf, int fps,
                              const std::string& mode, bool auto_comp,
                              bool start_win, bool advanced, bool updates) {
      k6wp::StudioSettings s = k6wp::DefaultStudioSettings();
      s.compress_output_dir = (scratch / L"K6WP" / L"wallpapers").wstring();
      s.cache_dir = (scratch / L"K6WP" / L"cache").wstring();
      s.lockscreen_sync = sync;
      s.lockscreen_offset_sec = offset;
      s.default_crf = crf;
      s.default_fps = fps;
      s.default_resolution_mode = mode;
      s.auto_compress_on_import = auto_comp;
      s.start_with_windows = start_win;
      s.compress_advanced_visible = advanced;
      s.check_updates = updates;
      k6wp::SaveStudioSettings(k6wp::DefaultStudioSettingsPath(), s);
    };
    // A: every forbidden field non-default, sync OFF -> engine sees OFF/1.0.
    write_settings(false, 1.0, 28, 24, "720p", false, true, true, false);
    Check(!k6wp::IsLockscreenSyncEnabled(),
          "contract forbidden fields do not enable sync");
    Check(k6wp::LockscreenOffsetSec() == 1.0,
          "contract offset default visible");
    // B: flip ONLY forbidden fields -> engine-visible behavior identical.
    write_settings(false, 1.0, 16, 1, "source", true, false, false, true);
    Check(!k6wp::IsLockscreenSyncEnabled(),
          "contract forbidden flip keeps sync off");
    Check(k6wp::LockscreenOffsetSec() == 1.0,
          "contract forbidden flip keeps offset");
    // C: flip the two allowed fields -> behavior follows them.
    write_settings(true, 2.5, 16, 1, "source", true, false, false, true);
    Check(k6wp::IsLockscreenSyncEnabled(),
          "contract lockscreen_sync=true visible");
    Check(k6wp::LockscreenOffsetSec() == 2.5,
          "contract lockscreen_offset_sec visible");
    // D: settings file missing -> safe defaults (never throws).
    std::filesystem::remove(k6wp::DefaultStudioSettingsPath(), ec);
    Check(!k6wp::IsLockscreenSyncEnabled(),
          "contract missing file sync defaults false");
    Check(k6wp::LockscreenOffsetSec() == 1.0,
          "contract missing file offset defaults 1.0");
    // E: corrupt file -> same safe defaults (never throws).
    {
      std::ofstream bad(k6wp::DefaultStudioSettingsPath(), std::ios::binary);
      bad << "{not valid json";
    }
    Check(!k6wp::IsLockscreenSyncEnabled(),
          "contract corrupt file sync defaults false");
    Check(k6wp::LockscreenOffsetSec() == 1.0,
          "contract corrupt file offset defaults 1.0");
    if (old_n > 0 && old_n < MAX_PATH) {
      (void)SetEnvironmentVariableW(L"LOCALAPPDATA", old_local);
    } else {
      (void)SetEnvironmentVariableW(L"LOCALAPPDATA", nullptr);
    }
    std::filesystem::remove_all(scratch, ec);
  }

  // 7. Dev-fallback gate (LOW-16): IsDevEngineFallbackEnabled() is true only
  // for a _DEBUG build with K6WP_DEV=1 in the environment — never silent.
  {
    (void)SetEnvironmentVariableW(L"K6WP_DEV", nullptr);
    Check(!k6wp::ApplyManager::IsDevEngineFallbackEnabled(),
          "gate K6WP_DEV unset -> disabled");
    (void)SetEnvironmentVariableW(L"K6WP_DEV", L"0");
    Check(!k6wp::ApplyManager::IsDevEngineFallbackEnabled(),
          "gate K6WP_DEV=0 -> disabled");
    (void)SetEnvironmentVariableW(L"K6WP_DEV", L"yes");
    Check(!k6wp::ApplyManager::IsDevEngineFallbackEnabled(),
          "gate K6WP_DEV=yes -> disabled");
    (void)SetEnvironmentVariableW(L"K6WP_DEV", L"1");
#ifdef _DEBUG
    Check(k6wp::ApplyManager::IsDevEngineFallbackEnabled(),
          "gate _DEBUG + K6WP_DEV=1 -> enabled");
#else
    Check(!k6wp::ApplyManager::IsDevEngineFallbackEnabled(),
          "gate release + K6WP_DEV=1 -> disabled");
#endif
    (void)SetEnvironmentVariableW(L"K6WP_DEV", nullptr);
  }

  // 8. First-run gate (MED-5 part 1, extracted from MainWindow::IsFirstRun):
  // settings file present always wins; otherwise both library-empty and
  // no-current-video are required.
  {
    Check(!k6wp::IsFirstRunCondition(true, true, true),
          "firstrun settings present -> false");
    Check(!k6wp::IsFirstRunCondition(true, false, false),
          "firstrun settings present, active user -> false");
    Check(!k6wp::IsFirstRunCondition(false, false, true),
          "firstrun library non-empty -> false");
    Check(!k6wp::IsFirstRunCondition(false, true, false),
          "firstrun current video set -> false");
    Check(k6wp::IsFirstRunCondition(false, true, true),
          "firstrun fresh profile -> true");
  }

  // 9. First-run entry builder (MED-5 part 1, from OnFirstRunProbed):
  // metadata lands only on probe success; thumb only when produced.
  {
    k6wp::VideoMetadata meta;
    meta.duration = 12.5;
    meta.codec = "h264";
    meta.width = 1920;
    meta.height = 1080;
    meta.fps = 29.97;
    const std::filesystem::path thumb(L"C:/lib/thumb.jpg");
    const k6wp::LibraryEntry e = k6wp::BuildFirstRunEntry(
        QString::fromStdString("C:/src/a.mp4"),
        QString::fromStdString("C:/lib/a.mp4"), true, meta, thumb);
    Check(e.src == std::filesystem::path(L"C:/src/a.mp4"),
          "firstrun entry src kept");
    Check(e.dst == std::filesystem::path(L"C:/lib/a.mp4"),
          "firstrun entry dst kept");
    Check(e.duration == 12.5, "firstrun entry duration kept");
    Check(e.codec == "h264", "firstrun entry codec kept");
    Check(e.width == 1920 && e.height == 1080, "firstrun entry dims kept");
    Check(e.fps == 30, "firstrun entry fps rounded");
    Check(e.res == "1920x1080", "firstrun entry res string");
    Check(e.thumb == thumb, "firstrun entry thumb kept");
    const k6wp::LibraryEntry bad = k6wp::BuildFirstRunEntry(
        QString::fromStdString("C:/src/b.mp4"),
        QString::fromStdString("C:/lib/b.mp4"), false, meta,
        std::filesystem::path());
    Check(bad.src == std::filesystem::path(L"C:/src/b.mp4"),
          "firstrun probe-fail src still kept");
    Check(bad.width == 0 && bad.height == 0 && bad.fps == 30,
          "firstrun probe-fail dims zero (fps default)");
    Check(bad.res == "0x0", "firstrun probe-fail res default");
    Check(bad.codec.empty(), "firstrun probe-fail codec empty");
    Check(bad.thumb.empty(), "firstrun empty thumb stays empty");
  }

  // 10. Engine-status decision (MED-5 part 1, from OnPollDone): the paint
  // contract — Connected/Paused/Degraded/NotRunning/Disconnected.
  {
    using Kind = k6wp::EngineStatusView::Kind;
    k6wp::IpcResult ok;
    ok.status = k6wp::IpcStatus::kOk;
    ok.raw = nlohmann::json::parse(
        "{\"state\":{\"pid\":1234,\"video\":\"C:/v/a.mp4\",\"paused\":false,"
        "\"headless_slots\":0,\"live\":true}}");
    const k6wp::EngineStatusView v = k6wp::DecideEngineStatus(ok);
    Check(v.kind == Kind::kConnected, "status playing -> Connected");
    Check(v.pid == 1234ULL, "status pid parsed");
    Check(v.video == QString::fromStdString("C:/v/a.mp4"),
          "status video parsed");
    k6wp::IpcResult paused = ok;
    paused.raw["state"]["paused"] = true;
    Check(k6wp::DecideEngineStatus(paused).kind == Kind::kPaused,
          "status paused flag -> Paused");
    k6wp::IpcResult headless = ok;
    headless.raw["state"]["headless_slots"] = 2;
    const k6wp::EngineStatusView vh = k6wp::DecideEngineStatus(headless);
    Check(vh.kind == Kind::kDegraded && vh.headless == 2,
          "status headless slots -> Degraded");
    k6wp::IpcResult idle = ok;
    idle.raw["state"]["live"] = false;
    Check(k6wp::DecideEngineStatus(idle).kind == Kind::kDegraded,
          "status live=false -> Degraded");
    k6wp::IpcResult empty_ok;
    empty_ok.status = k6wp::IpcStatus::kOk;
    const k6wp::EngineStatusView ve = k6wp::DecideEngineStatus(empty_ok);
    Check(ve.kind == Kind::kConnected && ve.video.isEmpty(),
          "status empty video tolerated -> Connected");
    k6wp::IpcResult malformed;
    malformed.status = k6wp::IpcStatus::kOk;
    malformed.raw = nlohmann::json::parse("{\"state\":42}");
    Check(k6wp::DecideEngineStatus(malformed).kind == Kind::kConnected,
          "status malformed state never throws -> Connected");
    k6wp::IpcResult dead;
    dead.status = k6wp::IpcStatus::kNotRunning;
    Check(k6wp::DecideEngineStatus(dead).kind == Kind::kNotRunning,
          "status engine dead -> NotRunning");
    k6wp::IpcResult broken;
    broken.status = k6wp::IpcStatus::kError;
    broken.error = "pipe timeout";
    const k6wp::EngineStatusView vb = k6wp::DecideEngineStatus(broken);
    Check(vb.kind == Kind::kDisconnected && vb.error == "pipe timeout",
          "status transport failure -> Disconnected + error");
  }

  // 11. Worker ready-wait (MED-5 part 1, from MainWindow::WaitForEngineReady):
  // live engine -> true at once; dead engine -> false after the budget.
  // Skips the dead-engine leg when a live engine answers (same rule as
  // sections 4-5 — never steal the real endpoint).
  {
    std::atomic<bool> cancel{false};
    k6wp::IpcClient client;
    if (skip_server) {
      Check(k6wp::WaitForEngineReady(client, cancel, 8000),
            "readywait live engine -> true");
    } else {
      Check(!k6wp::WaitForEngineReady(client, cancel, 1000),
            "readywait dead engine -> false");
      cancel.store(true);
      Check(!k6wp::WaitForEngineReady(client, cancel, 8000),
            "readywait pre-cancelled -> false at once");
    }
  }

  // 12. CompressController (MED-5 part 2 commit 1): pure request-building
  // + single-job queue wrapper. No widgets; QCoreApplication only (the
  // offscreen QPA plugin is absent — QApplication would hang init).
  {
    using k6wp::CompressController;
    int w = 0, h = 0;
    CompressController::ResolveSimpleRes("1080p", nullptr, w, h);
    Check(w == 1920 && h == 1080, "compress res 1080p fixed");
    CompressController::ResolveSimpleRes("720p", nullptr, w, h);
    Check(w == 1280 && h == 720, "compress res 720p fixed");
    CompressController::ResolveSimpleRes("2160p", nullptr, w, h);
    Check(w == 3840 && h == 2160, "compress res 2160p fixed");
    k6wp::VideoMetadata probed;
    probed.width = 640;
    probed.height = 480;
    CompressController::ResolveSimpleRes("source", &probed, w, h);
    Check(w == 640 && h == 480, "compress res source uses probe");
    CompressController::ResolveSimpleRes("source", nullptr, w, h);
    Check(w == 1280 && h == 720, "compress res source w/o probe falls back");
    CompressController::ResolveSimpleRes("match_monitor", &probed, w, h);
    Check(w > 0 && h > 0, "compress res match_monitor positive");

    const QString def_dir = CompressController::DefaultWallpapersDir();
    Check(!def_dir.isEmpty(), "compress default out dir non-empty");
    Check(def_dir.contains(QStringLiteral("wallpapers")),
          "compress default out dir is wallpapers");

    QTemporaryDir tmp;
    Check(tmp.isValid(), "compress test temp dir valid");
    const QString src = QDir(tmp.path()).filePath(QStringLiteral("clip.mp4"));
    // Auto-naming: UniqueOutPath stem + .mp4 under the resolved dir.
    k6wp::TabRequestInputs in;
    in.src = src;
    in.res_w = 640;
    in.res_h = 360;
    in.out_dir = CompressController::ResolveTabOutDir(tmp.path(), QString());
    Check(in.out_dir == QDir::cleanPath(tmp.path()),
          "compress out dir override wins");
    const k6wp::CompressRequest auto_req =
        CompressController::BuildTabRequest(in);
    Check(auto_req.out_path.endsWith(QStringLiteral(".mp4")),
          "compress auto name ends .mp4");
    Check(auto_req.out_path.contains(QStringLiteral("_k6wp")),
          "compress auto name carries _k6wp");
    Check(auto_req.fps == 30 && auto_req.crf == 22 &&
              auto_req.encoder == QStringLiteral("auto") && !auto_req.force,
          "compress simple mode safe defaults");
    // Override name gains .mp4; out-of-range simple prefs clamp.
    in.out_name_override = QStringLiteral("my clip");
    in.simple_fps = 99;
    in.simple_crf = 1;
    const k6wp::CompressRequest over_req =
        CompressController::BuildTabRequest(in);
    Check(over_req.out_path.endsWith(QStringLiteral("my clip.mp4")),
          "compress override name gains .mp4");
    Check(over_req.fps == 30 && over_req.crf == 22,
          "compress simple prefs clamp to safe range");
    // Advanced values pass through untouched.
    in.advanced = true;
    in.adv_fps = 15;
    in.adv_crf = 28;
    in.adv_encoder = QStringLiteral("x264");
    in.adv_force = true;
    const k6wp::CompressRequest adv_req =
        CompressController::BuildTabRequest(in);
    Check(adv_req.fps == 15 && adv_req.crf == 28 &&
              adv_req.encoder == QStringLiteral("x264") && adv_req.force,
          "compress advanced prefs pass through");

    // Ledger + validation (no compressor.exe needed for these legs).
    k6wp::CompressController ctl;
    Check(!ctl.IsRunning() && ctl.PendingCount() == 0,
          "compress controller starts idle");
    k6wp::CompressRequest empty;
    k6wp::JobMeta meta;
    QString err;
    Check(!ctl.Enqueue(empty, meta, &err) && !err.isEmpty(),
          "compress enqueue empty req refused with error");
    k6wp::CompressRequest taken;
    Check(!ctl.TakeRequest(424242, &taken) && taken.out_path.isEmpty(),
          "compress TakeRequest unknown id -> false + empty");
  }

  // 13. Live single-job queue (happy: progress NDJSON shows; failure: 2x fast
  // enqueue queues instead of doubling). Guarded: runs only when
  // compressor.exe sits next to this binary (POST_BUILD copy) AND ffmpeg is
  // findable (K6WP_FFMPEG env in ctest) AND the corpus fixture resolves.
  // Otherwise SKIP (never red) — the pure legs above still pin the logic.
  {
    const QString exe_dir = QCoreApplication::applicationDirPath();
    const QString compressor =
        QDir(exe_dir).filePath(QStringLiteral("compressor.exe"));
    const QString corpus = QDir(exe_dir + QStringLiteral("/../../tests/corpus"))
                               .filePath(QStringLiteral("slideshow.mp4"));
    // A longer fixture guarantees at least one ffmpeg progress line; the
    // 14 KB slideshow can finish inside a single progress interval (zero
    // Progress emissions — flaky under ctest load).
    const QString corpus_long =
        QDir(exe_dir + QStringLiteral("/../../tests/corpus"))
            .filePath(QStringLiteral("anime.mp4"));
    const bool ready = QFile::exists(compressor) &&
                       !k6wp::FindFfmpeg().empty() &&
                       QFile::exists(corpus) && QFile::exists(corpus_long);
    if (!ready) {
      std::printf("SKIP live compress queue (compressor=%d ffmpeg=%d corpus=%d)\n",
                  static_cast<int>(QFile::exists(compressor)),
                  static_cast<int>(!k6wp::FindFfmpeg().empty()),
                  static_cast<int>(QFile::exists(corpus)));
    } else {
      QTemporaryDir tmp;
      Check(tmp.isValid(), "live compress temp dir valid");
      k6wp::CompressController ctl;
      // Fresh cache dir per run: a warm default cache would turn the jobs
      // into cache_hits (zero ffmpeg progress lines, still Finished-ok).
      ctl.SetCacheDir(
          QDir(tmp.path()).filePath(QStringLiteral("cache-dir")));
      int started = 0;
      int finished_ok = 0;
      int finished_fail = 0;
      int progress_lines = 0;
      int concurrent = 0;
      int max_concurrent = 0;
      int queue_summaries = 0;
      int summary_ok = -1;
      QObject::connect(ctl.service(), &k6wp::CompressService::Started, &app,
                       [&](const k6wp::JobMeta&) {
                         ++started;
                         ++concurrent;
                         max_concurrent =
                             std::max(max_concurrent, concurrent);
                       });
      QObject::connect(ctl.service(), &k6wp::CompressService::Progress, &app,
                       [&](int, int) { ++progress_lines; });
      QObject::connect(ctl.service(), &k6wp::CompressService::Finished, &app,
                       [&](const k6wp::JobMeta& m, bool ok, const QString&,
                           const k6wp::CompressOkInfo&) {
                         --concurrent;
                         k6wp::CompressRequest req;
                         Check(ctl.TakeRequest(m.job_id, &req),
                               "live compress completion has ledger entry");
                         if (ok) {
                           ++finished_ok;
                         } else {
                           ++finished_fail;
                         }
                       });
      QObject::connect(ctl.service(), &k6wp::CompressService::QueueSummary,
                       &app, [&](int ok_count, int) {
                         ++queue_summaries;
                         summary_ok = ok_count;
                       });
      // Two back-to-back enqueues = the 2x-OnCompressCurrent failure shape
      // (PrepareAndEnqueue funnels into CompressController::Enqueue; the
      // widget slot itself needs QApplication, which hangs offscreen).
      // q1 uses the long fixture so progress NDJSON is guaranteed; q2 the
      // tiny one (different cache key, still a real encode).
      k6wp::TabRequestInputs in;
      in.src = corpus_long;
      in.res_w = 320;
      in.res_h = 240;
      in.advanced = true;
      in.adv_fps = 15;
      in.adv_crf = 28;
      in.adv_encoder = QStringLiteral("x264");
      in.out_dir = tmp.path();
      k6wp::JobMeta m1;
      m1.origin = k6wp::JobMeta::Origin::kTab;
      m1.label = QStringLiteral("live-q1");
      m1.src = corpus;
      QString e1;
      in.out_name_override = QStringLiteral("live-q1");
      Check(ctl.Enqueue(k6wp::CompressController::BuildTabRequest(in), m1,
                        &e1),
            "live compress q1 enqueued");
      k6wp::JobMeta m2 = m1;
      m2.label = QStringLiteral("live-q2");
      QString e2;
      in.out_name_override = QStringLiteral("live-q2");
      Check(ctl.Enqueue(k6wp::CompressController::BuildTabRequest(in), m2,
                        &e2),
            "live compress q2 enqueued");
      Check(ctl.PendingCount() == 1,
            "live compress 2x fast -> exactly 1 waiting (no double job)");
      Check(started <= 1, "live compress 2x fast -> at most 1 started");
      QElapsedTimer timer;
      timer.start();
      while (finished_ok + finished_fail < 2 && timer.elapsed() < 180000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        QThread::msleep(50);
      }
      Check(finished_ok + finished_fail == 2,
            "live compress both jobs finished");
      Check(finished_fail == 0, "live compress both jobs ok");
      Check(progress_lines > 0, "live compress progress NDJSON shown");
      Check(max_concurrent == 1, "live compress never ran 2 jobs at once");
      Check(queue_summaries >= 1 && summary_ok == 2,
            "live compress queue summary reports 2 ok");
      Check(ctl.PendingCount() == 0 && !ctl.IsRunning(),
            "live compress idle after drain");
    }
  }

  // 14. Compress-first threshold (compress_first_offer.hpp): the gate the
  //     whole "Video Besar" offer hangs on, which had no test at all. The
  //     boundary is exact - size <= 20 MiB declines, one byte more offers -
  //     and the reported number is truncated whole MiB, not rounded.
  {
    QTemporaryDir offer_dir;
    Check(offer_dir.isValid(), "offer threshold temp dir is valid");
    if (offer_dir.isValid()) {
      const QDir base(offer_dir.path());
      const auto write_sized = [&base](const char* leaf, qint64 bytes) {
        QFile f(base.filePath(QString::fromLatin1(leaf)));
        if (!f.open(QIODevice::WriteOnly)) {
          return QString();
        }
        // resize() extends the length without writing the bytes, so a 20 MiB
        // boundary case costs no disk.
        f.resize(bytes);
        f.close();
        return f.fileName();
      };

      const QString tiny = write_sized("tiny.mp4", 1024);
      const QString exact =
          write_sized("exact.mp4", k6wp::kCompressFirstThresholdBytes);
      const QString over =
          write_sized("over.mp4", k6wp::kCompressFirstThresholdBytes + 1);

      // The fixtures read this same constant, so without a pinned literal
      // they would all stay green if the threshold were raised.
      Check(k6wp::kCompressFirstThresholdBytes == 20LL * 1024 * 1024,
            "threshold is still 20 MiB (value pinned, not merely consistent)");
      Check(!tiny.isEmpty() && !exact.isEmpty() && !over.isEmpty(),
            "offer threshold fixtures written");
      Check(k6wp::CompressFirstOfferMb(tiny) == -1,
            "under threshold -> no offer (-1)");
      Check(k6wp::CompressFirstOfferMb(exact) == -1,
            "exactly 20 MiB -> no offer (-1; the test is <=)");
      Check(k6wp::CompressFirstOfferMb(over) == 20,
            "20 MiB + 1 byte -> offered, reported as 20 MB");
      Check(k6wp::CompressFirstOfferMb(
                base.filePath(QStringLiteral("missing.mp4"))) == -1,
            "missing file -> no offer (-1)");
      Check(k6wp::CompressFirstOfferMb(base.path()) == -1,
            "a directory -> no offer (-1)");
    }
  }

  // 15. CompressBridge's no-result contract: a job that ends without an output
  //     must announce it, because resultChanged() never fires and a pending
  //     one-shot apply intent would outlive the job that raised it. A missing
  //     source reaches that state deterministically and cheaply - the encode
  //     fails and nothing is written. Confirmed to have teeth: removing the
  //     emit makes this fail. The refused-enqueue branch (kInvalid) is fixed
  //     as well but is NOT covered here, because reaching it needs
  //     compressor.exe to be unfindable - moving a shared binary mid-suite.
  {
    k6wp::CompressBridge bridge;
    int no_result = 0;
    int result_changed = 0;
    QObject::connect(&bridge, &k6wp::CompressBridge::jobFinishedWithoutResult,
                     [&no_result]() { ++no_result; });
    QObject::connect(&bridge, &k6wp::CompressBridge::resultChanged,
                     [&result_changed]() { ++result_changed; });

    const auto missing = TempConfigPath("k6wp_no_such_source.mp4");
    bridge.setSourcePath(QString::fromStdWString(missing.wstring()));
    bridge.start();

    // The probe runs on a worker thread and the decision lands back on this
    // one, so the signal can only arrive through the event loop.
    QElapsedTimer timer;
    timer.start();
    while (no_result == 0 && timer.elapsed() < 30000) {
      QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
      QThread::msleep(50);
    }

    Check(no_result > 0, "a compress that yields no output announces it");
    Check(result_changed == 0,
          "and it reports no result (resultChanged stays silent)");
  }

  // 16. Studio import path: the per-file subprocess waits are OFF the calling
  //     thread. Each leg arms a fake ffmpeg/ffprobe that blocks 3s (the real
  //     budget is 10s) and measures the event loop of the thread that calls
  //     into the model, so a stall shows up as a number.
  {
    const QString self = ArmFakeMediaTools("ffmpeg", 3000);
    Check(QFileInfo(self).isFile(), "fake ffmpeg stand-in resolves to this exe");

    // 16a. The grid thumbnail. ensureThumbnail used to defer GetThumb with
    //      QTimer::singleShot(0, this, ...), which posts to the GUI thread -
    //      the ffmpeg spawn+wait then ran there and froze the UI.
    QTemporaryDir thumb_dir;
    Check(thumb_dir.isValid(), "grid test temp dir is valid");
    if (thumb_dir.isValid()) {
      const QDir media(thumb_dir.path());
      const QString video = WriteSizedFile(media, "grid.mp4", 4096);
      const auto lib_json = ToPath(media.filePath(QStringLiteral("library.json")));
      const std::wstring thumbs_root =
          media.filePath(QStringLiteral("thumbs")).toStdWString();
      (void)SetEnvironmentVariableW(L"K6WP_THUMBS_DIR", thumbs_root.c_str());
      (void)SetEnvironmentVariableW(L"K6WP_LIBRARY_JSON", lib_json.wstring().c_str());

      k6wp::LibraryEntry seed;
      seed.src = seed.dst = ToPath(video);
      seed.res = "1280x720";
      seed.width = 1280;
      seed.height = 720;
      seed.codec = "h264";
      seed.duration = 12.5;
      seed.fps = 30;
      {
        k6wp::LibraryManager seed_mgr(lib_json);
        seed_mgr.Add(seed);
      }

      k6wp::LibraryGridModel model;
      Check(model.count() == 1, "grid model loaded the seeded library row");

      int resets = 0;
      QObject::connect(&model, &k6wp::LibraryGridModel::countChanged,
                       [&resets]() { ++resets; });

      EventLoopWatchdog watch;
      model.ensureThumbnail(0);
      const bool thumb_landed = watch.PumpUntil([&resets]() { return resets > 0; },
                                                30000);

      Check(thumb_landed,
            "grid thumbnail completion reached the model's own thread");
      Check(watch.maxGapMs() < kGuiFreezeBudgetMs,
            "ffmpeg spawn+wait does not stall the event loop (max gap " +
                std::to_string(watch.maxGapMs()) + "ms, budget " +
                std::to_string(kGuiFreezeBudgetMs) + "ms)");
      Check(watch.beats() > 5,
            "and the loop kept delivering callbacks while it ran (" +
                std::to_string(watch.beats()) + " beats)");

      k6wp::LibraryManager after_thumb(lib_json);
      after_thumb.Load();
      const auto thumb_items = after_thumb.ListItems();
      Check(thumb_items.size() == 1 && !thumb_items[0].thumb.empty(),
            "the generated thumbnail is persisted back to library.json");
    }

    // 16b. The import metadata probe. Add() used to run FfprobeHelper::Probe
    //      inline, so a multi-select froze the GUI thread for 10s per file.
    QTemporaryDir import_dir;
    Check(import_dir.isValid(), "import test temp dir is valid");
    if (import_dir.isValid()) {
      const QDir media(import_dir.path());
      const QStringList videos{
          WriteSizedFile(media, "one.mp4", 4096),
          WriteSizedFile(media, "two.mp4", 8192),
      };
      const auto lib_json = ToPath(media.filePath(QStringLiteral("library.json")));
      const std::wstring thumbs_root =
          media.filePath(QStringLiteral("thumbs")).toStdWString();
      (void)SetEnvironmentVariableW(L"K6WP_THUMBS_DIR", thumbs_root.c_str());
      (void)SetEnvironmentVariableW(L"K6WP_LIBRARY_JSON", lib_json.wstring().c_str());
      (void)SetEnvironmentVariableW(kFakeModeEnv, L"ffprobe");

      k6wp::LibraryGridModel model;
      Check(model.isEmpty(), "import model starts empty");

      EventLoopWatchdog watch;
      QElapsedTimer import_timer;
      import_timer.start();
      const int added = model.importPaths(videos);
      const qint64 import_ms = import_timer.elapsed();

      Check(added == 2, "importPaths reports both files added");
      Check(import_ms < kGuiFreezeBudgetMs,
            "importPaths does not block on ffprobe (took " +
                std::to_string(import_ms) + "ms, budget " +
                std::to_string(kGuiFreezeBudgetMs) + "ms)");
      Check(model.count() == 2,
            "both rows are visible immediately (metadata arrives later)");

      const bool probed = watch.PumpUntil(
          [&lib_json]() { return ProbedEntryCount(lib_json) == 2; }, 30000);
      Check(probed, "both async metadata probes landed");
      Check(watch.maxGapMs() < kGuiFreezeBudgetMs,
            "ffprobe waits do not stall the event loop (max gap " +
                std::to_string(watch.maxGapMs()) + "ms, budget " +
                std::to_string(kGuiFreezeBudgetMs) + "ms)");
      Check(watch.beats() > 5,
            "and the loop kept delivering callbacks while they ran (" +
                std::to_string(watch.beats()) + " beats)");

      k6wp::LibraryManager after_import(lib_json);
      after_import.Load();
      const auto items = after_import.ListItems();
      bool all_meta = items.size() == 2;
      for (const auto& e : items) {
        all_meta = all_meta && e.width == 1280 && e.height == 720 &&
                   e.codec == "h264" && e.res == "1280x720" && e.fps == 30 &&
                   e.duration == 12.5;
      }
      Check(all_meta,
            "async probe metadata (dims/codec/res/fps/duration) is persisted");
      Check(model.count() == 2, "the grid still shows both rows afterwards");
    }

    DisarmFakeMediaTools();
    (void)SetEnvironmentVariableW(L"K6WP_LIBRARY_JSON", nullptr);
    (void)SetEnvironmentVariableW(L"K6WP_THUMBS_DIR", nullptr);
  }

  // 17. First-run wizard reachability. An over-threshold first pick used to
  //     produce an EMPTY firstRunFile, because the only source QML had was
  //     Library.dstAt(0) and that file was deliberately not imported - so the
  //     wizard could never be completed. "Nanti saja" also never wrote the
  //     settings marker, so the wizard reappeared on every start.
  {
    // The settings path resolves %LOCALAPPDATA% on every call, so redirecting
    // it keeps the test from writing the real user preferences file (which
    // would suppress the wizard on this machine for good).
    const std::wstring real_localappdata = LocalAppDataDir();
    QTemporaryDir fake_home;
    Check(fake_home.isValid(), "firstrun test temp dir is valid");
    const bool redirected =
        fake_home.isValid() && SetLocalAppDataDir(fake_home.path());
    Check(redirected, "LOCALAPPDATA redirected into the temp dir");
    if (redirected) {
      const QDir media(fake_home.path());
      const auto settings_json =
          media.filePath(QStringLiteral("K6WP/studio_settings.json"));
      const auto lib_json = ToPath(media.filePath(QStringLiteral("library.json")));
      (void)SetEnvironmentVariableW(L"K6WP_LIBRARY_JSON", lib_json.wstring().c_str());

      Check(!QFileInfo::exists(settings_json),
            "no settings file yet -> this is a first run");

      k6wp::LibraryGridModel model;
      Check(model.firstRunEligible(),
            "empty library + no settings file -> the wizard is armed");

      // The exact shape that deadlocked: over threshold, so the compress-first
      // offer takes it and nothing is imported.
      const QString big = WriteSizedFile(
          media, "big.mp4", k6wp::kCompressFirstThresholdBytes + 1);
      int offers = 0;
      QString offered_path;
      QObject::connect(&model, &k6wp::LibraryGridModel::compressFirstRequired,
                       [&offers, &offered_path](const QString& p) {
                         ++offers;
                         offered_path = p;
                       });

      const int added = model.importPaths({big});

      Check(added == 0, "an over-threshold pick is not imported into the grid");
      Check(offers == 1, "and it raises exactly one compress-first offer");
      Check(offered_path == big, "naming the file the user actually picked");
      Check(model.count() == 0, "the grid stays empty");
      Check(model.dstAt(0).isEmpty(),
            "the old wizard source dstAt(0) is empty - the dead end itself");
      Check(model.lastPickedPath() == big,
            "lastPickedPath still names the pick, so the wizard can preview it");

      // "Nanti saja": nothing imported, so the wizard must not come back - not
      // now, and not after a restart either.
      model.markFirstRunHandled();

      Check(QFileInfo::exists(settings_json),
            "declining writes the settings file that closes the gate");
      Check(!model.firstRunEligible(),
            "and the wizard is not eligible any more in this session");
      Check(model.lastPickedPath().isEmpty(),
            "the declined pick is forgotten, not carried into a later run");

      k6wp::LibraryGridModel after_restart;
      Check(!after_restart.firstRunEligible(),
            "a fresh model (i.e. a restart) agrees the wizard is handled");
      Check(after_restart.isEmpty(),
            "and declining really imported nothing");

      // The marker must not be re-stamped: the file now holds the user's
      // preferences, and rewriting it with defaults would wipe them.
      {
        QFile f(settings_json);
        Check(f.open(QIODevice::WriteOnly | QIODevice::Truncate),
              "settings file reopened for the no-clobber check");
        const QByteArray sentinel("{\"sentinel\":true}");
        f.write(sentinel);
        f.close();
      }
      model.markFirstRunHandled();
      {
        QFile f(settings_json);
        Check(f.open(QIODevice::ReadOnly), "settings file readable afterwards");
        Check(f.readAll().contains("\"sentinel\":true"),
              "declining twice does not overwrite existing user settings");
        f.close();
      }

      (void)SetEnvironmentVariableW(L"K6WP_LIBRARY_JSON", nullptr);
    }
    (void)SetLocalAppDataDir(QString::fromStdWString(real_localappdata));
  }

  // 18. SettingsBridge::reload() must not throw away the error it just
  //     computed. A corrupt studio_settings.json used to be reported through
  //     SetLastError and then cleared unconditionally a few lines later, so the
  //     user's settings were silently reset to defaults and the QML banner had
  //     nothing left to show - the same trap LibraryGridModel::reload already
  //     documents.
  {
    const std::wstring real_localappdata = LocalAppDataDir();
    QTemporaryDir fake_home;
    Check(fake_home.isValid(), "reload test temp dir is valid");
    const bool redirected =
        fake_home.isValid() && SetLocalAppDataDir(fake_home.path());
    Check(redirected, "reload: LOCALAPPDATA redirected into the temp dir");
    if (redirected) {
      const QDir media(fake_home.path());
      const auto settings_json =
          media.filePath(QStringLiteral("K6WP/studio_settings.json"));
      Check(QDir().mkpath(media.filePath(QStringLiteral("K6WP"))),
            "reload: fake K6WP data dir created");
      {
        QFile f(settings_json);
        Check(f.open(QIODevice::WriteOnly | QIODevice::Truncate),
              "reload: corrupt settings file written");
        f.write("{ this is not json");
        f.close();
      }
      {
        // The constructor is the real startup path: it reloads immediately.
        k6wp::SettingsBridge bridge;
        Check(bridge.lastError().contains(QString::fromUtf8("rusak")),
              "reload: a corrupt settings file leaves lastError set");
        Check(bridge.log().join(QLatin1Char('\n')).contains(
                  QString::fromUtf8("rusak")),
              "reload: and the reason reached the log too");
        Check(QFileInfo::exists(settings_json + ".bak"),
              "reload: the corrupt bytes are preserved as .bak");
        Check(ToPath(bridge.cacheDir()) == k6wp::DefaultStudioCacheDir(),
              "reload: the in-memory fallback is the default cache dir");
      }
      {
        // A good file must clear the error again - the fix is a guard, not a
        // sticky error.
        k6wp::StudioSettings good = k6wp::DefaultStudioSettings();
        good.default_crf = 27;
        k6wp::SaveStudioSettings(k6wp::DefaultStudioSettingsPath(), good);
        k6wp::SettingsBridge bridge;
        Check(bridge.lastError().isEmpty(),
              "reload: a clean settings file clears lastError");
        Check(bridge.defaultCrf() == 27,
              "reload: the clean file is actually loaded");
      }
    }
    (void)SetLocalAppDataDir(QString::fromStdWString(real_localappdata));
  }

  // 19. SettingsBridge::clearCache() refuses a cache_dir that is not a
  //     K6WP-managed cache. cache_dir is free text (ValidateStudioSettings only
  //     rejects the empty string), so one click could otherwise empty
  //     C:\Users\<me>\Videos. USERPROFILE is redirected at a fake tree so the
  //     user-content-folder cases are exercised against throwaway files: a
  //     regression that deleted a real profile would be a catastrophe in this
  //     suite, not a FAIL line.
  {
    const std::wstring real_localappdata = LocalAppDataDir();
    const std::wstring real_userprofile = UserProfileDir();
    QTemporaryDir fake_home;
    QTemporaryDir external;  // stands in for "a folder on another drive"
    Check(fake_home.isValid() && external.isValid(),
          "clearcache test temp dirs are valid");
    const QDir media(fake_home.path());
    const QDir outside(external.path());
    Check(QDir().mkpath(media.filePath(QStringLiteral("K6WP/cache"))),
          "clearcache: fake K6WP cache dir created");
    Check(QDir().mkpath(media.filePath(QStringLiteral("K6WP/wallpapers"))),
          "clearcache: fake K6WP wallpapers dir created");
    Check(QDir().mkpath(media.filePath(QStringLiteral("Pictures"))),
          "clearcache: fake user-content dir created");
    const bool redirected =
        SetLocalAppDataDir(fake_home.path()) &&
        SetUserProfileDir(fake_home.path().toStdWString());
    Check(redirected, "clearcache: LOCALAPPDATA + USERPROFILE redirected");
    if (redirected) {
      // A directory the user picked, marked as a cache the way pickCacheDir
      // marks one, and a file inside it that clearCache is supposed to remove.
      const auto picked = outside.filePath(QStringLiteral("k6wp-cache-drive"));
      Check(QDir().mkpath(picked), "clearcache: external cache dir created");
      const auto victim = picked + QStringLiteral("/holiday.mp4");
      Check(!WriteSizedFile(QDir(picked), "holiday.mp4", 64).isEmpty(),
            "clearcache: external file created");
      {
        QFile marker(picked + QStringLiteral("/.k6wp-cache"));
        Check(marker.open(QIODevice::WriteOnly | QIODevice::Truncate),
              "clearcache: opt-in marker written");
        marker.close();
      }

      k6wp::SettingsBridge bridge;

      // --- refused: the K6WP data root itself. Clearing it would take
      // studio_settings.json, config.json and library.json with it.
      const auto data_root = media.filePath(QStringLiteral("K6WP"));
      Check(!WriteSizedFile(QDir(data_root), "config.json", 32).isEmpty(),
            "clearcache: a file in the K6WP data root created");
      bridge.setCacheDir(data_root);
      Check(bridge.clearCache() == 0, "clearcache: the K6WP data root is refused");
      Check(CountFiles(ToPath(data_root)) == 1,
            "clearcache: and the K6WP data root is left untouched");
      Check(!bridge.lastError().isEmpty(),
            "clearcache: refusing the data root sets lastError");

      // --- refused: %LOCALAPPDATA% itself, one level above the data root.
      const auto local_root = media.path();
      Check(!WriteSizedFile(media, "stray.json", 32).isEmpty(),
            "clearcache: a file beside the K6WP dir created");
      bridge.setCacheDir(local_root);
      Check(bridge.clearCache() == 0, "clearcache: %LOCALAPPDATA% is refused");
      Check(QFileInfo::exists(local_root + QStringLiteral("/stray.json")),
            "clearcache: and %LOCALAPPDATA% is left untouched");

      // --- refused: the user's own content folders, resolved from the
      // (redirected) profile.
      const auto pictures = media.filePath(QStringLiteral("Pictures"));
      Check(!WriteSizedFile(QDir(pictures), "family.png", 32).isEmpty(),
            "clearcache: a picture created");
      bridge.setCacheDir(pictures);
      Check(bridge.clearCache() == 0, "clearcache: Pictures is refused");
      Check(CountFiles(ToPath(pictures)) == 1, "clearcache: Pictures survives");
      Check(bridge.lastError().contains(pictures),
            "clearcache: the refusal names the folder it refused");

      // --- refused: a K6WP dir that holds user output, not cache.
      const auto wallpapers =
          media.filePath(QStringLiteral("K6WP/wallpapers"));
      Check(!WriteSizedFile(QDir(wallpapers), "out.mp4", 32).isEmpty(),
            "clearcache: a compressed video created");
      bridge.setCacheDir(wallpapers);
      Check(bridge.clearCache() == 0,
            "clearcache: the wallpapers dir is refused");
      Check(CountFiles(ToPath(wallpapers)) == 1,
            "clearcache: the compressed video survives");

      // --- refused: a volume root. Tested on a drive letter subst'd at the
      // throwaway tree rather than on a real volume: if the guard ever
      // regressed, "X:\" would then hold nothing but fixtures instead of a
      // system disk.
      {
        const char letter = FreeDriveLetter();
        const bool mapped =
            letter != 0 && std::system(
                              (std::string("subst ") + letter + ": \"" +
                               outside.path().toStdString() + "\" >nul 2>&1")
                                  .c_str()) == 0;
        if (mapped) {
          const QString root =
              QString(QLatin1Char(letter)) + QStringLiteral(":/");
          Check(CountFiles(ToPath(outside.path())) > 0,
                "clearcache: the mapped volume root starts non-empty");
          bridge.setCacheDir(root);
          Check(bridge.clearCache() == 0, "clearcache: a volume root is refused");
          Check(!bridge.lastError().isEmpty(),
                "clearcache: refusing a volume root sets lastError");
          Check(CountFiles(ToPath(outside.path())) > 0,
                "clearcache: a volume root is not walked at all");
        } else {
          std::printf("SKIP volume-root case (no free drive letter for subst)\n");
        }
        if (letter != 0) {
          const std::string undo = std::string("subst ") + letter + ": /D";
          (void)std::system(undo.c_str());
        }
      }

      // --- refused: an external directory the user never opted in. This is
      // the accident the guard exists for - a folder with the user's files and
      // no K6WP marker anywhere in it.
      bridge.setCacheDir(outside.path());
      Check(bridge.clearCache() == 0,
            "clearcache: an unmarked external dir is refused");
      Check(QFileInfo::exists(victim),
            "clearcache: the unmarked external dir is left untouched");

      // --- refused: a junction pointing at a folder outside the cache. The
      // configured name looks harmless; the resolved target is what the sweep
      // would walk.
      const auto linked = outside.filePath(QStringLiteral("link-to-pictures"));
      if (MakeJunction(linked, pictures)) {
        bridge.setCacheDir(linked);
        Check(bridge.clearCache() == 0,
              "clearcache: a junction to Pictures is refused");
        Check(CountFiles(ToPath(pictures)) == 1,
              "clearcache: the junction target survives");
      } else {
        std::printf(
            "SKIP junction case (no privilege to create a reparse point)\n");
      }

      // --- permitted: %LOCALAPPDATA%\K6WP\cache, the default. The whole point
      // of the guard is that it must not get in the way here.
      const auto default_cache = media.filePath(QStringLiteral("K6WP/cache"));
      const auto nested = default_cache + QStringLiteral("/v2");
      Check(QDir().mkpath(nested), "clearcache: nested cache subdir created");
      Check(!WriteSizedFile(QDir(default_cache), "a.bin", 16).isEmpty() &&
                !WriteSizedFile(QDir(nested), "b.bin", 16).isEmpty(),
            "clearcache: two cache files created");
      bridge.setCacheDir(default_cache);
      Check(bridge.clearCache() == 2,
            "clearcache: the default cache dir is swept recursively");
      Check(CountFiles(ToPath(default_cache)) == 0,
            "clearcache: the default cache dir is empty afterwards");
      Check(bridge.lastError().isEmpty(),
            "clearcache: a clean sweep reports no error");

      // --- permitted: an external directory the user opted in through the
      // folder picker, which is the legitimate "cache on another drive" case.
      bridge.setCacheDir(picked);
      Check(bridge.clearCache() == 1,
            "clearcache: an opted-in external dir is swept");
      Check(!QFileInfo::exists(victim),
            "clearcache: the opted-in file is gone");
      Check(QFileInfo::exists(picked + QStringLiteral("/.k6wp-cache")),
            "clearcache: the opt-in marker survives, so a second clear works");

      // --- permitted: a dedicated but so far empty external folder, with no
      // marker. There is nothing in it to lose, so the "point the cache at a
      // fresh folder on D:\" flow must not be blocked.
      const auto fresh = outside.filePath(QStringLiteral("fresh"));
      Check(QDir().mkpath(fresh), "clearcache: fresh external dir created");
      bridge.setCacheDir(fresh);
      Check(bridge.clearCache() == 0,
            "clearcache: an empty external dir is allowed (nothing to delete)");
      Check(bridge.lastError().isEmpty(),
            "clearcache: and it is not reported as a refusal");

      // --- a partial sweep must say so instead of logging a clean one. The
      // open handle denies FILE_SHARE_DELETE, so this file cannot be removed.
      // `fresh` is opted in now, because it is no longer empty and the guard
      // refuses an unmarked external dir that has files in it.
      {
        QFile marker(fresh + QStringLiteral("/.k6wp-cache"));
        Check(marker.open(QIODevice::WriteOnly | QIODevice::Truncate),
              "clearcache: fresh dir opted in for the partial sweep");
        marker.close();
      }
      Check(!WriteSizedFile(QDir(fresh), "free.bin", 16).isEmpty(),
            "clearcache: a removable cache file created");
      Check(!WriteSizedFile(QDir(fresh), "locked.bin", 16).isEmpty(),
            "clearcache: a locked cache file created");
      {
        QFile locked(fresh + QStringLiteral("/locked.bin"));
        Check(locked.open(QIODevice::ReadWrite),
              "clearcache: the second file is held open");
        bridge.setCacheDir(fresh);
        Check(bridge.clearCache() == 1,
              "clearcache: a partial sweep still reports what it removed");
        Check(!bridge.lastError().isEmpty(),
              "clearcache: a partial sweep sets lastError");
        Check(QFileInfo::exists(fresh + QStringLiteral("/locked.bin")),
              "clearcache: the locked file is still there");
        locked.close();
      }
    }
    (void)SetUserProfileDir(real_userprofile);
    (void)SetLocalAppDataDir(QString::fromStdWString(real_localappdata));
  }

  // 20. User-facing error wording (1.2.1). StudioBridge showed the engine /
  //     IPC / mpv technical strings verbatim, so a tooltip or status line could
  //     read "Apply: engine not running" or a raw libmpv sentence; ApplyManager
  //     strings carry absolute paths and .exe filenames too. The technical text
  //     still reaches the log pane, so what is tested here is only what the
  //     user reads.
  {
    // The whole point of the mapping: a user-facing sentence must not leak the
    // install layout or a developer knob, and must never come back empty.
    // Applied to every mapper so a future table entry cannot regress quietly.
    const auto safe_and_actionable = [](const QString& msg, const char* who) {
      std::string lower = msg.toStdString();
      std::transform(lower.begin(), lower.end(), lower.begin(),
                     [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
      Check(!msg.isEmpty(), std::string(who) + ": never returns empty");
      Check(!HasSubstr(lower, ".exe"),
            std::string(who) + ": no .exe filename in the visible message");
      Check(!HasSubstr(lower, "k6wp_ffmpeg") && !HasSubstr(lower, "k6wp_dev"),
            std::string(who) + ": no environment variable in the visible message");
      Check(!HasSubstr(lower, ":\\") && !HasSubstr(lower, ":/"),
            std::string(who) + ": no absolute path in the visible message");
      Check(!HasSubstr(lower, "apply:") && !HasSubstr(lower, "compress:"),
            std::string(who) + ": no log prefix in the visible message");
      Check(HasSubstr(lower, "coba lagi") || HasSubstr(lower, "cek log") ||
                HasSubstr(lower, "nyalakan") || HasSubstr(lower, "ekstrak"),
            std::string(who) + ": tells the user what to do next");
    };

    // Engine not running / not started — the two most common states.
    const QString no_engine = k6wp::FriendlyApplyError(
        QStringLiteral("engine not running"));
    Check(no_engine != QStringLiteral("engine not running"),
          "engine-not-running is not shown verbatim");
    safe_and_actionable(no_engine, "apply");

    // An absolute path and a .exe name in one string: exactly what
    // ApplyManager composes, and exactly what must not reach the status bar.
    const QString with_path = k6wp::FriendlyApplyError(
        QStringLiteral("failed to start C:\\Program Files\\K6WP\\engine.exe: "
                       "The system cannot find the file specified."));
    Check(!HasSubstrQ(with_path, "Program Files"),
          "apply: install path from the start failure is not shown");
    Check(!HasSubstrQ(with_path, "engine.exe"),
          "apply: engine.exe name from the start failure is not shown");
    safe_and_actionable(with_path, "apply");

    // The "Apply: " prefix: StudioBridge used to wrap the raw error in it, so
    // a log-only convention leaked into the UI. Same content, no prefix.
    const QString busy = k6wp::FriendlyApplyError(
        QStringLiteral("Engine sibuk - tunggu proses berjalan"));
    Check(!HasSubstrQ(busy, "Apply:"), "apply: busy message carries no prefix");
    safe_and_actionable(busy, "apply");

    // Empty input still has to say something a user can act on, not "".
    safe_and_actionable(k6wp::FriendlyApplyError(QString()), "apply(empty)");

    // IPC transport failures (status_hint_ used to be QString::fromStdString(
    // view.error) verbatim - the raw pipe error).
    const QString pipe = k6wp::FriendlyIpcError(
        QStringLiteral("pipe \\\\ .\\pipe\\k6wp-engine: The pipe has been "
                       "ended. (232)"));
    Check(!HasSubstrQ(pipe, "k6wp-engine"),
          "ipc: the pipe name is not shown to the user");
    safe_and_actionable(pipe, "ipc");

    const QString busy_pipe = k6wp::FriendlyIpcError(
        QStringLiteral("engine busy"));
    Check(!HasSubstrQ(busy_pipe, "busy") || !HasSubstrQ(busy_pipe, "engine busy"),
          "ipc: a busy engine is reworded, not echoed");
    safe_and_actionable(busy_pipe, "ipc");

    safe_and_actionable(k6wp::FriendlyIpcError(QString()), "ipc(empty)");

    // libmpv: mpv_error_string() English text reached the preview label
    // verbatim through error_detail_.
    const QString mpv_fail = k6wp::FriendlyPreviewError(
        QStringLiteral("Failed to open file: hresult: 0x80070002"));
    Check(!HasSubstrQ(mpv_fail, "hresult") && !HasSubstrQ(mpv_fail, "0x"),
          "preview: no HRESULT / hex error code in the visible message");
    Check(!HasSubstrQ(mpv_fail, "Failed to open file"),
          "preview: libmpv's English text is not shown verbatim");
    safe_and_actionable(mpv_fail, "preview");

    // mpv gave nothing usable - the label must still read as a sentence.
    safe_and_actionable(k6wp::FriendlyPreviewError(QString()), "preview(empty)");

    // The compression mapping is unchanged where it already worked: the 6
    // pre-existing rules must keep their exact sentences, since the app has
    // shipped with them, and the new compressor-side rules must not shadow
    // them (a missing compressor must not read as "ffmpeg not found").
    const QString cancelled = k6wp::FriendlyCompressError(
        QStringLiteral("{\"error\":{\"friendly\":\"dibatalkan\","
                       "\"technical\":\"cancelled by user\"}}"));
    Check(cancelled == QStringLiteral("Kompresi dibatalkan."),
          "compress: cancelled sentence is unchanged");
    const QString corrupt = k6wp::FriendlyCompressError(
        QStringLiteral("failed to read duration: corrupt file"));
    Check(HasSubstrQ(corrupt, "rusak"),
          "compress: corrupt/duration sentence is unchanged");
    const QString crf = k6wp::FriendlyCompressError(
        QStringLiteral("crf must be in range [0,51]"));
    Check(HasSubstrQ(crf, "16 sampai 28"),
          "compress: CRF sentence is unchanged");
    const QString unknown = k6wp::FriendlyCompressError(
        QStringLiteral("some brand new failure nobody mapped"));
    Check(unknown == QStringLiteral("Kompresi gagal. Lihat log untuk detail teknis."),
          "compress: unknown failure keeps the generic fallback");

    // The compressor's ffmpeg-missing sentence is now actionable (it used to
    // tell an ordinary user about a source-tree vendor/ folder and an
    // environment variable).
    const QString ffmpeg_gone = k6wp::FriendlyCompressError(
        QStringLiteral("ffmpeg.exe not found (K6WP_FFMPEG or "
                       "vendor/ffmpeg/ffmpeg.exe)"));
    Check(!HasSubstrQ(ffmpeg_gone, "vendor") &&
              !HasSubstrQ(ffmpeg_gone, "K6WP_FFMPEG"),
          "compress: ffmpeg-missing no longer names vendor/ or K6WP_FFMPEG");
    Check(HasSubstrQ(ffmpeg_gone, "Ekstrak ulang") ||
              HasSubstrQ(ffmpeg_gone, "pasang ulang"),
          "compress: ffmpeg-missing tells the user to re-extract / reinstall");
    safe_and_actionable(ffmpeg_gone, "compress");

    // A missing compressor is its own case and must not be swallowed by the
    // ffmpeg rule (its own text says "kompresor", but the generic compressor
    // paths mention the binary).
    const QString comp_gone = k6wp::FriendlyCompressError(
        QStringLiteral("compressor.exe not found at "
                       "C:\\Users\\me\\AppData\\Local\\K6WP\\compressor.exe"));
    Check(!HasSubstrQ(comp_gone, "ffmpeg"),
          "compress: missing compressor is not reported as an ffmpeg problem");
    Check(!HasSubstrQ(comp_gone, ".exe") && !HasSubstrQ(comp_gone, ":\\"),
          "compress: missing compressor leaks no path or .exe name");
    safe_and_actionable(comp_gone, "compress");

    const QString comp_start = k6wp::FriendlyCompressError(
        QStringLiteral("failed to start compressor.exe: access denied"));
    Check(!HasSubstrQ(comp_start, "access denied"),
          "compress: start failure no longer shows the raw Win32 reason");
    safe_and_actionable(comp_start, "compress");

    const QString comp_exit = k6wp::FriendlyCompressError(
        QStringLiteral("compressor exited with code 3221225781"));
    Check(!HasSubstrQ(comp_exit, "3221225781"),
          "compress: exit code is not dumped into the visible message");
    safe_and_actionable(comp_exit, "compress");
  }

  // 21. Display model bridge (row 19): Studio.displays /
  //     displayCapability / duplicateModeNotice plus assignVideoToMonitor /
  //     clearMonitorAssignment. Acceptance (i)-(iv): fixture geometry through
  //     the property, get_state capability detect, the IS-7 duplicate
  //     notice, and persist-then-notify on assign (unknown key = no write +
  //     Indonesian lastError + no displaysChanged).
  {
    // --- (ii) capability: pure decision over ParseEngineState -------------
    const nlohmann::json without_cap = {{"state", {{"pid", 42ULL}}}};
    Check(!k6wp::DisplayCapabilityFromState(k6wp::ParseEngineState(without_cap)),
          "displayCapability false when get_state lacks the key");
    const nlohmann::json with_cap = {
        {"state", {{"pid", 42ULL}, {"display_capability", 1}}}};
    Check(k6wp::DisplayCapabilityFromState(k6wp::ParseEngineState(with_cap)),
          "displayCapability true when get_state carries display_capability:1");
    const nlohmann::json wrong_type = {
        {"state", {{"pid", 42ULL}, {"display_capability", "yes"}}}};
    Check(!k6wp::DisplayCapabilityFromState(k6wp::ParseEngineState(wrong_type)),
          "displayCapability false when the key has a non-integer value");

    // --- fixture monitors: landscape primary + portrait secondary ----------
    k6wp::MonitorInfo primary;
    primary.id = 0;
    primary.x = 0;
    primary.y = 0;
    primary.width = 1920;
    primary.height = 1080;
    primary.is_primary = true;
    primary.device_name = L"\\\\.\\DISPLAY1";
    primary.orientation = 0;
    primary.refresh_hz = 60;
    primary.scale_pct = 100;

    k6wp::MonitorInfo portrait;
    portrait.id = 1;
    portrait.x = 1920;
    portrait.y = 0;
    portrait.width = 1080;
    portrait.height = 1920;
    portrait.is_primary = false;
    portrait.device_name = L"\\\\.\\DISPLAY2";
    portrait.orientation = 1;
    portrait.refresh_hz = 60;
    portrait.scale_pct = 125;

    const std::vector<k6wp::MonitorInfo> fixture = {primary, portrait};
    k6wp::DisplaysConfig empty_store;
    const std::map<std::string, std::string> no_coverage;

    // Pure builder (the seam refreshDisplays uses): entry shape + geometry.
    const QVariantList pure =
        k6wp::BuildDisplayEntries(fixture, empty_store, no_coverage);
    Check(pure.size() == 2, "displays fixture: two entries for two monitors");
    const QVariantMap e0 = pure.at(0).toMap();
    const QVariantMap e1 = pure.at(1).toMap();
    Check(e0.value("x").toInt() == 0 && e0.value("y").toInt() == 0 &&
              e0.value("width").toInt() == 1920 &&
              e0.value("height").toInt() == 1080,
          "displays fixture: entry 0 carries primary x/y/w/h");
    Check(e0.value("isPrimary").toBool(), "displays fixture: entry 0 isPrimary");
    Check(e0.value("key").toString() ==
              QString::fromStdWString(primary.device_name),
          "displays fixture: entry 0 key is the GDI device name");
    Check(e1.value("x").toInt() == 1920 && e1.value("y").toInt() == 0 &&
              e1.value("width").toInt() == 1080 &&
              e1.value("height").toInt() == 1920,
          "displays fixture: entry 1 carries portrait x/y/w/h");
    Check(!e1.value("isPrimary").toBool(),
          "displays fixture: entry 1 is not primary");
    Check(e1.value("orientation").toString() == QStringLiteral("portrait"),
          "displays fixture: entry 1 orientation is portrait");
    Check(e0.value("orientation").toString() == QStringLiteral("landscape"),
          "displays fixture: entry 0 orientation is landscape");
    Check(e0.value("scalePercent").toInt() == 100 &&
              e1.value("scalePercent").toInt() == 125,
          "displays fixture: scalePercent follows MonitorInfo.scale_pct");
    Check(e1.value("refreshHz").toInt() == 60 &&
              e1.value("resolutionLabel").toString() ==
                  QStringLiteral("1080x1920"),
          "displays fixture: refreshHz + resolutionLabel on the portrait entry");

    // QA-HAPPY dump (task-19 evidence): full entry field dump.
    std::printf("QA-HAPPY displays fixture entries:\n");
    for (const QVariant& v : pure) {
      const QVariantMap m = v.toMap();
      std::printf(
          "  key=%s label=%s x=%d y=%d width=%d height=%d isPrimary=%d "
          "orientation=%s scalePercent=%d refreshHz=%d resolutionLabel=%s "
          "assignedPath=%s assignedExists=%d coverage=%s\n",
          m.value("key").toString().toUtf8().constData(),
          m.value("label").toString().toUtf8().constData(), m.value("x").toInt(),
          m.value("y").toInt(), m.value("width").toInt(),
          m.value("height").toInt(), m.value("isPrimary").toBool() ? 1 : 0,
          m.value("orientation").toString().toUtf8().constData(),
          m.value("scalePercent").toInt(), m.value("refreshHz").toInt(),
          m.value("resolutionLabel").toString().toUtf8().constData(),
          m.value("assignedPath").toString().toUtf8().constData(),
          m.value("assignedExists").toBool() ? 1 : 0,
          m.value("coverage").toString().toUtf8().constData());
    }

    // Assignment join + coverage passthrough in the pure builder.
    k6wp::DisplaysConfig assigned_store;
    assigned_store.assignments[primary.device_name] =
        k6wp::MonitorAssignment{L"C:\\Videos\\a.mp4", true};
    const std::map<std::string, std::string> coverage = {
        {"\\\\.\\DISPLAY2", "covered"}};
    const QVariantList joined =
        k6wp::BuildDisplayEntries(fixture, assigned_store, coverage);
    const QVariantMap j0 = joined.at(0).toMap();
    const QVariantMap j1 = joined.at(1).toMap();
    Check(HasSubstrQ(j0.value("assignedPath").toString(), "a.mp4"),
          "displays fixture: assignedPath joins the store for the key");
    Check(j0.value("assignedExists").toBool(),
          "displays fixture: assignedExists follows the store hint");
    Check(j1.value("assignedPath").toString().isEmpty() &&
              !j1.value("assignedExists").toBool(),
          "displays fixture: an unassigned monitor reports empty path");
    Check(j1.value("coverage").toString() == QStringLiteral("covered"),
          "displays fixture: coverage is an opaque passthrough token");

    // --- (iii) duplicateModeNotice: IS-7 collision ------------------------
    k6wp::MonitorInfo clone_b = primary;
    clone_b.id = 1;
    clone_b.is_primary = false;
    // Clone/duplicate mode: two HMONITORs report the same szDevice.
    const std::vector<k6wp::MonitorInfo> colliding_monitors = {primary,
                                                               clone_b};
    k6wp::DisplaysConfig colliding_store;
    colliding_store.assignments[primary.device_name] =
        k6wp::MonitorAssignment{L"C:\\Videos\\a.mp4", true};
    const auto colliding_keys =
        k6wp::DetectKeyCollision(colliding_store, colliding_monitors);
    Check(!colliding_keys.empty(),
          "duplicate collision: DetectKeyCollision reports the key");
    const QString notice = k6wp::DuplicateModeNoticeText(colliding_keys);
    Check(!notice.isEmpty(),
          "duplicateModeNotice non-empty for a colliding fixture");
    Check(HasSubstrQ(notice, "duplikat") || HasSubstrQ(notice, "Duplikat"),
          "duplicateModeNotice is the Indonesian IS-7 refusal sentence");
    Check(k6wp::DuplicateModeNoticeText({}).isEmpty(),
          "duplicateModeNotice empty when no keys collide");

    k6wp::MonitorInfo second;
    second.id = 1;
    second.x = 1920;
    second.y = 0;
    second.width = 1080;
    second.height = 1920;
    second.is_primary = false;
    second.device_name = L"\\\\.\\DISPLAY2";
    second.scale_pct = 125;
    k6wp::DisplaysConfig distinct_store;
    distinct_store.assignments[primary.device_name] =
        k6wp::MonitorAssignment{L"C:\\Videos\\a.mp4", true};
    distinct_store.assignments[second.device_name] =
        k6wp::MonitorAssignment{L"C:\\Videos\\b.mp4", true};
    const auto distinct_keys =
        k6wp::DetectKeyCollision(distinct_store, {primary, second});
    Check(distinct_keys.empty() && k6wp::DuplicateModeNoticeText(distinct_keys)
                                       .isEmpty(),
          "duplicateModeNotice empty for a non-colliding pair");

    // --- bridge instance over a redirected LOCALAPPDATA --------------------
    const std::wstring real_localappdata = LocalAppDataDir();
    QTemporaryDir fake_home;
    Check(fake_home.isValid(), "display-bridge temp dir is valid");
    const bool redirected =
        fake_home.isValid() && SetLocalAppDataDir(fake_home.path());
    Check(redirected, "display-bridge: LOCALAPPDATA redirected");
    if (redirected) {
      const QDir media(fake_home.path());
      Check(QDir().mkpath(media.filePath(QStringLiteral("K6WP"))),
            "display-bridge: fake K6WP data dir created");
      // Startup path: studio_settings.json with the update check OFF so the
      // ctor does not spawn a WinHTTP worker in this suite.
      {
        k6wp::StudioSettings s = k6wp::DefaultStudioSettings();
        s.check_updates = false;
        k6wp::SaveStudioSettings(k6wp::DefaultStudioSettingsPath(), s);
      }
      k6wp::StudioBridge bridge;
      Check(!bridge.displayCapability(),
            "displayCapability false before any successful get_state");

      int displays_changed = 0;
      QObject::connect(&bridge, &k6wp::StudioBridge::displaysChanged, &bridge,
                       [&displays_changed]() { ++displays_changed; });

      // (i) through the property: ApplyDisplayModel is the seam
      // refreshDisplays() uses; tests feed it a fixture monitor list.
      bridge.ApplyDisplayModel(fixture, empty_store, no_coverage);
      const QVariantList entries = bridge.displays();
      Check(entries.size() == 2, "bridge displays(): fixture yields two entries");
      const QVariantMap b0 = entries.at(0).toMap();
      const QVariantMap b1 = entries.at(1).toMap();
      Check(b0.value("x").toInt() == 0 && b0.value("width").toInt() == 1920 &&
                b0.value("isPrimary").toBool(),
            "bridge displays(): entry 0 x/y/w/h + isPrimary per MonitorInfo");
      Check(b1.value("x").toInt() == 1920 && b1.value("height").toInt() == 1920 &&
                b1.value("orientation").toString() ==
                    QStringLiteral("portrait") &&
                b1.value("scalePercent").toInt() == 125,
            "bridge displays(): portrait entry geometry + orientation + scale");
      Check(displays_changed >= 1, "ApplyDisplayModel emits displaysChanged");

      // (iii) through the bridge property.
      bridge.ApplyDisplayModel(colliding_monitors, colliding_store,
                               no_coverage);
      Check(!bridge.duplicateModeNotice().isEmpty(),
            "bridge duplicateModeNotice non-empty for colliding fixture");
      bridge.ApplyDisplayModel({primary, second}, distinct_store, no_coverage);
      Check(bridge.duplicateModeNotice().isEmpty(),
            "bridge duplicateModeNotice empty for non-colliding fixture");
      // Re-seat the fixture for the assign checks.
      bridge.ApplyDisplayModel(fixture, empty_store, no_coverage);

      // (iv) valid key: persist displays.json + emit displaysChanged.
      const QString key1 = QString::fromStdWString(primary.device_name);
      const QString video = media.filePath(QStringLiteral("wall.mp4"));
      {
        QFile f(video);
        Check(f.open(QIODevice::WriteOnly | QIODevice::Truncate),
              "assign: stand-in video file created");
        f.write("x");
        f.close();
      }
      const auto displays_path = k6wp::DefaultDisplaysPath();
      Check(!std::filesystem::exists(displays_path),
            "assign: displays.json absent before the first write");
      const int changed_before = displays_changed;
      bridge.assignVideoToMonitor(key1, video);
      Check(std::filesystem::exists(displays_path),
            "assign: displays.json written for a valid key");
      k6wp::DisplaysConfig expected;
      expected.assignments[primary.device_name] = k6wp::MonitorAssignment{
          video.toStdWString(), true};
      const auto expected_path =
          fake_home.filePath(QStringLiteral("expected-displays.json"));
      k6wp::SaveDisplays(ToPath(expected_path), expected);
      const auto got = ReadAllBytes(displays_path);
      const auto want = ReadAllBytes(ToPath(expected_path));
      Check(!got.empty() && got == want,
            "assign: displays.json byte-matches SaveDisplays of the expected "
            "store");
      Check(displays_changed > changed_before,
            "assign: displaysChanged emitted on a valid-key assign");
      Check(bridge.lastError().isEmpty(),
            "assign: a valid-key assign clears lastError");
      const QVariantMap after_assign = bridge.displays().at(0).toMap();
      Check(HasSubstrQ(after_assign.value("assignedPath").toString(),
                       "wall.mp4"),
            "assign: the model entry carries the new assignedPath");

      // QA-FAIL (unknown key): file unchanged, Indonesian lastError, no
      // displaysChanged.
      const auto before_unknown = ReadAllBytes(displays_path);
      const int changed_before_unknown = displays_changed;
      bridge.assignVideoToMonitor(QStringLiteral("\\\\.\\DISPLAY99"), video);
      Check(ReadAllBytes(displays_path) == before_unknown,
            "unknown-key assign: displays.json is byte-unchanged");
      Check(displays_changed == changed_before_unknown,
            "unknown-key assign: displaysChanged is NOT emitted");
      Check(!bridge.lastError().isEmpty(), "unknown-key assign: lastError is set");
      Check(HasSubstrQ(bridge.lastError(), "tidak dikenal"),
            "unknown-key assign: lastError is an Indonesian message");
      // QA-FAIL dump for the fail evidence file.
      const auto after_unknown = ReadAllBytes(displays_path);
      std::printf("QA-FAIL unknown-key assign:\n");
      std::printf("  key=\\\\.\\DISPLAY99 path=%s\n", video.toUtf8().constData());
      std::printf(
          "  displays.json bytes before=%zu after=%zu (unchanged=%d)\n",
          before_unknown.size(), after_unknown.size(),
          before_unknown == after_unknown ? 1 : 0);
      std::printf("  displaysChanged delta=%d (expected 0)\n",
                  displays_changed - changed_before_unknown);
      std::printf("  lastError=%s\n", bridge.lastError().toUtf8().constData());

      // clearMonitorAssignment: erase + persist + notify.
      const int changed_before_clear = displays_changed;
      bridge.clearMonitorAssignment(key1);
      k6wp::DisplaysConfig cleared;
      const auto expected_cleared =
          fake_home.filePath(QStringLiteral("expected-cleared.json"));
      k6wp::SaveDisplays(ToPath(expected_cleared), cleared);
      Check(ReadAllBytes(displays_path) == ReadAllBytes(ToPath(expected_cleared)),
            "clear: displays.json byte-matches an empty store");
      Check(displays_changed > changed_before_clear,
            "clear: displaysChanged emitted");

      // Live cross-check: refreshDisplays() rebuilds from ListMonitors() and
      // every entry's geometry matches the MonitorInfo that produced it.
      bridge.refreshDisplays();
      const auto live = k6wp::ListMonitors();
      const QVariantList live_entries = bridge.displays();
      Check(live_entries.size() == static_cast<int>(live.size()),
            "refreshDisplays: one entry per live monitor");
      bool geometry_ok = !live.empty();
      for (int i = 0; i < live_entries.size() &&
                      i < static_cast<int>(live.size());
           ++i) {
        const QVariantMap m = live_entries.at(i).toMap();
        const k6wp::MonitorInfo& info = live[static_cast<size_t>(i)];
        if (m.value("x").toInt() != info.x || m.value("y").toInt() != info.y ||
            m.value("width").toInt() != info.width ||
            m.value("height").toInt() != info.height ||
            m.value("isPrimary").toBool() != info.is_primary) {
          geometry_ok = false;
          break;
        }
      }
      Check(geometry_ok,
            "refreshDisplays: entry geometry matches ListMonitors()");
    }
    (void)SetLocalAppDataDir(QString::fromStdWString(real_localappdata));
  }

  // 22. Friendly status surface (plan todo 4 / B2 / brief C-3 + C-20 +
  //     glossary §5): StatusTitleFor / StatusVideoNameFor are the pure
  //     derivations the statusTitle / statusVideoName properties read.
  //     Covers every BridgeStatusKind mapping, the unknown-kind default
  //     (failure drill - the default branch is asserted explicitly), the
  //     empty-video-name contract, and detail passthrough.
  {
    using K = k6wp::BridgeStatusKind;
    const QString active_detail = QStringLiteral("Wallpaper aktif • a.mp4");
    const QString paused_detail = QStringLiteral("Dijeda — a.mp4");
    const QString degraded_detail =
        QStringLiteral("Wallpaper aktif tapi tak tampil • a.mp4");
    const QString idle_detail =
        QStringLiteral("Tidak aktif — pilih video untuk mulai");

    // --- each StatusKind mapping -------------------------------------------
    Check(k6wp::StatusTitleFor(K::kConnected, active_detail, true) ==
              QStringLiteral("Wallpaper aktif"),
          "statusTitle connected+video -> Wallpaper aktif");
    Check(k6wp::StatusTitleFor(K::kConnected, idle_detail, false) ==
              QStringLiteral("Tidak aktif"),
          "statusTitle connected without video -> Tidak aktif");
    Check(k6wp::StatusTitleFor(K::kPaused, paused_detail, true) ==
              QStringLiteral("Dijeda"),
          "statusTitle paused -> Dijeda");
    Check(k6wp::StatusTitleFor(K::kDegraded, degraded_detail, true) ==
              QStringLiteral("Wallpaper aktif"),
          "statusTitle degraded+video -> Wallpaper aktif");
    Check(k6wp::StatusTitleFor(K::kDegraded, idle_detail, false) ==
              QStringLiteral("Tidak aktif"),
          "statusTitle degraded without video -> Tidak aktif");
    Check(k6wp::StatusTitleFor(K::kNotRunning, idle_detail, false) ==
              QStringLiteral("Tidak aktif"),
          "statusTitle not-running -> Tidak aktif");
    Check(k6wp::StatusTitleFor(K::kDisconnected, idle_detail, false) ==
              QStringLiteral("Tidak aktif"),
          "statusTitle disconnected -> Tidak aktif");

    // --- video name derivation ---------------------------------------------
    Check(k6wp::StatusVideoNameFor(K::kConnected, active_detail, true) ==
              QStringLiteral("a.mp4"),
          "statusVideoName connected -> trailing file name");
    Check(k6wp::StatusVideoNameFor(K::kPaused, paused_detail, true) ==
              QStringLiteral("a.mp4"),
          "statusVideoName paused -> trailing file name");
    Check(k6wp::StatusVideoNameFor(K::kDegraded, degraded_detail, true) ==
              QStringLiteral("a.mp4"),
          "statusVideoName degraded -> trailing file name");

    // --- empty video name ---------------------------------------------------
    Check(k6wp::StatusVideoNameFor(K::kConnected, active_detail, false)
              .isEmpty(),
          "statusVideoName videoActive=false -> empty");
    Check(k6wp::StatusVideoNameFor(
              K::kPaused, QStringLiteral("Dijeda — (belum ada video aktif)"),
              false)
              .isEmpty(),
          "statusVideoName paused-without-video fallback -> empty");
    Check(k6wp::StatusVideoNameFor(K::kConnected,
                                   QStringLiteral("Wallpaper aktif • "),
                                   true)
              .isEmpty(),
          "statusVideoName trailing separator with no name -> empty");
    Check(k6wp::StatusVideoNameFor(K::kConnected, QString(), true).isEmpty(),
          "statusVideoName empty detail -> empty");

    // --- unknown-kind default + detail passthrough (failure drill) ----------
    const K unknown = static_cast<K>(99);
    Check(k6wp::StatusTitleFor(unknown, QStringLiteral("mentah"), true) ==
              QStringLiteral("mentah"),
          "statusTitle unknown kind + detail -> detail passthrough");
    Check(k6wp::StatusTitleFor(unknown, QString(), true) ==
              QStringLiteral("Tidak aktif"),
          "statusTitle unknown kind + empty detail -> default Tidak aktif");
    Check(k6wp::StatusVideoNameFor(unknown, active_detail, true) ==
              QStringLiteral("a.mp4"),
          "statusVideoName unknown kind still derives the name from detail");
  }

  // 22b. Invokable safety (todo 4 B3/B9): copyToClipboard must ignore empty
  //      text without crashing, and togglePause on a dead pipe must be a
  //      no-op (no IPC, no lastError). QCoreApplication has no
  //      QGuiApplication, so a non-null clipboard guard is the contract
  //      under test here - the real shell (QApplication) is covered by that
  //      same guard.
  {
    const std::wstring real_localappdata = LocalAppDataDir();
    QTemporaryDir fake_home;
    Check(fake_home.isValid(), "status-invokable temp dir is valid");
    const bool redirected =
        fake_home.isValid() && SetLocalAppDataDir(fake_home.path());
    Check(redirected, "status-invokable: LOCALAPPDATA redirected");
    if (redirected) {
      const QDir media(fake_home.path());
      Check(QDir().mkpath(media.filePath(QStringLiteral("K6WP"))),
            "status-invokable: fake K6WP data dir created");
      {
        k6wp::StudioSettings s = k6wp::DefaultStudioSettings();
        s.check_updates = false;
        k6wp::SaveStudioSettings(k6wp::DefaultStudioSettingsPath(), s);
      }
      k6wp::StudioBridge bridge;
      // Fresh bridge: default EngineStatusView kind is kDisconnected, so
      // the derived surface must already read as idle.
      Check(bridge.statusTitle() == QStringLiteral("Tidak aktif"),
            "fresh bridge statusTitle -> Tidak aktif (default kind)");
      Check(bridge.statusVideoName().isEmpty(),
            "fresh bridge statusVideoName -> empty");
      // Empty text must not reach the clipboard API (no crash).
      bridge.copyToClipboard(QString());
      // Non-empty with no QGuiApplication clipboard: silent no-op, no crash.
      bridge.copyToClipboard(QStringLiteral("Salin untuk dukungan"));
      // Dead pipe: togglePause is a no-op - nothing to toggle, no error.
      bridge.togglePause();
      Check(bridge.lastError().isEmpty(),
            "togglePause on a dead pipe leaves lastError empty (no-op)");
    }
    (void)SetLocalAppDataDir(QString::fromStdWString(real_localappdata));
  }

  // 23. Library card roles (plan todo 6 / brief B1): DisplayNameFor /
  //     MissingFor / OptimizedFor are the pure derivations the
  //     displayName/missing/optimized roles read. The legacy label keeps its
  //     badge text byte for byte; QML consumes the structured roles instead.
  //     Failure drill: a clean SMALL-res label must be optimized=false —
  //     optimized tracks the resolution floor, NOT the absence of a badge.
  {
    const std::string bullet = "\xE2\x80\xA2";   // •
    const std::string emdash = "\xE2\x80\x94";    // —

    // Threshold pinned (like kCompressFirstThresholdBytes): if someone
    // changes the floor, this suite must go red rather than flip polarity
    // silently.
    Check(k6wp::kOptimizedMinLongSide == 1280 &&
              k6wp::kOptimizedMinShortSide == 720,
          "roles: optimized floor pinned at 1280 long / 720 short");

    // Legacy role values must never be renumbered; new roles append after
    // kSizeRole.
    Check(k6wp::LibraryGridModel::kLabelRole == Qt::UserRole + 1 &&
              k6wp::LibraryGridModel::kThumbUrlRole == Qt::UserRole + 2 &&
              k6wp::LibraryGridModel::kSizeRole == Qt::UserRole + 8,
          "roles: legacy role values unchanged (UserRole+1..+8)");
    Check(k6wp::LibraryGridModel::kDisplayNameRole == Qt::UserRole + 9 &&
              k6wp::LibraryGridModel::kMissingRole == Qt::UserRole + 10 &&
              k6wp::LibraryGridModel::kOptimizedRole == Qt::UserRole + 11,
          "roles: new roles appended after kSizeRole (never renumbered)");

    // --- legacy EntryLabel bytes must stay identical -------------------------
    k6wp::LibraryEntry legacy;
    legacy.dst = std::filesystem::path(L"C:/lib/clip.mp4");
    legacy.res = "1920x1080";
    legacy.duration = 370.0;  // 6 mnt 10 dtk
    legacy.broken = true;
    const std::string legacy_broken = k6wp::EntryLabel(legacy);
    Check(legacy_broken == "clip.mp4\n1920x1080 " + bullet +
                               " 6 mnt 10 dtk\n[file hilang]",
          "roles: legacy label for a missing file is byte-identical");
    legacy.broken = false;
    legacy.src = legacy.dst;  // in-place reference
    legacy.duration = 0.0;
    const std::string legacy_inplace = k6wp::EntryLabel(legacy);
    Check(legacy_inplace == "clip.mp4\n1920x1080 " + bullet + " " + emdash +
                                "\n[belum dioptimasi]",
          "roles: legacy label for an in-place entry is byte-identical "
          "(em-dash + badge intact)");

    // --- DisplayNameFor ------------------------------------------------------
    const std::string badged =
        "clip.mp4\n1920x1080 " + bullet + " 6 mnt 10 dtk\n[file hilang]";
    const std::string unopt_badged =
        "clip.mp4\n640x480 " + bullet + " 30 dtk\n[belum dioptimasi]";
    const std::string clean =
        "clip.mp4\n1920x1080 " + bullet + " 6 mnt 10 dtk";
    const std::string no_meta = "clip.mp4\n" + emdash + " " + bullet + " " +
                                emdash;

    Check(k6wp::DisplayNameFor(badged) == clean,
          "roles: [file hilang] label -> displayName is the badge-free label");
    Check(!HasSubstr(k6wp::DisplayNameFor(badged), "[file hilang]"),
          "roles: displayName carries no [file hilang] text");
    Check(!HasSubstr(k6wp::DisplayNameFor(badged), "[belum dioptimasi]"),
          "roles: displayName carries no [belum dioptimasi] text");
    Check(k6wp::DisplayNameFor(unopt_badged) ==
              "clip.mp4\n640x480 " + bullet + " 30 dtk",
          "roles: [belum dioptimasi] label -> displayName strips that badge");
    Check(k6wp::DisplayNameFor(clean) == clean,
          "roles: a clean label is already its own displayName");
    Check(k6wp::DisplayNameFor(no_meta) == no_meta,
          "roles: em-dash metadata is formatting, not a badge - kept intact");
    Check(k6wp::DisplayNameFor("[file hilang]").empty(),
          "roles: a badge-only string -> empty displayName");
    Check(k6wp::DisplayNameFor("") == "",
          "roles: empty label -> empty displayName");

    // --- MissingFor ----------------------------------------------------------
    Check(k6wp::MissingFor(badged), "roles: [file hilang] label -> missing=true");
    Check(!k6wp::MissingFor(clean), "roles: clean label -> missing=false");
    Check(!k6wp::MissingFor(unopt_badged),
          "roles: [belum dioptimasi] badge is NOT missing");
    Check(!k6wp::MissingFor(no_meta),
          "roles: em-dash label is NOT missing");
    k6wp::LibraryEntry gone;
    gone.dst = std::filesystem::path(L"C:/lib/gone.mp4");
    gone.res = "1920x1080";
    gone.broken = true;
    Check(k6wp::MissingFor(gone), "roles: broken entry -> missing=true");
    gone.broken = false;
    Check(!k6wp::MissingFor(gone), "roles: intact entry -> missing=false");

    // --- OptimizedFor (polarity drill) ---------------------------------------
    // A label WITHOUT any badge but with a resolution below the floor must be
    // optimized=false. optimized tracks the resolution threshold — the absence
    // of [belum dioptimasi] does NOT mean optimized. Assert polarity twice so
    // an inverted implementation cannot sneak through on one leg.
    const std::string small_clean = "tiny.mp4\n640x480 " + bullet + " 30 dtk";
    Check(!HasSubstr(small_clean, "[belum dioptimasi]"),
          "roles: drill fixture has no badge at all");
    Check(k6wp::OptimizedFor(small_clean) == false,
          "roles: small clean label below floor -> optimized=false (polarity)");
    Check(k6wp::OptimizedFor("almost.mp4\n1279x719 " + bullet + " 1 mnt") ==
              false,
          "roles: 1279x719 one pixel below floor -> optimized=false");
    Check(k6wp::OptimizedFor("hd.mp4\n1280x720 " + bullet + " 1 mnt") == true,
          "roles: exactly 1280x720 at floor -> optimized=true");
    Check(k6wp::OptimizedFor(clean) == true,
          "roles: 1920x1080 at/above floor -> optimized=true");
    Check(k6wp::OptimizedFor(
              "portrait.mp4\n1080x1920 " + bullet + " 1 mnt") == true,
          "roles: portrait 1080x1920 counts via the long/short sides");
    Check(k6wp::OptimizedFor(no_meta) == false,
          "roles: em-dash res (unknown) -> optimized=false");
    Check(k6wp::OptimizedFor("") == false,
          "roles: empty label -> optimized=false");
    Check(k6wp::OptimizedFor("nolinebreak.mp4") == false,
          "roles: label without a res line -> optimized=false");
    Check(k6wp::OptimizedFor("max.mp4\n" + bullet + " 30 dtk") == false,
          "roles: filename x must not be parsed as a resolution");
    // OptimizedFor reads ONLY resolution: the missing-file badge does not
    // flip a 1080p verdict (missing is its own role, asserted above).
    Check(k6wp::OptimizedFor(badged) == true,
          "roles: 1920x1080 missing-file label keeps its resolution verdict");
    // Entry-shaped form (what data() reads).
    k6wp::LibraryEntry e;
    e.dst = std::filesystem::path(L"C:/lib/a.mp4");
    e.res = "640x480";
    Check(!k6wp::OptimizedFor(e), "roles: entry 640x480 -> optimized=false");
    e.res = "1920x1080";
    Check(k6wp::OptimizedFor(e), "roles: entry 1920x1080 -> optimized=true");
    e.res = "0x0";
    Check(!k6wp::OptimizedFor(e), "roles: unprobed 0x0 entry -> optimized=false");
    e.res = "";
    Check(!k6wp::OptimizedFor(e), "roles: empty res entry -> optimized=false");
    e.res = "abc";
    Check(!k6wp::OptimizedFor(e), "roles: garbage res entry -> optimized=false");

    // --- roleNames + data() wiring over a seeded library ---------------------
    QTemporaryDir role_dir;
    Check(role_dir.isValid(), "roles: temp library dir is valid");
    if (role_dir.isValid()) {
      const QDir media(role_dir.path());
      const QString video = WriteSizedFile(media, "ok.mp4", 4096);
      const auto lib_json =
          ToPath(media.filePath(QStringLiteral("library.json")));
      (void)SetEnvironmentVariableW(L"K6WP_LIBRARY_JSON",
                                    lib_json.wstring().c_str());

      // present: in-place reference (src==dst) -> [belum dioptimasi] badge,
      // 1080p -> optimized; gone: dst never written -> ListItems computes
      // broken=true -> [file hilang] + missing, 640x480 -> not optimized.
      k6wp::LibraryEntry present;
      present.src = present.dst = ToPath(video);
      present.res = "1920x1080";
      present.duration = 370.0;
      present.codec = "h264";
      present.width = 1920;
      present.height = 1080;
      k6wp::LibraryEntry vanished;
      vanished.src = vanished.dst =
          ToPath(media.filePath(QStringLiteral("vanished.mp4")));
      vanished.res = "640x480";
      vanished.duration = 30.0;
      vanished.codec = "h264";
      vanished.width = 640;
      vanished.height = 480;
      {
        k6wp::LibraryManager seed_mgr(lib_json);
        seed_mgr.Add(present);
        seed_mgr.Add(vanished);
      }

      k6wp::LibraryGridModel model;
      Check(model.count() == 2, "roles: seeded library yields 2 rows");

      const QHash<int, QByteArray> names = model.roleNames();
      Check(names.value(k6wp::LibraryGridModel::kLabelRole) == "label" &&
                names.value(k6wp::LibraryGridModel::kDisplayNameRole) ==
                    "displayName" &&
                names.value(k6wp::LibraryGridModel::kMissingRole) == "missing" &&
                names.value(k6wp::LibraryGridModel::kOptimizedRole) ==
                    "optimized",
            "roles: roleNames carries label + displayName + missing + optimized");

      // ListItems sorts by filename: ok.mp4 < vanished.mp4. Identify rows by
      // dst rather than assuming order.
      int visited = 0;
      for (int row = 0; row < model.count(); ++row) {
        const QModelIndex idx = model.index(row);
        const QString dst = model.dstAt(row);
        const QString label =
            model.data(idx, k6wp::LibraryGridModel::kLabelRole).toString();
        const QString display = model
                                    .data(idx, k6wp::LibraryGridModel::
                                                    kDisplayNameRole)
                                    .toString();
        const bool missing =
            model.data(idx, k6wp::LibraryGridModel::kMissingRole).toBool();
        const bool optimized =
            model.data(idx, k6wp::LibraryGridModel::kOptimizedRole).toBool();
        Check(!display.contains(QString::fromUtf8("[file hilang]")) &&
                  !display.contains(QString::fromUtf8("[belum dioptimasi]")),
              "roles: data() displayName never carries badge text");
        if (dst.contains(QStringLiteral("vanished"))) {
          ++visited;
          Check(label.contains(QString::fromUtf8("[file hilang]")),
                "roles: data() legacy label still shows the missing badge");
          Check(missing, "roles: data() missing=true for the vanished row");
          Check(!optimized,
                "roles: data() optimized=false for the 640x480 vanished row");
          Check(display ==
                    QString::fromStdString(k6wp::DisplayNameFor(
                        label.toStdString())),
                "roles: data() displayName matches the pure DisplayNameFor");
        } else {
          ++visited;
          Check(label.contains(QString::fromUtf8("[belum dioptimasi]")),
                "roles: data() legacy label still shows the in-place badge");
          Check(!missing, "roles: data() missing=false for the intact row");
          Check(optimized, "roles: data() optimized=true for the 1080p row");
        }
      }
      Check(visited == 2, "roles: both seeded rows visited");

      (void)SetEnvironmentVariableW(L"K6WP_LIBRARY_JSON", nullptr);
    }
  }

  // 24. Studio settings plan todo 8 (GAP-7): closeToTray / playlistSource /
  //     performancePreset are additive inside schema v3 with safe old-file
  //     defaults. A file written before they existed must load cleanly to
  //     the defaults; Save must write the keys; a fresh reload must return
  //     identical values; deleting ONE key from a saved file must still
  //     load that key at its default (the failure drill that proves
  //     old-file tolerance — production files look exactly like that);
  //     out-of-set enum values normalize instead of failing the load.
  {
    QTemporaryDir settings_dir;
    Check(settings_dir.isValid(), "todo8 settings temp dir is valid");
    const auto p = ToPath(
        settings_dir.filePath(QStringLiteral("studio_settings.json")));
    std::error_code ec;
    std::filesystem::create_directories(p.parent_path(), ec);

    // --- Old-file tolerance: every pre-todo-8 key present, none of the
    // three new ones (byte-shape of a real production file today).
    {
      const nlohmann::json old_file = {
          {"version", 3},
          {"auto_compress_on_import", true},
          {"compress_output_dir", "C:\\K6WP\\wallpapers"},
          {"default_crf", 22},
          {"default_fps", 30},
          {"default_resolution_mode", "match_monitor"},
          {"start_with_windows", false},
          {"cache_dir", "C:\\K6WP\\cache"},
          {"lockscreen_sync", false},
          {"lockscreen_offset_sec", 1.0},
          {"compress_advanced_visible", false},
          {"check_updates", true},
      };
      {
        std::ofstream out(p, std::ios::binary);
        out << old_file.dump(2);
      }
      k6wp::StudioSettings s;
      bool loaded = false;
      try {
        s = k6wp::LoadStudioSettings(p);
        loaded = true;
      } catch (...) {
        loaded = false;
      }
      Check(loaded, "todo8 old file without new keys loads cleanly");
      Check(s.version == 3, "todo8 old file stays at schema v3 (no bump)");
      Check(s.close_to_tray == false,
            "todo8 old file -> close_to_tray defaults false");
      Check(s.playlist_source == "all",
            "todo8 old file -> playlist_source defaults all");
      Check(s.performance_preset == "Seimbang",
            "todo8 old file -> performance_preset defaults Seimbang");
      // The loader self-heals: the rewritten file now carries the keys.
      {
        std::ifstream in(p, std::ios::binary);
        const std::string rewritten((std::istreambuf_iterator<char>(in)),
                                    std::istreambuf_iterator<char>());
        Check(HasSubstr(rewritten, "\"closeToTray\"") &&
                  HasSubstr(rewritten, "\"playlistSource\"") &&
                  HasSubstr(rewritten, "\"performancePreset\""),
              "todo8 self-heal rewrite writes the new keys");
      }
    }

    // --- Round-trip + cross-instance persistence: Save writes the keys,
    // a fresh Load returns identical values.
    {
      k6wp::StudioSettings s = k6wp::DefaultStudioSettings();
      s.close_to_tray = true;
      s.playlist_source = "custom";
      s.performance_preset = "Maksimal";
      k6wp::SaveStudioSettings(p, s);
      {
        std::ifstream in(p, std::ios::binary);
        const std::string text((std::istreambuf_iterator<char>(in)),
                               std::istreambuf_iterator<char>());
        Check(HasSubstr(text, "\"closeToTray\"") &&
                  HasSubstr(text, "\"playlistSource\"") &&
                  HasSubstr(text, "\"performancePreset\""),
              "todo8 save writes the three keys into the JSON");
        Check(HasSubstr(text, "\"playlistSource\": \"custom\"") &&
                  HasSubstr(text, "\"performancePreset\": \"Maksimal\""),
              "todo8 saved JSON carries the non-default values");
      }
      k6wp::StudioSettings back;
      bool loaded = false;
      try {
        back = k6wp::LoadStudioSettings(p);
        loaded = true;
      } catch (...) {
        loaded = false;
      }
      Check(loaded, "todo8 round-trip reload succeeds");
      Check(loaded && back.close_to_tray == true &&
                back.playlist_source == "custom" &&
                back.performance_preset == "Maksimal",
            "todo8 reload returns identical values (cross-instance)");
    }

    // --- Failure drill: delete ONE key from the saved file -> that key
    // loads at its default, the surviving keys keep their saved values.
    {
      nlohmann::json j;
      {
        std::ifstream in(p, std::ios::binary);
        j = nlohmann::json::parse(
            std::string((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>()));
      }
      j.erase("performancePreset");
      {
        std::ofstream out(p, std::ios::binary | std::ios::trunc);
        out << j.dump(2);
      }
      k6wp::StudioSettings s;
      bool loaded = false;
      try {
        s = k6wp::LoadStudioSettings(p);
        loaded = true;
      } catch (...) {
        loaded = false;
      }
      Check(loaded, "todo8 file with deleted key still loads");
      Check(s.performance_preset == "Seimbang",
            "todo8 deleted performancePreset key -> default Seimbang");
      Check(s.close_to_tray == true && s.playlist_source == "custom",
            "todo8 surviving keys keep their saved values");
    }

    // --- Validation: out-of-set enum values normalize to the safe defaults
    // (playlistSource anything else -> all; performancePreset anything else
    // -> Seimbang); a valid sibling key survives untouched.
    {
      nlohmann::json j = {
          {"version", 3},
          {"auto_compress_on_import", true},
          {"compress_output_dir", "C:\\K6WP\\wallpapers"},
          {"default_crf", 22},
          {"default_fps", 30},
          {"default_resolution_mode", "match_monitor"},
          {"start_with_windows", false},
          {"cache_dir", "C:\\K6WP\\cache"},
          {"lockscreen_sync", false},
          {"lockscreen_offset_sec", 1.0},
          {"compress_advanced_visible", false},
          {"check_updates", true},
          {"closeToTray", true},
          {"playlistSource", "bogus"},
          {"performancePreset", "Ultra"},
      };
      {
        std::ofstream out(p, std::ios::binary | std::ios::trunc);
        out << j.dump(2);
      }
      k6wp::StudioSettings s;
      bool loaded = false;
      try {
        s = k6wp::LoadStudioSettings(p);
        loaded = true;
      } catch (...) {
        loaded = false;
      }
      Check(loaded, "todo8 out-of-set enums still load cleanly");
      Check(s.playlist_source == "all",
            "todo8 playlistSource bogus -> normalized to all");
      Check(s.performance_preset == "Seimbang",
            "todo8 performancePreset bogus -> normalized to Seimbang");
      Check(s.close_to_tray == true,
            "todo8 valid closeToTray preserved alongside normalization");
    }
  }

  // 25. Settings.playlistSource (plan todo 9): Q_PROPERTY over the todo-8
  //     studio_settings key. Default "all"; set "custom" persists via
  //     apply(); unknown values are refused without moving the property.
  {
    const std::wstring real_localappdata = LocalAppDataDir();
    QTemporaryDir fake_home;
    Check(fake_home.isValid(), "playlistSource temp dir is valid");
    const bool redirected =
        fake_home.isValid() && SetLocalAppDataDir(fake_home.path());
    Check(redirected, "playlistSource: LOCALAPPDATA redirected");
    if (redirected) {
      const QDir media(fake_home.path());
      Check(QDir().mkpath(media.filePath(QStringLiteral("K6WP"))),
            "playlistSource: fake K6WP data dir created");
      {
        k6wp::SettingsBridge bridge;
        Check(bridge.playlistSource() == QStringLiteral("all"),
              "playlistSource defaults to all");
        bridge.setPlaylistSource(QStringLiteral("custom"));
        Check(bridge.playlistSource() == QStringLiteral("custom"),
              "setPlaylistSource custom accepted");
        bridge.setPlaylistSource(QStringLiteral("bogus"));
        Check(bridge.playlistSource() == QStringLiteral("custom"),
              "bogus playlistSource refused, property unchanged");
        Check(!bridge.lastError().isEmpty(),
              "bogus playlistSource sets lastError");
        bridge.apply();
        Check(bridge.lastError().isEmpty(),
              "apply after a valid set clears lastError");
      }
      {
        k6wp::SettingsBridge again;
        Check(again.playlistSource() == QStringLiteral("custom"),
              "playlistSource persists through apply + reload");
      }
    }
    (void)SetLocalAppDataDir(QString::fromStdWString(real_localappdata));
  }

  // 26. Studio.playlistLive* (plan todo 9): read-only get_state playlist
  //     snapshot. Pure ParseEngineState decode + bridge property defaults +
  //     ApplyPlaylistLive seam (the same shape as displayCapability).
  {
    // --- pure decode over fixture ack payloads --------------------------
    const nlohmann::json no_pl = {{"state", {{"pid", 7ULL}}}};
    k6wp::PlaylistLiveState live =
        k6wp::PlaylistLiveFromState(k6wp::ParseEngineState(no_pl));
    Check(!live.enabled && live.size == 0 && live.index == -1,
          "playlistLive defaults when get_state lacks the keys");

    const nlohmann::json with_pl = {
        {"state",
         {{"pid", 7ULL},
          {"playlist_enabled", true},
          {"playlist_size", 3},
          {"playlist_index", 1}}}};
    live = k6wp::PlaylistLiveFromState(k6wp::ParseEngineState(with_pl));
    Check(live.enabled && live.size == 3 && live.index == 1,
          "playlistLive parses enabled/size/index from the ack");

    const nlohmann::json wrong = {
        {"state",
         {{"pid", 7ULL},
          {"playlist_enabled", "yes"},
          {"playlist_size", "three"},
          {"playlist_index", 1.5}}}};
    live = k6wp::PlaylistLiveFromState(k6wp::ParseEngineState(wrong));
    Check(!live.enabled && live.size == 0 && live.index == -1,
          "playlistLive wrong-typed keys fall back to defaults");

    // --- bridge property defaults + ApplyPlaylistLive seam --------------
    const std::wstring real_localappdata = LocalAppDataDir();
    QTemporaryDir fake_home;
    Check(fake_home.isValid(), "playlistLive bridge temp dir is valid");
    const bool redirected =
        fake_home.isValid() && SetLocalAppDataDir(fake_home.path());
    Check(redirected, "playlistLive: LOCALAPPDATA redirected");
    if (redirected) {
      const QDir media(fake_home.path());
      Check(QDir().mkpath(media.filePath(QStringLiteral("K6WP"))),
            "playlistLive: fake K6WP data dir created");
      {
        k6wp::StudioSettings s = k6wp::DefaultStudioSettings();
        s.check_updates = false;
        k6wp::SaveStudioSettings(k6wp::DefaultStudioSettingsPath(), s);
      }
      k6wp::StudioBridge bridge;
      Check(!bridge.playlistLiveEnabled() && bridge.playlistLiveSize() == 0 &&
                bridge.playlistLiveIndex() == -1,
            "bridge playlistLive* start at the old-engine defaults");
      int status_repaints = 0;
      QObject::connect(&bridge, &k6wp::StudioBridge::engineStatusChanged,
                       &bridge, [&status_repaints]() { ++status_repaints; });
      bridge.ApplyPlaylistLive(k6wp::PlaylistLiveState{true, 3, 2});
      Check(bridge.playlistLiveEnabled() && bridge.playlistLiveSize() == 3 &&
                bridge.playlistLiveIndex() == 2,
            "ApplyPlaylistLive surfaces enabled/size/index on the bridge");
      Check(status_repaints >= 1,
            "ApplyPlaylistLive repaints via engineStatusChanged");
      bridge.ApplyPlaylistLive(k6wp::PlaylistLiveState{});
      Check(!bridge.playlistLiveEnabled() && bridge.playlistLiveSize() == 0 &&
                bridge.playlistLiveIndex() == -1,
            "ApplyPlaylistLive defaults reset the bridge properties");
    }
    (void)SetLocalAppDataDir(QString::fromStdWString(real_localappdata));
  }

  std::printf("checks=%d failures=%d\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
