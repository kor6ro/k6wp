// engine/src/timer_ids.hpp
// Central registry of engine hidden-window timer IDs (SetTimer/KillTimer/
// WM_TIMER). Every timer ID in the engine lives here so a new timer can
// never silently collide with an existing one: the static_asserts below
// fail the build when any two IDs are equal.
//
// Windows.h-free (std types only) so config_watch.hpp can include it
// without dragging Win32 into its TU. Cast to UINT_PTR at the Win32 call
// sites (std::uintptr_t and UINT_PTR are the same type on MSVC x64).
#pragma once

#include <cstdint>

namespace k6wp {

// ConfigWatcher debounce timer ('K6WP'). 250 ms quiet-period debounce for
// the event-driven config watch (config_watch.hpp). Carried as uintptr_t
// and cast to UINT_PTR at the SetTimer/KillTimer/WM_TIMER sites.
inline constexpr std::uintptr_t kDebounceTimerId = 0x4B365750u;  // 'K6WP'

// P4.1: one-shot working-set trim timer ('K6WP'+1). Fires ~2 s after the
// first frame marker, kills itself, runs TrimWorkingSetOnce. Never
// periodic (periodic trim = page-in thrashing).
inline constexpr std::uintptr_t kWorkingSetTrimTimerId = 0x4B365751u;

// HOTFIX: debounced occlusion-poke timer ('K6WP'+2). Transient only (armed
// on destroy/foreground pokes, killed on fire) — never a periodic timer,
// so the paused zero-wakeup claim holds.
inline constexpr std::uintptr_t kOcclusionPokeTimerId = 0x4B365752u;

// Pairwise-distinct guarantee: an accidental duplicate timer ID fails the
// build instead of silently aliasing two timers on the same hidden window.
static_assert(kDebounceTimerId != kWorkingSetTrimTimerId,
              "engine timer IDs must be pairwise distinct");
static_assert(kDebounceTimerId != kOcclusionPokeTimerId,
              "engine timer IDs must be pairwise distinct");
static_assert(kWorkingSetTrimTimerId != kOcclusionPokeTimerId,
              "engine timer IDs must be pairwise distinct");

}  // namespace k6wp