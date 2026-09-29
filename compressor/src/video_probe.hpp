// video_probe.hpp — single-spawn ffprobe video probe (MED-9).
//
// ProbeVideoProps runs ONE `ffprobe -of json` invocation (format=duration +
// all streams) and parses it with nlohmann/json from k6wp_shared, mirroring
// studio/src/ffprobe_helper.cpp. The previous implementation spawned TWO
// ffprobe processes (video lines + audio lines) and parsed line-ordered
// `-of default=noprint_wrappers=1:nokey=1` output, which is order-fragile.
//
// ParseProbeJson is pure and order-independent (JSON keys, not line order)
// so it is unit-testable without an ffprobe binary.

#pragma once

#include <filesystem>
#include <string>

namespace k6wp::compressor {

struct VideoProps {
  std::string video_codec;
  int width = 0;
  int height = 0;
  double fps = 0.0;
  double duration = 0.0;  // seconds, from format.duration (new in MED-9;
                          // the old two-spawn probe never captured this)
  bool has_audio = false;
  bool valid = false;
};

// Parses `ffprobe -of json` output (format + streams) into props.
// Returns false on ANY bad input (empty/truncated/garbage JSON, no video
// stream, zero dimensions, missing codec). Never throws.
[[nodiscard]] bool ParseProbeJson(const std::string& json_text,
                                  VideoProps& out);

// Probes a video file with a single ffprobe spawn. Returns invalid props
// (valid == false) on ANY failure. Never throws.
[[nodiscard]] VideoProps ProbeVideoProps(const std::filesystem::path& in);

}  // namespace k6wp::compressor
