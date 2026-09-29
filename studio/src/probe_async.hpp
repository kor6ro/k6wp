#pragma once

// Async ffprobe helper (Step 7.2): runs FfprobeHelper::Probe off the GUI
// thread via QtConcurrent::run and invokes the callback on the context
// object's thread. The ONLY synchronous FfprobeHelper::Probe call in GUI
// flows lives inside this worker; GUI slots never block on ffprobe.

#include <filesystem>
#include <functional>

#include "ffprobe_helper.hpp"

class QObject;

namespace k6wp {

void ProbeVideoAsync(const std::filesystem::path& path,
                     std::function<void(bool ok, VideoMetadata meta)> callback,
                     QObject* context);

}  // namespace k6wp
