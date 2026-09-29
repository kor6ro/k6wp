#pragma once

// Central ffmpeg/ffprobe path resolution (Step 1.1).
// Header stays Win32-free (same pattern as config_schema.hpp / thumbnailer.hpp);
// windows.h lives only in ffmpeg_path.cpp.

#include <filesystem>

namespace k6wp {

// Resolves bundled ffmpeg.exe. Candidate order:
//   (a) %K6WP_FFMPEG% when it exists;
//   (b) <exe_dir>\ffmpeg.exe (FLAT — release/portable/installer layout);
//   (c) <exe_dir>\..\..\vendor\ffmpeg\ffmpeg.exe (build layout);
//   (d) <exe_dir>\vendor\ffmpeg\ffmpeg.exe.
// Returns an empty path when all candidates are absent. Never throws.
std::filesystem::path FindFfmpeg();

// Resolves bundled ffprobe.exe. Same order with %K6WP_FFPROBE% / ffprobe.exe.
// Returns an empty path when all candidates are absent. Never throws.
std::filesystem::path FindFfprobe();

}  // namespace k6wp
