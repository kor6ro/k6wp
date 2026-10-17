#pragma once

// Pure topology diff (design 02 §R7.1 / §R12.2; brief §4.6).
//
// windows.h-free; the computation is closed over its two arguments and the
// output ordering is deterministic (each key vector sorted, keys compared by
// stable_key only). Pattern: desktop_placement.hpp — no Win32, no clock.

#include <string>
#include <vector>

#include "topology_snapshot.hpp"

namespace k6wp {

struct TopologyDiff {
  std::vector<std::string> added_keys;        // in new, not old        (sorted)
  std::vector<std::string> removed_keys;      // in old, not new        (sorted)
  std::vector<std::string> moved_keys;        // x/y changed            (sorted)
  std::vector<std::string> resized_keys;      // width/height changed   (sorted)
  std::vector<std::string> dpi_keys;          // scale_pct changed      (sorted)
  std::vector<std::string> orientation_keys;  // orientation changed    (sorted)
  bool primary_changed = false;
  bool hash_changed = false;
  bool empty() const;  // all vectors empty && !primary_changed
};

// Pure: closed computation over its two arguments; deterministic ordering.
// Identical snapshots -> an empty diff (idempotent no-op).
TopologyDiff diff_topology(const TopologySnapshot& old_s,
                           const TopologySnapshot& new_s);

}  // namespace k6wp
