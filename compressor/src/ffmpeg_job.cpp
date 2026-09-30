#include "ffmpeg_job.hpp"

#include "encoder_detect.hpp"
#include "ffmpeg_path.hpp"

#include <windows.h>

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace k6wp::compressor {
namespace {

// ---- cancel handling (Ctrl+C kills ffmpeg, partial deleted by caller) ----

std::atomic<bool> g_cancel{false};
std::atomic<bool> g_job_active{false};

BOOL WINAPI CancelHandler(DWORD ctrl_type) {
  if ((ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_BREAK_EVENT ||
       ctrl_type == CTRL_CLOSE_EVENT) &&
      g_job_active.load(std::memory_order_acquire)) {
    g_cancel.store(true, std::memory_order_release);
    return TRUE;  // swallow; the wait loop kills ffmpeg and we exit cleanly
  }
  return FALSE;
}

// ---- friendly error formatting ----

std::string FriendlyId(const char* id) {
  if (std::strcmp(id, "ffmpeg_not_found") == 0)
    return "ffmpeg tidak ditemukan di folder aplikasi. Ekstrak ulang atau pasang ulang K6WP.";
  if (std::strcmp(id, "input_not_found") == 0)
    return "File input tidak ditemukan.";
  if (std::strcmp(id, "launch_failed") == 0)
    return "Gagal menjalankan ffmpeg.";
  if (std::strcmp(id, "cancelled") == 0)
    return "Kompresi dibatalkan oleh pengguna.";
  if (std::strcmp(id, "timeout") == 0)
    return "Kompresi melebihi batas waktu.";
  if (std::strcmp(id, "ffmpeg_failed") == 0)
    return "Kompresi gagal. Periksa file input dan coba lagi.";
  return "Terjadi kesalahan tidak diketahui.";
}

std::string FormatError(const char* friendly_id, const std::string& technical_detail) {
  return std::string("{\"friendly\":\"") + FriendlyId(friendly_id) +
         "\",\"technical\":\"" + technical_detail + "\"}";
}

// ---- RAII handle guard ----

class HandleGuard {
 public:
  explicit HandleGuard(HANDLE h = nullptr) : h_(h) {}
  ~HandleGuard() { reset(); }
  HandleGuard(const HandleGuard&) = delete;
  HandleGuard& operator=(const HandleGuard&) = delete;
  HandleGuard(HandleGuard&& o) noexcept : h_(o.h_) { o.h_ = nullptr; }
  HandleGuard& operator=(HandleGuard&& o) noexcept {
    if (this != &o) {
      reset();
      h_ = o.h_;
      o.h_ = nullptr;
    }
    return *this;
  }
  HANDLE get() const { return h_; }
  HANDLE* out() { return &h_; }
  void reset(HANDLE h = nullptr) {
    if (h_ != nullptr && h_ != INVALID_HANDLE_VALUE) CloseHandle(h_);
    h_ = h;
  }

 private:
  HANDLE h_;
};

// ---- ffmpeg path resolution (centralized in k6wp_shared) ----

std::wstring FindFfmpeg() {
  try {
    const std::filesystem::path p = ::k6wp::FindFfmpeg();
    if (p.empty()) return L"";
    return p.wstring();
  } catch (...) {
    return L"";
  }
}

// Decode UTF-8 to UTF-16 (MED-10). CP_UTF8 with dwFlags=0 is deliberately
// lenient (invalid sequences -> U+FFFD, never a hard failure) so a bad byte
// can't drop the whole command line; the old byte-wise widen sign-extended
// every byte >= 0x80 (0xC3 -> U+FFC3) and never decoded multibyte sequences.
std::wstring Widen(const std::string& s) {
  if (s.empty()) return L"";
  const int n = ::MultiByteToWideChar(CP_UTF8, 0, s.data(),
                                      static_cast<int>(s.size()), nullptr, 0);
  if (n <= 0) return L"";
  std::wstring out(static_cast<size_t>(n), L'\0');
  ::MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()),
                        out.data(), n);
  return out;
}

// ---- progress parsing ----

// Parse "HH:MM:SS.xx" (ffmpeg Duration / out_time) into microseconds.
// Returns -1 on failure.
long long ParseTimestampUs(const char* s) {
  int h = 0, m = 0;
  double sec = 0.0;
  if (::sscanf_s(s, "%d:%d:%lf", &h, &m, &sec) != 3) return -1;
  if (h < 0 || m < 0 || sec < 0.0) return -1;
  return static_cast<long long>((h * 3600.0 + m * 60.0 + sec) * 1000000.0);
}

bool StartsWith(const std::string& line, const char* prefix) {
  return line.compare(0, std::strlen(prefix), prefix) == 0;
}

}  // namespace

std::string ResolveEncoderName(const std::string& label_or_auto) {
  if (label_or_auto == "auto") return PickEncoder();
  if (label_or_auto == "nvenc") return "h264_nvenc";
  if (label_or_auto == "qsv") return "h264_qsv";
  if (label_or_auto == "amf") return "h264_amf";
  if (label_or_auto == "x264") return "libx264";
  return label_or_auto;  // already a full ffmpeg encoder name
}

bool BuildFfmpegCmdline(const FfmpegJobOptions& opts, std::wstring& cmdline,
                        std::string& error) {
  const std::wstring ffmpeg = FindFfmpeg();
  if (ffmpeg.empty()) {
    error = FormatError("ffmpeg_not_found",
                        "ffmpeg.exe not found (K6WP_FFMPEG or vendor/ffmpeg/ffmpeg.exe)");
    return false;
  }
  if (!std::filesystem::exists(opts.in)) {
    error = FormatError("input_not_found",
                        "input file does not exist: " + opts.in.u8string());
    return false;
  }

  std::filesystem::path actual_out = opts.out;
  std::filesystem::path tmp_out;
  if (opts.lockframe) {
    tmp_out = opts.out;
    tmp_out += L".tmp";
    actual_out = tmp_out;
  }

  cmdline = L"\"" + ffmpeg + L"\" -hide_banner -y";

  // Lockframe mode: extract single frame as JPEG
  if (opts.lockframe) {
    std::string vf;
    if (opts.res_w > 0 && opts.res_h > 0) {
      vf = "scale=" + std::to_string(opts.res_w) + ":" +
           std::to_string(opts.res_h);
    }
    std::string seek = "-ss " + std::to_string(opts.offset_sec) + " ";
    cmdline += L" " + Widen(seek) +
               L" -i \"" + opts.in.wstring() + L"\"";
    if (!vf.empty()) {
      cmdline += L" -vf \"" + Widen(vf) + L"\"";
    }
    cmdline += L" -frames:v 1 -q:v " + std::to_wstring(opts.quality) +
               L" \"" + actual_out.wstring() + L"\"";
  } else {
    // Normal compression mode
    const std::string enc = ResolveEncoderName(opts.encoder);

    // Per-encoder quality flags. CRF range 16-28 is enforced by the CLI.
    std::string quality;
    // DPB pinning: wallpaper loops gain nothing from deep reference
    // lists, but the decoder's DXVA surface pool scales with the SPS
    // num_ref_frames. refs=2/bf=1 keeps the pool minimal (A/B 2026-09-20
    // targets -50..-150 MB system RAM @4K decode). NVENC + libx264 only:
    // the QSV/AMF legs never run on this machine, so their equivalent
    // flags stay unset rather than risk an untested encode failure.
    std::string dpb;
    // Decode-friendly tuning: libx264 only (the x264-style CRF leg).
    // NVENC/QSV/AMF stay untouched — -tune is an x264 option.
    std::string tune;
    if (enc == "h264_nvenc") {
      quality = "-cq " + std::to_string(opts.crf) + " -preset p4";
      dpb = "-refs 2 -bf 1";
    } else if (enc == "h264_qsv") {
      quality = "-global_quality " + std::to_string(opts.crf);
    } else if (enc == "h264_amf") {
      quality = "-qp_i " + std::to_string(opts.crf) + " -qp_p " +
                std::to_string(opts.crf);
    } else {
      // libx264 and any unknown encoder: x264-style CRF.
      quality = "-crf " + std::to_string(opts.crf) + " -preset veryfast";
      dpb = "-x264-params ref=2 -bf 1";
      tune = "-tune fastdecode";
    }

    // Video filter: exact scale when --res was given, else keep source size.
    // fps is always applied (CLI caps it at 30).
    std::string vf;
    if (opts.res_w > 0 && opts.res_h > 0) {
      vf = "scale=" + std::to_string(opts.res_w) + ":" +
           std::to_string(opts.res_h) + ",fps=" + std::to_string(opts.fps);
    } else {
      vf = "fps=" + std::to_string(opts.fps);
    }

    // Thread count: N = max(1, logical processors - 1), runtime-computed
    // (P1.4). Harmless for hw encoders, bounds libx264's frame threads.
    SYSTEM_INFO si{};
    GetSystemInfo(&si);
    const int n = static_cast<int>(si.dwNumberOfProcessors) - 1;
    const int threads = n < 1 ? 1 : n;

    std::wstring extra;
    if (!tune.empty()) extra = L" " + Widen(tune);
    cmdline += L" -i \"" + opts.in.wstring() + L"\" -vf \"" +
               Widen(vf) + L"\" -an -c:v " + Widen(enc) + L" " + Widen(quality) +
               L" " + Widen(dpb) + extra +
               L" -threads " + std::to_wstring(threads) +
               L" -pix_fmt yuv420p -profile:v high -movflags +faststart \"" +
               actual_out.wstring() + L"\" -progress - -nostats";
  }
  return true;
}

int RunFfmpegJob(const FfmpegJobOptions& opts, std::string& error) {
  std::wstring cmdline;
  if (!BuildFfmpegCmdline(opts, cmdline, error)) return 1;
  const std::wstring ffmpeg = FindFfmpeg();
  std::filesystem::path tmp_out;
  if (opts.lockframe) {
    tmp_out = opts.out;
    tmp_out += L".tmp";
  }

  SECURITY_ATTRIBUTES sa{};
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;
  HandleGuard read_pipe;
  HANDLE write_raw = nullptr;
  if (!CreatePipe(read_pipe.out(), &write_raw, &sa, 0)) {
    error = FormatError("launch_failed", "CreatePipe failed");
    return 1;
  }
  HandleGuard write_pipe(write_raw);

  STARTUPINFOW si{};
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdOutput = write_pipe.get();
  si.hStdError = write_pipe.get();
  si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

  PROCESS_INFORMATION pi{};
  std::vector<wchar_t> cmd_buf(cmdline.begin(), cmdline.end());
  cmd_buf.push_back(L'\0');
  // lpApplicationName = exe path avoids cmd.exe quote-stripping (Todo 14).
  const BOOL ok =
      CreateProcessW(ffmpeg.c_str(), cmd_buf.data(), nullptr, nullptr, TRUE,
                     CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
  write_pipe.reset();  // parent drops the write end so EOF works
  if (!ok) {
    error = FormatError("launch_failed", "failed to launch ffmpeg");
    return 1;
  }
  HandleGuard proc(pi.hProcess);
  CloseHandle(pi.hThread);

  g_cancel.store(false, std::memory_order_release);
  g_job_active.store(true, std::memory_order_release);
  SetConsoleCtrlHandler(CancelHandler, TRUE);

  const ULONGLONG start_ms = GetTickCount64();
  ULONGLONG last_emit_ms = 0;
  const ULONGLONG timeout_ms =
      opts.timeout_sec > 0.0
          ? start_ms + static_cast<ULONGLONG>(opts.timeout_sec * 1000.0)
          : 0;
  bool timed_out = false;

  std::string acc;  // unparsed pipe bytes
  long long total_us = -1;
  long long done_us = 0;
  bool end_seen = false;

  // Slice the wait so Ctrl+C/timeout/progress stay responsive.
  for (;;) {
    DWORD avail = 0;
    if (PeekNamedPipe(read_pipe.get(), nullptr, 0, nullptr, &avail, nullptr) &&
        avail > 0) {
      char tmp[4096];
      DWORD n = 0;
      if (ReadFile(read_pipe.get(), tmp, sizeof(tmp), &n, nullptr) && n > 0) {
        acc.append(tmp, n);
        // Drain complete lines.
        for (;;) {
          const auto nl = acc.find('\n');
          if (nl == std::string::npos) break;
          std::string line = acc.substr(0, nl);
          acc.erase(0, nl + 1);
          if (!line.empty() && line.back() == '\r') line.pop_back();
          // ffmpeg indents this line ("  Duration: ..."), so substring-match.
          const auto dur_pos = line.find("Duration: ");
          if (total_us < 0 && dur_pos != std::string::npos) {
            const long long t =
                ParseTimestampUs(line.c_str() + dur_pos + 10);
            if (t > 0) total_us = t;
          } else if (StartsWith(line, "out_time_ms=")) {
            // NOTE: despite the name, ffmpeg reports MICROseconds here.
            char* e = nullptr;
            const long long v = std::strtoll(line.c_str() + 12, &e, 10);
            if (e != line.c_str() + 12 && v >= 0) done_us = v;
          } else if (StartsWith(line, "out_time_us=")) {
            char* e = nullptr;
            const long long v = std::strtoll(line.c_str() + 12, &e, 10);
            if (e != line.c_str() + 12 && v >= 0) done_us = v;
          } else if (StartsWith(line, "out_time=")) {
            const long long t = ParseTimestampUs(line.c_str() + 9);
            if (t >= 0) done_us = t;
          } else if (line == "progress=end") {
            end_seen = true;
          }
        }
      }
    }

    const ULONGLONG now_ms = GetTickCount64();
    if (total_us > 0 && now_ms - last_emit_ms >= 500) {
      last_emit_ms = now_ms;
      int pct = static_cast<int>(done_us * 100 / total_us);
      if (pct < 0) pct = 0;
      if (pct > 100) pct = 100;
      long long eta_s = -1;
      const double elapsed_s = (now_ms - start_ms) / 1000.0;
      if (done_us > 0 && done_us < total_us && elapsed_s > 0.0) {
        eta_s = static_cast<long long>(elapsed_s * (total_us - done_us) /
                                       done_us);
      } else if (done_us >= total_us) {
        eta_s = 0;
      }
      std::printf("{\"progress\":%d,\"eta_s\":%lld}\n", pct, eta_s);
      std::fflush(stdout);
    }

    const DWORD w = WaitForSingleObject(proc.get(), 100);
    if (w == WAIT_OBJECT_0) break;  // ffmpeg exited
    if (g_cancel.load(std::memory_order_acquire)) break;
    if (timeout_ms != 0 && GetTickCount64() >= timeout_ms) {
      timed_out = true;
      break;
    }
  }

  const bool cancelled =
      g_cancel.load(std::memory_order_acquire) || timed_out;
  if (cancelled) {
    TerminateProcess(proc.get(), 1);
    WaitForSingleObject(proc.get(), 1000);  // fast cancel: ~1s max
  } else {
    // Exited on its own — drain any trailing pipe bytes before exit code.
    for (;;) {
      char tmp[4096];
      DWORD n = 0;
      if (!ReadFile(read_pipe.get(), tmp, sizeof(tmp), &n, nullptr) || n == 0)
        break;
      acc.append(tmp, n);
    }
  }

  DWORD exit_code = 0;
  GetExitCodeProcess(proc.get(), &exit_code);

  SetConsoleCtrlHandler(CancelHandler, FALSE);
  g_job_active.store(false, std::memory_order_release);

  auto remove_partial = [&] {
    std::error_code ec;
    if (opts.lockframe) {
      std::filesystem::remove(tmp_out, ec);
    } else {
      std::filesystem::remove(opts.out, ec);
    }
  };

  if (cancelled) {
    remove_partial();
    error = timed_out ? FormatError("timeout", "ffmpeg job timed out")
                      : FormatError("cancelled", "ffmpeg job cancelled");
    return 1;
  }
  if (exit_code != 0) {
    remove_partial();
    char buf[64];
    std::snprintf(buf, sizeof(buf), "ffmpeg exited with code %lu",
                  static_cast<unsigned long>(exit_code));
    error = FormatError("ffmpeg_failed", buf);
    return 1;
  }

  if (end_seen) {
    done_us = total_us > 0 ? total_us : done_us;
  }
  std::printf("{\"progress\":100,\"eta_s\":0}\n");
  std::fflush(stdout);

  // Atomic move for lockframe mode: replace target with temp file.
  if (opts.lockframe) {
    if (!MoveFileExW(tmp_out.c_str(), opts.out.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
      error = FormatError("ffmpeg_failed",
                          "MoveFileEx failed (error " +
                              std::to_string(GetLastError()) + ")");
      std::error_code ec;
      std::filesystem::remove(tmp_out, ec);
      return 1;
    }
  }

  return 0;
}

}  // namespace k6wp::compressor
