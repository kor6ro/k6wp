#pragma once

// The unload state machine (task 1.1) - the PURE core.
//
// The engine's decode surface has three states:
//
//   kActive   - the file is decoded and shown.
//   kPaused   - the playback is paused (the picture stays on screen).
//   kUnloaded - the mpv instance is GONE (the RAM and the GPU are released).
//
// The transitions:
//
//   Active --(paused)--> Paused
//   Paused --(the deadline elapsed AND an owner hides the wallpaper)-->
//            Unloaded      (the engine unloads the decoder here)
//   Unloaded --(the mask is clear)--> Active
//                     (the engine reloads the decoder here)
//
// The rules that made the difference:
//
//   - an EXPLICIT user pause keeps the picture: the unload only runs for the
//     owners that make the wallpaper INVISIBLE (fullscreen, screen-off,
//     session-lock, suspend) - see PauseHidesWallpaper();
//   - the deadline is measured on the SAME clock as the pause start (the
//     steady clock), never on the loop's relative elapsed time;
//   - a resume reloads immediately (it must not wait for the next tick).
//
// The state, the clock and the deadline are PARAMETERS here, so the whole
// machine is a table of inputs -> outputs - which is what the golden
// fixtures (task G5) pin. The engine calls exactly these functions.
//
// windows.h-free: no clock, no Win32, no process.

#include "pause_controller.hpp"

namespace k6wp {

enum class DecodeState { kActive, kPaused, kUnloaded };

// The pure state of the machine.
struct UnloadMachine {
  DecodeState state = DecodeState::kActive;
  // When the current pause started (the engine clock, ms). 0 = not paused.
  long long pause_started_ms = 0;
  // The configured deadline: unload after this many ms of a continuous
  // pause. 0 = never unload.
  int unload_after_ms = 0;
};

// What the engine must do after a decision.
struct UnloadDecision {
  DecodeState state = DecodeState::kActive;
  long long pause_started_ms = 0;
  // The engine must destroy the decoder (and release the RAM + the GPU).
  bool unload_due = false;
  // The engine must create a fresh surface + decoder and re-issue the video.
  bool reload_due = false;
};

// The pause mask changed: `paused` is true when a slot-pausing owner is
// active.
//
// Pure: the reload itself talks to mpv, so the caller does it outside its
// lock (the decision says what to do, not how).
inline UnloadDecision UnloadOnPauseChanged(const UnloadMachine& machine,
                                            bool paused, long long now_ms) {
  UnloadDecision d;
  d.state = machine.state;
  d.pause_started_ms = machine.pause_started_ms;

  if (paused) {
    // A pause that is already running keeps its original start (the
    // deadline must not be pushed back by another owner).
    if (machine.state == DecodeState::kActive) {
      d.state = DecodeState::kPaused;
      d.pause_started_ms = now_ms;
    }
    return d;
  }

  // The mask is clear: the engine is active again. If the file was unloaded,
  // bring it back now (a resume must not wait for the next tick).
  d.reload_due = (machine.state == DecodeState::kUnloaded);
  d.state = DecodeState::kActive;
  d.pause_started_ms = 0;
  return d;
}

// The periodic deadline check (the engine calls it from its message loop).
//
// `mask` is the current pause mask: only the owners that make the wallpaper
// invisible qualify for the unload.
inline UnloadDecision UnloadOnTick(const UnloadMachine& machine, int mask,
                                   long long now_ms) {
  UnloadDecision d;
  d.state = machine.state;
  d.pause_started_ms = machine.pause_started_ms;

  if (machine.unload_after_ms <= 0) return d;
  if (machine.state != DecodeState::kPaused) return d;
  if (machine.pause_started_ms <= 0) return d;
  if (now_ms - machine.pause_started_ms < machine.unload_after_ms) return d;
  // Only the owners that make the wallpaper invisible qualify: an explicit
  // user pause keeps the picture.
  if (!PauseHidesWallpaper(mask)) return d;

  d.state = DecodeState::kUnloaded;
  d.unload_due = true;
  return d;
}

// The stable name of a state (the log surface).
inline const char* DecodeStateName(DecodeState state) {
  switch (state) {
    case DecodeState::kActive:
      return "active";
    case DecodeState::kPaused:
      return "paused";
    case DecodeState::kUnloaded:
      return "unloaded";
  }
  return "active";
}

}  // namespace k6wp
