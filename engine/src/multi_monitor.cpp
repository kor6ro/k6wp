// Multi-monitor N-handle manager (Todo 33, fase-5). See multi_monitor.hpp.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <unordered_map>

#include "multi_monitor.hpp"
#include "topology_snapshot.hpp"
#include "topology_diff.hpp"
#include "lock_order.hpp"
#include "reconcile_policy.hpp"
#include "renderer_graveyard.hpp"

namespace k6wp {

namespace {

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

// Row 11: test-only GetSpanGeometry override (SetSpanGeometryOverride).
const SpanGeometry* g_span_geometry_override = nullptr;

bool SamePlacementRect(const PlacementRect& a, const PlacementRect& b) {
  return a.left == b.left && a.top == b.top && a.right == b.right &&
         a.bottom == b.bottom;
}

// Row 15: UTF-8 GDI device key -> wide (MonitorInfo::device_name is wide,
// MONITORINFOEXW.szDevice). Empty/undecodable input -> empty wide key,
// which no slot matches (unknown device).
std::wstring WidenDeviceKey(const std::string& device) {
  if (device.empty()) return {};
  const int n =
      MultiByteToWideChar(CP_UTF8, 0, device.c_str(), -1, nullptr, 0);
  if (n <= 0) return {};
  std::wstring out(static_cast<std::size_t>(n - 1), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, device.c_str(), -1, out.data(), n);
  return out;
}

// Row 17: wide -> UTF-8 for the row's display: log lines (%s on device keys
// and assignment paths). Empty/undecodable input -> empty string.
std::string NarrowUtf8(const std::wstring& w) {
  if (w.empty()) return {};
  const int n =
      WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
  if (n <= 0) return {};
  std::string out(static_cast<std::size_t>(n - 1), '\0');
  WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, out.data(), n, nullptr, nullptr);
  return out;
}

}  // namespace

MultiMonitor::MultiMonitor(LogFn log, SlotFactory factory)
    : log_(log), factory_(std::move(factory)) {}

MultiMonitor::~MultiMonitor() { ClearSlots(); }

MultiMonitor::MultiMonitor(MultiMonitor&& other) noexcept
    : log_(other.log_),
      inject_mode_(other.inject_mode_),
      adapter_pin_(std::move(other.adapter_pin_)),
      last_video_(std::move(other.last_video_)),
      active_monitor_(other.active_monitor_.load(std::memory_order_relaxed)),
      filter_armed_(other.filter_armed_),
      factory_(std::move(other.factory_)),
      slots_(std::move(other.slots_)),
      retained_assignments_(std::move(other.retained_assignments_)),
      initialized_(other.initialized_),
      headless_host_(other.headless_host_),
      shared_host_(other.shared_host_),
      attached_host_rect_(other.attached_host_rect_),
      global_paused_(other.global_paused_.load(std::memory_order_relaxed)) {
  other.initialized_ = false;
}

MultiMonitor& MultiMonitor::operator=(MultiMonitor&& other) noexcept {
  if (this != &other) {
    ClearSlots();
    log_ = other.log_;
    inject_mode_ = other.inject_mode_;
    adapter_pin_ = std::move(other.adapter_pin_);
    last_video_ = std::move(other.last_video_);
    active_monitor_.store(other.active_monitor_.load(std::memory_order_relaxed),
                          std::memory_order_relaxed);
    filter_armed_ = other.filter_armed_;
    factory_ = std::move(other.factory_);
    slots_ = std::move(other.slots_);
    retained_assignments_ = std::move(other.retained_assignments_);
    initialized_ = other.initialized_;
    headless_host_ = other.headless_host_;
    shared_host_ = other.shared_host_;
    attached_host_rect_ = other.attached_host_rect_;
    global_paused_.store(other.global_paused_.load(std::memory_order_relaxed),
                         std::memory_order_relaxed);
    other.initialized_ = false;
  }
  return *this;
}

SpanGeometry MultiMonitor::GetSpanGeometry() noexcept {
  if (g_span_geometry_override != nullptr) {
    return *g_span_geometry_override;
  }
  SpanGeometry g;
  g.x = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
  g.y = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
  g.width = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
  g.height = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
  return g;
}

void MultiMonitor::SetSpanGeometryOverride(const SpanGeometry* g) {
  g_span_geometry_override = g;
}

void MultiMonitor::ClearSlots() {
  // DEFERRED TEARDOWN (Bug B): the mpv handles and the injected surfaces go
  // to the renderer graveyard instead of being destroyed inline. The old
  // code let ~MpvRenderer run mpv_terminate_destroy() on the message-loop
  // thread, which waits for the mpv VO thread - unboundedly, when the VO is
  // stuck on a display that just disappeared. That was the freeze.
  for (auto& kv : slots_) {
    DestroySlotLocked(kv.second);
  }
  slots_.clear();
  initialized_ = false;
}

// Row 7: resolve the desktop host once for the current attach pass. Every
// subsequent SetSharedHost in this pass hands the SAME SharedHost to each
// slot injector - no per-slot FindDesktopWindows / 0x052C spawn.
void MultiMonitor::ResolveHostForPass() {
  shared_host_ = ResolveSharedHost(inject_mode_, log_);
}

void MultiMonitor::RememberHostPlacement() {
  // A failed resolution yields a null host and a zeroed rect; keeping the
  // last known placement prevents a later successful pass from seeing a
  // spurious move and destroying a live surface.
  if (shared_host_.host != nullptr) {
    attached_host_rect_ = shared_host_.client_rect;
  }
}

bool MultiMonitor::AttachSlot(const MonitorInfo& mi) {
  Slot slot;
  slot.info = mi;
  // The slot is bound to the MONITOR, not to a position: the identity is
  // what survives a topology change (see reconcile_policy.hpp).
  slot.identity = SlotIdentityKey(mi);
  slot.injector = factory_.make_injector(log_);
  slot.injector->SetInjectMode(inject_mode_);
  // Row 7: pass the pass's resolved host down before Attach - Attach itself
  // no longer discovers or spawns a host.
  slot.injector->SetSharedHost(shared_host_);
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
  slot.renderer = factory_.make_renderer();
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

void MultiMonitor::ReloadSlotVideoLocked(Slot& slot) {
  if (!slot.renderer || slot.video_path.empty()) return;
  // force=true: LoadLoop's same-path guard would otherwise treat this as a
  // no-op, but the player has no file after its VO window was destroyed.
  if (!slot.renderer->LoadLoop(slot.video_path, /*force=*/true)) {
    char buf[320];
    std::snprintf(buf, sizeof(buf),
                  "multi_monitor: post-reattach reload failed: %s",
                  slot.video_path.c_str());
    LogLine(log_, buf);
    return;
  }
  if (global_paused_.load(std::memory_order_relaxed) ||
      slot.paused.load(std::memory_order_relaxed)) {
    slot.renderer->Pause();
  }
}

void MultiMonitor::ReleaseSlotMediaLocked(Slot& slot) {
  // The renderer and the injected surface are handed to the graveyard
  // TOGETHER: the renderer is destroyed on the graveyard worker (the
  // blocking mpv_terminate_destroy must never run on the message-loop
  // thread) and the surface is destroyed on the message-loop thread once
  // the renderer is really gone. Destroying the surface while the VO still
  // renders into it is the cross-thread destroy deadlock this path exists
  // to avoid.
  //
  // The surface is RELEASED (hidden, unparented) right away so the old
  // wallpaper cannot stay visible while the teardown is in flight.
  void* surface = nullptr;
  if (slot.injector) {
    surface = slot.injector->ReleaseSurface();
  }
  if (slot.renderer) {
    std::unique_ptr<MpvRenderer> doomed = std::move(slot.renderer);
    if (!RendererGraveyard::Submit(std::move(doomed), surface, log_)) {
      // The graveyard refused the work (the worker thread could not be
      // created). Destroying the renderer here would block the message-loop
      // thread - the very thing this path avoids - so the renderer is
      // leaked on purpose and the failure is logged loudly. A leaked mpv
      // instance is far better than a frozen engine.
      char buf[224];
      std::snprintf(buf, sizeof(buf),
                    "multi_monitor: CRITICAL - graveyard unavailable, leaked "
                    "one renderer (teardown not blocking the loop)");
      LogLine(log_, buf);
    }
  } else if (surface != nullptr) {
    // No renderer: the window can go straight to the graveyard.
    RendererGraveyard::Submit(nullptr, surface, log_);
  }
}

void MultiMonitor::DestroySlotLocked(Slot& slot) {
  ReleaseSlotMediaLocked(slot);
  slot.injector.reset();
}

std::map<int, MultiMonitor::Slot>::iterator MultiMonitor::FindSlotByIdentityLocked(
    const MonitorInfo& mi) {
  const std::string key = SlotIdentityKey(mi);
  for (auto it = slots_.begin(); it != slots_.end(); ++it) {
    if (it->second.identity == key) return it;
  }
  return slots_.end();
}

std::map<int, MultiMonitor::Slot>::const_iterator
MultiMonitor::FindSlotByIdentityLocked(const MonitorInfo& mi) const {
  const std::string key = SlotIdentityKey(mi);
  for (auto it = slots_.begin(); it != slots_.end(); ++it) {
    if (it->second.identity == key) return it;
  }
  return slots_.end();
}

bool MultiMonitor::ReplaceSlotSurfaceLocked(Slot& slot, int x, int y, int w,
                                            int h, bool reassert) {
  // 1. The old surface and the old mpv handle go to the graveyard. The VO
  //    keeps rendering into the old (now hidden) surface until its handle is
  //    really destroyed - the window is never yanked from under it.
  ReleaseSlotMediaLocked(slot);
  if (!slot.injector) return false;
  // 2. A fresh surface for the new geometry.
  slot.injector->SetSharedHost(shared_host_);
  const bool attached = slot.injector->Attach(x, y, w, h);
  if (!attached) {
    char buf[160];
    std::snprintf(buf, sizeof(buf),
                  "multi_monitor: re-attach failed for monitor %d (%dx%d), "
                  "headless renderer fallback",
                  slot.info.id, w, h);
    LogLine(log_, buf);
    // The slot keeps its injector; the renderer points at the hidden host.
  }
  if (reassert) {
    slot.injector->ReassertFrameless();
  }
  void* hwnd = slot.injector->injected_hwnd();
  if (hwnd == nullptr) {
    // Re-attach failed (Progman missing / SetParent error): point the
    // renderer at the hidden host so mpv never spawns its own framed
    // window (plan Must-NOT-Have).
    hwnd = headless_host_;
  }
  // 3. A NEW renderer for the new surface. Re-pointing the old VO with
  //    SetHWND would restart a VO that is still attached to the old
  //    window; recreating it is the stable choice.
  auto fresh = factory_.make_renderer();
  if (!adapter_pin_.empty()) fresh->SetAdapterPin(adapter_pin_);
  if (!fresh->Create(hwnd)) {
    char buf[160];
    std::snprintf(buf, sizeof(buf),
                  "multi_monitor: MpvRenderer::Create failed for monitor %d "
                  "after re-attach",
                  slot.info.id);
    LogLine(log_, buf);
    return false;
  }
  slot.renderer = std::move(fresh);
  // 4. The new renderer has no file: re-issue the slot's own video.
  ReloadSlotVideoLocked(slot);
  return true;
}

void MultiMonitor::SetHeadlessHost(void* hwnd) { headless_host_ = hwnd; }

void MultiMonitor::SetInjectMode(InjectMode mode) { inject_mode_ = mode; }

void MultiMonitor::SetAdapterPin(const std::string& substr) {
  adapter_pin_ = substr;
}

void MultiMonitor::SetSlotFactory(SlotFactory factory) {
  factory_ = std::move(factory);
}

void MultiMonitor::SetActiveMonitor(int id) {
  if (id == active_monitor_.load(std::memory_order_acquire)) {
    return;
  }
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  active_monitor_.store(id, std::memory_order_release);
  if (initialized_ || filter_armed_) {
    ApplyActiveFilter(FilterMonitors(active_monitor_.load(std::memory_order_acquire)));
    ReapplyRetainedLocked();
  }
}

int MultiMonitor::active_monitor() const {
  return active_monitor_.load(std::memory_order_acquire);
}

void MultiMonitor::ApplyActiveFilter(const std::vector<MonitorInfo>& desired) {
  // Row 7: one host resolution covers this whole pass - the teardown below,
  // every newcomer AttachSlot, and (when this is Init/OnDisplayChange/Reanchor
  // converging the set) the survivors re-attached by the caller.
  ResolveHostForPass();
  // Tear down slots whose monitor vanished (or that were filtered out by the
  // active target). The match is by STABLE IDENTITY, not by position index:
  // unplugging a middle monitor renumbers the others, and the old
  // position-index match tore down the WRONG slot (and kept a renderer that
  // belonged to another monitor).
  for (auto it = slots_.begin(); it != slots_.end();) {
    const std::string& identity = it->second.identity;
    const bool wanted =
        std::any_of(desired.begin(), desired.end(),
                    [&](const MonitorInfo& mi) {
                      return SlotIdentityKey(mi) == identity;
                    });
    if (!wanted) {
      char buf[160];
      std::snprintf(buf, sizeof(buf),
                    "multi_monitor: slot %d (%s) out of target set, tearing down",
                    it->first, identity.c_str());
      LogLine(log_, buf);
      // Keep the slot's video so a RE-PLUGGED monitor gets its assignment
      // back: the retained map is re-applied by ReapplyRetainedLocked as
      // soon as the device returns. Without this an unplug/replug cycle
      // silently dropped the per-monitor assignment forever.
      if (!it->second.video_path.empty() &&
          !it->second.info.device_name.empty()) {
        retained_assignments_[it->second.info.device_name] =
            WidenDeviceKey(it->second.video_path);
      }
      DestroySlotLocked(it->second);
      it = slots_.erase(it);
    } else {
      ++it;
    }
  }
  // Re-key the survivors: the map key is the monitor's CURRENT position id
  // (the user-facing id), while the slot's identity is what binds it to its
  // monitor. A survivor whose id changed (primary switch, middle monitor
  // unplugged) is moved to its new key - the SLOT follows the MONITOR.
  {
    std::map<int, Slot> rebuilt;
    for (auto& kv : slots_) {
      int new_key = kv.first;
      for (const MonitorInfo& mi : desired) {
        if (SlotIdentityKey(mi) == kv.second.identity) {
          new_key = mi.id;
          break;
        }
      }
      Slot moved = std::move(kv.second);
      rebuilt.emplace(new_key, std::move(moved));
    }
    slots_.swap(rebuilt);
  }
  // Attach the missing desired slots.
  for (const MonitorInfo& mi : desired) {
    if (FindSlotByIdentityLocked(mi) == slots_.end()) {
      AttachSlot(mi);
    }
  }
  initialized_ = !slots_.empty();
}

bool MultiMonitor::Init() {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  ClearSlots();
  ApplyActiveFilter(FilterMonitors(active_monitor_.load(std::memory_order_acquire)));
  filter_armed_ = true;
  RememberHostPlacement();
  if (!initialized_) {
    LogLine(log_, "multi_monitor: Init found no monitors, engine keeps running");
  }
  return initialized_;
}

void MultiMonitor::Shutdown() {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  ClearSlots();
}

void MultiMonitor::OnDisplayChange() {
  // Display-change handling must never take the engine down: allocation or
  // attach failures degrade to fewer live slots, never an exception.
  // H1: the lock is taken before the try so an early exception cannot leave
  // the map half-walked; lock_guard unlocks on unwind.
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  try {
    const std::vector<MonitorInfo> desired =
        FilterMonitors(active_monitor_.load(std::memory_order_acquire));
    // 1. Step 5: converge the slot set to the active target (vanished
    //    monitors torn down, filtered-out monitors removed, newcomers
    //    attached). Reuses the OnDisplayChange teardown/attach pattern.
    ApplyActiveFilter(desired);
    // Row 11: a host that moved or was recreated (Explorer restart, DPI
    // change) must re-place survivors even when every monitor rect is
    // unchanged - children are parented to the host client origin. A failed
    // resolution leaves shared_host_.host null; never treat that as a move
    // or a transient failure would destroy a live surface.
    const bool host_moved =
        shared_host_.host != nullptr &&
        !SamePlacementRect(shared_host_.client_rect, attached_host_rect_);
    // 2. Refresh sizes of survivors. The survivor is found by its STABLE
    //    IDENTITY, not by position index: after a primary switch or a
    //    middle monitor being unplugged the position ids shift, and the
    //    old index match handed monitor A's renderer to monitor B.
    for (const MonitorInfo& mi : desired) {
      const auto it = FindSlotByIdentityLocked(mi);
      if (it == slots_.end()) {
        continue;
      }
      Slot& slot = it->second;
      const bool resized = (slot.info.width != mi.width ||
                            slot.info.height != mi.height ||
                            slot.info.x != mi.x || slot.info.y != mi.y);
      slot.info = mi;
      if ((resized || host_moved) && slot.injector) {
        // The surface is REPLACED (new window + new renderer) instead of
        // being destroyed under a live VO and re-pointed: the old code's
        // DesktopInjector::OnDisplayChange destroyed the window while mpv
        // was still rendering into it, then re-pointed the same VO with
        // SetHWND. Both halves of that are the cross-thread destroy
        // hazard. The slot's own video is re-issued afterwards.
        ReplaceSlotSurfaceLocked(slot, mi.x, mi.y, mi.width, mi.height,
                                 /*reassert=*/false);
      }
    }
    ReapplyRetainedLocked();
    RememberHostPlacement();
    initialized_ = !slots_.empty();
  } catch (...) {
    LogLine(log_, "multi_monitor: OnDisplayChange failed, keeping live slots");
  }
}

bool MultiMonitor::LoadLoopAll(const std::string& path, bool force) {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  bool any = false;
  for (auto& kv : slots_) {
    if (kv.second.renderer && kv.second.renderer->LoadLoop(path, force)) {
      any = true;
      if (!path.empty()) kv.second.video_path = path;
    }
  }
  if (any && !path.empty()) last_video_ = path;
  return any;
}

bool MultiMonitor::CaptureLiveFrame(const std::string& path) {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  for (auto& kv : slots_) {
    if (kv.second.renderer && kv.second.renderer->ScreenshotToFile(path)) {
      return true;
    }
  }
  return false;
}

void MultiMonitor::SeekAllTo(double seconds) {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  for (auto& kv : slots_) {
    if (kv.second.renderer) kv.second.renderer->SeekTo(seconds);
  }
}

bool MultiMonitor::ReadLiveCounter(const std::string& name,
                                   long long& out) const {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  for (const auto& kv : slots_) {
    if (kv.second.renderer) {
      long long value = 0;
      if (kv.second.renderer->GetInt64Property(name, value)) {
        out = value;
        return true;
      }
      // The property is unavailable on this mpv build: keep looking, but do
      // NOT report a value from a different slot.
      return false;
    }
  }
  return false;
}

int MultiMonitor::UnloadAllVideos() {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  int count = 0;
  for (auto& kv : slots_) {
    if (kv.second.renderer && kv.second.renderer->UnloadVideo()) ++count;
  }
  return count;
}

int MultiMonitor::UnloadAllSlots() {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  int count = 0;
  for (auto& kv : slots_) {
    Slot& slot = kv.second;
    if (!slot.injector) continue;
    // 1. Hide mpv's VO child inside OUR surface (asynchronous, local).
    //    The layered surface then paints nothing and the shell's wallpaper
    //    shows through it: no black frame, no flash.
    slot.injector->HideVoChildren();
    // 2. Hand the mpv handle AND the surface window to the graveyard: the
    //    teardown happens on the background worker, never on the message
    //    loop, and the whole instance goes (the RAM, not just the file).
    ReleaseSlotMediaLocked(slot);
    ++count;
  }
  return count;
}

int MultiMonitor::ReattachAllSlots() {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  int count = 0;
  for (auto& kv : slots_) {
    Slot& slot = kv.second;
    if (!slot.injector) continue;
    // A fresh surface + a fresh renderer + the slot's video re-issued.
    // ReplaceSlotSurfaceLocked is the same path the re-anchor uses.
    if (ReplaceSlotSurfaceLocked(slot, slot.info.x, slot.info.y,
                                 slot.info.width, slot.info.height,
                                 /*reassert=*/true)) {
      ++count;
    }
  }
  return count;
}

int MultiMonitor::ReloadAllVideos() {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  int count = 0;
  for (auto& kv : slots_) {
    if (kv.second.renderer && kv.second.renderer->ReloadVideo()) ++count;
  }
  return count;
}

bool MultiMonitor::any_video_unloaded() const {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  for (const auto& kv : slots_) {
    if (kv.second.renderer && kv.second.renderer->video_unloaded()) return true;
  }
  return false;
}

bool MultiMonitor::ReadLiveCounterText(const std::string& name,
                                       std::string& out) const {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  for (const auto& kv : slots_) {
    if (kv.second.renderer) {
      return kv.second.renderer->GetPropertyValue(name, out);
    }
  }
  return false;
}

bool MultiMonitor::DumpLivePropertyList(const std::wstring& path) const {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  for (const auto& kv : slots_) {
    if (kv.second.renderer) {
      return kv.second.renderer->DumpPropertyList(path);
    }
  }
  return false;
}

bool MultiMonitor::HasAssignment(const std::string& device) const {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  const std::wstring key = WidenDeviceKey(device);
  if (key.empty()) return false;
  for (const auto& kv : slots_) {
    if (kv.second.info.device_name == key) return true;
  }
  return false;
}

bool MultiMonitor::LoadLoopSlot(const std::string& device,
                                const std::string& path,
                                const std::string& fit_mode) {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  const std::wstring key = WidenDeviceKey(device);
  if (key.empty() || path.empty()) return false;
  for (auto& kv : slots_) {
    Slot& slot = kv.second;
    if (slot.info.device_name != key) continue;
    if (!slot.renderer || !slot.renderer->LoadLoop(path)) return false;
    slot.video_path = path;
    if (!fit_mode.empty()) {
      double aspect = 0.0;
      if (slot.info.width > 0 && slot.info.height > 0) {
        aspect = static_cast<double>(slot.info.width) /
                 static_cast<double>(slot.info.height);
      }
      slot.renderer->SetFitMode(fit_mode, aspect);
    }
    return true;
  }
  return false;
}

BootAssignmentStats MultiMonitor::ApplyBootAssignments(
    const DisplaysConfig& cfg, const std::string& fit_mode) {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  BootAssignmentStats stats;
  for (const auto& [key, assignment] : cfg.assignments) {
    if (key.empty() || assignment.path.empty()) continue;
    const std::string device = NarrowUtf8(key);
    const std::string path_utf8 = NarrowUtf8(assignment.path);
    bool slot_live = false;
    for (const auto& kv : slots_) {
      if (kv.second.info.device_name == key) {
        slot_live = true;
        break;
      }
    }
    if (!slot_live) {
      retained_assignments_[key] = assignment.path;
      ++stats.retained;
      char buf[320];
      std::snprintf(buf, sizeof(buf),
                    "display: retained assignment for absent %s",
                    device.c_str());
      LogLine(log_, buf);
      continue;
    }
    std::error_code ec;
    const std::filesystem::path as_path(assignment.path);
    if (!std::filesystem::exists(as_path, ec) || ec) {
      ++stats.skipped_missing;
      char buf[512];
      std::snprintf(buf, sizeof(buf),
                    "display: assignment path missing for %s: %s - falling "
                    "back to default",
                    device.c_str(), path_utf8.c_str());
      LogLine(log_, buf);
      continue;
    }
    if (LoadLoopSlot(device, path_utf8, fit_mode)) {
      ++stats.applied;
      char buf[320];
      std::snprintf(buf, sizeof(buf), "display: applied %s to %s",
                    path_utf8.c_str(), device.c_str());
      LogLine(log_, buf);
    } else {
      char buf[320];
      std::snprintf(buf, sizeof(buf),
                    "display: assignment apply failed for %s: %s",
                    device.c_str(), path_utf8.c_str());
      LogLine(log_, buf);
    }
  }
  return stats;
}

std::vector<std::wstring> MultiMonitor::DetectKeyCollisionForCommand(
    const DisplaysConfig& cfg, const DisplayVideoCommand& cmd,
    const std::vector<MonitorInfo>& monitors, LogFn log) {
  // Row 33 (IS-7/GAP-14): prospective assignment map = cfg with cmd
  // applied (assign overwrites the device key; clear erases it), then the
  // REAL DetectKeyCollision. Pure gate — callers own the mutation.
  DisplaysConfig prospective = cfg;
  if (cmd.clear) {
    prospective.assignments.erase(WidenDeviceKey(cmd.device));
  } else {
    MonitorAssignment assignment;
    assignment.path = WidenDeviceKey(cmd.path);
    std::error_code ec;
    assignment.exists = !cmd.path.empty() &&
                        std::filesystem::exists(
                            std::filesystem::u8path(cmd.path), ec) &&
                        !ec;
    prospective.assignments[WidenDeviceKey(cmd.device)] = std::move(assignment);
  }
  const std::vector<std::wstring> hits =
      DetectKeyCollision(prospective, monitors);
  if (hits.empty()) return hits;
  if (log != nullptr) {
    std::string keys;
    for (std::size_t i = 0; i < hits.size(); ++i) {
      if (i != 0) keys += ", ";
      keys += NarrowUtf8(hits[i]);
    }
    char buf[512];
    std::snprintf(buf, sizeof(buf),
                  "ipc: set_display_video refused (duplicate-mode collision): %s",
                  keys.c_str());
    LogLine(log, buf);
  }
  return hits;
}

std::vector<std::wstring> MultiMonitor::DropCollidingAssignments(
    DisplaysConfig& cfg, const std::vector<MonitorInfo>& monitors, LogFn log) {
  // Row 33 (IS-7/GAP-14): keep-first load-path gate. DetectKeyCollision
  // returns sorted keys (it sorts + uniques at the end); cfg.assignments
  // is std::map keyed by the same wide device names, so hits[0] is the
  // first colliding key in map order — it survives, the rest drop.
  const std::vector<std::wstring> hits = DetectKeyCollision(cfg, monitors);
  if (hits.empty()) return {};
  if (log != nullptr) {
    std::string keys;
    for (std::size_t i = 0; i < hits.size(); ++i) {
      if (i != 0) keys += ", ";
      keys += NarrowUtf8(hits[i]);
    }
    char buf[512];
    std::snprintf(buf, sizeof(buf),
                  "display: refusing assignments with colliding keys: %s",
                  keys.c_str());
    LogLine(log, buf);
  }
  for (std::size_t i = 1; i < hits.size(); ++i) {
    cfg.assignments.erase(hits[i]);
  }
  return std::vector<std::wstring>(hits.begin() + 1, hits.end());
}

void MultiMonitor::ReapplyRetainedLocked() {
  if (retained_assignments_.empty()) return;
  for (auto it = retained_assignments_.begin();
       it != retained_assignments_.end();) {
    const std::wstring& key = it->first;
    const std::wstring& path = it->second;
    Slot* slot = nullptr;
    for (auto& kv : slots_) {
      if (kv.second.info.device_name == key) {
        slot = &kv.second;
        break;
      }
    }
    if (slot == nullptr || slot->renderer == nullptr || path.empty()) {
      ++it;
      continue;
    }
    std::error_code ec;
    if (!std::filesystem::exists(std::filesystem::path(path), ec) || ec) {
      ++it;
      continue;
    }
    const std::string path_utf8 = NarrowUtf8(path);
    if (slot->renderer->LoadLoop(path_utf8)) {
      slot->video_path = path_utf8;
      char buf[320];
      std::snprintf(buf, sizeof(buf), "display: applied %s to %s",
                    path_utf8.c_str(), NarrowUtf8(key).c_str());
      LogLine(log_, buf);
      it = retained_assignments_.erase(it);
    } else {
      ++it;
    }
  }
}

int MultiMonitor::VerifyPinAndRevert() {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
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
    auto fresh = factory_.make_renderer();
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
    const std::string& reload =
        slot.video_path.empty() ? last_video_ : slot.video_path;
    if (!reload.empty()) fresh->LoadLoop(reload);
    if (prev_paused) fresh->Pause();
    // The old (pinned) renderer is destroyed on the graveyard worker: its
    // destructor blocks in mpv_terminate_destroy() and must never run on
    // the message-loop thread.
    std::unique_ptr<MpvRenderer> doomed = std::move(slot.renderer);
    RendererGraveyard::Submit(std::move(doomed), nullptr, log_);
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
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
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

void MultiMonitor::ApplyFpsCapAll(int fps) {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  for (auto& kv : slots_) {
    if (kv.second.renderer) kv.second.renderer->SetFpsCap(fps);
  }
}

void MultiMonitor::PauseAll() {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  global_paused_.store(true, std::memory_order_relaxed);
  for (auto& kv : slots_) {
    if (kv.second.renderer) kv.second.renderer->Pause();
  }
}

void MultiMonitor::ResumeAll() {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
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
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
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
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  const Slot* slot = SlotAt(idx);
  if (slot == nullptr) {
    return global_paused_.load(std::memory_order_relaxed);
  }
  return global_paused_.load(std::memory_order_relaxed) ||
         slot->paused.load(std::memory_order_relaxed);
}

void MultiMonitor::Reanchor() {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  try {
    const std::vector<MonitorInfo> desired =
        FilterMonitors(active_monitor_.load(std::memory_order_acquire));
    // Step 5: converge to the active target first (filtered-out slots torn
    // down, newcomers attached), then re-anchor the survivors. A survivor
    // whose surface is still alive and unchanged keeps its renderer and
    // stream; only a real re-attach (surface gone, host moved, geometry
    // changed) destroys the old surface and force-reloads the slot video.
    ApplyActiveFilter(desired);
    // A failed host resolution leaves shared_host_.host null; treat the
    // placement as unchanged rather than destroying a live surface.
    const bool host_moved =
        shared_host_.host != nullptr &&
        !SamePlacementRect(shared_host_.client_rect, attached_host_rect_);
    for (const MonitorInfo& mi : desired) {
      const auto it = FindSlotByIdentityLocked(mi);
      if (it == slots_.end()) {
        continue;
      }
      Slot& slot = it->second;
      if (!slot.injector) {
        continue;
      }
      const bool geometry_changed =
          slot.info.x != mi.x || slot.info.y != mi.y ||
          slot.info.width != mi.width || slot.info.height != mi.height;
      slot.info = mi;
      // Row 7: survivors take the pass's shared host too (ApplyActiveFilter
      // above resolved it for this Reanchor pass).
      slot.injector->SetSharedHost(shared_host_);
      if (!geometry_changed && !host_moved && slot.injector->IsSurfaceAlive()) {
        // Power resume: the desktop host AND the injected child survived
        // sleep. Replacing the surface here would destroy the VO window and
        // end the stream (`mpv: end-file reason=quit`) — the original
        // resume bug.
        continue;
      }
      // A real re-attach: the old surface is released (not destroyed under
      // the live VO), a fresh surface is attached and a fresh renderer is
      // created for it. The slot's own video is re-issued afterwards.
      ReplaceSlotSurfaceLocked(slot, mi.x, mi.y, mi.width, mi.height,
                               /*reassert=*/true);
    }
    RememberHostPlacement();
    ReapplyRetainedLocked();
    initialized_ = !slots_.empty();
  } catch (...) {
    LogLine(log_, "multi_monitor: Reanchor failed, keeping live slots");
  }
}

size_t MultiMonitor::slot_count() const {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  return slots_.size();
}

bool MultiMonitor::has_headless_slots() const {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  for (const auto& kv : slots_) {
    if (kv.second.injector && kv.second.injector->injected_hwnd() == nullptr) {
      return true;
    }
  }
  return false;
}

int MultiMonitor::headless_slot_count() const {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  int n = 0;
  for (const auto& kv : slots_) {
    if (kv.second.injector && kv.second.injector->injected_hwnd() == nullptr) {
      ++n;
    }
  }
  return n;
}

std::string MultiMonitor::SlotCoverageReason(int monitor_id) const {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  const auto it = slots_.find(monitor_id);
  if (it == slots_.end()) return {};
  return it->second.coverage_reason;
}

std::vector<SlotSnapshot> MultiMonitor::SnapshotSlots() const {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  std::vector<SlotSnapshot> out;
  out.reserve(slots_.size());
  for (const auto& kv : slots_) {
    SlotSnapshot s;
    s.device = NarrowUtf8(kv.second.info.device_name);
    s.coverage = kv.second.coverage_reason;
    s.headless = kv.second.injector != nullptr &&
                 kv.second.injector->injected_hwnd() == nullptr;
    out.push_back(std::move(s));
  }
  return out;
}

std::vector<int> MultiMonitor::monitor_ids() const {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  std::vector<int> ids;
  ids.reserve(slots_.size());
  for (const auto& kv : slots_) {
    ids.push_back(kv.first);
  }
  return ids;
}



// P3: free function TopologyDivergesFromMonitors (review item R12.1)
// Returns true when any monitor's geometry/scale/orientation/primary differs.
// Key matching: stable_key if non-empty, else gdi_name.
bool TopologyDivergesFromMonitors(const TopologySnapshot& snap,
                                   const std::vector<MonitorInfo>& live) {
  // Build a key->MonitorInfo map from live
  std::unordered_map<std::string, const MonitorInfo*> live_by_key;
  live_by_key.reserve(live.size());
  for (const auto& mi : live) {
    std::string key = mi.stable_key.empty() ? NarrowUtf8(mi.device_name) : mi.stable_key;
    live_by_key.emplace(key, &mi);
  }
  // Compare each snapshot monitor against live
  for (const auto& sm : snap.monitors) {
    auto it = live_by_key.find(sm.stable_key.empty() ? sm.gdi_name : sm.stable_key);
    if (it == live_by_key.end()) return true;  // snapshot monitor not in live
    const MonitorInfo* lm = it->second;
    if (sm.x != lm->x || sm.y != lm->y || sm.width != lm->width ||
        sm.height != lm->height || sm.scale_pct != lm->scale_pct ||
        sm.orientation != lm->orientation || sm.is_primary != lm->is_primary) {
      return true;
    }
  }
  // Check live monitors not in snapshot
  std::unordered_map<std::string, bool> seen;
  for (const auto& sm : snap.monitors) {
    std::string key = sm.stable_key.empty() ? sm.gdi_name : sm.stable_key;
    seen[key] = true;
  }
  for (const auto& mi : live) {
    std::string key = mi.stable_key.empty() ? NarrowUtf8(mi.device_name) : mi.stable_key;
    if (!seen.count(key)) return true;
  }
  return false;
}

TopologySnapshot SnapshotTopology() {
  std::vector<MonitorInfo> raw = ListMonitors();
  std::sort(raw.begin(), raw.end(), [](const MonitorInfo& a, const MonitorInfo& b) {
    if (a.is_primary != b.is_primary) return a.is_primary > b.is_primary;
    return a.stable_key < b.stable_key;
  });
  std::vector<TopologyMonitor> monitors;
  monitors.reserve(raw.size());
  for (const auto& mi : raw) {
    TopologyMonitor tm;
    tm.stable_key = mi.stable_key;
    tm.gdi_name = NarrowUtf8(mi.device_name);
    tm.x = mi.x; tm.y = mi.y;
    tm.width = mi.width; tm.height = mi.height;
    tm.scale_pct = mi.scale_pct; tm.orientation = mi.orientation;
    tm.is_primary = mi.is_primary; tm.degraded = mi.identity_degraded;
    monitors.push_back(std::move(tm));
  }
  TopologySnapshot snap;
  snap.monitors = std::move(monitors);
  SpanGeometry g = MultiMonitor::GetSpanGeometry();
  snap.virtual_x = g.x; snap.virtual_y = g.y;
  snap.virtual_width = g.width; snap.virtual_height = g.height;
  // FNV-1a 64 hash over canonical serialization
  std::string buf;
  for (const auto& m : snap.monitors) {
    buf += m.stable_key; buf += ",";
    buf += m.gdi_name; buf += ",";
    char tmp[64];
    std::snprintf(tmp, sizeof(tmp), "%d,%d,%d,%d,%d,%d,%d",
                  m.x, m.y, m.width, m.height, m.scale_pct, m.orientation,
                  m.is_primary ? 1 : 0);
    buf += tmp;
  }
  char vis[64];
  std::snprintf(vis, sizeof(vis), "%d,%d,%d,%d",
                snap.virtual_x, snap.virtual_y, snap.virtual_width, snap.virtual_height);
  buf += vis;
  uint64_t h = 0xcbf29ce484222325ULL;
  for (char c : buf) {
    h ^= static_cast<uint64_t>(c);
    h *= 0x100000001b3ULL;
  }
  char out[17];
  std::snprintf(out, sizeof(out), "%016llx",
                static_cast<unsigned long long>(h));
  snap.hash_snapshot = out;
  return snap;
}

std::vector<MonitorInfo> MultiMonitor::LiveMonitorInfos() const {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  std::vector<MonitorInfo> out;
  out.reserve(slots_.size());
  for (const auto& kv : slots_) out.push_back(kv.second.info);
  return out;
}

MultiMonitor::CoverageCensus MultiMonitor::VerifyCoverage() const {
  k6wp::lock_order::RecursiveSlotsGuard guard(slots_mutex_);
  CoverageCensus census;
  census.total = static_cast<int>(slots_.size());
  int covered = 0;
  std::vector<std::string> degraded_keys;
  for (const auto& kv : slots_) {
    const auto& slot = kv.second;
    if (slot.coverage_reason == "placement: covered") ++covered;
    if (slot.info.identity_degraded) {
      degraded_keys.push_back(slot.info.stable_key.empty()
                                  ? NarrowUtf8(slot.info.device_name)
                                  : slot.info.stable_key);
    }
  }
  census.covered = covered;
  census.degraded_keys = std::move(degraded_keys);
  return census;
}

MultiMonitor::TopologyApplyResult MultiMonitor::ApplyTopology(
    const TopologySnapshot&, const TopologyDiff&) {
  // Shadow mode: read-only no-op. Never mutates slots_, never writes
  // displays.json (B14). Returns zero counts.
  return {};
}

std::string MultiMonitor::SharedHostBranch() const {
  if (shared_host_.client_rect.right <= shared_host_.client_rect.left ||
      shared_host_.client_rect.bottom <= shared_host_.client_rect.top) {
    return "(none)";
  }
  char buf[256];
  std::snprintf(buf, sizeof(buf), "%dx%d@%d+%d",
                shared_host_.client_rect.right - shared_host_.client_rect.left,
                shared_host_.client_rect.bottom - shared_host_.client_rect.top,
                shared_host_.client_rect.left, shared_host_.client_rect.top);
  return std::string(buf);
}

}  // namespace k6wp
