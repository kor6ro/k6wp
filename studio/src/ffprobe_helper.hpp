#pragma once

// ffprobe metadata extraction for video files.
// Probes ONCE at import time: duration, codec, width, height, fps.
// Returns false on any failure (missing ffprobe, corrupt file, non-video).
// Header stays Win32-free; windows.h lives only in the .cpp.

#include <filesystem>
#include <string>

namespace k6wp {

struct VideoMetadata {
  double duration = 0.0;      // seconds
  std::string codec = "";     // e.g. "h264", "hevc", "vp9"
  int width = 0;              // video width in pixels
  int height = 0;             // video height in pixels
  double fps = 0.0;           // frames per second
};

class FfprobeHelper {
 public:
  FfprobeHelper();

  // Probes a video file for metadata. Returns false on ANY failure
  // (missing ffprobe, corrupt input, non-video, I/O error). Never throws.
  [[nodiscard]] bool Probe(const std::filesystem::path& video,
                           VideoMetadata& out) const;

 private:
  std::filesystem::path ffprobe_path_;
};

}  // namespace k6wp