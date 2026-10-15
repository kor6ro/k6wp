// Property tests for the P1 pure core (display resilience, prompt v5.1 §5.3).
//
// Self-written seeded PRNG only (no external library): a SplitMix64 generator
// drives every scenario. Fixed seeds 1..32 always run (regression corpus);
// then 500 pseudorandom seeds are generated from a fixed root so a failing
// run is reproducible. The failing seed is printed.
//
// Invariants exercised at the pure-diff level (no slots, no Win32):
//   (i)   reconcile twice with no change == no-op (identical snapshot -> empty
//         diff, and re-diffing an unchanged topology collapses to a no-op);
//   (ii)  assignment follows key: a key present in both snapshots is never
//         classified added/removed, and its modelled slot handle is preserved;
//   (iii) every still-present key reaches Attached/Degraded within the budget,
//         simulated purely with retry_schedule_due (0/250/1000/3000 ms, budget
//         5000 ms);
//   (iv)  no slot is kept for a removed key.
// Also asserted: deterministic, sorted output and hash_changed wiring.
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "retry_schedule.hpp"
#include "topology_diff.hpp"

namespace {

int g_checks = 0;
int g_failures = 0;

bool Expect(uint64_t seed, int iter, bool cond, const std::string& msg) {
  ++g_checks;
  if (cond) return true;
  ++g_failures;
  std::cout << "[FAIL] seed=" << seed << " iter=" << iter << " " << msg << "\n";
  return false;
}

// ---- Self-written PRNG (SplitMix64) ---------------------------------------
struct Rng {
  uint64_t s_;
  explicit Rng(uint64_t seed) : s_(seed) {}
  uint64_t Next() {
    uint64_t z = (s_ += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
  }
  uint32_t Below(uint32_t n) {
    return n == 0 ? 0u : static_cast<uint32_t>(Next() % n);
  }
  int Range(int lo, int hi) {
    if (hi <= lo) return lo;
    return lo + static_cast<int>(Below(static_cast<uint32_t>(hi - lo + 1)));
  }
};

const std::vector<std::string>& KeyPool() {
  static const std::vector<std::string> k = {
      "ccd:A", "ccd:B", "ccd:C", "edid:m:1:0", "edid:m:2:5", "gdi:DISPLAY1"};
  return k;
}

// ---- Canonical serialization + deterministic hash (mirrors §R5) -----------
std::string Canonical(const k6wp::TopologySnapshot& s) {
  std::vector<const k6wp::TopologyMonitor*> v;
  for (const auto& m : s.monitors) v.push_back(&m);
  std::sort(v.begin(), v.end(),
            [](const k6wp::TopologyMonitor* a, const k6wp::TopologyMonitor* b) {
              return a->stable_key < b->stable_key;
            });
  std::ostringstream oss;
  oss << s.virtual_x << '|' << s.virtual_y << '|' << s.virtual_width << '|'
      << s.virtual_height << ';';
  for (const auto* m : v) {
    oss << m->stable_key << '|' << m->gdi_name << '|' << m->x << '|' << m->y
        << '|' << m->width << '|' << m->height << '|' << m->scale_pct << '|'
        << m->orientation << '|' << (m->is_primary ? 1 : 0) << '|'
        << (m->degraded ? 1 : 0) << ';';
  }
  return oss.str();
}

uint64_t Fnv1a(const std::string& s) {
  uint64_t h = 1469598103934665603ULL;
  for (unsigned char c : s) {
    h ^= c;
    h *= 1099511628211ULL;
  }
  return h;
}

std::string CanonicalHash(const k6wp::TopologySnapshot& s) {
  return std::to_string(Fnv1a(Canonical(s)));
}

// ---- Snapshot generators ---------------------------------------------------
k6wp::TopologyMonitor MakeMon(const std::string& key, Rng& r) {
  k6wp::TopologyMonitor m;
  m.stable_key = key;
  m.gdi_name = "\\\\.\\DISPLAY_" + key;
  m.x = r.Range(-2000, 2000);
  m.y = r.Range(-1200, 1200);
  m.width = r.Range(640, 2560);
  m.height = r.Range(480, 1440);
  m.scale_pct = 100 + 50 * r.Range(0, 2);
  m.orientation = r.Range(0, 3);
  m.is_primary = false;
  m.degraded = r.Below(4) == 0;
  return m;
}

k6wp::TopologySnapshot GenSnap(Rng& r) {
  const int n = r.Range(0, 4);
  std::vector<int> idx;
  for (int i = 0; i < static_cast<int>(KeyPool().size()); ++i) idx.push_back(i);
  for (int i = 0; i < n; ++i) {
    const int j = r.Range(i, static_cast<int>(idx.size()) - 1);
    std::swap(idx[static_cast<size_t>(i)], idx[static_cast<size_t>(j)]);
  }
  k6wp::TopologySnapshot s;
  for (int i = 0; i < n; ++i) {
    s.monitors.push_back(MakeMon(KeyPool()[static_cast<size_t>(idx[static_cast<size_t>(i)])], r));
  }
  if (n > 0 && r.Below(2) == 1) {
    s.monitors[static_cast<size_t>(r.Below(static_cast<uint32_t>(n)))].is_primary = true;
  }
  // Deliberately shuffle order so diff output ordering cannot depend on input.
  for (int i = 0; i < n; ++i) {
    const int j = r.Range(0, n - 1);
    std::swap(s.monitors[static_cast<size_t>(i)], s.monitors[static_cast<size_t>(j)]);
  }
  s.virtual_x = r.Range(-2000, 0);
  s.virtual_y = r.Range(-1200, 0);
  s.virtual_width = r.Range(1920, 5120);
  s.virtual_height = r.Range(1080, 2160);
  s.hash_snapshot = CanonicalHash(s);
  return s;
}

bool HasKey(const k6wp::TopologySnapshot& s, const std::string& key) {
  for (const auto& m : s.monitors)
    if (m.stable_key == key) return true;
  return false;
}

k6wp::TopologySnapshot Mutate(const k6wp::TopologySnapshot& a, Rng& r) {
  k6wp::TopologySnapshot b = a;
  const int ops = r.Range(0, 4);
  for (int o = 0; o < ops; ++o) {
    const int type = r.Range(0, 6);
    switch (type) {
      case 0: {  // add a monitor
        std::vector<std::string> cand;
        for (const auto& k : KeyPool())
          if (!HasKey(b, k)) cand.push_back(k);
        if (!cand.empty()) {
          b.monitors.push_back(
              MakeMon(cand[static_cast<size_t>(r.Below(static_cast<uint32_t>(cand.size())))], r));
        }
        break;
      }
      case 1:  // remove a monitor
        if (!b.monitors.empty()) {
          b.monitors.erase(b.monitors.begin() +
                           static_cast<std::ptrdiff_t>(r.Below(static_cast<uint32_t>(b.monitors.size()))));
        }
        break;
      case 2:  // move
        if (!b.monitors.empty()) {
          k6wp::TopologyMonitor& m =
              b.monitors[static_cast<size_t>(r.Below(static_cast<uint32_t>(b.monitors.size())))];
          m.x += r.Range(1, 50);
        }
        break;
      case 3:  // resize
        if (!b.monitors.empty()) {
          k6wp::TopologyMonitor& m =
              b.monitors[static_cast<size_t>(r.Below(static_cast<uint32_t>(b.monitors.size())))];
          m.width += r.Range(1, 50);
        }
        break;
      case 4:  // dpi
        if (!b.monitors.empty()) {
          k6wp::TopologyMonitor& m =
              b.monitors[static_cast<size_t>(r.Below(static_cast<uint32_t>(b.monitors.size())))];
          m.scale_pct = (m.scale_pct == 100) ? 150 : 100;
        }
        break;
      case 5:  // orientation
        if (!b.monitors.empty()) {
          k6wp::TopologyMonitor& m =
              b.monitors[static_cast<size_t>(r.Below(static_cast<uint32_t>(b.monitors.size())))];
          m.orientation = (m.orientation + 1) % 4;
        }
        break;
      default:  // primary swap
        if (!b.monitors.empty()) {
          for (auto& m : b.monitors) m.is_primary = false;
          b.monitors[static_cast<size_t>(r.Below(static_cast<uint32_t>(b.monitors.size())))].is_primary =
              true;
        }
        break;
    }
  }
  b.hash_snapshot = CanonicalHash(b);
  return b;
}

// ---- Helpers ---------------------------------------------------------------
std::map<std::string, const k6wp::TopologyMonitor*> Index(
    const k6wp::TopologySnapshot& s) {
  std::map<std::string, const k6wp::TopologyMonitor*> m;
  for (const auto& mon : s.monitors) m[mon.stable_key] = &mon;
  return m;
}

bool Sorted(const std::vector<std::string>& v) {
  return std::is_sorted(v.begin(), v.end());
}

bool SameDiff(const k6wp::TopologyDiff& x, const k6wp::TopologyDiff& y) {
  return x.added_keys == y.added_keys && x.removed_keys == y.removed_keys &&
         x.moved_keys == y.moved_keys && x.resized_keys == y.resized_keys &&
         x.dpi_keys == y.dpi_keys &&
         x.orientation_keys == y.orientation_keys &&
         x.primary_changed == y.primary_changed &&
         x.hash_changed == y.hash_changed;
}

bool Has(const std::vector<std::string>& v, const std::string& key) {
  return std::find(v.begin(), v.end(), key) != v.end();
}

std::vector<std::string> SortedVec(const std::set<std::string>& s) {
  return std::vector<std::string>(s.begin(), s.end());
}

bool PrimaryChangedExpected(
    const std::map<std::string, const k6wp::TopologyMonitor*>& am,
    const std::map<std::string, const k6wp::TopologyMonitor*>& bm) {
  std::set<std::string> keys;
  for (const auto& kv : am) keys.insert(kv.first);
  for (const auto& kv : bm) keys.insert(kv.first);
  for (const auto& k : keys) {
    bool pa = false;
    bool pb = false;
    const auto ia = am.find(k);
    if (ia != am.end()) pa = ia->second->is_primary;
    const auto ib = bm.find(k);
    if (ib != bm.end()) pb = ib->second->is_primary;
    if (pa != pb) return true;
  }
  return false;
}

// ---- Invariant (iii): retry-budget simulation on the pure schedule ---------
bool RetrySimulation(Rng& r, uint64_t seed, int iter,
                     const std::vector<std::string>& keys) {
  std::map<std::string, long long> cover;
  for (const auto& k : keys) {
    if (r.Below(2) == 0) {
      cover[k] = k6wp::kRetryOffsetsMs[r.Below(static_cast<uint32_t>(k6wp::kRetryCount))];
    } else {
      cover[k] = 4000 + static_cast<long long>(r.Below(4000));  // beyond the schedule
    }
  }

  k6wp::RetryClock c;
  c.first_pass_ms = 0;
  c.attempts_done = 0;
  long long now = 0;
  std::map<std::string, bool> attached;
  for (const auto& k : keys) attached[k] = false;

  int guard = 0;
  while (guard++ < 32) {
    if (k6wp::retry_schedule_due(now, c)) {
      ++c.attempts_done;
      for (const auto& k : keys)
        if (!attached[k] && cover[k] <= now) attached[k] = true;
      bool all = true;
      for (const auto& k : keys)
        if (!attached[k]) { all = false; break; }
      if (all) break;
      if (k6wp::retry_schedule_exhausted(c)) break;
    } else {
      if (k6wp::retry_schedule_exhausted(c)) break;
      const long long delay = k6wp::retry_next_delay_ms(now, c);
      if (delay <= 0) break;
      now += delay;
    }
  }

  bool ok = true;
  ok &= Expect(seed, iter, now <= 5000,
               "(iii) simulated convergence stays inside the 5000 ms budget");
  for (const auto& k : keys) {
    const bool degraded = !attached[k];
    ok &= Expect(seed, iter, attached[k] != degraded,
                 "(iii) key ends in exactly one terminal state");
    if (cover[k] <= k6wp::kRetryOffsetsMs[k6wp::kRetryCount - 1]) {
      ok &= Expect(seed, iter, attached[k],
                   "(iii) coverable key reaches Attached");
    } else {
      ok &= Expect(seed, iter, degraded,
                   "(iii) never-coverable key reaches Degraded");
      ok &= Expect(seed, iter, k6wp::retry_schedule_exhausted(c),
                   "(iii) never-coverable key exhausts the schedule");
    }
  }
  return ok;
}

// ---- One randomized reconciliation scenario --------------------------------
bool OneIteration(Rng& r, uint64_t seed, int iter) {
  const k6wp::TopologySnapshot a = GenSnap(r);
  const k6wp::TopologySnapshot b = Mutate(a, r);

  const k6wp::TopologyDiff d = k6wp::diff_topology(a, b);
  const k6wp::TopologyDiff d_again = k6wp::diff_topology(a, b);

  bool ok = true;
  ok &= Expect(seed, iter, SameDiff(d, d_again),
               "diff is deterministic across repeated calls");

  // (i) idempotent no-op.
  ok &= Expect(seed, iter, k6wp::diff_topology(a, a).empty(),
               "(i) diff(A,A) is empty (no-op)");
  ok &= Expect(seed, iter, k6wp::diff_topology(b, b).empty(),
               "(i) diff(B,B) is empty (second reconcile is a no-op)");
  ok &= Expect(seed, iter, SameDiff(k6wp::diff_topology(b, b), k6wp::TopologyDiff{}),
               "(i) re-diff of an unchanged topology equals a default diff");

  // Classification vs independently computed expectations.
  const auto am = Index(a);
  const auto bm = Index(b);
  std::set<std::string> ka;
  std::set<std::string> kb;
  for (const auto& kv : am) ka.insert(kv.first);
  for (const auto& kv : bm) kb.insert(kv.first);

  std::set<std::string> added;
  std::set<std::string> removed;
  std::set<std::string> inter;
  for (const auto& k : kb)
    if (!ka.count(k)) added.insert(k);
  for (const auto& k : ka) {
    if (!kb.count(k)) {
      removed.insert(k);
    } else {
      inter.insert(k);
    }
  }

  ok &= Expect(seed, iter, d.added_keys == SortedVec(added),
               "added_keys matches the set difference");
  ok &= Expect(seed, iter, d.removed_keys == SortedVec(removed),
               "removed_keys matches the set difference");
  ok &= Expect(seed, iter,
               Sorted(d.added_keys) && Sorted(d.removed_keys) &&
                   Sorted(d.moved_keys) && Sorted(d.resized_keys) &&
                   Sorted(d.dpi_keys) && Sorted(d.orientation_keys),
               "every diff key vector is sorted");

  for (const auto& k : inter) {
    const k6wp::TopologyMonitor* p = am.at(k);
    const k6wp::TopologyMonitor* q = bm.at(k);
    const bool moved = p->x != q->x || p->y != q->y;
    const bool resized = p->width != q->width || p->height != q->height;
    const bool dpi = p->scale_pct != q->scale_pct;
    const bool orient = p->orientation != q->orientation;
    // (ii) assignment follows key: a common key is never added/removed and its
    // per-attribute classification is exact.
    ok &= Expect(seed, iter, !Has(d.added_keys, k) && !Has(d.removed_keys, k),
                 "(ii) common key is never classified added/removed");
    ok &= Expect(seed, iter, Has(d.moved_keys, k) == moved,
                 "(ii) moved classification exact");
    ok &= Expect(seed, iter, Has(d.resized_keys, k) == resized,
                 "(ii) resized classification exact");
    ok &= Expect(seed, iter, Has(d.dpi_keys, k) == dpi,
                 "(ii) dpi classification exact");
    ok &= Expect(seed, iter, Has(d.orientation_keys, k) == orient,
                 "(ii) orientation classification exact");
  }

  ok &= Expect(seed, iter, d.primary_changed == PrimaryChangedExpected(am, bm),
               "primary_changed matches the per-key primary comparison");
  ok &= Expect(seed, iter, d.hash_changed == (Canonical(a) != Canonical(b)),
               "hash_changed reflects the canonical snapshot content");

  // (ii)/(iv) modelled slot-handle application driven BY the diff, so a
  // misclassification corrupts the model and trips the invariant.
  std::map<std::string, int> handles;
  int next_handle = 1;
  for (const auto& k : ka) handles[k] = next_handle++;
  const std::map<std::string, int> before = handles;
  for (const auto& k : d.removed_keys) handles.erase(k);
  for (const auto& k : d.added_keys) handles[k] = next_handle++;
  for (const auto& k : inter) {
    const auto it = handles.find(k);
    ok &= Expect(seed, iter,
                 it != handles.end() && it->second == before.at(k),
                 "(ii) common key keeps its slot handle (assignment follows key)");
  }
  for (const auto& k : removed) {
    ok &= Expect(seed, iter, handles.count(k) == 0,
                 "(iv) no slot survives for a removed key");
  }
  std::set<std::string> final_keys;
  for (const auto& kv : handles) final_keys.insert(kv.first);
  ok &= Expect(seed, iter, final_keys == kb,
               "applying the diff yields exactly the new key set");

  // (iii) budget: every still-present key reaches a terminal state in time.
  ok &= RetrySimulation(r, seed, iter, std::vector<std::string>(kb.begin(), kb.end()));
  return ok;
}

bool RunSeed(uint64_t seed) {
  Rng r(seed);
  for (int iter = 0; iter < 8; ++iter) {
    if (!OneIteration(r, seed, iter)) return false;
  }
  return true;
}

}  // namespace

int main() {
  int failed_seeds = 0;

  // Fixed regression corpus: seeds 1..32 always run.
  for (uint64_t seed = 1; seed <= 32; ++seed) {
    if (!RunSeed(seed)) {
      std::cout << "[RED-SEED] fixed seed=" << seed << "\n";
      ++failed_seeds;
    }
  }

  // N=500 pseudorandom seeds per run, derived from a fixed root so a failure
  // is reproducible (re-run with the printed seed).
  Rng seed_gen(0xC0FFEEULL);
  for (int i = 0; i < 500; ++i) {
    const uint64_t seed = seed_gen.Next();
    if (!RunSeed(seed)) {
      std::cout << "[RED-SEED] random seed=" << seed << "\n";
      ++failed_seeds;
    }
  }

  std::cout << "topology_property_test: " << g_checks << " checks, "
            << g_failures << " failures, " << failed_seeds << " failing seeds\n";
  return g_failures == 0 ? 0 : 1;
}
