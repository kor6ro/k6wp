// Pure topology diff (design 02 §R7.1). No Win32, no logging: every function
// below is a closed computation over its arguments so the suite runs headless.
// <windows.h> is deliberately NOT included.

#include "topology_diff.hpp"

namespace k6wp {

// RED (P1 red commit): behavior intentionally unimplemented so the P1 tests
// fail on their intended assertions. Implemented for real in the green commit.
bool TopologyDiff::empty() const { return true; }

TopologyDiff diff_topology(const TopologySnapshot& /*old_s*/,
                           const TopologySnapshot& /*new_s*/) {
  return TopologyDiff{};
}

}  // namespace k6wp
