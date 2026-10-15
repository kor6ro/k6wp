// lock_order_test.cpp - Engine Display Resilience, P3/Fase 5.
//
// Tests the lock_order guards:
//   * RecursiveSlotsGuard: depth bookkeeping, recursion, slots-only assert
//   * LeafGuard: depth bookkeeping, leaf-leaf allowed, slots-not-held assert
//
// The violation-abort path (slots + leaf held together -> assert) is NOT
// testable in-process (it would abort the test binary). Coverage for the
// assert is provided by the engine Debug smoke-run
// (engine --exit-after-ms) instead.
//
// Header-only test: includes lock_order.hpp from engine/src. No .cpp link
// deps, no windows.h, no mutex-held-across-threads needed (all
// thread_local counters are per-thread and this test runs on one thread).
//
// No external test framework: plain Check() counter; exit 0 = pass.

#include <cstdio>
#include <iostream>
#include <mutex>
#include <string>

#include "lock_order.hpp"

namespace {

int g_checks = 0;
int g_failures = 0;

void Check(bool cond, const std::string& name) {
  ++g_checks;
  if (cond) {
    std::cout << "[PASS] " << name << "\n";
  } else {
    ++g_failures;
    std::cout << "[FAIL] " << name << "\n";
  }
}

}  // namespace

int main(int argc, char** argv) {
  const std::string sel = argc > 1 ? argv[1] : "all";

  using namespace k6wp::lock_order;

  // ---- RecursiveSlotsGuard: depth bookkeeping ----
  {
    std::recursive_mutex m;
    Check(g_slots_depth == 0, "lock-slots: depth starts at 0");
    {
      RecursiveSlotsGuard g1(m);
      Check(g_slots_depth == 1, "lock-slots: depth 1 after first lock");
      {
        RecursiveSlotsGuard g2(m);  // recursion
        Check(g_slots_depth == 2, "lock-slots: depth 2 after recursive lock");
      }
      Check(g_slots_depth == 1, "lock-slots: depth back to 1 after inner unlock");
    }
    Check(g_slots_depth == 0, "lock-slots: depth 0 after outer unlock");
  }

  // ---- RecursiveSlotsGuard: slots-only assert (leaf not held) ----
  // (No LeafGuard active, so the slots assert should not fire.)
  {
    std::recursive_mutex m;
    RecursiveSlotsGuard g(m);
    Check(g_slots_depth == 1 && g_leaf_depth == 0,
          "lock-slots: leaf depth 0 while slots held");
  }

  // ---- LeafGuard: depth bookkeeping ----
  {
    std::mutex m1, m2;
    Check(g_leaf_depth == 0, "lock-leaf: depth starts at 0");
    {
      LeafGuard g1(m1);
      Check(g_leaf_depth == 1, "lock-leaf: depth 1 after first leaf lock");
      {
        LeafGuard g2(m2);  // leaf-leaf nesting allowed (deviation)
        Check(g_leaf_depth == 2,
              "lock-leaf: depth 2 after leaf-leaf nesting");
      }
      Check(g_leaf_depth == 1, "lock-leaf: depth back to 1 after inner unlock");
    }
    Check(g_leaf_depth == 0, "lock-leaf: depth 0 after outer unlock");
  }

  // ---- LeafGuard: asserts slots not held ----
  // (No RecursiveSlotsGuard active, so the leaf assert should not fire.)
  {
    std::mutex m;
    LeafGuard g(m);
    Check(g_leaf_depth == 1 && g_slots_depth == 0,
          "lock-leaf: slots depth 0 while leaf held");
  }

  // ---- RecursiveSlotsGuard + LeafGuard: never held together ----
  // Slots acquired first, then released before leaf.
  {
    std::recursive_mutex sm;
    std::mutex lm;
    {
      RecursiveSlotsGuard sg(sm);
      Check(g_slots_depth == 1 && g_leaf_depth == 0,
            "lock-order: slots held, leaf not");
    }
    Check(g_slots_depth == 0, "lock-order: slots released");
    {
      LeafGuard lg(lm);
      Check(g_leaf_depth == 1 && g_slots_depth == 0,
            "lock-order: leaf held, slots not");
    }
    Check(g_leaf_depth == 0, "lock-order: leaf released");
  }

  // ---- LeafGuard + RecursiveSlotsGuard: never held together ----
  {
    std::mutex lm;
    std::recursive_mutex sm;
    {
      LeafGuard lg(lm);
      Check(g_leaf_depth == 1 && g_slots_depth == 0,
            "lock-order-rev: leaf held, slots not");
    }
    {
      RecursiveSlotsGuard sg(sm);
      Check(g_slots_depth == 1 && g_leaf_depth == 0,
            "lock-order-rev: slots held, leaf not");
    }
  }

  std::cout << g_checks << " checks, " << g_failures << " failures\n";
  return g_failures == 0 ? 0 : 1;
}
