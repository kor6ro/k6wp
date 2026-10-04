// Pure desktop-placement geometry (plan row 3). No Win32 calls, no logging:
// every function below is a closed computation over its arguments so the
// suite runs headless. <windows.h> is deliberately NOT included.

#include "desktop_placement.hpp"

namespace k6wp {

ClientOffset HostClientOffset(const PlacementRect& host_window_rect,
                              const PlacementRect& monitor_rect) {
  ClientOffset offset;
  offset.dx = monitor_rect.left - host_window_rect.left;
  offset.dy = monitor_rect.top - host_window_rect.top;
  return offset;
}

PlacementRect ChildRectInClient(const ClientOffset& offset,
                                const PlacementRect& monitor_rect,
                                const PlacementRect& /*host_client_rect*/) {
  PlacementRect child;
  child.left = offset.dx;
  child.top = offset.dy;
  child.right = offset.dx + (monitor_rect.right - monitor_rect.left);
  child.bottom = offset.dy + (monitor_rect.bottom - monitor_rect.top);
  return child;
}

CoverageVerdict CoversMonitor(const PlacementRect& child_in_client,
                              const PlacementRect& monitor_as_client) {
  // Disjoint (touching edges do not overlap) -> nothing visible.
  if (child_in_client.right <= monitor_as_client.left ||
      child_in_client.left >= monitor_as_client.right ||
      child_in_client.bottom <= monitor_as_client.top ||
      child_in_client.top >= monitor_as_client.bottom) {
    return CoverageVerdict::kOutOfBounds;
  }
  if (child_in_client.left > monitor_as_client.left) {
    return CoverageVerdict::kClippedLeft;
  }
  if (child_in_client.top > monitor_as_client.top) {
    return CoverageVerdict::kClippedTop;
  }
  if (child_in_client.right < monitor_as_client.right) {
    return CoverageVerdict::kClippedRight;
  }
  if (child_in_client.bottom < monitor_as_client.bottom) {
    return CoverageVerdict::kClippedBottom;
  }
  return CoverageVerdict::kCovered;
}

const char* CoverageReason(CoverageVerdict verdict) {
  switch (verdict) {
    case CoverageVerdict::kCovered:
      return "placement: covered";
    case CoverageVerdict::kClippedLeft:
      return "placement: CLIPPED-left";
    case CoverageVerdict::kClippedTop:
      return "placement: CLIPPED-top";
    case CoverageVerdict::kClippedRight:
      return "placement: CLIPPED-right";
    case CoverageVerdict::kClippedBottom:
      return "placement: CLIPPED-bottom";
    case CoverageVerdict::kOutOfBounds:
      return "placement: OUT-OF-BOUNDS";
  }
  return "placement: OUT-OF-BOUNDS";
}

}  // namespace k6wp
