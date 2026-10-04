// Multi-monitor N-handle manager (Todo 33, fase-5). See multi_monitor.hpp.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <cstdio>

#include "multi_monitor.hpp"

namespace k6wp {

namespace {

// Span-mode slot key. Monitor ids from ListMonitors() are >= 0, so -1 can
// never collide with a per-monitor slot.
constexpr int kSpanSlotId = -1;

void LogLine(LogFn log, const char* msg) {
  if (log != nullptr && msg != nullptr) {
    log("%s", msg);
  }
}

// Step 5: the desired slot set for an active target. -1 = every monitor;
// >=0 = only that id (absent id = empty set, engine keeps running).
std::vector<MonitorInfo> FilterMonitors(int active) {
  const std::vector<MonitorInfo> all = ListMonitors();
  if (active < 0) {
    return all;
  }
  std::vector<MonitorInfo> out;
  for (const MonitorInfo& mi : all) {
    if (mi.id == active) {
      out.push_back(mi);
    }
  }
  return out;
}

}  // namespace

MultiMonitor::MultiMonitor(LogFn log) : log_(log) {}

MultiMonitor::~MultiMonitor() { ClearSlots(); }

MultiMonitor::MultiMonitor(MultiMonitor&& other) noexcept
    : log_(other.log_),
      mode_(other.mode_),
      inject_mode_(other.inject_mode_),
      adapter_pin_(std::move(other.adapter_pin_)),
      last_video_(std::move(other.last_video_)),
      active_monitor_(other.active_monitor_.load(std::memory_order_relaxed)),
      span_filter_logged_(other.span_filter_logged_),
      filter_armed_(other.filter_armed_),
      slots_(std::move(other.slots_)),
      initialized_(other.initialized_),
      headless_host_(other.headless_host_),
      global_paused_(other.global_paused_.load(std::memory_order_relaxed)) {
  other.initialized_ = false;
}

MultiMonitor& MultiMonitor::operator=(MultiMonitor&& other) noexcept {
  if (this != &other) {
    ClearSlots();
    log_ = other.log_;
    mode_ = other.mode_;
    inject_mode_ = other.inject_mode_;
    adapter_pin_ = std::move(other.adapter_pin_);
    last_video_ = std::move(other.last_video_);
    active_monitor_.store(other.active_monitor_.load(std::memory_order_relaxed),
                          std::memory_order_relaxed);
    span_filter_logged_ = other.span_filter_logged_;
    filter_armed_ = other.filter_armed_;
    slots_ = std::move(other.slots_);
    initialized_ = other.initialized_;
    headless_host_ = other.headless_host_;
    global_paused_.store(other.global_paused_.load(std::memory_order_relaxed),
                         std::memory_order_relaxed);
    other.initialized_ = false;
  }
  return *this;
}

SpanGeometry MultiMonitor::GetSpanGeometry() noexcept {
  SpanGeometry g;
  g.x = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
  g.y = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
  g.width = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
  g.height = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
  return g;
}

void MultiMonitor::ClearSlots() {
  // unique_ptr dtors do the teardown: ~DesktopInjector Detaches the child
  // window, ~MpvRenderer destroys the mpv instance (mutex-serialized).
  slots_.clear();
  initialized_ = false;
}

bool MultiMonitor::AttachSlot(const MonitorInfo& mi) {
  Slot slot;
  slot.info = mi;
  slot.injector = std::make_unique<DesktopInjector>(log_);
  slot.injector->SetInjectMode(inject_mode_);
  // Attach failure degrades to a headless renderer: it embeds mpv into the
  // hidden host (headless_host_, set by EngineApp) so mpv never spawns its
  // own framed window (Todo 2). A monitor that refuses injection must not
  // kill the engine or the surviving monitors.
  const bool attached = slot.injector->Attach(mi.x, mi.y, mi.width, mi.height);
  // Row 4: persist the attach path's coverage verdict on the slot so rows
  // 15/19 can surface it as get_state display_coverage (logging only here).
  slot.coverage_reason = slot.injector->last_coverage_reason();
  if (!attached) {
    char buf[160];
    std::snprintf(buf, sizeof(buf),
                  "multi_monitor: Attach failed for monitor %d (%dx%d), "
                  "headless renderer fallback",
                  mi.id, mi.width, mi.height);
    LogLine(log_, buf);
  }
  slot.renderer = std::make_unique<MpvRenderer>();
  if (!adapter_pin_.empty()) slot.renderer->SetAdapterPin(adapter_pin_);
  void* hwnd = attached ? slot.injector->injected_hwnd() : headless_host_;
  if (!slot.renderer->Create(hwnd)) {
    char buf[128];
    std::snprintf(buf, sizeof(buf),
                  "multi_monitor: MpvRenderer::Create failed for monitor %d",
                  mi.id);
    LogLine(log_, buf);
    return false;
  }
  slots_.emplace(mi.id, std::move(slot));
  // A slot attached while the engine is globally paused starts paused so
  // the global override holds for newcomers (display-change arrivals).
  const auto inserted = slots_.find(mi.id);
  if (inserted != slots_.end() && inserted->second.renderer &&
      global_paused_.load(std::memory_order_relaxed)) {
    inserted->second.renderer->Pause();
  }
  return true;
}

bool MultiMonitor::AttachSpanSlot() {
  SpanGeometry g = GetSpanGeometry();
  MonitorInfo mi;
  mi.id = kSpanSlotId;
  mi.x = g.x;
  mi.y = g.y;
  mi.width = g.width;
  mi.height = g.height;
  mi.is_primary = true;
  if (mi.width <= 0 || mi.height <= 0) {
    // Virtual-screen query failed; fall back to the primary resolution so
    // span mode still shows something instead of dying.
    const MonitorInfo primary = GetPrimaryMonitor();
    mi.x = primary.x;
    mi.y = primary.y;
    mi.width = primary.width;
    mi.height = primary.height;
    mi.device_name = primary.device_name;
    if (mi.width <= 0 || mi.height <= 0) {
      LogLine(log_, "multi_monitor: no usable span geometry (no monitors)");
      return false;
    }
  }
  return AttachSlot(mi);
}

// Span re-anchor shared by OnDisplayChange and Reanchor. `slot.injector` must
// be non-null. Falls back to the primary resolution when the virtual-screen
// query yields a non-positive size (same policy as AttachSpanSlot) and to the
// hidden host when the re-attach fails. `reassert` re-applies the sticky
// frameless style after a successful attach (Reanchor); OnDisplayChange passes
// false. No logging here so the callers' distinct fall-back lines stay at the
// call sites. Structural slot changes are main-thread-only (see the header).
void MultiMonitor::ReattachSpanLocked(Slot& slot, int x, int y, int w, int h,
                                      bool reassert) {
  if (w <= 0 || h <= 0) {
    const MonitorInfo primary = GetPrimaryMonitor();
    w = primary.width;
    h = primary.height;
  }
  slot.injector->Detach();
  if (slot.injector->Attach(x, y, w, h) && slot.renderer) {
    if (reassert) {
      slot.injector->ReassertFrameless();
    }
    slot.renderer->SetHWND(slot.injector->injected_hwnd());
  } else if (slot.renderer) {
    slot.renderer->SetHWND(headless_host_);
  }
}

void MultiMonitor::SetHeadlessHost(void* hwnd) { headless_host_ = hwnd; }

void MultiMonitor::SetInjectMode(InjectMode mode) { inject_mode_ = mode; }

void MultiMonitor::SetAdapterPin(const std::string& substr) {
  adapter_pin_ = substr;
}

void MultiMonitor::SetActiveMonitor(int id) {
  if (id == active_monitor_.load(std::memory_order_acquire)) {
    return;
  }
  active_monitor_.store(id, std::memory_order_release);
  if (mode_ == MultiMonitorMode::Span) {
    if (!span_filter_logged_) {
      span_filter_logged_ = true;
      char buf[128];
      std::snprintf(buf, sizeof(buf),
                    "multi_monitor: active monitor %d ignored in Span mode",
                    id);
      LogLine(log_, buf);
    }
    return;
  }
  if (initialized_ || filter_armed_) {
    ApplyActiveFilter(FilterMonitors(active_monitor_.load(std::memory_order_acquire)));
  }
}

int MultiMonitor::active_monitor() const {
  return active_monitor_.load(std::memory_order_acquire);
}

void MultiMonitor::ApplyActiveFilter(const std::vector<MonitorInfo>& desired) {
  // Tear down slots outside the desired set (vanished monitor or filtered
  // out by the active target).
  for (auto it = slots_.begin(); it != slots_.end();) {
    const bool wanted = std::any_of(
        desired.begin(), desired.end(),
        [&](const MonitorInfo& mi) { return mi.id == it->first; });
    if (!wanted) {
      char buf[128];
      std::snprintf(buf, sizeof(buf),
                    "multi_monitor: slot %d out of target set, tearing down",
                    it->first);
      LogLine(log_, buf);
      it = slots_.erase(it);
    } else {
      ++it;
    }
  }
  // Attach the missing desired slots.
  for (const MonitorInfo& mi : desired) {
    if (slots_.find(mi.id) == slots_.end()) {
      AttachSlot(mi);
    }
  }
  initialized_ = !slots_.empty();
}

bool MultiMonitor::Init(MultiMonitorMode mode) {
  ClearSlots();
  mode_ = mode;
  if (mode_ == MultiMonitorMode::Span) {
    if (active_monitor_.load(std::memory_order_acquire) >= 0 &&
        !span_filter_logged_) {
      span_filter_logged_ = true;
      LogLine(log_, "multi_monitor: active monitor ignored in Span mode");
    }
    if (AttachSpanSlot()) {
      initialized_ = true;
    }
    filter_armed_ = true;
    return initialized_;
  }
  ApplyActiveFilter(FilterMonitors(active_monitor_.load(std::memory_order_acquire)));
  filter_armed_ = true;
  if (!initialized_) {
    LogLine(log_, "multi_monitor: Init found no monitors, engine keeps running");
  }
  return initialized_;
}

void MultiMonitor::Shutdown() { ClearSlots(); }

void MultiMonitor::OnDisplayChange() {
  // Display-change handling must never take the engine down: allocation or
  // attach failures degrade to fewer live slots, never an exception.
  try {
    if (mode_ == MultiMonitorMode::Span) {
      const SpanGeometry g = GetSpanGeometry();
      const auto it = slots_.find(kSpanSlotId);
      if (it == slots_.end()) {
        AttachSpanSlot();
        return;
      }
      Slot& slot = it->second;
      if (slot.info.x == g.x && slot.info.y == g.y &&
          slot.info.width == g.width && slot.info.height == g.height) {
        return;  // Geometry unchanged — nothing to do.
      }
      // Re-attach the injector at the new span size, keep the renderer
      // instance (no video reload needed — SetHWND re-points it).
      if (slot.injector) {
        ReattachSpanLocked(slot, g.x, g.y, g.width, g.height, /*reassert=*/false);
      }
      slot.info.x = g.x;
      slot.info.y = g.y;
      slot.info.width = g.width;
      slot.info.height = g.height;
      return;
    }
    const std::vector<MonitorInfo> desired =
        FilterMonitors(active_monitor_.load(std::memory_order_acquire));
    // 1. Step 5: converge the slot set to the active target (vanished
    //    monitors torn down, filtered-out monitors removed, newcomers
    //    attached). Reuses the OnDisplayChange teardown/attach pattern.
    ApplyActiveFilter(desired);
    // 2. Refresh sizes of survivors.
    for (const MonitorInfo& mi : desired) {
      const auto it = slots_.find(mi.id);
      if (it == slots_.end()) {
        continue;
      }
      {
        const bool resized = (it->second.info.width != mi.width ||
                              it->second.info.height != mi.height ||
                              it->second.info.x != mi.x ||
                              it->second.info.y != mi.y);
        it->second.info = mi;
        if (resized && it->second.injector) {
          it->second.injector->OnDisplayChange(mi.x, mi.y, mi.width, mi.height);
          if (it->second.renderer) {
            void* hwnd = it->second.injector->injected_hwnd();
            if (hwnd == nullptr) {
              // Re-attach failed (Progman missing / SetParent error): point
              // the renderer at the hidden host so mpv never spawns its own
              // framed window (plan Must-NOT-Have). The next display-change
              // re-anchors.
              hwnd = headless_host_;
            }
            it->second.renderer->SetHWND(hwnd);
          }
        }
      }
    }
    initialized_ = !slots_.empty();
  } catch (...) {
    LogLine(log_, "multi_monitor: OnDisplayChange failed, keeping live slots");
  }
}

bool MultiMonitor::LoadLoopAll(const std::string& path, bool force) {
  bool any = false;
  for (auto& kv : slots_) {
    if (kv.second.renderer && kv.second.renderer->LoadLoop(path, force)) {
      any = true;
    }
  }
  if (any && !path.empty()) last_video_ = path;
  return any;
}

int MultiMonitor::VerifyPinAndRevert() {
  // PATCH A: revert = full renderer recreate WITHOUT pin + reload +
  // re-verify (never a runtime property swap). Per-slot, never throws:
  // allocation/Create failures keep the old (pinned) renderer instead of
  // leaving the slot dead.
  int reverted = 0;
  for (auto& kv : slots_) {
    Slot& slot = kv.second;
    if (!slot.renderer || !slot.renderer->pin_active()) continue;
    if (!slot.renderer->EverStarted()) continue;  // transient "no" is normal
    if (slot.renderer->IsHwdecActive()) continue;  // d3d11va OR dxva2: fine
    char buf[192];
    std::snprintf(buf, sizeof(buf),
                  "gpu-pin: slot %d hwdec inactive post-start with pin, "
                  "reverting to unpinned",
                  kv.first);
    LogLine(log_, buf);
    auto fresh = std::make_unique<MpvRenderer>();
    void* hwnd = (slot.injector && slot.injector->injected_hwnd() != nullptr)
                     ? slot.injector->injected_hwnd()
                     : headless_host_;
    const bool prev_paused =
        slot.paused.load(std::memory_order_relaxed) ||
        global_paused_.load(std::memory_order_relaxed);
    if (!fresh->Create(hwnd)) {
      LogLine(log_, "gpu-pin: unpinned recreate failed, keeping pinned slot");
      continue;
    }
    if (!last_video_.empty()) fresh->LoadLoop(last_video_);
    if (prev_paused) fresh->Pause();
    slot.renderer = std::move(fresh);
    ++reverted;
  }
  if (reverted > 0) {
    char buf[128];
    std::snprintf(buf, sizeof(buf),
                  "gpu-pin: reverted %d slot(s), re-verifying next pass",
                  reverted);
    LogLine(log_, buf);
  }
  return reverted;
}

void MultiMonitor::ApplyFitModeAll(const std::string& fit_mode) {
  for (auto& kv : slots_) {
    if (!kv.second.renderer) continue;
    double aspect = 0.0;
    if (kv.second.info.width > 0 && kv.second.info.height > 0) {
      aspect = static_cast<double>(kv.second.info.width) /
               static_cast<double>(kv.second.info.height);
    }
    kv.second.renderer->SetFitMode(fit_mode, aspect);
  }
}

void MultiMonitor::PauseAll() {
  global_paused_.store(true, std::memory_order_relaxed);
  for (auto& kv : slots_) {
    if (kv.second.renderer) kv.second.renderer->Pause();
  }
}

void MultiMonitor::ResumeAll() {
  global_paused_.store(false, std::memory_order_relaxed);
  for (auto& kv : slots_) {
    if (!kv.second.renderer) continue;
    // Preserve per-slot state: a slot individually paused via PauseSlot
    // (occlusion) stays paused — re-assert it so the global resume cannot
    // leak a resume underneath.
    if (kv.second.paused.load(std::memory_order_relaxed)) {
      kv.second.renderer->Pause();
    } else {
      kv.second.renderer->Resume();
    }
  }
}

MultiMonitor::Slot* MultiMonitor::SlotAt(size_t idx) {
  size_t i = 0;
  for (auto& kv : slots_) {
    if (i == idx) return &kv.second;
    ++i;
  }
  return nullptr;
}

const MultiMonitor::Slot* MultiMonitor::SlotAt(size_t idx) const {
  size_t i = 0;
  for (const auto& kv : slots_) {
    if (i == idx) return &kv.second;
    ++i;
  }
  return nullptr;
}

void MultiMonitor::PauseSlot(size_t idx, bool pause) {
  Slot* slot = SlotAt(idx);
  if (slot == nullptr) {
    return;
  }
  slot->paused.store(pause, std::memory_order_relaxed);
  if (slot->renderer == nullptr) {
    return;
  }
  if (pause) {
    slot->renderer->Pause();
    return;
  }
  // Global override: a per-slot resume while globally paused only clears
  // the per-slot flag — the renderer stays paused until ResumeAll clears
  // the global state.
  if (global_paused_.load(std::memory_order_relaxed)) {
    return;
  }
  slot->renderer->Resume();
}

bool MultiMonitor::IsSlotPaused(size_t idx) const {
  const Slot* slot = SlotAt(idx);
  if (slot == nullptr) {
    return global_paused_.load(std::memory_order_relaxed);
  }
  return global_paused_.load(std::memory_order_relaxed) ||
         slot->paused.load(std::memory_order_relaxed);
}

void MultiMonitor::Reanchor() {
  try {
    if (mode_ == MultiMonitorMode::Span) {
      // Span mode: a single slot keyed by kSpanSlotId. Explorer restart /
      // power-resume recreate the desktop windows even when the geometry is
      // unchanged, so re-attach the span slot at the current virtual-screen
      // size. Never create per-monitor slots here — that would duplicate the
      // span window.
      const auto it = slots_.find(kSpanSlotId);
      if (it == slots_.end()) {
        AttachSpanSlot();
      } else if (it->second.injector) {
        const SpanGeometry g = GetSpanGeometry();
        ReattachSpanLocked(it->second, it->second.info.x, it->second.info.y,
                           g.width, g.height, /*reassert=*/true);
      }
      initialized_ = !slots_.empty();
      return;
    }
    const std::vector<MonitorInfo> desired =
        FilterMonitors(active_monitor_.load(std::memory_order_acquire));
    // Step 5: converge to the active target first (filtered-out slots torn
    // down, newcomers attached), then force re-attach of the survivors
    // (Explorer restart destroys the desktop windows even when geometry is
    // unchanged). Keeps renderer instances (no video reload — SetHWND
    // re-points them).
    ApplyActiveFilter(desired);
    for (const MonitorInfo& mi : desired) {
      const auto it = slots_.find(mi.id);
      if (it == slots_.end()) {
        continue;
      } else if (it->second.injector) {
        it->second.info = mi;
        it->second.injector->Detach();
        if (it->second.injector->Attach(mi.x, mi.y, mi.width, mi.height) &&
            it->second.renderer) {
          it->second.injector->ReassertFrameless();
          it->second.renderer->SetHWND(it->second.injector->injected_hwnd());
        } else if (it->second.renderer) {
          // Attach failed: fall back to the hidden host (see span branch).
          it->second.renderer->SetHWND(headless_host_);
        }
      }
    }
    initialized_ = !slots_.empty();
  } catch (...) {
    LogLine(log_, "multi_monitor: Reanchor failed, keeping live slots");
  }
}

size_t MultiMonitor::slot_count() const { return slots_.size(); }

bool MultiMonitor::has_headless_slots() const {
  for (const auto& kv : slots_) {
    if (kv.second.injector && kv.second.injector->injected_hwnd() == nullptr) {
      return true;
    }
  }
  return false;
}

int MultiMonitor::headless_slot_count() const {
  int n = 0;
  for (const auto& kv : slots_) {
    if (kv.second.injector && kv.second.injector->injected_hwnd() == nullptr) {
      ++n;
    }
  }
  return n;
}

std::string MultiMonitor::SlotCoverageReason(int monitor_id) const {
  const auto it = slots_.find(monitor_id);
  if (it == slots_.end()) return {};
  return it->second.coverage_reason;
}

std::vector<int> MultiMonitor::monitor_ids() const {
  std::vector<int> ids;
  ids.reserve(slots_.size());
  for (const auto& kv : slots_) {
    ids.push_back(kv.first);
  }
  return ids;
}

MultiMonitorMode MultiMonitor::mode() const { return mode_; }

}  // namespace k6wp
