#include "ffprobe_helper.hpp"

#include "ffmpeg_path.hpp"
#include "proc_util.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "thirdparty/json.hpp"

// Task 23: production logging. Diagnostics mirror to stderr only in verbose
// builds (root CMake option K6WP_VERBOSE); default build stays console-clean.
#ifndef K6WP_VERBOSE
#define K6WP_VERBOSE 0
#endif

namespace k6wp {

namespace {

// MSVC-safe getenv wrapper (windows.h-free; _dupenv_s lives in <cstdlib>).
std::string GetEnvStr(const char* name) {
  char* buf = nullptr;
  std::size_t len = 0;
  if (_dupenv_s(&buf, &len, name) != 0 || buf == nullptr) {
    return "";
  }
  std::string value(buf);
  std::free(buf);
  return value;
}

void LogFfprobeError(const std::string& msg) {
#if K6WP_VERBOSE
  std::fputs(("[ffprobe] " + msg + "\n").c_str(), stderr);
  std::fflush(stderr);
#else
  (void)msg;  // production: console-clean; failures surface via return value
#endif
}

}  // namespace

FfprobeHelper::FfprobeHelper() : ffprobe_path_(FindFfprobe()) {}

bool FfprobeHelper::Probe(const std::filesystem::path& video,
                          VideoMetadata& out) const {
  try {
    std::error_code ec;
    if (!std::filesystem::is_regular_file(video, ec) || ec) {
      LogFfprobeError("not a regular file: " + video.u8string());
      return false;
    }
    if (ffprobe_path_.empty() ||
        !std::filesystem::exists(ffprobe_path_, ec) || ec) {
      LogFfprobeError("ffprobe not found");
      return false;
    }

    // ffprobe command to extract format + stream info in JSON:
    // ffprobe -v error -show_entries format=duration -show_entries
    // stream=codec_name,codec_type,width,height,r_frame_rate -of json
    // codec_type is required for the first-video-stream scan below.
    std::wstring cmdline = L"\"" + ffprobe_path_.wstring() + L"\" " +
                           L"-v error " +
                           L"-show_entries format=duration " +
                           L"-show_entries stream=codec_name,codec_type,width,height,r_frame_rate " +
                           L"-of json " +
                           L"\"" + video.wstring() + L"\"";

    // Spawn goes through the central k6wp::RunCaptured helper (single
    // spawn site, kProbeTimeoutMs budget, orphan-kill on timeout).
    const ProcResult rr =
        RunCaptured(ffprobe_path_, cmdline, kProbeTimeoutMs);
    if (!rr.spawned) {
      LogFfprobeError("probe spawn failed for: " + video.u8string());
      return false;
    }
    if (rr.timed_out) {
      LogFfprobeError("probe timed out for: " + video.u8string());
      return false;
    }

    if (rr.exit_code != 0) {
      LogFfprobeError("ffprobe exited with code " + std::to_string(rr.exit_code) +
                      " for: " + video.u8string());
      return false;
    }
    const std::string json_out = rr.output;

    // Parse JSON output (nlohmann/json via k6wp_shared thirdparty/json.hpp,
    // included at file top).
    nlohmann::json root;
    try {
      root = nlohmann::json::parse(json_out);
    } catch (const nlohmann::json::exception& e) {
      LogFfprobeError("JSON parse failed: " + std::string(e.what()) +
                      " for: " + video.u8string());
      return false;
    }

    // Extract duration from format. ffprobe JSON reports duration as a
    // string ("30.000000"), so accept string or number.
    double duration = 0.0;
    if (root.contains("format") && root["format"].is_object()) {
      const auto& fmt = root["format"];
      if (fmt.contains("duration")) {
        const auto& d = fmt["duration"];
        try {
          if (d.is_string()) {
            duration = std::stod(d.get<std::string>());
          } else if (d.is_number()) {
            duration = d.get<double>();
          }
        } catch (...) {
          duration = 0.0;
        }
      }
    }

    // Find the first video stream.
    std::string codec = "";
    int width = 0;
    int height = 0;
    double fps = 0.0;

    if (root.contains("streams") && root["streams"].is_array()) {
      for (const auto& stream : root["streams"]) {
        std::string codec_type = stream.value("codec_type", "");
        if (codec_type != "video") continue;

        codec = stream.value("codec_name", "");
        width = stream.value("width", 0);
        height = stream.value("height", 0);

        // r_frame_rate is a string like "30000/1001"
        std::string r_frame_rate = stream.value("r_frame_rate", "");
        auto slash_pos = r_frame_rate.find('/');
        if (slash_pos != std::string::npos) {
          try {
            double num = std::stod(r_frame_rate.substr(0, slash_pos));
            double den = std::stod(r_frame_rate.substr(slash_pos + 1));
            if (den > 0.0) fps = num / den;
          } catch (...) {
            fps = 0.0;
          }
        }
        break;  // first video stream only
      }
    }

    if (width == 0 || height == 0) {
      LogFfprobeError("no video stream found: " + video.u8string());
      return false;
    }

    out.duration = duration;
    out.codec = codec;
    out.width = width;
    out.height = height;
    out.fps = fps;
    return true;
  } catch (const std::exception& e) {
    LogFfprobeError(std::string("exception: ") + e.what() + " for: " +
                    video.u8string());
    return false;
  } catch (...) {
    LogFfprobeError("unknown exception for: " + video.u8string());
    return false;
  }
}

}  // namespace k6wp