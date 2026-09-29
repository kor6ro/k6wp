#include "cache_manager.hpp"
#include "cli.hpp"
#include "encoder_detect.hpp"
#include "ffmpeg_job.hpp"
#include "ffmpeg_path.hpp"
#include "video_probe.hpp"

#include <windows.h>

#include <cstdio>
#include <string>
#include <vector>

namespace {

std::string JsonEscape(const std::string& s) {
  std::string out;
  out.reserve(s.size() + 8);
  for (const char c : s) {
    switch (c) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          char buf[8];
          std::snprintf(buf, sizeof(buf), "\\u%04x", c);
          out += buf;
        } else {
          out += c;
        }
    }
  }
  return out;
}

std::string Utf8FromWide(const std::wstring& w) {
  if (w.empty()) return {};
  const int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(),
                                    static_cast<int>(w.size()), nullptr, 0,
                                    nullptr, nullptr);
  if (n <= 0) return {};
  std::string out(static_cast<std::size_t>(n), '\0');
  WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()),
                      out.data(), n, nullptr, nullptr);
  return out;
}

std::filesystem::path DefaultWallpapersDir() {
  char* buf = nullptr;
  std::size_t len = 0;
  if (_dupenv_s(&buf, &len, "LOCALAPPDATA") == 0 && buf != nullptr) {
    std::string local(buf);
    std::free(buf);
    return std::filesystem::path(local) / "K6WP" / "wallpapers";
  }
  if (_dupenv_s(&buf, &len, "USERPROFILE") == 0 && buf != nullptr) {
    std::string profile(buf);
    std::free(buf);
    return std::filesystem::path(profile) / "AppData" / "Local" / "K6WP" / "wallpapers";
  }
  std::error_code ec;
  return std::filesystem::temp_directory_path(ec) / "K6WP" / "wallpapers";
}

std::filesystem::path DefaultLockscreenPath() {
  wchar_t buf[MAX_PATH];
  const DWORD n = GetEnvironmentVariableW(L"PROGRAMDATA", buf, MAX_PATH);
  if (n > 0 && n < MAX_PATH) {
    return std::filesystem::path(buf) / L"K6WP" / L"lockscreen.jpg";
  }
  std::error_code ec;
  return std::filesystem::temp_directory_path(ec) / L"K6WP" / L"lockscreen.jpg";
}

bool IsOptimal(const k6wp::compressor::VideoProps& props, int target_w, int target_h,
               int target_fps, const std::string&) {
  if (!props.valid) return false;
  if (props.video_codec != "h264") return false;
  if (props.has_audio) return false;
  if (target_w > 0 && target_h > 0) {
    if (props.width != target_w || props.height != target_h) return false;
  }
  if (target_fps > 0) {
    if (std::abs(props.fps - target_fps) > 1.0) return false;
  }
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  // DLL-planting hardening: per-user install dir is user-writable; restrict
  // DLL search to the application directory and System32 only. Ignore return
  // value explicitly.
  (void)SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
                                 LOAD_LIBRARY_SEARCH_SYSTEM32);
  // P1.5: run the whole process (and every child it spawns) at BELOW_NORMAL
  // priority. Never fatal: a failed class change logs and continues.
  if (!SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS)) {
    std::fprintf(stderr,
                 "warning: SetPriorityClass(BELOW_NORMAL_PRIORITY_CLASS) failed (error %lu)\n",
                 static_cast<unsigned long>(GetLastError()));
  } else {
    std::fprintf(stderr, "priority: BELOW_NORMAL_PRIORITY_CLASS set\n");
  }
  using k6wp::compressor::CliOptions;
  using k6wp::compressor::CliResult;

  CliOptions opts;
  std::string error;
  const CliResult result = k6wp::compressor::ParseCli(argc, argv, opts, error);

  if (result == CliResult::Help) {
    std::fputs(k6wp::compressor::CliUsage(), stdout);
    return 0;
  }
  if (result == CliResult::Error) {
    std::fprintf(stderr, "{\"error\":\"%s\"}\n", JsonEscape(error).c_str());
    return 2;
  }

  if (opts.probe_encoders) {
    std::vector<k6wp::compressor::EncoderInfo> encoders =
        k6wp::compressor::ListEncoders();
    for (auto& e : encoders) {
      e.works = k6wp::compressor::Probe1Frame(e.name);
    }
    const std::string chosen = k6wp::compressor::PickEncoder();

    std::printf("{\"encoders\":[");
    for (size_t i = 0; i < encoders.size(); ++i) {
      if (i > 0) std::printf(",");
      std::printf("{\"name\":\"%s\",\"label\":\"%s\",\"available\":%s,"
                  "\"works\":%s}",
                  JsonEscape(encoders[i].name).c_str(),
                  JsonEscape(encoders[i].label).c_str(),
                  encoders[i].available ? "true" : "false",
                  encoders[i].works ? "true" : "false");
    }
    std::printf("],\"chosen\":\"%s\"}\n", JsonEscape(chosen).c_str());
    return 0;
  }

  // Lockframe mode: extract single frame as lockscreen image
  if (opts.lockframe) {
    if (opts.out.empty()) {
      opts.out = DefaultLockscreenPath();
    }
    {
      std::error_code ec;
      if (!opts.out.parent_path().empty()) {
        std::filesystem::create_directories(opts.out.parent_path(), ec);
      }
    }
    const std::string encoder_name =
        k6wp::compressor::ResolveEncoderName(opts.encoder);
    k6wp::compressor::FfmpegJobOptions job;
    job.in = opts.in;
    job.out = opts.out;
    job.res_w = opts.res_w;
    job.res_h = opts.res_h;
    job.fps = opts.fps;
    job.crf = opts.crf;
    job.encoder = encoder_name;
    job.lockframe = true;
    job.offset_sec = opts.offset_sec;
    job.quality = opts.lockframe_q;

    std::string job_error;
    if (k6wp::compressor::RunFfmpegJob(job, job_error) != 0) {
      std::fprintf(stderr, "{\"error\":\"%s\"}\n",
                   JsonEscape(job_error).c_str());
      return 1;
    }
    std::printf("{\"ok\":true,\"out\":\"%s\",\"lockframe\":true}\n",
                JsonEscape(opts.out.u8string()).c_str());
    return 0;
  }

  if (opts.out.empty()) {
    const auto default_dir = DefaultWallpapersDir();
    std::error_code ec;
    std::filesystem::create_directories(default_dir, ec);
    opts.out = default_dir / opts.in.filename();
  }

  if (opts.dry_run) {
    std::printf("{\"ok\":true,\"in\":\"%s\",\"out\":\"%s\",\"dry_run\":true}\n",
                JsonEscape(opts.in.u8string()).c_str(),
                JsonEscape(opts.out.u8string()).c_str());
    const std::string encoder_name =
        k6wp::compressor::ResolveEncoderName(opts.encoder);
    k6wp::compressor::FfmpegJobOptions job;
    job.in = opts.in;
    job.out = opts.out;
    job.res_w = opts.res_w;
    job.res_h = opts.res_h;
    job.fps = opts.fps;
    job.crf = opts.crf;
    job.encoder = encoder_name;
    std::wstring cmdline;
    std::string job_error;
    if (!k6wp::compressor::BuildFfmpegCmdline(job, cmdline, job_error)) {
      std::fprintf(stderr, "{\"error\":\"%s\"}\n",
                   JsonEscape(job_error).c_str());
      return 1;
    }
    std::printf("%s\n", Utf8FromWide(cmdline).c_str());
    return 0;
  }

  const std::string encoder_name =
      k6wp::compressor::ResolveEncoderName(opts.encoder);
  k6wp::compressor::FfmpegJobOptions job;
  job.in = opts.in;
  job.out = opts.out;
  job.res_w = opts.res_w;
  job.res_h = opts.res_h;
  job.fps = opts.fps;
  job.crf = opts.crf;
  job.encoder = encoder_name;

  const k6wp::compressor::VideoProps props =
      k6wp::compressor::ProbeVideoProps(opts.in);
  if (IsOptimal(props, opts.res_w, opts.res_h, opts.fps, encoder_name)) {
    std::error_code ec;
    std::filesystem::copy_file(opts.in, opts.out,
                               std::filesystem::copy_options::overwrite_existing,
                               ec);
    if (ec) {
      std::fprintf(stderr, "{\"error\":\"%s\"}\n",
                   JsonEscape("optimal copy failed: " + ec.message()).c_str());
      return 1;
    }
    const std::string cache_key = k6wp::compressor::CacheKeyHex(
        opts.in, opts.res_w, opts.res_h, opts.fps, opts.crf, encoder_name);
    if (!cache_key.empty()) {
      std::string store_error;
      k6wp::compressor::CacheStoreFromOut(cache_key, opts.out, store_error);
    }
    std::printf(
        "{\"ok\":true,\"out\":\"%s\",\"encoder\":\"%s\",\"cache_hit\":false,"
        "\"skip_optimal\":true,\"cache_key\":\"%s\"}\n",
        JsonEscape(opts.out.u8string()).c_str(),
        JsonEscape(encoder_name).c_str(), cache_key.c_str());
    return 0;
  }

  const std::string cache_key = k6wp::compressor::CacheKeyHex(
      opts.in, opts.res_w, opts.res_h, opts.fps, opts.crf, encoder_name);
  if (!cache_key.empty() &&
      k6wp::compressor::CacheHas(cache_key)) {
    std::string cache_error;
    if (k6wp::compressor::CacheFetchToOut(cache_key, opts.out, cache_error) !=
        0) {
      std::fprintf(stderr, "{\"error\":\"%s\"}\n",
                   JsonEscape(cache_error).c_str());
      return 1;
    }
    std::printf(
        "{\"ok\":true,\"out\":\"%s\",\"encoder\":\"%s\",\"cache_hit\":true,"
        "\"cache_key\":\"%s\"}\n",
        JsonEscape(opts.out.u8string()).c_str(),
        JsonEscape(encoder_name).c_str(), cache_key.c_str());
    return 0;
  }

  std::string job_error;
  if (k6wp::compressor::RunFfmpegJob(job, job_error) != 0) {
    std::fprintf(stderr, "{\"error\":\"%s\"}\n",
                 JsonEscape(job_error).c_str());
    return 1;
  }
  if (!cache_key.empty()) {
    std::string store_error;
    if (k6wp::compressor::CacheStoreFromOut(cache_key, opts.out,
                                            store_error) != 0) {
      std::fprintf(stderr, "{\"error\":\"%s\"}\n",
                   JsonEscape(store_error).c_str());
      return 1;
    }
  }
  std::printf(
      "{\"ok\":true,\"out\":\"%s\",\"encoder\":\"%s\",\"cache_hit\":false,"
      "\"cache_key\":\"%s\"}\n",
      JsonEscape(opts.out.u8string()).c_str(),
      JsonEscape(encoder_name).c_str(), cache_key.c_str());
  return 0;
}