#pragma once

// Multi-monitor N-handle manager (Todo 33, fase-5).
//
// Per-monitor mode: 1 DesktopInjector (Todo 9) + 1 MpvRenderer (Todo 10)
// pair per monitor, enumerated via k6wp::ListMonitors() (Todo 7).
// Span mode: a single pair stretched across the virtual screen
// (SM_XVIRTUALSCREEN / SM_YVIRTUALSCREEN / SM_CXVIRTUALSCREEN /
// SM_CYVIRTUALSCREEN).
// OnDisplayChange() re-enumerates and re-attaches: a removed monitor's slot
// is torn down while the engine keeps running on the rest (QA-fail path).
//
// windows.h lives in the .cpp only — this header exposes std types.

#include <atomic>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "desktop_inject.hpp"
#include "displays_schema.hpp"
#include "ipc_marshal.hpp"  // DisplayVideoCommand — row 33 command gate
#include "monitor_util.hpp"
#include "mpv_renderer.hpp"

namespace k6wp {

enum class MultiMonitorMode { PerMonitor, Span };

// Virtual-screen geometry, pixels in virtual-screen coordinates.
struct SpanGeometry {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
};

// Injectable per-slot construction seam (extend-setup row 6, GAP-11/IS-5):
// MultiMonitor asks a SlotFactory to build each slot's DesktopInjector and
// MpvRenderer instead of calling make_unique inline, so tests can swap the
// construction without touching desktop_inject.cpp / mpv_renderer.cpp (and
// therefore without libmpv) at link time. Production passes
// DefaultSlotFactory() - identical objects, arguments and call order as the
// pre-seam code (the test suite asserts that order verbatim).
struct SlotFactory {
  // Builds a slot's DesktopInjector; receives MultiMonitor's LogFn (the
  // pre-seam code passed log_ to make_unique<DesktopInjector>).
  std::function<std::unique_ptr<DesktopInjector>(LogFn)> make_injector;
  // Builds a slot's MpvRenderer (AttachSlot and the pin-revert recreate).
  std::function<std::unique_ptr<MpvRenderer>()> make_renderer;
};

// The production factory: make_unique<DesktopInjector>(log) and
// make_unique<MpvRenderer>() - exactly what AttachSlot/VerifyPinAndRevert
// called before the seam. inline so the make_unique calls are emitted ONLY
// in TUs that materialise the MultiMonitor constructor's default argument
// (engine_app.cpp in production); multi_monitor.cpp itself never references
// them, so a test linking multi_monitor.cpp without libmpv still links.
inline SlotFactory DefaultSlotFactory() {
  SlotFactory factory;
  factory.make_injector = [](LogFn log) {
    return std::make_unique<DesktopInjector>(log);
  };
  factory.make_renderer = []() { return std::make_unique<MpvRenderer>(); };
  return factory;
}

// Row 17: outcome of one MultiMonitor::ApplyBootAssignments pass (engine
// boot summary + test observables).
struct BootAssignmentStats {
  int applied = 0;          // live slot loaded the assignment path
  int retained = 0;         // device absent -> kept for OnDisplayChange
  int skipped_missing = 0;  // path fails exists -> slot keeps the default
};

class MultiMonitor {
 public:
  // `factory` defaults to DefaultSlotFactory() (production behaviour).
  // Tests inject a fake here or via SetSlotFactory(); both members must be
  // non-empty (the default argument guarantees that in production).
  explicit MultiMonitor(LogFn log = nullptr,
                        SlotFactory factory = DefaultSlotFactory());
  ~MultiMonitor();

  MultiMonitor(const MultiMonitor&) = delete;
  MultiMonitor& operator=(const MultiMonitor&) = delete;
  MultiMonitor(MultiMonitor&&) noexcept;
  MultiMonitor& operator=(MultiMonitor&&) noexcept;

  // Hidden host HWND handed to mpv as `wid` for slots whose injection
  // failed. Without it the fallback MpvRenderer is created with a null HWND
  // and mpv spawns its own framed top-level window once a video loads (plan
  // Must-NOT-Have). EngineApp sets the never-shown message window before
  // Init(); null keeps the legacy null-HWND path.
  void SetHeadlessHost(void* hwnd);

  // Active monitor target (Step 5: honoring monitor_id). -1 = every monitor
  // (legacy behavior); >=0 = only that monitor id (absent id = zero slots,
  // engine keeps running). Set before Init(); when already initialized it
  // applies immediately. Span mode ignores it (logged once).
  void SetActiveMonitor(int id);
  int active_monitor() const;

  // Injection strategy for every slot (Step 4: --wallpaper-mode). Set before
  // Init(); stored per injector, so AttachSlot, OnDisplayChange, and Reanchor
  // (all Attach calls) honor it without further calls.
  void SetInjectMode(InjectMode mode);

  // P3L.3: `d3d11-adapter` substring for every slot renderer. Set before
  // Init(); applied at each renderer's Create (VO-init level, PATCH A).
  // Empty = unpinned. Survives OnDisplayChange/Reanchor (instances kept);
  // VerifyPinAndRevert() drops it per-slot on revert.
  void SetAdapterPin(const std::string& substr);

  // Test seam (row 6): replace the per-slot construction. Call before
  // Init(); affects slots created AFTER the call only (live slots keep
  // their instances, like SetAdapterPin). Both members must be non-empty -
  // an empty std::function makes the next slot creation throw
  // std::bad_function_call. Production never calls this.
  void SetSlotFactory(SlotFactory factory);

  // Enumerate via ListMonitors() and attach. PerMonitor: one slot per
  // monitor; Span: a single slot across the virtual screen. Returns true
  // when at least one slot is live (a headless renderer counts — Attach
  // failure degrades to the hidden host, never fatal). False only when no
  // monitors exist at all; the engine keeps running either way.
  bool Init(MultiMonitorMode mode);

  // Tear down all slots. Idempotent.
  void Shutdown();

  // Re-enumerate and diff: tear down slots whose monitor vanished, attach
  // slots for newly arrived monitors, refresh live sizes, recompute span
  // geometry in Span mode. Never throws; engine keeps running on survivors.
  void OnDisplayChange();

  // Load+loop one video path (UTF-8) on every live renderer. Returns true
  // when at least one renderer accepted it; false with no live slots.
  // force=true re-issues loadfile on every slot (device-lost recovery).
  bool LoadLoopAll(const std::string& path, bool force = false);

  // Row 15: true when a live slot's GDI device name matches `device`
  // (UTF-8, e.g. \\.\DISPLAY1). The set_display_video resolve predicate:
  // an unknown/absent key returns false and the caller rejects the command
  // before touching any slot or displays.json. Does not read
  // displays.json — "assignment" here means a live slot for that device
  // key, not a persisted path.
  bool HasAssignment(const std::string& device) const;

  // Row 15: per-slot variant of LoadLoopAll — load+loop `path` on the ONE
  // live slot whose GDI device name matches `device`, then apply
  // `fit_mode` to that slot only (empty = leave the slot's fit mode
  // alone). Returns true when that slot's renderer accepted the path;
  // false when no slot matches the key (unknown device), the path is
  // empty, or the renderer refused. Does not touch other slots and does
  // NOT update last_video_ (a per-slot assignment is not the global
  // load; pin-revert keeps reloading last_video_ — rows 16/17 own the
  // assignment re-apply story).
  bool LoadLoopSlot(const std::string& device, const std::string& path,
                    const std::string& fit_mode = std::string());

  // Row 17: boot-time assignment convergence. Call AFTER Init() and
  // LoadLoopAll(default) so live slots already carry the default video.
  // For each displays.json assignment, matched to a live slot by GDI device
  // key (MonitorInfo::device_name):
  //   (a) device key matches no live monitor -> RETAINED in memory and
  //       re-applied by OnDisplayChange when the device returns;
  //       logs "display: retained assignment for absent <device>";
  //   (b) path fails std::filesystem::exists -> skipped for that slot,
  //       which keeps the default video; logs "display: assignment path
  //       missing for <device>: <path> - falling back to default";
  //   (c) successful apply logs "display: applied <path> to <device>".
  // An empty cfg is a no-op: zero per-slot LoadLoop calls, zero logs - the
  // caller's own LoadLoopAll boot load is untouched (IS-4). Never throws.
  BootAssignmentStats ApplyBootAssignments(const DisplaysConfig& cfg,
                                           const std::string& fit_mode);

  // Row 33 (IS-7/GAP-14): command-path collision gate for
  // set_display_video. EngineApp::HandleSetDisplayVideo (engine_app.cpp) AND
  // display_assignment_test case (e) both call this — one production
  // decision, no harness copy. Builds the prospective assignment map from
  // cfg + cmd (assign overwrites the device key; clear erases it) and runs
  // the REAL DetectKeyCollision (shared/displays_schema) against `monitors`.
  // Returns the colliding keys; empty = the command may proceed to mutate.
  // When non-empty and `log` != nullptr, logs the exact refusal line
  // "ipc: set_display_video refused (duplicate-mode collision): <keys>"
  // (keys comma-joined, map order). Pure: no slot, file, or cfg mutation.
  // DetectKeyCollision semantics unchanged — this is a call site, not a
  // second detector.
  static std::vector<std::wstring> DetectKeyCollisionForCommand(
      const DisplaysConfig& cfg, const DisplayVideoCommand& cmd,
      const std::vector<MonitorInfo>& monitors, LogFn log);

  // Row 33 (IS-7/GAP-14): load-path collision gate for the displays.json
  // consumers (boot convergence + OnDisplaysFileChanged). Runs the REAL
  // DetectKeyCollision on cfg against `monitors`; on collision drops every
  // colliding key EXCEPT the first in map order (keep-first — std::map
  // iteration order matches DetectKeyCollision's sorted output) and, when
  // `log` != nullptr, logs the exact refusal line
  // "display: refusing assignments with colliding keys: <keys>". Returns
  // the dropped keys (empty = clean map). Mutates cfg in place. Does NOT
  // persist displays.json — the store stays dumb (SaveDisplays untouched);
  // enforcement lives at the consumers.
  static std::vector<std::wstring> DropCollidingAssignments(
      DisplaysConfig& cfg, const std::vector<MonitorInfo>& monitors,
      LogFn log);

  // P3L.3 pin verify/revert pass (PATCH A). For each slot created WITH a
  // pin that has started playback but reports hwdec inactive ("no" is only
  // meaningful post-start; "dxva2" counts as active and never reverts):
  // tear the renderer down, re-Create WITHOUT pin, reload the last video,
  // re-verify. Returns the number of reverted slots. Never throws.
  int VerifyPinAndRevert();

  // Apply a WallpaperConfig fit_mode to every live renderer (per-slot
  // window aspect feeds "stretch"; see MpvRenderer::SetFitMode). No-op
  // with no live slots. Called from the config-watch callback + set_video.
  void ApplyFitModeAll(const std::string& fit_mode);

  // Pause / resume every live renderer (tray menu + power hooks funnel).
  // Global-driven pause: sets the global override (see global_paused_);
  // ResumeAll clears ONLY the global-driven pause — a slot individually
  // paused via PauseSlot stays paused (occlusion state survives).
  void PauseAll();
  void ResumeAll();

  // Per-slot pause granularity (P2.6, consumer: occlusion watcher).
  // idx = position in slots_ iteration order (0..N-1 addresses exactly the
  // live slots; span mode kSpanSlotId=-1 is a single slot at index 0).
  // Out-of-range idx: PauseSlot is a silent no-op; IsSlotPaused returns the
  // global pause state. Never throws.
  // Thread-safety: the per-slot flag is atomic — the loop-thread occlusion
  // callback (PauseSlot) and the IPC-worker thread (ResumeAll) touch it
  // concurrently; plain bool would be a data race.
  void PauseSlot(size_t idx, bool pause);
  bool IsSlotPaused(size_t idx) const;

  // Forced re-attach of every live slot (Explorer restart / power-resume:
  // geometry may be unchanged but the desktop windows are gone). Keeps the
  // renderer instances (no video reload — SetHWND re-points them).
  void Reanchor();

  size_t slot_count() const;
  std::vector<int> monitor_ids() const;
  MultiMonitorMode mode() const;

  // True when at least one live slot is headless (DesktopInjector::Attach
  // failed -> renderer created with a null HWND). Self-maintaining: derived
  // from injected_hwnd() == nullptr, so a successful Reanchor clears it.
  bool has_headless_slots() const;

  // Slot census (Step 4, for get_state "headless_slots"): headless counts
  // slots whose injection failed. Unchanged behavior for has_headless_slots().
  int headless_slot_count() const;

  // Row 4: last CoverageReason token recorded for a slot's attach attempts
  // ("" when the monitor id has no live slot). Rows 15/19 map it to the
  // get_state display_coverage field.
  std::string SlotCoverageReason(int monitor_id) const;

  // Current virtual-screen geometry (GetSystemMetrics in the .cpp).
  static SpanGeometry GetSpanGeometry() noexcept;

  // Test-only seam (row 11 failure QA): when non-null, GetSpanGeometry()
  // returns *g instead of the live GetSystemMetrics values, so a headless
  // suite can force the non-positive virtual-screen fixture that drives
  // ReattachSpanLocked's primary-resolution fallback. Production never
  // calls this; pass nullptr to restore the live metrics.
  static void SetSpanGeometryOverride(const SpanGeometry* g);

 private:
  // MpvRenderer holds a std::mutex (non-movable), so both RAII members are
  // held via unique_ptr — no raw new/delete anywhere.
  struct Slot {
    MonitorInfo info;
    std::unique_ptr<DesktopInjector> injector;
    std::unique_ptr<MpvRenderer> renderer;
    std::atomic<bool> paused{false};
    // Row 4: last CoverageReason token from this slot's attach path
    // (DesktopInjector::last_coverage_reason, copied in AttachSlot).
    // Rows 15/19 map it to the get_state display_coverage field.
    std::string coverage_reason;
    Slot() = default;
    Slot(const Slot&) = delete;
    Slot& operator=(const Slot&) = delete;
    Slot(Slot&& other) noexcept
        : info(std::move(other.info)),
          injector(std::move(other.injector)),
          renderer(std::move(other.renderer)),
          coverage_reason(std::move(other.coverage_reason)) {
      paused.store(other.paused.load(std::memory_order_relaxed),
                   std::memory_order_relaxed);
    }
    Slot& operator=(Slot&& other) noexcept {
      if (this != &other) {
        info = std::move(other.info);
        injector = std::move(other.injector);
        renderer = std::move(other.renderer);
        coverage_reason = std::move(other.coverage_reason);
        paused.store(other.paused.load(std::memory_order_relaxed),
                     std::memory_order_relaxed);
      }
      return *this;
    }
  };

 public:
  // Live slot map (monitor id -> slot) for tests/diagnostics (row 6). The
  // slot payload stays private; hold the reference only across read-only
  // checks, never across Init/OnDisplayChange/Shutdown.
  const std::map<int, Slot>& slots() const { return slots_; }

 private:
  void ClearSlots();
  bool AttachSlot(const MonitorInfo& mi);
  bool AttachSpanSlot();
  // Row 7: resolve the desktop host ONCE for the current attach pass and
  // store it in shared_host_. Called at each pass entry that attaches or
  // re-attaches (ApplyActiveFilter, the span paths); every slot then receives
  // the SAME host via SetSharedHost instead of spawning its own.
  void ResolveHostForPass();
  // Shared span re-anchor for OnDisplayChange (reassert=false) and Reanchor
  // (reassert=true): primary-resolution size fallback + hidden-host attach
  // fallback. `slot.injector` must be non-null. No logging here so the
  // callers' distinct lines stay put. Structural slot changes are
  // main-thread-only, so no lock is taken despite the name.
  void ReattachSpanLocked(Slot& slot, int x, int y, int w, int h, bool reassert);
  // Diff the live slots against DesiredMonitors(): tear down the unwanted,
  // attach the missing. Shared by Init/OnDisplayChange/Reanchor/SetActive.
  void ApplyActiveFilter(const std::vector<MonitorInfo>& desired);
  // Row 17: re-apply retained_assignments_ onto live slots whose device key
  // has returned (OnDisplayChange pass, PerMonitor only - span mode skips:
  // the span slot is not keyed by a real GDI device name). Entries that
  // still have no live slot stay retained silently; a successful LoadLoop
  // erases the entry and logs "display: applied <path> to <device>".
  void ReapplyRetainedLocked();

  LogFn log_ = nullptr;
  MultiMonitorMode mode_ = MultiMonitorMode::PerMonitor;
  InjectMode inject_mode_ = InjectMode::kAuto;
  std::string adapter_pin_;  // P3L.3: d3d11-adapter substring for new slots
  std::string last_video_;   // P3L.3: last LoadLoopAll path (revert reload)
  // CRIT-1 (audit-remediation todo 11): get_state reads this from the IPC
  // worker thread (BuildStateJson "monitor" field) while the main loop
  // writes it (queued set_monitor executor, Init). Atomic — no mutex needed
  // for a single int; structural slot changes stay main-thread-only.
  std::atomic<int> active_monitor_{-1};
  bool span_filter_logged_ = false;
  // True once Init() has run: separates the pre-Init SetActiveMonitor (just
  // stores; Init attaches) from a live retarget to an empty set (must
  // re-attach immediately so recovery from an absent id restores slots).
  bool filter_armed_ = false;
  // Row 6: per-slot construction seam; non-empty via the constructor's
  // DefaultSlotFactory() default argument (production) or SetSlotFactory.
  SlotFactory factory_;
  std::map<int, Slot> slots_;
  // Row 17: assignments whose device key matched no live monitor at boot
  // (or while converged). Keyed by GDI device name (wide, same as
  // MonitorInfo::device_name); re-applied by OnDisplayChange -> 
  // ReapplyRetainedLocked when the device returns. Survives
  // Init/OnDisplayChange/Shutdown (engine keeps the map for hotplug).
  std::map<std::wstring, std::wstring> retained_assignments_;
  bool initialized_ = false;
  void* headless_host_ = nullptr;
  // Row 7: the attach pass's shared host (ResolveSharedHost). Persists across
  // passes so Reanchor/OnDisplayChange survivors get the fresh resolution and
  // row 11 can compare the measured client_rect against the live host.
  SharedHost shared_host_;
  // Row 11: host client_rect the live slots were last attached against.
  // Written ONLY at the end of Init/OnDisplayChange/Reanchor (pass-end),
  // never by ResolveHostForPass - OnDisplayChange compares the freshly
  // resolved shared_host_.client_rect against this to detect a moved or
  // recreated host while every monitor rect is unchanged. Update-in-
  // ResolveHostForPass would make that comparison vacuous (the pass refresh
  // would also refresh the snapshot before the survivor loop runs).
  PlacementRect attached_host_rect_{};
  std::atomic<bool> global_paused_{false};
  Slot* SlotAt(size_t idx);
  const Slot* SlotAt(size_t idx) const;
};

}  // namespace k6wp
