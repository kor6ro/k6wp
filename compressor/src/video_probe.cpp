// video_probe.cpp — single-spawn ffprobe video probe (MED-9).
//
// Pattern mirrors studio/src/ffprobe_helper.cpp::Probe: one
// `ffprobe -v error -show_entries format=duration -show_entries
// stream=codec_name,codec_type,width,height,r_frame_rate -of json`
// invocation, 10 s wait with TerminateProcess on timeout, exit-code check,
// then nlohmann/json parse. codec_type selects the first video stream and
// detects audio (replaces the old second `-select_streams a` spawn).

#include "video_probe.hpp"

#include "ffmpeg_path.hpp"
#include "proc_util.hpp"

#include "thirdparty/json.hpp"

namespace k6wp::compressor {

bool ParseProbeJson(const std::string& json_text, VideoProps& out) {
  out = VideoProps{};
  try {
    nlohmann::json root;
    try {
      root = nlohmann::json::parse(json_text);
    } catch (const nlohmann::json::exception&) {
      return false;
    }
    if (!root.is_object()) return false;

    // Duration from format. ffprobe reports it as a string ("12.500000");
    // accept a bare number too.
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

    // First video stream wins (old `-select_streams v:0` equivalent);
    // any audio stream sets has_audio (old `-select_streams a` equivalent).
    std::string codec;
    int width = 0;
    int height = 0;
    double fps = 0.0;
    bool saw_audio = false;
    bool found_video = false;

    if (root.contains("streams") && root["streams"].is_array()) {
      for (const auto& stream : root["streams"]) {
        if (!stream.is_object()) continue;
        const std::string codec_type = stream.value("codec_type", "");
        if (codec_type == "audio") {
          saw_audio = true;
          continue;
        }
        if (codec_type != "video" || found_video) continue;

        codec = stream.value("codec_name", "");
        width = stream.value("width", 0);
        height = stream.value("height", 0);

        // r_frame_rate is a string like "30000/1001".
        const std::string r_frame_rate = stream.value("r_frame_rate", "");
        const auto slash_pos = r_frame_rate.find('/');
        if (slash_pos != std::string::npos) {
          try {
            const double num = std::stod(r_frame_rate.substr(0, slash_pos));
            const double den = std::stod(r_frame_rate.substr(slash_pos + 1));
            if (den > 0.0) fps = num / den;
          } catch (...) {
            fps = 0.0;
          }
        }
        found_video = true;
      }
    }

    // Same validity bar as the old probe: codec + positive dimensions.
    if (!found_video || codec.empty() || width <= 0 || height <= 0) {
      return false;
    }

    out.video_codec = codec;
    out.width = width;
    out.height = height;
    out.fps = fps;
    out.duration = duration;
    out.has_audio = saw_audio;
    out.valid = true;
    return true;
  } catch (...) {
    out = VideoProps{};
    return false;
  }
}

VideoProps ProbeVideoProps(const std::filesystem::path& in) {
  VideoProps props;
  try {
    std::filesystem::path ffprobe_path;
    try {
      ffprobe_path = ::k6wp::FindFfprobe();
    } catch (...) {
    }
    if (ffprobe_path.empty()) return props;

    std::wstring cmdline = L"\"" + ffprobe_path.wstring() + L"\" " +
                           L"-v error " +
                           L"-show_entries format=duration " +
                           L"-show_entries "
                           L"stream=codec_name,codec_type,width,height,r_frame_rate "
                           L"-of json " +
                           L"\"" + in.wstring() + L"\"";

    // Single-spawn policy lives in k6wp::RunCaptured (central spawn site,
    // kProbeTimeoutMs budget, orphan-kill on timeout); this TU keeps only
    // the command line + JSON parse.
    const ::k6wp::ProcResult rr = ::k6wp::RunCaptured(
        ffprobe_path, cmdline, ::k6wp::kProbeTimeoutMs);
    if (!rr.spawned || rr.timed_out || rr.exit_code != 0) return props;

    (void)ParseProbeJson(rr.output, props);
    return props;
  } catch (...) {
    return VideoProps{};
  }
}

}  // namespace k6wp::compressor
