#pragma once

// Pure desktop-placement geometry (plan row 3).
//
// Extracts the coordinate math inlined in
// DesktopInjector::CreateAndAttach (desktop_inject.cpp:261-265) into
// headless-testable helpers over plain POD rects. The system clips a child
// window to its parent ("No part of a child window ever appears outside the
// borders of its parent window"), so a host that does not span the monitor
// leaves nothing visible — these helpers let the caller detect that without
// touching Win32.
//
// Header compiles WITHOUT <windows.h> (plain ints only). No logging here;
// the caller logs the CoverageReason token. Call sites are owned by rows 7
// and 9 — this module changes none.

namespace k6wp {

// Axis-aligned rect, virtual-screen or client coordinates depending on the
// call (each function documents its space). Mirrors RECT field order so a
// RECT can be copied field-by-field at the call site.
struct PlacementRect {
  int left;
  int top;
  int right;
  int bottom;
};

// Child-window origin in host-client coordinates.
struct ClientOffset {
  int dx;
  int dy;
};

// How a placed child covers the monitor rect mapped into the same client
// space. kOutOfBounds = disjoint (nothing visible); the kClipped* values
// name the first uncovered edge in Left/Top/Right/Bottom priority order.
enum class CoverageVerdict {
  kCovered,
  kClippedLeft,
  kClippedTop,
  kClippedRight,
  kClippedBottom,
  kOutOfBounds,
};

// (desktop_inject.cpp:261-265) Child coords are relative to the desktop
// host's client origin; the caller passes virtual-screen coords (monitor
// rect). Convert by subtracting the host's window origin so the child lands
// exactly on its monitor's screen rect regardless of virtual origin.
ClientOffset HostClientOffset(const PlacementRect& host_window_rect,
                              const PlacementRect& monitor_rect);

// Child rect in host-client coordinates: monitor size placed at the offset.
// host_client_rect documents the reference frame (same space as the result)
// and is intentionally unused by the computation.
PlacementRect ChildRectInClient(const ClientOffset& offset,
                                const PlacementRect& monitor_rect,
                                const PlacementRect& host_client_rect);

// Both rects in the same client space: does the placed child fully cover
// the monitor rect mapped into that space?
CoverageVerdict CoversMonitor(const PlacementRect& child_in_client,
                              const PlacementRect& monitor_as_client);

// Stable greppable log token for a verdict ("placement: covered",
// "placement: CLIPPED-left", ...). Returns a string literal; never null.
const char* CoverageReason(CoverageVerdict verdict);

}  // namespace k6wp
