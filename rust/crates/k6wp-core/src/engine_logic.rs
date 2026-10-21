//! The engine's PURE logic, ported to Rust (task G5).
//!
//! These are the decisions a user can SEE: where the wallpaper lands on a
//! multi-monitor desktop, when a load is retried, when the engine gives up
//! restarting itself, which pause owner hides the picture and which host
//! window may hold our surface.
//!
//! Every function here is a faithful port of the C++ one, and the golden
//! fixtures in `tests/golden/engine_logic.json` pin the behaviour on BOTH
//! sides: the C++ checks them (`engine_logic_test`) and the Rust replays
//! them (`tests/engine_logic_golden.rs`). If the two ever disagree, both
//! gates fail.
//!
//! Nothing here touches the clock, the filesystem or Win32 - the time is a
//! parameter, exactly like in the C++.

/// A rectangle in the virtual-screen coordinates.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct PlacementRect {
    pub left: i32,
    pub top: i32,
    pub right: i32,
    pub bottom: i32,
}

/// The verdict of the placement: does the surface cover the monitor?
///
/// The tokens are the log surface the engine already emits
/// (`placement: covered`, `placement: CLIPPED-left`, ...) - they are frozen
/// with the fixtures, so they are part of the contract.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CoverageVerdict {
    Covered,
    ClippedLeft,
    ClippedTop,
    ClippedRight,
    ClippedBottom,
    OutOfBounds,
}

impl CoverageVerdict {
    /// The stable log token (never changes - the fixtures pin it).
    pub fn as_str(self) -> &'static str {
        match self {
            CoverageVerdict::Covered => "placement: covered",
            CoverageVerdict::ClippedLeft => "placement: CLIPPED-left",
            CoverageVerdict::ClippedTop => "placement: CLIPPED-top",
            CoverageVerdict::ClippedRight => "placement: CLIPPED-right",
            CoverageVerdict::ClippedBottom => "placement: CLIPPED-bottom",
            CoverageVerdict::OutOfBounds => "placement: OUT-OF-BOUNDS",
        }
    }
}

/// Whether the child surface covers the monitor.
///
/// The order of the checks matters and is frozen by the fixtures: a surface
/// that is SMALLER than the monitor is reported as clipped on the left (the
/// first edge that does not reach), and only a surface that does not
/// overlap at all is out of bounds.
pub fn covers_monitor(child: PlacementRect, monitor: PlacementRect) -> CoverageVerdict {
    // Disjoint (touching edges do not overlap) -> nothing visible.
    if child.right <= monitor.left
        || child.left >= monitor.right
        || child.bottom <= monitor.top
        || child.top >= monitor.bottom
    {
        return CoverageVerdict::OutOfBounds;
    }
    if child.left > monitor.left {
        return CoverageVerdict::ClippedLeft;
    }
    if child.top > monitor.top {
        return CoverageVerdict::ClippedTop;
    }
    if child.right < monitor.right {
        return CoverageVerdict::ClippedRight;
    }
    if child.bottom < monitor.bottom {
        return CoverageVerdict::ClippedBottom;
    }
    CoverageVerdict::Covered
}

/// The WorkerW host rule (task C7): the host window may hold ONLY our own
/// (possibly stale) surface - a foreign child means the host is not ours.
///
/// An empty list is accepted: a host with no children is ours by
/// definition.
pub fn workerw_accepts_child_classes(child_classes: &[String], own_class: &str) -> bool {
    child_classes.iter().all(|class| class == own_class)
}

/// The load retry offsets (0 / 250 / 1000 / 3000 ms).
pub const RETRY_COUNT: i32 = 4;
pub const RETRY_OFFSETS_MS: [i64; RETRY_COUNT as usize] = [0, 250, 1000, 3000];

/// The retry clock: when the first pass ended and how many attempts are done.
#[derive(Debug, Clone, Copy, Default)]
pub struct RetryClock {
    /// The steady-clock ms at the pass end (caller-supplied).
    pub first_pass_ms: i64,
    /// The due attempts already executed (0..=RETRY_COUNT).
    pub attempts_done: i32,
}

/// True when attempt `#attempts_done` is due at `now_ms`.
pub fn retry_schedule_due(now_ms: i64, clock: &RetryClock) -> bool {
    if clock.attempts_done < 0 || clock.attempts_done >= RETRY_COUNT {
        return false;
    }
    (now_ms - clock.first_pass_ms) >= RETRY_OFFSETS_MS[clock.attempts_done as usize]
}

/// The ms until the next due attempt (0 when it is already due, -1 when the
/// schedule is exhausted).
pub fn retry_next_delay_ms(now_ms: i64, clock: &RetryClock) -> i64 {
    if clock.attempts_done < 0 || clock.attempts_done >= RETRY_COUNT {
        return -1;
    }
    let elapsed = now_ms - clock.first_pass_ms;
    let delay = RETRY_OFFSETS_MS[clock.attempts_done as usize] - elapsed;
    if delay > 0 {
        delay
    } else {
        0
    }
}

/// True when every scheduled attempt has been executed.
pub fn retry_schedule_exhausted(clock: &RetryClock) -> bool {
    clock.attempts_done >= RETRY_COUNT
}

/// The automatic-restart state.
#[derive(Debug, Clone, Copy, Default)]
pub struct CrashRestartState {
    /// The automatic restarts inside the window.
    pub count: i32,
    /// The monotonic ms when the window opened.
    pub window_start_ms: i64,
}

/// Advances the state for one automatic restart at `now_ms`.
///
/// A non-positive window disables the sliding behaviour: every restart is its
/// own window (the count resets to 1) - the safe interpretation of a
/// misconfigured window, never an unbounded counter.
pub fn crash_restart_advance(
    mut state: CrashRestartState,
    now_ms: i64,
    _max_restarts: i32,
    window_ms: i64,
) -> CrashRestartState {
    let new_window = state.count <= 0
        || window_ms <= 0
        || now_ms < state.window_start_ms
        || now_ms - state.window_start_ms >= window_ms;
    if new_window {
        state.count = 1;
        state.window_start_ms = now_ms;
    } else {
        state.count += 1;
    }
    state
}

/// True while the state is inside the allowed restart budget.
pub fn crash_restart_allowed(state: &CrashRestartState, max_restarts: i32) -> bool {
    state.count <= max_restarts
}

/// One monitor as the reconcile pipeline sees it.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct TopologyMonitor {
    /// The tiered identity (never GDI-only unless it fell back).
    pub stable_key: String,
    pub x: i32,
    pub y: i32,
    pub width: i32,
    pub height: i32,
    pub scale_pct: i32,
    pub orientation: i32,
    pub is_primary: bool,
    pub degraded: bool,
}

/// The pure snapshot of the monitor topology.
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct TopologySnapshot {
    /// The deterministic order: primary first, then the stable key.
    pub monitors: Vec<TopologyMonitor>,
    pub virtual_x: i32,
    pub virtual_y: i32,
    pub virtual_width: i32,
    pub virtual_height: i32,
    /// The stable hash over the canonical serialization.
    pub hash_snapshot: String,
}

/// What changed between two topology snapshots.
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct TopologyDiff {
    /// In the new snapshot, not in the old (sorted).
    pub added_keys: Vec<String>,
    /// In the old snapshot, not in the new (sorted).
    pub removed_keys: Vec<String>,
    /// The x/y changed (sorted).
    pub moved_keys: Vec<String>,
    /// The width/height changed (sorted).
    pub resized_keys: Vec<String>,
    /// The scale changed (sorted).
    pub dpi_keys: Vec<String>,
    /// The orientation changed (sorted).
    pub orientation_keys: Vec<String>,
    pub primary_changed: bool,
    pub hash_changed: bool,
}

impl TopologyDiff {
    /// True when nothing changed (the vectors are empty and the primary did
    /// not change).
    pub fn is_empty(&self) -> bool {
        self.added_keys.is_empty()
            && self.removed_keys.is_empty()
            && self.moved_keys.is_empty()
            && self.resized_keys.is_empty()
            && self.dpi_keys.is_empty()
            && self.orientation_keys.is_empty()
            && !self.primary_changed
    }
}

/// The pure topology diff.
///
/// The keys are identified ONLY by the stable key, and the output ordering is
/// deterministic (sorted), so the same pair of snapshots always produces the
/// same diff - and an identical pair produces an empty one.
pub fn diff_topology(old_s: &TopologySnapshot, new_s: &TopologySnapshot) -> TopologyDiff {
    use std::collections::BTreeMap;

    let mut d = TopologyDiff::default();
    let mut old_by_key: BTreeMap<&str, &TopologyMonitor> = BTreeMap::new();
    for m in &old_s.monitors {
        old_by_key.insert(m.stable_key.as_str(), m);
    }
    let mut new_by_key: BTreeMap<&str, &TopologyMonitor> = BTreeMap::new();
    for m in &new_s.monitors {
        new_by_key.insert(m.stable_key.as_str(), m);
    }

    for key in new_by_key.keys() {
        if !old_by_key.contains_key(key) {
            d.added_keys.push((*key).to_string());
        }
    }
    for key in old_by_key.keys() {
        if !new_by_key.contains_key(key) {
            d.removed_keys.push((*key).to_string());
        }
    }

    // Per-attribute changes for the keys present in both snapshots. A single
    // key can land in several categories at once (moved + resized).
    for (key, o) in &old_by_key {
        let Some(n) = new_by_key.get(key) else {
            continue;
        };
        if o.x != n.x || o.y != n.y {
            d.moved_keys.push((*key).to_string());
        }
        if o.width != n.width || o.height != n.height {
            d.resized_keys.push((*key).to_string());
        }
        if o.scale_pct != n.scale_pct {
            d.dpi_keys.push((*key).to_string());
        }
        if o.orientation != n.orientation {
            d.orientation_keys.push((*key).to_string());
        }
    }

    // The primary change over the key union: an absent key counts as
    // not-primary, so adding or removing the primary monitor is a primary
    // change too.
    let mut keys: std::collections::BTreeSet<&str> = std::collections::BTreeSet::new();
    keys.extend(old_by_key.keys().copied());
    keys.extend(new_by_key.keys().copied());
    for key in keys {
        let old_primary = old_by_key.get(key).is_some_and(|m| m.is_primary);
        let new_primary = new_by_key.get(key).is_some_and(|m| m.is_primary);
        if old_primary != new_primary {
            d.primary_changed = true;
            break;
        }
    }

    d.hash_changed = old_s.hash_snapshot != new_s.hash_snapshot;
    d
}

/// One monitor as the desktop sees it (the reconcile policy's input).
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct MonitorInfo {
    /// The stable id within a session; 0 = primary.
    pub id: i32,
    pub x: i32,
    pub y: i32,
    pub width: i32,
    pub height: i32,
    pub is_primary: bool,
    /// The device name as UTF-8 (e.g. `\\.\DISPLAY1`).
    pub device_name: String,
    pub orientation: i32,
    pub refresh_hz: i32,
    pub scale_pct: i32,
    /// The tiered identity (UTF-8).
    pub stable_key: String,
    pub identity_degraded: bool,
}

/// What the debounced reconcile door must do.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ReconcileAction {
    /// Nothing to do for this trigger.
    None,
    /// Re-enumerate and converge the slot set.
    ConvergeSlots,
    /// Re-anchor the live surfaces.
    ReanchorSlots,
}

/// The pure mapping from a trigger token to the action the door runs.
/// An unknown trigger does nothing.
pub fn decide_reconcile_action(trigger: &str) -> ReconcileAction {
    match trigger {
        "display-change" | "monitor-on" | "device-lost" => ReconcileAction::ConvergeSlots,
        "resume" | "session-unlock" | "taskbar-created" | "setting-change" => {
            ReconcileAction::ReanchorSlots
        }
        _ => ReconcileAction::None,
    }
}

/// The stable, log-friendly name of an action.
pub fn reconcile_action_name(action: ReconcileAction) -> &'static str {
    match action {
        ReconcileAction::ConvergeSlots => "converge",
        ReconcileAction::ReanchorSlots => "reanchor",
        ReconcileAction::None => "none",
    }
}

/// True when the action needs the monitor enumeration to have settled before
/// it runs.
pub fn reconcile_action_needs_stable_topology(action: ReconcileAction) -> bool {
    matches!(action, ReconcileAction::ReanchorSlots)
}

/// The identity key of a monitor: the stable key, else the GDI device name,
/// else its position id.
pub fn slot_identity_key(mi: &MonitorInfo) -> String {
    if !mi.stable_key.is_empty() {
        return mi.stable_key.clone();
    }
    if !mi.device_name.is_empty() {
        return format!("gdi:{}", mi.device_name);
    }
    format!("pos:{}", mi.id)
}

/// True when the two enumerations describe the SAME topology.
///
/// The comparison is order-insensitive. A duplicated identity (an ambiguous
/// EDID, two monitors without a device name) makes the topology
/// undecidable - and therefore not stable.
pub fn topology_stable(first: &[MonitorInfo], second: &[MonitorInfo]) -> bool {
    if first.len() != second.len() {
        return false;
    }
    let mut by_identity: std::collections::BTreeMap<String, &MonitorInfo> =
        std::collections::BTreeMap::new();
    for mi in second {
        let key = slot_identity_key(mi);
        if by_identity.insert(key, mi).is_some() {
            return false;
        }
    }
    for mi in first {
        let key = slot_identity_key(mi);
        let Some(other) = by_identity.get(&key) else {
            return false;
        };
        if mi.x != other.x
            || mi.y != other.y
            || mi.width != other.width
            || mi.height != other.height
            || mi.is_primary != other.is_primary
        {
            return false;
        }
    }
    true
}

/// The stable signature of an enumeration (the monitors joined in a stable
/// order).
pub fn topology_signature(monitors: &[MonitorInfo]) -> String {
    let mut parts: Vec<String> = Vec::with_capacity(monitors.len());
    for mi in monitors {
        parts.push(format!(
            "{}@{},{},{}x{}{}",
            slot_identity_key(mi),
            mi.x,
            mi.y,
            mi.width,
            mi.height,
            if mi.is_primary { "p" } else { "s" }
        ));
    }
    // The enumeration is sorted (primary first, then the device name), but
    // sort anyway so the signature does not depend on the caller's ordering.
    parts.sort();
    parts.join(";")
}

/// The verify delay after every (re)load, in milliseconds.
pub const PIN_VERIFY_DELAY_MS: i64 = 6000;

/// True when the verify pass is due: it is armed AND the deadline elapsed.
/// (An unarmed schedule is never due, whatever the clock says.)
pub fn pin_verify_due_at(armed: bool, deadline_ms: i64, now_ms: i64) -> bool {
    armed && now_ms >= deadline_ms
}

/// The lifetime revert counter: the new total after `n` reverts.
pub fn pin_verify_add_reverts(total: i32, n: i32) -> i32 {
    total + n
}

/// The decode surface's state.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DecodeState {
    /// The file is decoded and shown.
    Active,
    /// The playback is paused (the picture stays on screen).
    Paused,
    /// The mpv instance is gone (the RAM and the GPU are released).
    Unloaded,
}

impl DecodeState {
    /// The stable name of a state (the log surface).
    pub fn as_str(self) -> &'static str {
        match self {
            DecodeState::Active => "active",
            DecodeState::Paused => "paused",
            DecodeState::Unloaded => "unloaded",
        }
    }
}

/// The pure state of the unload machine.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct UnloadMachine {
    pub state: DecodeState,
    /// When the current pause started (the engine clock, ms). 0 = not paused.
    pub pause_started_ms: i64,
    /// Unload after this many ms of a continuous pause. 0 = never unload.
    pub unload_after_ms: i32,
}

impl Default for UnloadMachine {
    fn default() -> Self {
        UnloadMachine {
            state: DecodeState::Active,
            pause_started_ms: 0,
            unload_after_ms: 0,
        }
    }
}

/// What the engine must do after a decision.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct UnloadDecision {
    pub state: DecodeState,
    pub pause_started_ms: i64,
    /// The engine must destroy the decoder (and release the RAM + the GPU).
    pub unload_due: bool,
    /// The engine must create a fresh surface + decoder and re-issue the video.
    pub reload_due: bool,
}

/// The pause mask changed: `paused` is true when a slot-pausing owner is
/// active.
///
/// A pause that is already running keeps its original start (the deadline
/// must not be pushed back by another owner), and a resume from the unloaded
/// state asks for an immediate reload.
pub fn unload_on_pause_changed(
    machine: &UnloadMachine,
    paused: bool,
    now_ms: i64,
) -> UnloadDecision {
    let mut decision = UnloadDecision {
        state: machine.state,
        pause_started_ms: machine.pause_started_ms,
        unload_due: false,
        reload_due: false,
    };

    if paused {
        if machine.state == DecodeState::Active {
            decision.state = DecodeState::Paused;
            decision.pause_started_ms = now_ms;
        }
        return decision;
    }

    // The mask is clear: the engine is active again. If the file was
    // unloaded, bring it back now (a resume must not wait for the next tick).
    decision.reload_due = machine.state == DecodeState::Unloaded;
    decision.state = DecodeState::Active;
    decision.pause_started_ms = 0;
    decision
}

/// The periodic deadline check (the engine calls it from its message loop).
///
/// `mask` is the current pause mask: only the owners that make the wallpaper
/// invisible qualify for the unload (an explicit user pause keeps the
/// picture).
pub fn unload_on_tick(machine: &UnloadMachine, mask: i32, now_ms: i64) -> UnloadDecision {
    let mut decision = UnloadDecision {
        state: machine.state,
        pause_started_ms: machine.pause_started_ms,
        unload_due: false,
        reload_due: false,
    };

    if machine.unload_after_ms <= 0 {
        return decision;
    }
    if machine.state != DecodeState::Paused {
        return decision;
    }
    if machine.pause_started_ms <= 0 {
        return decision;
    }
    if now_ms - machine.pause_started_ms < machine.unload_after_ms as i64 {
        return decision;
    }
    if !pause::hides_wallpaper(mask) {
        return decision;
    }

    decision.state = DecodeState::Unloaded;
    decision.unload_due = true;
    decision
}

/// The pause ownership bits (the same values as the C++ `PauseController`).
pub mod pause {
    pub const USER: i32 = 1;
    pub const FULLSCREEN: i32 = 2;
    pub const SUSPEND: i32 = 4;
    pub const POWER: i32 = 8;
    pub const SESSION_LOCK: i32 = 16;
    pub const SCREEN_OFF: i32 = 32;

    /// The owners that make the wallpaper INVISIBLE - the ones the unload
    /// (task 1.1) may run for. An explicit user pause and the DC power cap
    /// are NOT in the set: in both cases the wallpaper is on screen and
    /// stays there.
    pub const INVISIBLE_OWNERS: i32 = FULLSCREEN | SCREEN_OFF | SESSION_LOCK | SUSPEND;

    /// The UI-visible pause: user, fullscreen, suspend, session-lock or
    /// screen-off.
    pub fn ui_paused(mask: i32) -> bool {
        (mask & (USER | FULLSCREEN | SUSPEND | SESSION_LOCK | SCREEN_OFF)) != 0
    }

    /// The slot pause: ANY owner (including the DC power cap and
    /// screen-off).
    pub fn slots_paused(mask: i32) -> bool {
        mask != 0
    }

    /// True when the pause mask contains an owner that hides the wallpaper.
    pub fn hides_wallpaper(mask: i32) -> bool {
        (mask & INVISIBLE_OWNERS) != 0
    }
}
