// Pure topology diff (design 02 §R7.1). No Win32, no logging: every function
// below is a closed computation over its arguments so the suite runs headless.
// <windows.h> is deliberately NOT included.

#include "topology_diff.hpp"

#include <algorithm>
#include <map>
#include <set>

namespace k6wp {

namespace {

void SortUnique(std::vector<std::string>& v) {
  std::sort(v.begin(), v.end());
  v.erase(std::unique(v.begin(), v.end()), v.end());
}

}  // namespace

bool TopologyDiff::empty() const {
  return added_keys.empty() && removed_keys.empty() && moved_keys.empty() &&
         resized_keys.empty() && dpi_keys.empty() &&
         orientation_keys.empty() && !primary_changed;
}

TopologyDiff diff_topology(const TopologySnapshot& old_s,
                           const TopologySnapshot& new_s) {
  TopologyDiff d;

  // Keys are identified ONLY by stable_key (§R7.1).
  std::map<std::string, const TopologyMonitor*> old_by_key;
  for (const auto& m : old_s.monitors) old_by_key[m.stable_key] = &m;
  std::map<std::string, const TopologyMonitor*> new_by_key;
  for (const auto& m : new_s.monitors) new_by_key[m.stable_key] = &m;

  for (const auto& kv : new_by_key) {
    if (old_by_key.find(kv.first) == old_by_key.end())
      d.added_keys.push_back(kv.first);
  }
  for (const auto& kv : old_by_key) {
    if (new_by_key.find(kv.first) == new_by_key.end())
      d.removed_keys.push_back(kv.first);
  }

  // Per-attribute changes for keys present in both snapshots. A single key can
  // land in several categories at once (e.g. moved + resized).
  for (const auto& kv : old_by_key) {
    const auto it = new_by_key.find(kv.first);
    if (it == new_by_key.end()) continue;
    const TopologyMonitor& o = *kv.second;
    const TopologyMonitor& n = *it->second;
    if (o.x != n.x || o.y != n.y) d.moved_keys.push_back(kv.first);
    if (o.width != n.width || o.height != n.height)
      d.resized_keys.push_back(kv.first);
    if (o.scale_pct != n.scale_pct) d.dpi_keys.push_back(kv.first);
    if (o.orientation != n.orientation)
      d.orientation_keys.push_back(kv.first);
  }

  // Primary change over the key union: an absent key counts as not-primary, so
  // adding or removing the primary monitor is reported as a primary change too.
  std::set<std::string> keys;
  for (const auto& kv : old_by_key) keys.insert(kv.first);
  for (const auto& kv : new_by_key) keys.insert(kv.first);
  for (const auto& k : keys) {
    const auto oi = old_by_key.find(k);
    const auto ni = new_by_key.find(k);
    const bool old_primary = oi != old_by_key.end() && oi->second->is_primary;
    const bool new_primary = ni != new_by_key.end() && ni->second->is_primary;
    if (old_primary != new_primary) {
      d.primary_changed = true;
      break;
    }
  }

  d.hash_changed = old_s.hash_snapshot != new_s.hash_snapshot;

  // Deterministic ordering regardless of input order (§R7.1).
  SortUnique(d.added_keys);
  SortUnique(d.removed_keys);
  SortUnique(d.moved_keys);
  SortUnique(d.resized_keys);
  SortUnique(d.dpi_keys);
  SortUnique(d.orientation_keys);

  return d;
}

}  // namespace k6wp
