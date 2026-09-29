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
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "desktop_inject.hpp"
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

class MultiMonitor {
 public:
  explicit MultiMonitor(LogFn log = nullptr);
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
  std::string adapter_pin() const { return adapter_pin_; }

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
  bool has_monitor(int id) const;
  std::vector<int> monitor_ids() const;
  MultiMonitorMode mode() const;

  // True when at least one live slot is headless (DesktopInjector::Attach
  // failed -> renderer created with a null HWND). Self-maintaining: derived
  // from injected_hwnd() == nullptr, so a successful Reanchor clears it.
  bool has_headless_slots() const;

  // Slot census (Step 4, for get_state "headless_slots"): layered counts
  // live injected slots on the layered path; headless counts slots whose
  // injection failed. Unchanged behavior for has_headless_slots().
  int layered_slot_count() const;
  int headless_slot_count() const;

  // Current virtual-screen geometry (GetSystemMetrics in the .cpp).
  static SpanGeometry GetSpanGeometry() noexcept;

 private:
  // MpvRenderer holds a std::mutex (non-movable), so both RAII members are
  // held via unique_ptr — no raw new/delete anywhere.
  struct Slot {
    MonitorInfo info;
    std::unique_ptr<DesktopInjector> injector;
    std::unique_ptr<MpvRenderer> renderer;
    std::atomic<bool> paused{false};
    Slot() = default;
    Slot(const Slot&) = delete;
    Slot& operator=(const Slot&) = delete;
    Slot(Slot&& other) noexcept
        : info(std::move(other.info)),
          injector(std::move(other.injector)),
          renderer(std::move(other.renderer)) {
      paused.store(other.paused.load(std::memory_order_relaxed),
                   std::memory_order_relaxed);
    }
    Slot& operator=(Slot&& other) noexcept {
      if (this != &other) {
        info = std::move(other.info);
        injector = std::move(other.injector);
        renderer = std::move(other.renderer);
        paused.store(other.paused.load(std::memory_order_relaxed),
                     std::memory_order_relaxed);
      }
      return *this;
    }
  };

  void ClearSlots();
  bool AttachSlot(const MonitorInfo& mi);
  bool AttachSpanSlot();
  // Diff the live slots against DesiredMonitors(): tear down the unwanted,
  // attach the missing. Shared by Init/OnDisplayChange/Reanchor/SetActive.
  void ApplyActiveFilter(const std::vector<MonitorInfo>& desired);

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
  std::map<int, Slot> slots_;
  bool initialized_ = false;
  void* headless_host_ = nullptr;
  std::atomic<bool> global_paused_{false};
  Slot* SlotAt(size_t idx);
  const Slot* SlotAt(size_t idx) const;
};

}  // namespace k6wp
