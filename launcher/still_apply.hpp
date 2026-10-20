#pragma once

#include "cli.hpp"

namespace k6wp::launcher {

// K6WP.exe --apply-still ...: the one-shot per-user helper that renders the
// first frame of a video and applies it as the desktop wallpaper and/or the
// lock-screen image. See shared/still.hpp for the cross-process contract.
//
// This is the ONLY place that touches WinRT (LockScreen::SetImageFileAsync),
// and it is a short-lived child process - never the resident engine.
//
// Returns a process exit code: 0 when every requested side succeeded, 3 when
// the frame could not be produced or an apply/restore step failed.
int RunApplyStill(const Options& opts);

}  // namespace k6wp::launcher
