#pragma once

// Debug lock-order assertion helpers (design 02 §R9.4).
//
// Two lock DOMAINS must never be held at the same time:
//   * the slots domain  — MultiMonitor::slots_mutex_ (recursive; the public
//     entries nest, e.g. ApplyBootAssignments -> LoadLoopSlot), and
//   * the leaf domain   — the EngineApp per-field mutexes (video_mutex_,
//     options_mutex_, displays_mutex_, renderer_mutex_).
// Holding a slots lock and a leaf lock together (in EITHER order) is the
// deadlock shape this guards against: every leaf site is entered without a
// slots lock held and every slots site without a leaf lock held.
//
// RecursiveSlotsGuard  — wraps slots_mutex_. On the 0->1 acquisition it asserts
//                        that no leaf lock is currently held by this thread.
//                        Nested acquisitions (recursion) only bump the depth.
// LeafGuard            — wraps a leaf mutex. Asserts that no slots lock is held
//                        by this thread.
//
// DEVIATION from design §R9.4 (locked decision, recorded in the PR artifacts):
// the design's LeafGuard also asserted g_leaf_depth == 0, forbidding leaf-leaf
// nesting. The existing engine boot-seed nests video_mutex_ then options_mutex_
// (engine_app.cpp Init ~L424-437), so that assertion would abort Debug engine
// boot. LeafGuard therefore asserts ONLY g_slots_depth == 0; leaf-leaf nesting
// stays legal.
//
// The counters are inline thread_local ints (one per thread), always
// maintained; the asserts vanish under NDEBUG (Release), where the guards
// degrade to a plain lock_guard with a counter bump. Header-only, std-only
// (assert / mutex), no Win32.

#include <cassert>
#include <mutex>

namespace k6wp {
namespace lock_order {

// Thread-local lock nesting depths (see the file banner).
inline thread_local int g_slots_depth = 0;
inline thread_local int g_leaf_depth = 0;

// RAII guard for MultiMonitor::slots_mutex_. Asserts on the outermost (0->1)
// acquisition that this thread holds no leaf lock.
class RecursiveSlotsGuard {
 public:
  explicit RecursiveSlotsGuard(std::recursive_mutex& m) : lock_(m) {
    if (g_slots_depth++ == 0) {
      assert(g_leaf_depth == 0 &&
             "lock-order: slots_mutex_ acquired while a leaf mutex is held");
    }
  }
  ~RecursiveSlotsGuard() { --g_slots_depth; }

  RecursiveSlotsGuard(const RecursiveSlotsGuard&) = delete;
  RecursiveSlotsGuard& operator=(const RecursiveSlotsGuard&) = delete;

 private:
  std::lock_guard<std::recursive_mutex> lock_;
};

// RAII guard for an EngineApp leaf mutex. Asserts that this thread holds no
// slots lock. Leaf-leaf nesting is allowed (see the DEVIATION note above).
class LeafGuard {
 public:
  explicit LeafGuard(std::mutex& m) : lock_(m) {
    assert(g_slots_depth == 0 &&
           "lock-order: leaf mutex acquired while slots_mutex_ is held");
    ++g_leaf_depth;
  }
  ~LeafGuard() { --g_leaf_depth; }

  LeafGuard(const LeafGuard&) = delete;
  LeafGuard& operator=(const LeafGuard&) = delete;

 private:
  std::lock_guard<std::mutex> lock_;
};

}  // namespace lock_order
}  // namespace k6wp
