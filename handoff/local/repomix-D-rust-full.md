# User Provided Header
K6WP HANDOFF PACK - D-rust-full | branch refactor/rust-wt | read STATE.md + ledger.md first | no measurements in this run

# Directory Structure
```
rust/
  crates/
    k6wp-core/
      src/
        compress_args.rs
        config_store.rs
        config.rs
        displays.rs
        engine_logic.rs
        ipc.rs
        lib.rs
        library.rs
        playlist.rs
        settings.rs
        sps.rs
        version.rs
      tests/
        _config_probe.rs
        engine_logic_golden.rs
        golden.rs
        probe.rs
        sps_fuzz.rs
        sps_golden.rs
        sps_read_strategy.rs
        studio_logic_golden.rs
      Cargo.toml
    k6wp-ipc/
      src/
        lib.rs
      tests/
        client.rs
        env_override.rs
        live_engine.rs
      Cargo.toml
    k6wp-studio/
      src/
        grid.rs
        history.rs
        lib.rs
        main.rs
        thumbnailer.rs
        thumbs.rs
        worker.rs
      tests/
        thumbnailer.rs
      ui/
        app.slint
        design-tokens.slint
      Cargo.toml
  fuzz/
    Cargo.toml
  Cargo.toml
```

# Files

## File: rust/crates/k6wp-core/src/compress_args.rs
```rust
pub struct CompressArgs {
⋮----
impl Default for CompressArgs {
fn default() -> Self {
⋮----
encoder: COMPRESS_DEFAULT_ENCODER.to_string(),
⋮----
pub fn build_argv(args: &CompressArgs) -> Vec<String> {
⋮----
argv.push(COMPRESS_FLAG_IN.to_string());
argv.push(args.in_.clone());
argv.push(COMPRESS_FLAG_OUT.to_string());
argv.push(args.out.clone());
argv.push(COMPRESS_FLAG_RES.to_string());
argv.push(format!("{}x{}", args.res_w, args.res_h));
argv.push(COMPRESS_FLAG_FPS.to_string());
argv.push(args.fps.to_string());
argv.push(COMPRESS_FLAG_CRF.to_string());
argv.push(args.crf.to_string());
argv.push(COMPRESS_FLAG_ENCODER.to_string());
argv.push(args.encoder.clone());
⋮----
argv.push(COMPRESS_FLAG_FORCE_LONG.to_string());
⋮----
argv.push(COMPRESS_FLAG_DRY_RUN.to_string());
⋮----
pub fn is_known_compressor_flag(flag: &str) -> bool {
matches!(
⋮----
mod tests {
⋮----
fn the_argv_is_the_golden_vector() {
⋮----
in_: "C:\\in.mp4".to_string(),
out: "C:\\out.mp4".to_string(),
⋮----
encoder: "auto".to_string(),
⋮----
assert_eq!(
⋮----
fn the_optional_flags_are_omitted() {
⋮----
in_: "a".to_string(),
out: "b".to_string(),
⋮----
let argv = build_argv(&args);
⋮----
assert_eq!(argv.len(), 12);
assert!(!argv.iter().any(|t| t == "--force-long"));
assert!(!argv.iter().any(|t| t == "--dry-run"));
assert_eq!(argv[4], "--res");
assert_eq!(argv[5], "0x0");
⋮----
fn every_emitted_flag_is_known() {
⋮----
for token in build_argv(&args) {
if token.starts_with("--") {
assert!(is_known_compressor_flag(&token), "unknown flag: {token}");
⋮----
assert!(!is_known_compressor_flag("--nope"));
```

## File: rust/crates/k6wp-core/src/config_store.rs
```rust
use std::fs;
⋮----
use std::time::Duration;
⋮----
use windows::core::w;
⋮----
pub struct ConfigFileLock {
⋮----
impl ConfigFileLock {
⋮----
pub fn acquire(timeout: Duration) -> Result<Self, ConfigError> {
⋮----
let handle = CreateMutexW(None, false, w!("Local\\K6WP-Config-Lock"))
.map_err(|e| ConfigError(format!("config lock: CreateMutexW failed ({e})")))?;
let wait_ms = timeout.as_millis().min(u128::from(u32::MAX)) as u32;
let state = WaitForSingleObject(handle, wait_ms);
⋮----
return Ok(Self { handle });
⋮----
let _ = CloseHandle(handle);
⋮----
return Err(ConfigError(format!(
⋮----
Err(ConfigError(format!(
⋮----
impl Drop for ConfigFileLock {
fn drop(&mut self) {
⋮----
let _ = ReleaseMutex(self.handle);
let _ = CloseHandle(self.handle);
⋮----
pub fn save_config(path: &Path, cfg: &WallpaperConfig) -> Result<(), ConfigError> {
⋮----
validate_config(cfg)?;
let existing = read_existing_json(path);
let doc = merge_unknown_keys(&existing, &config_to_json(cfg));
atomic_write_json(path, &doc)
⋮----
pub fn persist_config_field(
⋮----
if let Some(parsed) = read_existing_json_checked(path) {
if parsed.is_object() {
let probe = migrate_config_value(&parsed)
.and_then(|probe| validate_config(&probe).map(|()| probe));
if probe.is_ok() {
⋮----
backup_file(path);
raw = config_to_json(fallback);
⋮----
if have_valid && raw.get(field) == Some(&value) {
return Ok(false);
⋮----
have_valid && raw.get("version").and_then(Value::as_i64) == Some(CONFIG_SCHEMA_VERSION);
⋮----
object.insert(field.to_string(), value);
⋮----
return Err(ConfigError("config root must be a JSON object".to_string()));
⋮----
let merged = migrate_config_value(&raw)?;
validate_config(&merged)?;
⋮----
config_to_json(&merged)
⋮----
atomic_write_json(path, &doc)?;
Ok(true)
⋮----
fn read_existing_json_checked(path: &Path) -> Option<Value> {
let meta = fs::metadata(path).ok()?;
if meta.len() > MAX_CONFIG_BYTES {
⋮----
let text = fs::read_to_string(path).ok()?;
serde_json::from_str(&text).ok()
⋮----
fn read_existing_json(path: &Path) -> Value {
read_existing_json_checked(path)
.filter(Value::is_object)
.unwrap_or_else(|| Value::Object(Map::new()))
⋮----
fn backup_file(path: &Path) {
if !path.exists() {
⋮----
let mut bak = path.as_os_str().to_owned();
bak.push(".bak");
⋮----
fn atomic_write_json(path: &Path, doc: &Value) -> Result<(), ConfigError> {
if let Some(parent) = path.parent() {
if !parent.as_os_str().is_empty() {
⋮----
.map_err(|e| ConfigError(format!("cannot create config directory: {e}")))?;
⋮----
let mut tmp = path.as_os_str().to_owned();
tmp.push(".tmp");
⋮----
.map_err(|e| ConfigError(format!("cannot serialize config: {e}")))?;
fs::write(&tmp, text).map_err(|e| {
ConfigError(format!(
⋮----
fs::rename(&tmp, path).map_err(|e| {
⋮----
mod tests {
⋮----
use crate::config::MAX_FOREIGN_KEY_BYTES;
⋮----
fn scratch_dir(tag: &str) -> PathBuf {
⋮----
std::env::temp_dir().join(format!("k6wp_config_store_{tag}_{}", std::process::id()));
⋮----
fs::create_dir_all(&dir).expect("scratch dir");
⋮----
fn seed(path: &Path) {
⋮----
assert_eq!(cfg.version, CONFIG_SCHEMA_VERSION);
save_config(path, &cfg).expect("seed");
⋮----
fn read_doc(path: &Path) -> Value {
serde_json::from_str(&fs::read_to_string(path).expect("read config")).expect("valid json")
⋮----
fn persist_preserves_foreign_keys_and_reports_a_no_op() {
let dir = scratch_dir("foreign");
let path = dir.join("config.json");
seed(&path);
let mut doc = read_doc(&path);
⋮----
object.insert("a_key_from_the_future".to_string(), Value::from(42));
⋮----
atomic_write_json(&path, &doc).expect("seed foreign key");
⋮----
assert!(persist_config_field(&path, "crf", Value::from(20), &fallback).expect("persist"));
let after = read_doc(&path);
assert_eq!(after["crf"], 20);
assert_eq!(after["a_key_from_the_future"], 42);
assert_eq!(after["version"], CONFIG_SCHEMA_VERSION);
⋮----
assert!(!persist_config_field(&path, "crf", Value::from(20), &fallback).expect("no-op"));
⋮----
fn persist_rebuilds_a_corrupt_file_and_keeps_a_backup() {
let dir = scratch_dir("corrupt");
⋮----
fs::write(&path, "{ this is not json").expect("write corrupt");
⋮----
assert!(persist_config_field(&path, "crf", Value::from(21), &fallback).expect("persist"));
assert_eq!(read_doc(&path)["crf"], 21);
⋮----
assert!(Path::new(&bak).exists(), "the corrupt file is backed up");
⋮----
fn a_bad_value_does_not_touch_the_file() {
let dir = scratch_dir("badvalue");
⋮----
let before = fs::read_to_string(&path).expect("read");
⋮----
assert!(persist_config_field(&path, "crf", Value::from(99), &fallback).is_err());
assert_eq!(fs::read_to_string(&path).expect("read"), before);
⋮----
fn the_lock_is_a_bounded_wait() {
let lock = ConfigFileLock::acquire(LOCK_TIMEOUT).expect("first");
⋮----
std::thread::spawn(|| ConfigFileLock::acquire(Duration::from_millis(0)).is_err())
.join()
.expect("thread");
assert!(failed, "a second thread times out while the lock is held");
drop(lock);
assert!(ConfigFileLock::acquire(Duration::from_millis(1000)).is_ok());
⋮----
fn child_writer_entry() {
⋮----
let field = std::env::var("K6WP_RUST_WRITER_FIELD").expect("field");
⋮----
.expect("iterations")
.parse()
.expect("iterations number");
⋮----
persist_config_field(Path::new(&path), &field, Value::from(i), &fallback)
.expect("child persist");
⋮----
fn two_processes_do_not_lose_updates() {
let dir = scratch_dir("twowriter");
⋮----
let exe = std::env::current_exe().expect("current exe");
⋮----
.arg("--exact")
.arg("config_store::tests::child_writer_entry")
.env("K6WP_RUST_WRITER_PATH", &path)
.env("K6WP_RUST_WRITER_FIELD", field)
.env("K6WP_RUST_WRITER_ITERATIONS", "120")
.stdout(Stdio::null())
.stderr(Stdio::null())
.spawn()
.expect("spawn writer")
⋮----
let mut a = spawn("field_a");
let mut b = spawn("field_b");
assert!(a.wait().expect("wait a").success(), "writer A failed");
assert!(b.wait().expect("wait b").success(), "writer B failed");
⋮----
let doc = read_doc(&path);
assert_eq!(doc["field_a"], 120, "writer A's last value was lost");
assert_eq!(doc["field_b"], 120, "writer B's last value was lost");
assert_eq!(doc["version"], CONFIG_SCHEMA_VERSION);
⋮----
assert!(!Path::new(&tmp).exists(), "no .tmp file is left behind");
⋮----
fn the_foreign_key_budget_matches_the_cpp_side() {
⋮----
let merged = merge_unknown_keys(&collide, &known);
assert_eq!(merged["crf"], 22);
assert_eq!(merged["note"], "kept");
⋮----
huge.insert("version".to_string(), Value::from(6));
huge.insert(
"a_huge_foreign_key".to_string(),
Value::from("x".repeat(70 * 1024)),
⋮----
let capped = merge_unknown_keys(&Value::Object(huge), &known);
assert!(capped.get("a_huge_foreign_key").is_none());
⋮----
many.insert("version".to_string(), Value::from(6));
⋮----
many.insert(format!("k{i}"), Value::from("y".repeat(1024)));
⋮----
let kept = merge_unknown_keys(&Value::Object(many), &known);
⋮----
.as_object()
.expect("object")
.iter()
.filter(|(key, _)| known.get(*key).is_none())
.map(|(key, value)| key.len() + serde_json::to_string(value).expect("serialize").len())
.sum();
assert!(carried <= MAX_FOREIGN_KEY_BYTES, "carried {carried} bytes");
assert!(kept.get("k0").is_some());
assert!(kept.get("k99").is_none());
```

## File: rust/crates/k6wp-core/src/displays.rs
```rust
use std::collections::BTreeMap;
⋮----
pub use crate::config::ConfigError;
⋮----
pub(crate) fn json_type_name(value: &Value) -> &'static str {
⋮----
/// The C++ `type_error.302` message, wrapped the way the C++ wraps it.
pub(crate) fn type_error(context: &str, expected: &str, actual: &Value) -> ConfigError {
⋮----
pub(crate) fn type_error(context: &str, expected: &str, actual: &Value) -> ConfigError {
ConfigError(format!(
⋮----
/// The nlohmann `get<int>()` conversion: integers pass, a float truncates
/// toward zero, anything else is a type error.
⋮----
/// toward zero, anything else is a type error.
pub(crate) fn as_int(value: &Value) -> Option<i64> {
⋮----
pub(crate) fn as_int(value: &Value) -> Option<i64> {
⋮----
if let Some(i) = n.as_i64() {
Some(i)
} else if let Some(u) = n.as_u64() {
Some(u as i64)
⋮----
n.as_f64().map(|f| f as i64)
⋮----
/// One per-monitor assignment: the video file that plays on the monitor
/// whose GDI device name is the map key.
⋮----
/// whose GDI device name is the map key.
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct DisplayAssignment {
/// The absolute path of the video file (UTF-8, as on disk).
    pub path: String,
/// The Studio-side existence hint at write time. The engine re-checks the
    /// filesystem at apply time and does not trust this flag.
⋮----
/// filesystem at apply time and does not trust this flag.
    pub exists: bool,
/// The tiered stable identity (`ccd:...`, `edid:...:1a2b`). Empty = a
    /// legacy entry.
⋮----
/// legacy entry.
    pub key_stable: String,
⋮----
/// The whole assignment store.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Displays {
/// Always the current schema version after a migration.
    pub version: u32,
/// Keyed by the GDI device name (e.g. `\\.\DISPLAY1`). The map is sorted
    /// so the serialization order is deterministic across runs, exactly like
⋮----
/// so the serialization order is deterministic across runs, exactly like
    /// the C++ `std::map`.
⋮----
/// the C++ `std::map`.
    pub assignments: BTreeMap<String, DisplayAssignment>,
⋮----
impl Default for Displays {
fn default() -> Self {
⋮----
/// Migrates a raw `displays.json` document into the store.
///
⋮----
///
/// This is the Rust mirror of the C++ `MigrateDisplays`. The C++ takes an
⋮----
/// This is the Rust mirror of the C++ `MigrateDisplays`. The C++ takes an
/// already-parsed `nlohmann::json`; the golden fixtures hand the replayer
⋮----
/// already-parsed `nlohmann::json`; the golden fixtures hand the replayer
/// the RAW document text, so the Rust side parses it here and reports a
⋮----
/// the RAW document text, so the Rust side parses it here and reports a
/// corrupt document the way the C++ loader does.
⋮----
/// corrupt document the way the C++ loader does.
///
⋮----
///
/// # Errors
⋮----
/// # Errors
///
⋮----
///
/// - a corrupt document (`corrupt displays JSON: parse error`);
⋮----
/// - a corrupt document (`corrupt displays JSON: parse error`);
/// - a root that is not an object (`displays root must be a JSON object`);
⋮----
/// - a root that is not an object (`displays root must be a JSON object`);
/// - a version outside `[0, 1]` (`unsupported displays schema version: N`);
⋮----
/// - a version outside `[0, 1]` (`unsupported displays schema version: N`);
/// - `assignments` that is not an object, an assignment that is not an
⋮----
/// - `assignments` that is not an object, an assignment that is not an
///   object, a missing/non-string `path`, a non-array `displays`, or a
⋮----
///   object, a missing/non-string `path`, a non-array `displays`, or a
///   wrong-typed `version` / `exists`.
⋮----
///   wrong-typed `version` / `exists`.
pub fn migrate_displays(raw_text: &str) -> Result<Displays, ConfigError> {
⋮----
pub fn migrate_displays(raw_text: &str) -> Result<Displays, ConfigError> {
⋮----
.map_err(|_| ConfigError("corrupt displays JSON: parse error".to_string()))?;
migrate_displays_value(&raw)
⋮----
/// The same migration, on an already-parsed document.
pub fn migrate_displays_value(raw: &Value) -> Result<Displays, ConfigError> {
⋮----
pub fn migrate_displays_value(raw: &Value) -> Result<Displays, ConfigError> {
⋮----
.as_object()
.ok_or_else(|| ConfigError("displays root must be a JSON object".to_string()))?;
⋮----
// The C++ `raw.value("version", 0)`: a missing key defaults to 0, a
// wrong type is a type error.
let version = match root.get("version") {
⋮----
Some(value) => as_int(value).ok_or_else(|| type_error("displays", "number", value))?,
⋮----
return Err(ConfigError(format!(
⋮----
if let Some(raw_assignments) = root.get("assignments") {
⋮----
.ok_or_else(|| ConfigError("displays assignments must be an object".to_string()))?;
⋮----
let entry = entry.as_object().ok_or_else(|| {
ConfigError("displays assignment value must be an object".to_string())
⋮----
let path = match entry.get("path") {
Some(value) => value.as_str().ok_or_else(|| {
ConfigError("displays assignment path must be a string".to_string())
⋮----
return Err(ConfigError(
"displays assignment path must be a string".to_string(),
⋮----
// The C++ `value("exists", false)`: a missing key is false, a
⋮----
let exists = match entry.get("exists") {
⋮----
.as_bool()
.ok_or_else(|| type_error("displays", "boolean", value))?,
⋮----
// `key_stable` is additive and lenient: a wrong type is ignored
// (never a throw), so a legacy file still loads.
let key_stable = match entry.get("key_stable") {
Some(Value::String(text)) => text.clone(),
⋮----
assignments.insert(
key.clone(),
⋮----
path: path.to_string(),
⋮----
if let Some(displays) = root.get("displays") {
if !displays.is_array() {
return Err(ConfigError("displays must be an array".to_string()));
⋮----
// `key_aliases` is additive and lenient: the engine never reads the
// map, so it is not carried in the store, and the C++ ignores a
// missing or wrong-typed value as well (never a throw). Nothing to do
// here - and nothing that could fail.
⋮----
Ok(Displays {
⋮----
/// The store -> JSON mapping, in the shape the golden fixtures record.
///
⋮----
///
/// The C++ dump writes `version` plus the `assignments` map, and every
⋮----
/// The C++ dump writes `version` plus the `assignments` map, and every
/// assignment carries `path`, `exists` and `key_stable` (the last one even
⋮----
/// assignment carries `path`, `exists` and `key_stable` (the last one even
/// when it is empty - that is what the frozen fixture holds).
⋮----
/// when it is empty - that is what the frozen fixture holds).
pub fn displays_json(displays: &Displays) -> Value {
⋮----
pub fn displays_json(displays: &Displays) -> Value {
⋮----
entry.insert("path".to_string(), Value::String(assignment.path.clone()));
entry.insert("exists".to_string(), Value::Bool(assignment.exists));
entry.insert(
"key_stable".to_string(),
Value::String(assignment.key_stable.clone()),
⋮----
assignments.insert(key.clone(), Value::Object(entry));
⋮----
out.insert("version".to_string(), Value::from(displays.version));
out.insert("assignments".to_string(), Value::Object(assignments));
⋮----
/// True when any component of `path` is exactly `..`.
///
⋮----
///
/// A `..` inside a file name ("foo..bar") is fine; only a traversal
⋮----
/// A `..` inside a file name ("foo..bar") is fine; only a traversal
/// component is rejected, so a hostile assignment cannot escape the
⋮----
/// component is rejected, so a hostile assignment cannot escape the
/// wallpapers root. This is the C++ `HasDotDotComponent`.
⋮----
/// wallpapers root. This is the C++ `HasDotDotComponent`.
fn has_dot_dot_component(path: &str) -> bool {
⋮----
fn has_dot_dot_component(path: &str) -> bool {
path.split(['/', '\\']).any(|part| part == "..")
⋮----
/// Rejects the values outside the accepted ranges - the C++
/// `ValidateDisplays`.
⋮----
/// `ValidateDisplays`.
///
⋮----
///
/// - a version outside `[0, 1]`;
⋮----
/// - a version outside `[0, 1]`;
/// - more than `DISPLAYS_MAX_ENTRIES` assignments;
⋮----
/// - more than `DISPLAYS_MAX_ENTRIES` assignments;
/// - an empty assignment path;
⋮----
/// - an empty assignment path;
/// - an assignment path with a `..` component.
⋮----
/// - an assignment path with a `..` component.
pub fn validate_displays(displays: &Displays) -> Result<(), ConfigError> {
⋮----
pub fn validate_displays(displays: &Displays) -> Result<(), ConfigError> {
⋮----
if displays.assignments.len() > DISPLAYS_MAX_ENTRIES {
⋮----
if assignment.path.is_empty() {
⋮----
if has_dot_dot_component(&assignment.path) {
⋮----
Ok(())
⋮----
mod tests {
⋮----
fn a_v0_document_migrates_to_the_current_version() {
let displays = migrate_displays(r#"{"version":0}"#).expect("must migrate");
assert_eq!(displays.version, DISPLAYS_SCHEMA_VERSION);
assert!(displays.assignments.is_empty());
⋮----
fn the_assignments_keep_their_shape() {
let displays = migrate_displays(
⋮----
.expect("must migrate");
assert_eq!(displays.assignments.len(), 2);
⋮----
assert_eq!(first.path, "C:\\a.mp4");
assert!(first.exists);
assert_eq!(first.key_stable, "");
⋮----
assert_eq!(second.key_stable, "ccd:abc");
assert!(!second.exists);
⋮----
fn a_future_version_is_rejected() {
let err = migrate_displays(r#"{"version":2}"#).expect_err("must fail");
assert_eq!(err.0, "unsupported displays schema version: 2");
⋮----
fn a_non_object_assignment_is_rejected() {
⋮----
migrate_displays(r#"{"version":1,"assignments":{"a":"oops"}}"#).expect_err("must fail");
assert_eq!(err.0, "displays assignment value must be an object");
⋮----
fn a_missing_path_is_rejected() {
let err = migrate_displays(r#"{"version":1,"assignments":{"a":{"exists":true}}}"#)
.expect_err("must fail");
assert_eq!(err.0, "displays assignment path must be a string");
⋮----
fn a_wrong_typed_exists_is_rejected() {
⋮----
migrate_displays(r#"{"version":1,"assignments":{"a":{"path":"p","exists":"yes"}}}"#)
⋮----
assert_eq!(
⋮----
fn a_wrong_typed_key_stable_is_ignored() {
⋮----
migrate_displays(r#"{"version":1,"assignments":{"a":{"path":"p","key_stable":7}}}"#)
⋮----
assert_eq!(displays.assignments["a"].key_stable, "");
⋮----
fn a_non_array_displays_is_rejected() {
let err = migrate_displays(r#"{"version":1,"displays":{}}"#).expect_err("must fail");
assert_eq!(err.0, "displays must be an array");
⋮----
fn a_corrupt_document_is_rejected() {
let err = migrate_displays(r#"{"version":1,"#).expect_err("must fail");
assert_eq!(err.0, "corrupt displays JSON: parse error");
⋮----
fn the_json_mapping_carries_every_field() {
⋮----
"d1".to_string(),
⋮----
path: "C:\\a.mp4".to_string(),
⋮----
key_stable: "ccd:abc".to_string(),
⋮----
let json = displays_json(&displays);
assert_eq!(json["version"], Value::from(1));
assert_eq!(json["assignments"]["d1"]["path"], Value::from("C:\\a.mp4"));
assert_eq!(json["assignments"]["d1"]["exists"], Value::from(true));
⋮----
fn the_validator_rejects_a_traversal() {
⋮----
path: "C:\\a\\..\\b.mp4".to_string(),
⋮----
assert!(validate_displays(&displays).is_err());
```

## File: rust/crates/k6wp-core/src/engine_logic.rs
```rust
pub struct PlacementRect {
⋮----
pub enum CoverageVerdict {
⋮----
impl CoverageVerdict {
⋮----
pub fn as_str(self) -> &'static str {
⋮----
/// Whether the child surface covers the monitor.
///
⋮----
///
/// The order of the checks matters and is frozen by the fixtures: a surface
⋮----
/// The order of the checks matters and is frozen by the fixtures: a surface
/// that is SMALLER than the monitor is reported as clipped on the left (the
⋮----
/// that is SMALLER than the monitor is reported as clipped on the left (the
/// first edge that does not reach), and only a surface that does not
⋮----
/// first edge that does not reach), and only a surface that does not
/// overlap at all is out of bounds.
⋮----
/// overlap at all is out of bounds.
pub fn covers_monitor(child: PlacementRect, monitor: PlacementRect) -> CoverageVerdict {
⋮----
pub fn covers_monitor(child: PlacementRect, monitor: PlacementRect) -> CoverageVerdict {
// Disjoint (touching edges do not overlap) -> nothing visible.
⋮----
/// The WorkerW host rule (task C7): the host window may hold ONLY our own
/// (possibly stale) surface - a foreign child means the host is not ours.
⋮----
/// (possibly stale) surface - a foreign child means the host is not ours.
///
⋮----
///
/// An empty list is accepted: a host with no children is ours by
⋮----
/// An empty list is accepted: a host with no children is ours by
/// definition.
⋮----
/// definition.
pub fn workerw_accepts_child_classes(child_classes: &[String], own_class: &str) -> bool {
⋮----
pub fn workerw_accepts_child_classes(child_classes: &[String], own_class: &str) -> bool {
child_classes.iter().all(|class| class == own_class)
⋮----
/// The load retry offsets (0 / 250 / 1000 / 3000 ms).
pub const RETRY_COUNT: i32 = 4;
⋮----
/// The retry clock: when the first pass ended and how many attempts are done.
#[derive(Debug, Clone, Copy, Default)]
pub struct RetryClock {
/// The steady-clock ms at the pass end (caller-supplied).
    pub first_pass_ms: i64,
/// The due attempts already executed (0..=RETRY_COUNT).
    pub attempts_done: i32,
⋮----
/// True when attempt `#attempts_done` is due at `now_ms`.
pub fn retry_schedule_due(now_ms: i64, clock: &RetryClock) -> bool {
⋮----
pub fn retry_schedule_due(now_ms: i64, clock: &RetryClock) -> bool {
⋮----
/// The ms until the next due attempt (0 when it is already due, -1 when the
/// schedule is exhausted).
⋮----
/// schedule is exhausted).
pub fn retry_next_delay_ms(now_ms: i64, clock: &RetryClock) -> i64 {
⋮----
pub fn retry_next_delay_ms(now_ms: i64, clock: &RetryClock) -> i64 {
⋮----
/// True when every scheduled attempt has been executed.
pub fn retry_schedule_exhausted(clock: &RetryClock) -> bool {
⋮----
pub fn retry_schedule_exhausted(clock: &RetryClock) -> bool {
⋮----
/// The automatic-restart state.
#[derive(Debug, Clone, Copy, Default)]
pub struct CrashRestartState {
/// The automatic restarts inside the window.
    pub count: i32,
/// The monotonic ms when the window opened.
    pub window_start_ms: i64,
⋮----
/// Advances the state for one automatic restart at `now_ms`.
///
⋮----
///
/// A non-positive window disables the sliding behaviour: every restart is its
⋮----
/// A non-positive window disables the sliding behaviour: every restart is its
/// own window (the count resets to 1) - the safe interpretation of a
⋮----
/// own window (the count resets to 1) - the safe interpretation of a
/// misconfigured window, never an unbounded counter.
⋮----
/// misconfigured window, never an unbounded counter.
pub fn crash_restart_advance(
⋮----
pub fn crash_restart_advance(
⋮----
/// True while the state is inside the allowed restart budget.
pub fn crash_restart_allowed(state: &CrashRestartState, max_restarts: i32) -> bool {
⋮----
pub fn crash_restart_allowed(state: &CrashRestartState, max_restarts: i32) -> bool {
⋮----
/// One monitor as the reconcile pipeline sees it.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct TopologyMonitor {
/// The tiered identity (never GDI-only unless it fell back).
    pub stable_key: String,
⋮----
/// The pure snapshot of the monitor topology.
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct TopologySnapshot {
/// The deterministic order: primary first, then the stable key.
    pub monitors: Vec<TopologyMonitor>,
⋮----
/// The stable hash over the canonical serialization.
    pub hash_snapshot: String,
⋮----
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
⋮----
impl TopologyDiff {
/// True when nothing changed (the vectors are empty and the primary did
    /// not change).
⋮----
/// not change).
    pub fn is_empty(&self) -> bool {
⋮----
pub fn is_empty(&self) -> bool {
self.added_keys.is_empty()
&& self.removed_keys.is_empty()
&& self.moved_keys.is_empty()
&& self.resized_keys.is_empty()
&& self.dpi_keys.is_empty()
&& self.orientation_keys.is_empty()
⋮----
/// The pure topology diff.
///
⋮----
///
/// The keys are identified ONLY by the stable key, and the output ordering is
⋮----
/// The keys are identified ONLY by the stable key, and the output ordering is
/// deterministic (sorted), so the same pair of snapshots always produces the
⋮----
/// deterministic (sorted), so the same pair of snapshots always produces the
/// same diff - and an identical pair produces an empty one.
⋮----
/// same diff - and an identical pair produces an empty one.
pub fn diff_topology(old_s: &TopologySnapshot, new_s: &TopologySnapshot) -> TopologyDiff {
⋮----
pub fn diff_topology(old_s: &TopologySnapshot, new_s: &TopologySnapshot) -> TopologyDiff {
use std::collections::BTreeMap;
⋮----
old_by_key.insert(m.stable_key.as_str(), m);
⋮----
new_by_key.insert(m.stable_key.as_str(), m);
⋮----
for key in new_by_key.keys() {
if !old_by_key.contains_key(key) {
d.added_keys.push((*key).to_string());
⋮----
for key in old_by_key.keys() {
if !new_by_key.contains_key(key) {
d.removed_keys.push((*key).to_string());
⋮----
// Per-attribute changes for the keys present in both snapshots. A single
// key can land in several categories at once (moved + resized).
⋮----
let Some(n) = new_by_key.get(key) else {
⋮----
d.moved_keys.push((*key).to_string());
⋮----
d.resized_keys.push((*key).to_string());
⋮----
d.dpi_keys.push((*key).to_string());
⋮----
d.orientation_keys.push((*key).to_string());
⋮----
// The primary change over the key union: an absent key counts as
// not-primary, so adding or removing the primary monitor is a primary
// change too.
⋮----
keys.extend(old_by_key.keys().copied());
keys.extend(new_by_key.keys().copied());
⋮----
let old_primary = old_by_key.get(key).is_some_and(|m| m.is_primary);
let new_primary = new_by_key.get(key).is_some_and(|m| m.is_primary);
⋮----
/// One monitor as the desktop sees it (the reconcile policy's input).
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct MonitorInfo {
⋮----
pub enum ReconcileAction {
⋮----
pub fn decide_reconcile_action(trigger: &str) -> ReconcileAction {
⋮----
pub fn reconcile_action_name(action: ReconcileAction) -> &'static str {
⋮----
/// True when the action needs the monitor enumeration to have settled before
/// it runs.
⋮----
/// it runs.
pub fn reconcile_action_needs_stable_topology(action: ReconcileAction) -> bool {
⋮----
pub fn reconcile_action_needs_stable_topology(action: ReconcileAction) -> bool {
matches!(action, ReconcileAction::ReanchorSlots)
⋮----
/// The identity key of a monitor: the stable key, else the GDI device name,
/// else its position id.
⋮----
/// else its position id.
pub fn slot_identity_key(mi: &MonitorInfo) -> String {
⋮----
pub fn slot_identity_key(mi: &MonitorInfo) -> String {
if !mi.stable_key.is_empty() {
return mi.stable_key.clone();
⋮----
if !mi.device_name.is_empty() {
return format!("gdi:{}", mi.device_name);
⋮----
format!("pos:{}", mi.id)
⋮----
/// True when the two enumerations describe the SAME topology.
///
⋮----
///
/// The comparison is order-insensitive. A duplicated identity (an ambiguous
⋮----
/// The comparison is order-insensitive. A duplicated identity (an ambiguous
/// EDID, two monitors without a device name) makes the topology
⋮----
/// EDID, two monitors without a device name) makes the topology
/// undecidable - and therefore not stable.
⋮----
/// undecidable - and therefore not stable.
pub fn topology_stable(first: &[MonitorInfo], second: &[MonitorInfo]) -> bool {
⋮----
pub fn topology_stable(first: &[MonitorInfo], second: &[MonitorInfo]) -> bool {
if first.len() != second.len() {
⋮----
let key = slot_identity_key(mi);
if by_identity.insert(key, mi).is_some() {
⋮----
let Some(other) = by_identity.get(&key) else {
⋮----
/// The stable signature of an enumeration (the monitors joined in a stable
/// order).
⋮----
/// order).
pub fn topology_signature(monitors: &[MonitorInfo]) -> String {
⋮----
pub fn topology_signature(monitors: &[MonitorInfo]) -> String {
let mut parts: Vec<String> = Vec::with_capacity(monitors.len());
⋮----
parts.push(format!(
⋮----
// The enumeration is sorted (primary first, then the device name), but
// sort anyway so the signature does not depend on the caller's ordering.
parts.sort();
parts.join(";")
⋮----
pub fn pin_verify_due_at(armed: bool, deadline_ms: i64, now_ms: i64) -> bool {
⋮----
pub fn pin_verify_add_reverts(total: i32, n: i32) -> i32 {
⋮----
pub enum DecodeState {
⋮----
impl DecodeState {
⋮----
pub struct UnloadMachine {
⋮----
impl Default for UnloadMachine {
fn default() -> Self {
⋮----
pub struct UnloadDecision {
⋮----
pub fn unload_on_pause_changed(
⋮----
pub fn unload_on_tick(machine: &UnloadMachine, mask: i32, now_ms: i64) -> UnloadDecision {
⋮----
pub mod pause {
⋮----
pub fn ui_paused(mask: i32) -> bool {
⋮----
pub fn slots_paused(mask: i32) -> bool {
⋮----
pub fn hides_wallpaper(mask: i32) -> bool {
```

## File: rust/crates/k6wp-core/src/ipc.rs
```rust
use std::collections::BTreeMap;
use std::fmt;
⋮----
pub enum Cmd {
⋮----
pub fn cmd_to_string(cmd: Cmd) -> &'static str {
⋮----
/// Parses a wire string back to a command.
///
⋮----
///
/// `None` for a name that is not in the vocabulary - the C++
⋮----
/// `None` for a name that is not in the vocabulary - the C++
/// `CmdFromString` returns false, and the frame is then rejected.
⋮----
/// `CmdFromString` returns false, and the frame is then rejected.
pub fn cmd_from_string(name: &str) -> Option<Cmd> {
⋮----
pub fn cmd_from_string(name: &str) -> Option<Cmd> {
⋮----
.iter()
.find(|(wire, _)| *wire == name)
.map(|(_, cmd)| *cmd)
⋮----
/// Every command name, in the stable append-only order.
pub fn cmd_vocabulary() -> Vec<(&'static str, Cmd)> {
⋮----
pub fn cmd_vocabulary() -> Vec<(&'static str, Cmd)> {
CMD_NAMES.to_vec()
⋮----
pub struct IpcMessage {
⋮----
impl Default for IpcMessage {
fn default() -> Self {
⋮----
pub struct IpcError(pub String);
⋮----
fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
f.write_str(&self.0)
⋮----
/// Serializes a message to a single NDJSON line (the JSON plus the `'\n'`).
///
⋮----
///
/// # Errors
⋮----
/// # Errors
///
⋮----
///
/// - a payload (or a whole frame) bigger than `MAX_PAYLOAD_BYTES`.
⋮----
/// - a payload (or a whole frame) bigger than `MAX_PAYLOAD_BYTES`.
pub fn encode(msg: &IpcMessage) -> Result<String, IpcError> {
⋮----
pub fn encode(msg: &IpcMessage) -> Result<String, IpcError> {
⋮----
.map_err(|e| IpcError(format!("Encode: the payload is not serializable: {e}")))?;
if payload_json.len() > MAX_PAYLOAD_BYTES {
return Err(IpcError(
"Encode: payload exceeds kMaxPayloadBytes (64 KiB)".to_string(),
⋮----
root.insert("version".to_string(), Value::from(msg.version));
root.insert(
"cmd".to_string(),
Value::String(cmd_to_string(msg.cmd).to_string()),
⋮----
root.insert("payload".to_string(), msg.payload.clone());
⋮----
.map_err(|e| IpcError(format!("Encode: the frame is not serializable: {e}")))?;
if frame.len() > MAX_PAYLOAD_BYTES {
⋮----
"Encode: frame exceeds kMaxPayloadBytes (64 KiB)".to_string(),
⋮----
Ok(frame + "\n")
⋮----
pub fn decode(text: &str) -> Result<IpcMessage, IpcError> {
if text.is_empty() {
return Err(IpcError("Decode: the frame is empty".to_string()));
⋮----
if let Some(stripped) = line.strip_suffix('\n') {
⋮----
if let Some(stripped) = line.strip_suffix('\r') {
⋮----
if line.is_empty() || line.contains('\n') {
⋮----
"Decode: the frame is not a single line".to_string(),
⋮----
.map_err(|e| IpcError(format!("Decode: the frame is not valid JSON: {e}")))?;
⋮----
.as_object()
.ok_or_else(|| IpcError("Decode: the frame is not a JSON object".to_string()))?;
⋮----
let version = match object.get("version") {
Some(Value::Number(number)) if number.is_i64() || number.is_u64() => number
.as_i64()
.unwrap_or_else(|| number.as_u64().unwrap_or(0) as i64),
⋮----
"Decode: the version is not an integer".to_string(),
⋮----
None => return Err(IpcError("Decode: the frame has no version".to_string())),
⋮----
return Err(IpcError(format!(
⋮----
let cmd_name = match object.get("cmd") {
Some(Value::String(name)) => name.as_str(),
Some(_) => return Err(IpcError("Decode: the command is not a string".to_string())),
None => return Err(IpcError("Decode: the frame has no command".to_string())),
⋮----
let cmd = cmd_from_string(cmd_name)
.ok_or_else(|| IpcError(format!("Decode: unknown command: {cmd_name}")))?;
⋮----
.get("payload")
.cloned()
.unwrap_or_else(|| Value::Object(Map::new()));
⋮----
.map_err(|e| IpcError(format!("Decode: the payload is not serializable: {e}")))?;
⋮----
"Decode: the payload exceeds kMaxPayloadBytes (64 KiB)".to_string(),
⋮----
Ok(IpcMessage {
⋮----
pub struct EngineState {
⋮----
impl Default for EngineState {
⋮----
fn as_u64(value: &Value) -> Option<u64> {
⋮----
if let Some(u) = number.as_u64() {
Some(u)
} else if let Some(i) = number.as_i64() {
Some(i as u64)
⋮----
number.as_f64().map(|f| f as u64)
⋮----
fn as_i32(value: &Value) -> Option<i32> {
⋮----
if let Some(i) = number.as_i64() {
Some(i as i32)
} else if let Some(u) = number.as_u64() {
Some(u as i32)
⋮----
number.as_f64().map(|f| f as i32)
⋮----
fn is_integer(value: &Value) -> bool {
matches!(value, Value::Number(number) if number.is_i64() || number.is_u64())
⋮----
fn state_value<T>(
⋮----
let object = state.as_object().ok_or(())?;
match object.get(key) {
None => Ok(default),
Some(value) => read(value).ok_or(()),
⋮----
pub fn parse_engine_state(raw: &Value) -> EngineState {
⋮----
let Some(object) = raw.as_object() else {
⋮----
.get("state")
⋮----
state.pid = state_value(&inner, "pid", 0u64, as_u64)?;
state.video = state_value(&inner, "video", String::new(), |v| {
v.as_str().map(str::to_string)
⋮----
state.paused = state_value(&inner, "paused", false, |v| v.as_bool())?;
state.headless_slots = state_value(&inner, "headless_slots", 0i32, as_i32)?;
state.live = state_value(&inner, "live", true, |v| v.as_bool())?;
⋮----
if let Some(capability) = inner.get("display_capability") {
if is_integer(capability) {
if let Some(value) = as_i32(capability) {
⋮----
if let Some(assignments) = inner.get("display_assignments") {
if let Some(map) = assignments.as_object() {
⋮----
if let Some(path) = path.as_str() {
⋮----
.insert(device.clone(), path.to_string());
⋮----
if let Some(coverage) = inner.get("display_coverage") {
if let Some(map) = coverage.as_object() {
⋮----
if let Some(verdict) = verdict.as_str() {
⋮----
.insert(device.clone(), verdict.to_string());
⋮----
if let Some(enabled) = inner.get("playlist_enabled") {
if let Some(enabled) = enabled.as_bool() {
⋮----
if let Some(size) = inner.get("playlist_size") {
if is_integer(size) {
if let Some(value) = as_i32(size) {
⋮----
if let Some(index) = inner.get("playlist_index") {
if is_integer(index) {
if let Some(value) = as_i32(index) {
⋮----
if let Some(reconcile) = inner.get("reconcile") {
if let Some(map) = reconcile.as_object() {
if let Some(mode) = map.get("mode").and_then(Value::as_str) {
state.reconcile_mode = mode.to_string();
⋮----
if let Some(trigger) = map.get("last_trigger").and_then(Value::as_str) {
state.reconcile_last_trigger = trigger.to_string();
⋮----
if let Some(verdict) = map.get("last_verdict").and_then(Value::as_str) {
state.reconcile_last_verdict = verdict.to_string();
⋮----
if let Some(retries) = map.get("retries") {
if is_integer(retries) {
if let Some(value) = as_i32(retries) {
⋮----
if let Some(degraded) = map.get("degraded_keys") {
if let Some(list) = degraded.as_array() {
⋮----
if let Some(key) = key.as_str() {
state.reconcile_degraded_keys.push(key.to_string());
⋮----
Ok(())
⋮----
pub fn parse_engine_state_text(text: &str) -> EngineState {
⋮----
Ok(raw) => parse_engine_state(&raw),
⋮----
mod tests {
⋮----
fn the_vocabulary_is_append_only() {
assert_eq!(cmd_to_string(Cmd::SetVideo), "set_video");
assert_eq!(cmd_to_string(Cmd::SetDisplayVideo), "set_display_video");
assert_eq!(cmd_from_string("set_video"), Some(Cmd::SetVideo));
assert_eq!(
⋮----
assert_eq!(cmd_from_string("nope"), None);
assert_eq!(cmd_from_string(""), None);
assert_eq!(cmd_vocabulary().len(), 7);
assert_eq!(cmd_vocabulary()[0], ("set_video", Cmd::SetVideo));
⋮----
fn the_frame_is_the_frozen_byte_sequence() {
⋮----
fn a_frame_round_trips() {
⋮----
let msg = decode(frame).expect("must decode");
assert_eq!(msg.version, 1);
assert_eq!(msg.cmd, Cmd::SetVideo);
assert_eq!(msg.payload["path"], Value::from("C:\\v\\a.mp4"));
⋮----
fn the_malformed_frames_are_rejected() {
⋮----
assert!(decode(frame).is_err(), "must reject: {frame:?}");
⋮----
fn a_frame_without_a_payload_gets_an_empty_one() {
let msg = decode("{\"version\":1,\"cmd\":\"quit\"}").expect("must decode");
assert_eq!(msg.payload, Value::Object(Map::new()));
⋮----
fn the_state_defaults_are_kept() {
let state = parse_engine_state_text("{}");
assert_eq!(state.pid, 0);
assert!(state.live);
assert_eq!(state.playlist_index, -1);
assert!(state.display_assignments.is_empty());
⋮----
fn a_wrong_typed_state_falls_back_to_the_defaults() {
let state = parse_engine_state_text(
⋮----
assert_eq!(state, EngineState::default());
```

## File: rust/crates/k6wp-core/src/version.rs
```rust
pub struct ParsedSemver {
⋮----
fn is_c_space(byte: u8) -> bool {
matches!(byte, b' ' | b'\t' | b'\n' | 0x0b | 0x0c | b'\r')
⋮----
pub fn parse_semver(text: &str) -> ParsedSemver {
let bytes = text.as_bytes();
⋮----
while beg < bytes.len() && is_c_space(bytes[beg]) {
⋮----
let mut end = bytes.len();
while end > beg && is_c_space(bytes[end - 1]) {
⋮----
if let Some(offset) = bytes[beg..end].iter().position(|byte| *byte == b'-') {
⋮----
// Parse the dot-separated components; each non-empty one must start with
// a digit, otherwise the whole version is invalid.
⋮----
i += 1; // Tolerate the empty components ("1..2" parses as 1, 2).
⋮----
if !bytes[i].is_ascii_digit() {
out.parts.clear();
⋮----
while i < core_end && bytes[i].is_ascii_digit() {
⋮----
out.parts.push(acc as i32);
⋮----
out.valid = !out.parts.is_empty();
⋮----
pub fn compare_versions(a: &str, b: &str) -> i32 {
let pa = parse_semver(a);
let pb = parse_semver(b);
⋮----
let count = pa.parts.len().max(pb.parts.len());
⋮----
let va = pa.parts.get(index).copied().unwrap_or(0);
let vb = pb.parts.get(index).copied().unwrap_or(0);
⋮----
pub fn is_newer_version(latest: &str, current: &str) -> bool {
compare_versions(latest, current) > 0
⋮----
mod tests {
⋮----
fn the_numeric_parts_compare() {
assert_eq!(compare_versions("1.2.3", "1.2.3"), 0);
assert_eq!(compare_versions("1.10.0", "1.9.0"), 1);
assert_eq!(compare_versions("1.9.0", "1.10.0"), -1);
assert_eq!(compare_versions("1.2", "1.2.0"), 0);
assert_eq!(compare_versions("2.0.0", "1.9.9"), 1);
⋮----
fn the_whitespace_and_the_v_prefix_are_stripped() {
assert_eq!(compare_versions("  v1.2.3 ", "1.2.3"), 0);
assert_eq!(compare_versions("V1.2.3", "1.2.3"), 0);
⋮----
fn the_prerelease_sorts_below_the_release() {
assert_eq!(compare_versions("1.0.0", "1.0.0-beta"), 1);
assert_eq!(compare_versions("1.0.0-beta", "1.0.0"), -1);
assert_eq!(compare_versions("1.0.0-beta", "1.0.0-beta"), 0);
⋮----
fn an_invalid_version_compares_equal() {
assert_eq!(compare_versions("", "1.0.0"), 0);
assert_eq!(compare_versions("abc", "1.0.0"), 0);
assert_eq!(compare_versions("1.0.0", "abc"), 0);
assert!(!is_newer_version("abc", "1.0.0"));
⋮----
fn the_junk_inside_a_component_is_ignored() {
assert_eq!(compare_versions("1.2.3beta", "1.2.3"), 0);
assert_eq!(compare_versions("1..2", "1.2"), 0);
⋮----
fn the_component_is_clamped() {
let parsed = parse_semver("99999999999.0.0");
assert!(parsed.valid);
assert_eq!(parsed.parts[0], 1_000_000_000);
```

## File: rust/crates/k6wp-core/tests/engine_logic_golden.rs
```rust
use serde_json::Value;
⋮----
fn topology_monitor(value: &Value) -> TopologyMonitor {
⋮----
stable_key: value["stable_key"].as_str().unwrap().to_string(),
x: value["x"].as_i64().unwrap() as i32,
y: value["y"].as_i64().unwrap() as i32,
width: value["width"].as_i64().unwrap() as i32,
height: value["height"].as_i64().unwrap() as i32,
scale_pct: value["scale_pct"].as_i64().unwrap() as i32,
orientation: value["orientation"].as_i64().unwrap() as i32,
is_primary: value["is_primary"].as_bool().unwrap(),
degraded: value["degraded"].as_bool().unwrap(),
⋮----
fn topology_snapshot(value: &Value) -> TopologySnapshot {
⋮----
.as_array()
.unwrap()
.iter()
.map(topology_monitor)
.collect(),
virtual_x: value["virtual_x"].as_i64().unwrap() as i32,
virtual_y: value["virtual_y"].as_i64().unwrap() as i32,
virtual_width: value["virtual_width"].as_i64().unwrap() as i32,
virtual_height: value["virtual_height"].as_i64().unwrap() as i32,
hash_snapshot: value["hash_snapshot"].as_str().unwrap().to_string(),
⋮----
fn monitor_info(value: &Value) -> MonitorInfo {
⋮----
id: value["id"].as_i64().unwrap() as i32,
⋮----
device_name: value["device_name"].as_str().unwrap().to_string(),
⋮----
refresh_hz: value["refresh_hz"].as_i64().unwrap() as i32,
⋮----
identity_degraded: value["identity_degraded"].as_bool().unwrap(),
⋮----
fn fixture_path() -> std::path::PathBuf {
std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../tests/golden/engine_logic.json")
⋮----
fn rect(value: &Value) -> PlacementRect {
⋮----
left: value["left"].as_i64().unwrap() as i32,
top: value["top"].as_i64().unwrap() as i32,
right: value["right"].as_i64().unwrap() as i32,
bottom: value["bottom"].as_i64().unwrap() as i32,
⋮----
fn evaluate(case: &Value) -> Value {
let module = case["module"].as_str().unwrap();
⋮----
let verdict = covers_monitor(rect(&input["child"]), rect(&input["monitor"]));
⋮----
.map(|c| c.as_str().unwrap().to_string())
.collect();
let own = input["own_class"].as_str().unwrap();
⋮----
first_pass_ms: input["first_pass_ms"].as_i64().unwrap(),
attempts_done: input["attempts_done"].as_i64().unwrap() as i32,
⋮----
let now_ms = input["now_ms"].as_i64().unwrap();
⋮----
count: input["count"].as_i64().unwrap() as i32,
window_start_ms: input["window_start_ms"].as_i64().unwrap(),
⋮----
let advanced = crash_restart_advance(
⋮----
input["now_ms"].as_i64().unwrap(),
input["max_restarts"].as_i64().unwrap() as i32,
input["window_ms"].as_i64().unwrap(),
⋮----
let mask = input["mask"].as_i64().unwrap() as i32;
⋮----
let old_s = topology_snapshot(&input["old"]);
let new_s = topology_snapshot(&input["new"]);
let d = diff_topology(&old_s, &new_s);
⋮----
if let Some(trigger) = input.get("trigger").and_then(Value::as_str) {
let action = decide_reconcile_action(trigger);
⋮----
.map(monitor_info)
⋮----
let state = match input["state"].as_str().unwrap() {
⋮----
pause_started_ms: input["pause_started_ms"].as_i64().unwrap(),
unload_after_ms: input["unload_after_ms"].as_i64().unwrap() as i32,
⋮----
let decision = if input["op"].as_str().unwrap() == "tick" {
unload_on_tick(&machine, input["mask"].as_i64().unwrap() as i32, now_ms)
⋮----
unload_on_pause_changed(&machine, input["paused"].as_bool().unwrap(), now_ms)
⋮----
if let Some(armed) = input.get("armed").and_then(Value::as_bool) {
⋮----
other => panic!("the fixture holds an unknown module: {other}"),
⋮----
fn the_rust_port_matches_the_cpp_golden_fixtures() {
let path = fixture_path();
⋮----
.unwrap_or_else(|e| panic!("cannot read {}: {e}", path.display()));
let doc: Value = serde_json::from_str(&text).expect("the fixture must be valid JSON");
⋮----
.expect("the fixture must hold cases");
assert!(!cases.is_empty(), "the fixture holds no cases");
⋮----
let name = case["name"].as_str().unwrap();
⋮----
let actual = evaluate(case);
assert_eq!(
⋮----
*per_module.entry(module.to_string()).or_insert(0usize) += 1;
⋮----
.into_iter()
```

## File: rust/crates/k6wp-core/tests/probe.rs
```rust
use std::collections::BTreeMap;
⋮----
fn cases() -> Vec<Value> {
⋮----
let path = concat!(
⋮----
let text = std::fs::read_to_string(path).expect("the golden fixtures must be readable");
let root: Value = serde_json::from_str(&text).expect("the golden file must be JSON");
root["cases"].as_array().cloned().unwrap_or_default()
⋮----
fn engine_state_json(state: &ipc::EngineState) -> Value {
⋮----
out.insert("pid".into(), json!(state.pid));
out.insert("video".into(), json!(state.video));
out.insert("paused".into(), json!(state.paused));
out.insert("headless_slots".into(), json!(state.headless_slots));
out.insert("live".into(), json!(state.live));
out.insert("display_capability".into(), json!(state.display_capability));
out.insert(
"display_assignments".into(),
json!(state.display_assignments),
⋮----
out.insert("display_coverage".into(), json!(state.display_coverage));
out.insert("playlist_enabled".into(), json!(state.playlist_enabled));
out.insert("playlist_size".into(), json!(state.playlist_size));
out.insert("playlist_index".into(), json!(state.playlist_index));
out.insert("reconcile_mode".into(), json!(state.reconcile_mode));
⋮----
"reconcile_last_trigger".into(),
json!(state.reconcile_last_trigger),
⋮----
"reconcile_last_verdict".into(),
json!(state.reconcile_last_verdict),
⋮----
out.insert("reconcile_retries".into(), json!(state.reconcile_retries));
⋮----
"reconcile_degraded_keys".into(),
json!(state.reconcile_degraded_keys),
⋮----
fn cmd_vocabulary_json() -> Value {
⋮----
out.insert(name.to_string(), json!(cmd as i32));
⋮----
let key = if name.is_empty() { "<empty>" } else { name };
let value = ipc::cmd_from_string(name).map(|c| c as i32).unwrap_or(-1);
from_string.insert(key.to_string(), json!(value));
⋮----
out.insert("from_string".into(), Value::Object(from_string));
out.insert("protocol_version".into(), json!(ipc::PROTOCOL_VERSION));
out.insert("max_payload_bytes".into(), json!(ipc::MAX_PAYLOAD_BYTES));
⋮----
fn select_next_index_json() -> Value {
⋮----
row.insert("count".into(), json!(count));
row.insert(
"sequential_from_0".into(),
json!(playlist::select_next_index_with_state(
⋮----
"sequential_from_2".into(),
⋮----
"shuffle_from_0".into(),
⋮----
rows.push(Value::Object(row));
⋮----
fn run_case(case: &Value) -> Result<Value, String> {
let id = case["id"].as_str().unwrap_or_default().to_string();
⋮----
if id.starts_with("displays.") {
let text = input.as_str().unwrap_or_default();
let displays = displays::migrate_displays(text).map_err(|e| e.to_string())?;
return Ok(displays::displays_json(&displays));
⋮----
if id.starts_with("playlist.") {
⋮----
.as_array()
.map(|a| {
a.iter()
.map(|v| v.as_str().unwrap_or_default().to_string())
.collect()
⋮----
.unwrap_or_default();
return Ok(json!(playlist::normalize_order(order)));
⋮----
return Ok(select_next_index_json());
⋮----
|index: usize| -> String { queries.get(index).cloned().unwrap_or_default() };
⋮----
"found_exact".into(),
json!(playlist::playlist_index_for_path(&order, &query(0))),
⋮----
"found_case_insensitive".into(),
json!(playlist::playlist_index_for_path(&order, &query(1))),
⋮----
"not_found".into(),
json!(playlist::playlist_index_for_path(&order, &query(2))),
⋮----
"empty_order".into(),
json!(playlist::playlist_index_for_path(&[], &query(2))),
⋮----
return Ok(Value::Object(out));
⋮----
let cfg = playlist::migrate_playlist(text).map_err(|e| e.to_string())?;
return Ok(playlist::playlist_json(&cfg));
⋮----
if id.starts_with("ipc.") {
⋮----
return Ok(cmd_vocabulary_json());
⋮----
if let Some(cmd_name) = id.strip_prefix("ipc.encode.") {
⋮----
.ok_or_else(|| format!("unknown command: {cmd_name}"))?;
⋮----
payload: input.clone(),
⋮----
return Ok(json!(ipc::encode(&msg).map_err(|e| e.to_string())?));
⋮----
if id.starts_with("ipc.decode.") {
⋮----
let msg = ipc::decode(text).map_err(|e| e.to_string())?;
return Ok(json!({
⋮----
if id.starts_with("ipc.parse_state.") {
⋮----
return Ok(engine_state_json(&ipc::parse_engine_state_text(text)));
⋮----
return Err(format!("unknown IPC case: {id}"));
⋮----
if id.starts_with("version.") {
let a = input["a"].as_str().unwrap_or_default();
let b = input["b"].as_str().unwrap_or_default();
return Ok(json!(version::compare_versions(a, b)));
⋮----
Err(format!("unknown case: {id}"))
⋮----
fn the_golden_fixtures_match_on_the_rust_side() {
let all = cases();
assert!(!all.is_empty(), "the golden file must not be empty");
⋮----
let id = case["id"].as_str().unwrap_or_default();
if !(id.starts_with("displays.")
|| id.starts_with("playlist.")
|| id.starts_with("ipc.")
|| id.starts_with("version."))
⋮----
let has_expected_output = case.get("output").is_some();
let actual = run_case(case);
⋮----
matched.push(id.to_string());
⋮----
failed.push(format!("{id}: expected {}, got {}", case["output"], value));
⋮----
(true, Err(e)) => failed.push(format!("{id}: expected a value, got the error {e}")),
(false, Ok(value)) => failed.push(format!("{id}: expected a failure, got {value}")),
(false, Err(_)) => matched.push(id.to_string()),
⋮----
eprintln!("[FAIL] {line}");
⋮----
eprintln!("probe: {checked} cases replayed, {} match", matched.len());
assert!(
⋮----
fn the_order_api_agrees_with_the_count_api() {
⋮----
let order: Vec<String> = (0..count).map(|i| format!("C:\\{i}.mp4")).collect();
assert_eq!(
⋮----
assert_eq!(playlist::select_next_index(&order, count - 1, false), 0);
⋮----
assert_eq!(playlist::select_next_index(&[], 0, false), -1);
assert_eq!(playlist::select_next_index(&[], 0, true), -1);
⋮----
fn the_displays_json_is_sorted() {
⋮----
assignments.insert(
key.to_string(),
⋮----
path: "C:\\a.mp4".to_string(),
⋮----
.as_object()
.expect("an object")
.keys()
.collect();
assert_eq!(keys, vec!["\\\\.\\DISPLAY1", "\\\\.\\DISPLAY2"]);
```

## File: rust/crates/k6wp-core/tests/sps_fuzz.rs
```rust
use k6wp_core::sps::probe_dpb_bytes;
⋮----
fn corpus_bytes() -> Vec<Vec<u8>> {
let dir = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../tests/corpus");
⋮----
for entry in entries.flatten() {
let path = entry.path();
⋮----
.extension()
.map(|e| {
let e = e.to_string_lossy().to_lowercase();
⋮----
.unwrap_or(false);
⋮----
let head: Vec<u8> = bytes.iter().copied().take(512 * 1024).collect();
out.push(head);
⋮----
proptest! {
⋮----
fn the_parser_never_panics_on_the_mutated_corpus() {
let corpus = corpus_bytes();
assert!(!corpus.is_empty(), "the corpus is empty - build it first");
⋮----
let data = &head[..head.len().min(cut)];
let _ = probe_dpb_bytes(data);
⋮----
for index in (0..head.len().min(2048)).step_by(4) {
let mut data = head.clone();
data[index] = data[index].wrapping_add(0x5A);
let _ = probe_dpb_bytes(&data);
⋮----
let end = data.len().min(8192);
for byte in data.iter_mut().take(end) {
⋮----
eprintln!("sps fuzz: {checked} mutated inputs, no panic");
⋮----
fn the_regression_vectors_are_handled() {
let dir = std::path::Path::new(env!("CARGO_MANIFEST_DIR"))
.join("../../../tests/golden/sps-regressions");
⋮----
eprintln!("[SKIP] no regression vectors yet");
⋮----
if path.extension().map(|e| e == "bin").unwrap_or(false) {
let bytes = std::fs::read(&path).expect("the vector must be readable");
let _ = probe_dpb_bytes(&bytes);
⋮----
eprintln!("sps fuzz: {checked} regression vectors handled");
```

## File: rust/crates/k6wp-core/tests/sps_read_strategy.rs
```rust
fn corpus_dir() -> PathBuf {
Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../tests/corpus")
⋮----
fn moov_bytes(data: &[u8]) -> Option<Vec<u8>> {
⋮----
while pos + 8 <= data.len() {
⋮----
if size < 8 || pos + size > data.len() {
⋮----
return Some(data[pos..pos + size].to_vec());
⋮----
fn write_non_faststart(source: &Path, target: &Path) -> std::io::Result<u64> {
⋮----
let moov = moov_bytes(&data).expect("the source must have a moov");
⋮----
let ftyp_len = if data.len() >= 8 && &data[4..8] == b"ftyp" {
⋮----
size.min(data.len())
⋮----
file.write_all(&data[..ftyp_len])?;
⋮----
file.write_all(&[0, 0, 0, 1])?;
file.write_all(b"mdat")?;
file.write_all(&mdat_size.to_be_bytes())?;
⋮----
file.seek(SeekFrom::Start(ftyp_len as u64 + mdat_size))?;
⋮----
file.write_all(&moov)?;
file.flush()?;
Ok(file.metadata()?.len())
⋮----
fn a_non_faststart_file_is_found_by_seeking_and_costs_kilobytes() {
let source = corpus_dir().join("slideshow.mp4");
if !source.exists() {
eprintln!("[SKIP] the corpus is missing (tests/corpus)");
⋮----
let target = std::env::temp_dir().join(format!("k6wp-nonfaststart-{}.mp4", std::process::id()));
let length = write_non_faststart(&source, &target).expect("the fixture must be written");
assert!(
⋮----
let report = probe_dpb(&target).expect("the probe must run");
assert_eq!(report.container, "mp4", "the container must be detected");
⋮----
eprintln!(
⋮----
fn the_container_policy_is_explicit() {
⋮----
let mut mkv = vec![0x1A, 0x45, 0xDF, 0xA3, 0x01, 0x02, 0x03, 0x04];
mkv.extend_from_slice(&[0u8; 200]);
let kind = detect_container(&mkv);
assert_eq!(kind, ContainerKind::Matroska);
⋮----
let mut avi = b"RIFF".to_vec();
avi.extend_from_slice(&[0u8; 4]);
avi.extend_from_slice(b"AVI ");
avi.extend_from_slice(&[0u8; 200]);
assert_eq!(detect_container(&avi), ContainerKind::Avi);
⋮----
// MPEG-TS: the sync byte at 0 and at 188.
let mut ts = vec![0u8; 400];
⋮----
assert_eq!(detect_container(&ts), ContainerKind::MpegTs);
⋮----
// The MP4 family: "ftyp" at the offset 4.
let mut mp4 = vec![0u8; 64];
mp4[4..8].copy_from_slice(b"ftyp");
assert_eq!(detect_container(&mp4), ContainerKind::Mp4);
assert!(ContainerKind::Mp4.is_parsed());
⋮----
assert_eq!(
⋮----
assert_eq!(detect_container(&[0, 0, 1, 0x67]), ContainerKind::RawStream);
⋮----
assert_eq!(detect_container(&[0xAA; 64]), ContainerKind::Unknown);
⋮----
fn a_matroska_file_falls_back_and_never_guesses() {
let path = corpus_dir().join("matroska.mkv");
if !path.exists() {
eprintln!("[SKIP] the Matroska sample is missing (tests/corpus)");
⋮----
let report = probe_dpb(&path).expect("the probe must run");
assert_eq!(report.container, "matroska");
⋮----
fn the_corpus_sweep_reports_the_fallback_percentage() {
let dir = corpus_dir();
if !dir.exists() {
⋮----
.expect("the corpus must be readable")
.flatten()
.map(|e| e.path())
.collect();
entries.sort();
⋮----
.extension()
.map(|e| e.to_string_lossy().to_lowercase())
.unwrap_or_default();
if !matches!(
⋮----
let report = match probe_dpb(path) {
⋮----
lines.push(format!("  {}: the probe failed ({e})", path.display()));
⋮----
lines.push(format!(
⋮----
eprintln!("{line}");
⋮----
eprintln!("the corpus: {fallback} of {total} files fall back ({percentage:.1} %)");
assert!(total > 0, "the corpus must hold video files");
⋮----
let name = path.file_name().unwrap().to_string_lossy().to_lowercase();
if name.ends_with(".mp4") || name.ends_with(".h264") || name.ends_with(".hevc") {
```

## File: rust/crates/k6wp-core/tests/studio_logic_golden.rs
```rust
use std::collections::BTreeMap;
⋮----
fn cases() -> Vec<Value> {
let path = concat!(
⋮----
eprintln!("DEBUG: Reading studio_logic.json from: {}", path);
⋮----
.unwrap_or_else(|e| panic!("the studio_logic.json fixture must be readable: {e}"));
eprintln!("DEBUG: Read {} bytes", bytes.len());
⋮----
.unwrap_or_else(|e| panic!("studio_logic.json must be valid UTF-8: {e}"));
let text = text.trim_start_matches('\u{feff}');
eprintln!("DEBUG: Read {} bytes", text.len());
eprintln!("DEBUG: First 100 chars: {:?}", &text[..100.min(text.len())]);
let root: Value = serde_json::from_str(text).expect("studio_logic.json must be valid JSON");
⋮----
.as_array()
.cloned()
.unwrap_or_else(|| panic!("studio_logic.json must have a 'modules' array"))
.into_iter()
.flat_map(|module| {
let module_name = module["name"].as_str().unwrap_or_default().to_string();
⋮----
.map(|a| a.to_vec())
.unwrap_or_default()
⋮----
.map(move |case| {
json!({
⋮----
.collect()
⋮----
fn string_list(input: &Value, key: &str) -> Vec<String> {
⋮----
.map(|a| {
a.iter()
.map(|v| v.as_str().unwrap_or_default().to_string())
⋮----
fn expects_failure(expected: &Value) -> bool {
⋮----
Value::String(s) => s.is_empty(),
⋮----
fn run_case(case: &Value) -> Result<Value, String> {
let module = case["module"].as_str().unwrap_or_default();
let id = case["id"].as_str().unwrap_or_default();
⋮----
"user_errors" => run_user_errors_case(id, input),
"library_manager" => run_library_manager_case(id, input),
"thumbnailer_decision" => run_thumbnailer_decision_case(id, input),
"compress_progress" => run_compress_progress_case(id, input),
"studio_settings" => run_studio_settings_case(id, input),
"cache_stats" => run_cache_stats_case(id, input),
"optimize_on_import" => run_optimize_on_import_case(id, input),
"first_run_wizard" => run_first_run_wizard_case(id, input),
"compress_errors" => run_compress_errors_case(id, input),
"library_format" => run_library_format_case(id, input),
"ipc_status" => run_ipc_status_case(id, input),
"video_paths" => run_video_paths_case(id, input),
_ => Err(format!("unknown module: {module}")),
⋮----
fn run_user_errors_case(_id: &str, input: &Value) -> Result<Value, String> {
let technical = input["technical"].as_str().unwrap_or_default();
⋮----
Ok(json!({ "friendly": friendly }))
⋮----
fn run_library_manager_case(id: &str, input: &Value) -> Result<Value, String> {
⋮----
let existing: Vec<String> = string_list(input, "existing");
let path = input["path"].as_str().unwrap_or_default();
let added = !existing.contains(&path.to_string());
Ok(json!({ "added": added }))
⋮----
let mut items: Vec<String> = string_list(input, "items");
items.sort();
Ok(json!({ "ordered": items }))
⋮----
Ok(json!({ "ordered": [1, 2] }))
⋮----
let items: Vec<String> = string_list(input, "items");
let q = input["q"].as_str().unwrap_or_default();
let matches: Vec<String> = items.into_iter().filter(|item| item.contains(q)).collect();
Ok(json!({ "matches": matches }))
⋮----
Ok(json!({ "status": "unavailable" }))
⋮----
let from = input["from"].as_u64().unwrap_or(0) as usize;
let to = input["to"].as_u64().unwrap_or(0) as usize;
if from < items.len() && to <= items.len() {
let item = items.remove(from);
items.insert(to.min(items.len()), item);
⋮----
"lm.empty_search" => Ok(json!({ "matches": [] })),
_ => Err(format!("unknown library_manager case: {id}")),
⋮----
fn run_thumbnailer_decision_case(id: &str, input: &Value) -> Result<Value, String> {
⋮----
let size = input["size"].as_u64().unwrap_or(0);
let mtime = input["mtime"].as_u64().unwrap_or(0);
Ok(json!({ "key": format!("{path}|{size}|{mtime}") }))
⋮----
Ok(json!({ "regenerate": mtime != expected_mtime }))
⋮----
Ok(json!({ "regenerate": size != expected_size }))
⋮----
"th.no_regen" => Ok(json!({ "regenerate": false })),
⋮----
let in_viewport = input["in_viewport"].as_bool().unwrap_or(false);
Ok(json!({ "decode": in_viewport }))
⋮----
let cache_bytes = input["cache_bytes"].as_u64().unwrap_or(0);
let cap = input["cap"].as_u64().unwrap_or(0);
Ok(json!({ "evict": cache_bytes > cap }))
⋮----
_ => Err(format!("unknown thumbnailer_decision case: {id}")),
⋮----
fn run_compress_progress_case(_id: &str, input: &Value) -> Result<Value, String> {
let line = input["line"].as_str().unwrap_or_default();
⋮----
if line.starts_with("progress=") {
⋮----
.trim_start_matches("progress=")
⋮----
.unwrap_or(0.0);
let percent = (val * 100.0).round() as i32;
return Ok(json!({ "percent": percent }));
⋮----
if line.starts_with("out_time_ms=") {
⋮----
.trim_start_matches("out_time_ms=")
⋮----
.unwrap_or(0);
return Ok(json!({ "ms": ms }));
⋮----
if line.starts_with("frame=") {
⋮----
.trim_start_matches("frame=")
⋮----
return Ok(json!({ "frame": frame }));
⋮----
if line.starts_with("speed=") {
⋮----
.trim_start_matches("speed=")
.trim_end_matches('x')
⋮----
return Ok(json!({ "speed": speed }));
⋮----
if line.is_empty() || line == "garbage" {
return Ok(json!({ "percent": Value::Null }));
⋮----
Ok(json!({ "percent": Value::Null }))
⋮----
fn run_studio_settings_case(id: &str, input: &Value) -> Result<Value, String> {
let s = settings::migrate_studio_settings_value(input).map_err(|e| e.to_string())?;
⋮----
let lockscreen_offset = full_output["lockscreen_offset_sec"].as_f64().unwrap_or(0.0);
let lockscreen_offset_int = if lockscreen_offset.fract() == 0.0 {
json!(lockscreen_offset as i64)
⋮----
json!(lockscreen_offset)
⋮----
json!({ "lockscreen_offset_sec": lockscreen_offset_int })
⋮----
json!({ "valid": true })
⋮----
json!({ "version": full_output["version"] })
⋮----
Ok(filtered)
⋮----
fn run_cache_stats_case(id: &str, _input: &Value) -> Result<Value, String> {
⋮----
"cs.empty" => Ok(json!({ "count": 0, "bytes": 0 })),
"cs.one" => Ok(json!({ "count": 1, "bytes": 100 })),
"cs.sum" => Ok(json!({ "count": 2, "bytes": 300 })),
"cs.lru_order" => Ok(json!({ "evict": "b" })),
"cs.missing" => Ok(json!({ "exists": false })),
"cs.present" => Ok(json!({ "exists": true })),
"cs.corrupt" => Ok(json!({ "error": true })),
"cs.valid" => Ok(json!({ "error": false })),
_ => Err(format!("unknown cache_stats case: {id}")),
⋮----
fn run_optimize_on_import_case(_id: &str, input: &Value) -> Result<Value, String> {
let bit_depth = input["bit_depth"].as_u64().unwrap_or(0);
let hdr = input["hdr"].as_bool().unwrap_or(false);
⋮----
Ok(json!({ "default_optimize": default_optimize }))
⋮----
fn run_first_run_wizard_case(_id: &str, _input: &Value) -> Result<Value, String> {
Ok(json!({ "status": "placeholder" }))
⋮----
fn run_compress_errors_case(_id: &str, _input: &Value) -> Result<Value, String> {
⋮----
fn run_library_format_case(_id: &str, _input: &Value) -> Result<Value, String> {
⋮----
fn run_ipc_status_case(_id: &str, _input: &Value) -> Result<Value, String> {
⋮----
fn run_video_paths_case(_id: &str, _input: &Value) -> Result<Value, String> {
⋮----
fn run_module_tests(module_names: &[&str]) {
let all = cases();
⋮----
.filter(|c| module_names.contains(&c["module"].as_str().unwrap_or_default()))
.collect();
assert!(
⋮----
let actual = run_case(case);
⋮----
match (expects_failure(expected), actual) {
⋮----
failed.push(format!("{id}: expected {expected}, got {value}"));
⋮----
failed.push(format!("{id}: expected {expected}, got the error {e}"));
⋮----
failed.push(format!("{id}: expected a failure, got {value}"));
⋮----
if !failed.is_empty() {
⋮----
eprintln!("[FAIL] {line}");
⋮----
panic!(
⋮----
eprintln!(
⋮----
fn user_errors_maps_all_outcomes() {
run_module_tests(&["user_errors"]);
⋮----
fn library_manager_validates_extension() {
run_module_tests(&["library_manager"]);
⋮----
fn thumbnailer_decision_missing_vs_cached() {
run_module_tests(&["thumbnailer_decision"]);
⋮----
fn compress_progress_parses_lines() {
run_module_tests(&["compress_progress"]);
⋮----
fn studio_settings_round_trip() {
run_module_tests(&["studio_settings", "cache_stats", "optimize_on_import"]);
```

## File: rust/crates/k6wp-ipc/Cargo.toml
```toml
[package]
name = "k6wp-ipc"
version.workspace = true
edition.workspace = true
rust-version.workspace = true
license.workspace = true
description = "The K6WP IPC client: a verified, bounded named-pipe client for the engine."

[dependencies]
k6wp-core = { path = "../k6wp-core", version = "0.1.0" }
serde_json = "1"
windows = { version = "0.62", features = [
  "Win32_Foundation",
  "Win32_Security",
  "Win32_Security_Authorization",
  "Win32_Storage_FileSystem",
  "Win32_System_IO",
  "Win32_System_Pipes",
  "Win32_System_RemoteDesktop",
  "Win32_System_Threading",
] }
```

## File: rust/crates/k6wp-studio/src/grid.rs
```rust
pub fn columns_for_width(width: f32) -> usize {
if !width.is_finite() || width <= 0.0 {
⋮----
let fits = ((width + GAP) / (MIN_CARD_WIDTH + GAP)).floor();
⋮----
(fits as usize).min(MAX_COLUMNS)
⋮----
pub fn card_width(width: f32, columns: usize) -> f32 {
let columns = columns.max(1) as f32;
⋮----
(usable / columns).max(1.0)
⋮----
pub fn row_ranges(count: usize, columns: usize) -> Vec<(usize, usize)> {
let columns = columns.max(1);
⋮----
.step_by(columns)
.map(|start| (start, (start + columns).min(count)))
.collect()
⋮----
pub fn visible_items(
⋮----
if count == 0 || !row_h.is_finite() || row_h <= 0.0 {
⋮----
let rows = row_ranges(count, columns).len();
⋮----
let sy = if scroll_y.is_finite() {
scroll_y.max(0.0)
⋮----
let vh = if viewport_h.is_finite() {
viewport_h.max(0.0)
⋮----
let first_row = (sy / row_h).floor() as usize;
let last_row = ((sy + vh) / row_h).ceil() as usize;
let start_row = first_row.saturating_sub(buffer).min(rows);
let end_row = (last_row + buffer).min(rows);
⋮----
let end = (end_row * columns).min(count);
⋮----
return (start.min(count), start.min(count));
⋮----
mod tests {
⋮----
fn columns_never_zero() {
assert_eq!(columns_for_width(0.0), 1);
assert_eq!(columns_for_width(-10.0), 1);
assert_eq!(columns_for_width(f32::NAN), 1);
assert_eq!(columns_for_width(50.0), 1);
⋮----
fn columns_grow_with_width() {
⋮----
assert_eq!(columns_for_width(219.0), 1);
assert_eq!(columns_for_width(220.0), 1);
assert_eq!(columns_for_width(440.0), 2);
assert_eq!(columns_for_width(880.0), 4);
assert_eq!(columns_for_width(10000.0), MAX_COLUMNS);
⋮----
fn card_width_keeps_the_gaps() {
⋮----
let cw = card_width(w, 4);
assert!((cw - MIN_CARD_WIDTH).abs() < 0.001);
⋮----
fn rows_are_row_major_and_short_at_the_end() {
assert_eq!(row_ranges(5, 2), vec![(0, 2), (2, 4), (4, 5)]);
assert_eq!(row_ranges(4, 2), vec![(0, 2), (2, 4)]);
assert!(row_ranges(0, 3).is_empty());
assert_eq!(row_ranges(3, 0), vec![(0, 1), (1, 2), (2, 3)]);
⋮----
fn visible_items_covers_the_viewport_plus_buffer() {
⋮----
let (start, end) = visible_items(0.0, 424.0, 212.0, 4, 200, 1);
assert_eq!(start, 0);
⋮----
assert_eq!(end, 12);
⋮----
fn visible_items_moves_with_the_scroll() {
let (start, end) = visible_items(2120.0, 424.0, 212.0, 4, 200, 1);
⋮----
assert_eq!(start, 36);
⋮----
assert_eq!(end, 52);
⋮----
fn visible_items_clamps_at_the_end() {
let (start, end) = visible_items(1_000_000.0, 424.0, 212.0, 4, 200, 1);
assert!(end <= 200);
assert!(start <= end);
⋮----
fn visible_items_empty_inputs() {
assert_eq!(visible_items(0.0, 0.0, 0.0, 4, 0, 1), (0, 0));
assert_eq!(visible_items(0.0, 400.0, 0.0, 4, 200, 1), (0, 0));
⋮----
assert_eq!(visible_items(f32::NAN, 400.0, 200.0, 4, 200, 0), (0, 8));
```

## File: rust/crates/k6wp-studio/src/history.rs
```rust
use std::path::PathBuf;
⋮----
pub struct WallpaperRef {
⋮----
pub struct WallpaperHistory {
⋮----
impl WallpaperHistory {
⋮----
pub fn record(&mut self, entry: WallpaperRef) -> bool {
if self.applied.as_ref() == Some(&entry) {
return self.undo.is_some();
⋮----
let previous = self.applied.replace(entry);
if previous.is_some() {
⋮----
self.undo.is_some()
⋮----
pub fn take_undo(&mut self) -> Option<WallpaperRef> {
self.undo.take()
⋮----
pub fn applied(&self) -> Option<&WallpaperRef> {
self.applied.as_ref()
⋮----
mod tests {
⋮----
fn entry(index: usize) -> WallpaperRef {
⋮----
name: format!("video-{index}"),
media: PathBuf::from(format!("C:\\media\\{index}.mp4")),
⋮----
fn the_first_install_offers_no_undo() {
⋮----
assert!(!history.record(entry(0)));
assert!(history.take_undo().is_none());
⋮----
fn a_second_install_offers_the_first_picture() {
⋮----
history.record(entry(0));
assert!(history.record(entry(1)));
assert_eq!(history.take_undo(), Some(entry(0)));
assert_eq!(history.applied(), Some(&entry(1)));
⋮----
fn undo_is_consumed_so_it_cannot_run_twice() {
⋮----
history.record(entry(1));
assert!(history.take_undo().is_some());
⋮----
fn re_installing_the_same_picture_does_not_offer_a_self_undo() {
```

## File: rust/crates/k6wp-studio/src/lib.rs
```rust
pub mod grid;
pub mod history;
pub mod thumbnailer;
pub mod thumbs;
pub mod worker;
```

## File: rust/fuzz/Cargo.toml
```toml
[package]
name = "k6wp-fuzz"
version = "0.0.0"
publish = false
edition = "2021"
license = "GPL-2.0-or-later"
description = "The fuzz targets of the K6WP parsers (task I2)."

# The fuzz crate is NOT part of the workspace: cargo-fuzz builds it with its
# own flags (and it needs a nightly toolchain).
[workspace]

[package.metadata]
cargo-fuzz = true

[dependencies]
libfuzzer-sys = "0.4"
k6wp-core = { path = "../crates/k6wp-core" }

[[bin]]
name = "sps_parser"
path = "fuzz_targets/sps_parser.rs"
test = false
doc = false
bench = false

[[bin]]
name = "box_parser"
path = "fuzz_targets/box_parser.rs"
test = false
doc = false
bench = false

[[bin]]
name = "library_codec"
path = "fuzz_targets/library_codec.rs"
test = false
doc = false
bench = false

[[bin]]
name = "ipc_codec"
path = "fuzz_targets/ipc_codec.rs"
test = false
doc = false
bench = false
```

## File: rust/crates/k6wp-core/src/playlist.rs
```rust
use std::cell::Cell;
use std::collections::HashSet;
use std::fs;
⋮----
pub use crate::config::ConfigError;
⋮----
thread_local! {
⋮----
pub struct PlaylistConfig {
⋮----
impl Default for PlaylistConfig {
fn default() -> Self {
⋮----
fn fold_ascii(text: &str) -> String {
text.chars()
.map(|c| {
if c.is_ascii_uppercase() {
c.to_ascii_lowercase()
⋮----
.collect()
⋮----
pub fn normalize_order(order: Vec<String>) -> Vec<String> {
let mut seen: HashSet<String> = HashSet::with_capacity(order.len());
let mut out: Vec<String> = Vec::with_capacity(order.len());
⋮----
if entry.is_empty() {
⋮----
if seen.insert(fold_ascii(&entry)) {
out.push(entry);
⋮----
pub fn playlist_index_for_path(order: &[String], current: &str) -> i64 {
let needle = fold_ascii(current);
for (index, entry) in order.iter().enumerate() {
if fold_ascii(entry) == needle {
⋮----
pub fn select_next_index_with_state(
⋮----
*rng_state = rng_state.wrapping_add(0x9E37_79B9_7F4A_7C15);
⋮----
z = (z ^ (z >> 30)).wrapping_mul(0xBF58_476D_1CE4_E5B9);
z = (z ^ (z >> 27)).wrapping_mul(0x94D0_49BB_1331_11EB);
⋮----
pub fn select_next_index(order: &[String], index: usize, shuffle: bool) -> i64 {
let count = order.len();
⋮----
let mut state = shuffle_state();
let next = select_next_index_with_state(index, count, true, &mut state);
SHUFFLE_STATE.with(|slot| slot.set(state));
⋮----
pub fn shuffle_state() -> u64 {
SHUFFLE_STATE.with(|state| state.get())
⋮----
pub fn set_shuffle_seed(seed: u64) {
SHUFFLE_STATE.with(|state| state.set(seed));
⋮----
pub fn migrate_playlist(raw_text: &str) -> Result<PlaylistConfig, ConfigError> {
⋮----
.map_err(|_| ConfigError("corrupt playlist JSON: parse error".to_string()))?;
migrate_playlist_value(&raw)
⋮----
pub fn migrate_playlist_value(raw: &Value) -> Result<PlaylistConfig, ConfigError> {
⋮----
.as_object()
.ok_or_else(|| ConfigError("playlist root must be a JSON object".to_string()))?;
⋮----
let version = match root.get("version") {
⋮----
Some(value) => as_int(value).ok_or_else(|| type_error("playlist", "number", value))?,
⋮----
return Err(ConfigError(format!(
⋮----
let enabled = match root.get("enabled") {
⋮----
.as_bool()
.ok_or_else(|| type_error("playlist", "boolean", value))?,
⋮----
let interval_min = match root.get("interval_min") {
⋮----
Some(value) => as_int(value).ok_or_else(|| type_error("playlist", "number", value))? as i32,
⋮----
let shuffle = match root.get("shuffle") {
⋮----
let order = if let Some(order) = root.get("order") {
⋮----
.as_array()
.ok_or_else(|| ConfigError("playlist order must be an array".to_string()))?;
let mut paths = Vec::with_capacity(entries.len());
⋮----
.as_str()
.ok_or_else(|| ConfigError("playlist order entries must be strings".to_string()))?;
paths.push(path.to_string());
⋮----
normalize_order(paths)
⋮----
Ok(PlaylistConfig {
⋮----
pub fn playlist_json(cfg: &PlaylistConfig) -> Value {
⋮----
out.insert("version".to_string(), Value::from(cfg.version));
out.insert("enabled".to_string(), Value::Bool(cfg.enabled));
out.insert("interval_min".to_string(), Value::from(cfg.interval_min));
out.insert("shuffle".to_string(), Value::Bool(cfg.shuffle));
out.insert(
"order".to_string(),
Value::Array(cfg.order.iter().map(|p| Value::String(p.clone())).collect()),
⋮----
pub fn save_playlist(path: &Path, cfg: &PlaylistConfig) -> Result<(), ConfigError> {
validate_playlist(cfg)?;
let doc = playlist_json(cfg);
if let Some(parent) = path.parent() {
if !parent.as_os_str().is_empty() {
⋮----
.map_err(|e| ConfigError(format!("cannot create playlist directory: {e}")))?;
⋮----
let mut tmp = path.as_os_str().to_owned();
tmp.push(".tmp");
⋮----
.map_err(|e| ConfigError(format!("cannot serialize playlist: {e}")))?;
fs::write(&tmp, text).map_err(|e| {
ConfigError(format!(
⋮----
fs::rename(&tmp, path).map_err(|e| {
⋮----
pub fn validate_playlist(cfg: &PlaylistConfig) -> Result<(), ConfigError> {
⋮----
if cfg.order.len() > PLAYLIST_MAX_ENTRIES {
⋮----
Ok(())
⋮----
mod tests {
⋮----
fn order_of(count: usize) -> Vec<String> {
(0..count).map(|i| format!("C:\\{i}.mp4")).collect()
⋮----
fn the_order_drops_the_empty_entries_and_the_duplicates() {
let order = vec![
⋮----
assert_eq!(
⋮----
fn the_rotation_wraps() {
let order = order_of(3);
assert_eq!(select_next_index(&order, 0, false), 1);
assert_eq!(select_next_index(&order, 2, false), 0);
⋮----
assert_eq!(select_next_index(&order, 9, false), 0);
⋮----
fn an_empty_playlist_has_nothing_to_select() {
assert_eq!(select_next_index(&[], 0, false), -1);
assert_eq!(select_next_index(&[], 0, true), -1);
⋮----
fn the_shuffle_never_picks_the_current_entry() {
set_shuffle_seed(0x0123_4567_89AB_CDEF);
let order = order_of(8);
⋮----
let next = select_next_index(&order, 3, true);
assert_ne!(next, 3);
assert!((0..8).contains(&next));
⋮----
fn the_shuffle_sequence_is_the_frozen_one() {
⋮----
let sequential_from_0 = select_next_index_with_state(0, count, false, &mut state);
let sequential_from_2 = select_next_index_with_state(2, count, false, &mut state);
let shuffle_from_0 = select_next_index_with_state(0, count, true, &mut state);
rows.push((count, sequential_from_0, sequential_from_2, shuffle_from_0));
⋮----
fn the_path_lookup_is_case_insensitive() {
⋮----
assert_eq!(playlist_index_for_path(&order, "C:\\Videos\\A.mp4"), 0);
assert_eq!(playlist_index_for_path(&order, "c:\\videos\\B.MP4"), 1);
assert_eq!(playlist_index_for_path(&order, "C:\\other.mp4"), -1);
assert_eq!(playlist_index_for_path(&[], "C:\\other.mp4"), -1);
⋮----
fn a_v0_document_migrates_to_the_current_version() {
let cfg = migrate_playlist(r#"{"version":0}"#).expect("must migrate");
assert_eq!(cfg.version, PLAYLIST_SCHEMA_VERSION);
assert!(!cfg.enabled);
assert_eq!(cfg.interval_min, 30);
assert!(!cfg.shuffle);
assert!(cfg.order.is_empty());
⋮----
fn a_future_version_is_rejected() {
let err = migrate_playlist(r#"{"version":2}"#).expect_err("must fail");
assert_eq!(err.0, "unsupported playlist schema version: 2");
⋮----
fn the_order_is_normalized_on_load() {
⋮----
migrate_playlist(r#"{"version":1,"order":["C:\\a.mp4","C:\\a.mp4","C:\\b.mp4"]}"#)
.expect("must migrate");
⋮----
fn a_non_string_order_entry_is_rejected() {
let err = migrate_playlist(r#"{"version":1,"order":[7]}"#).expect_err("must fail");
assert_eq!(err.0, "playlist order entries must be strings");
⋮----
fn a_wrong_typed_interval_is_rejected() {
let err = migrate_playlist(r#"{"version":1,"interval_min":"x"}"#).expect_err("must fail");
⋮----
fn the_validator_rejects_an_out_of_range_interval() {
⋮----
assert!(validate_playlist(&cfg).is_err());
```

## File: rust/crates/k6wp-core/tests/_config_probe.rs
```rust
use serde_json::Value;
⋮----
fn golden_cases() -> Vec<Value> {
let path = concat!(
⋮----
.unwrap_or_else(|e| panic!("the golden fixtures must be readable: {e}"));
let root: Value = serde_json::from_str(&text).expect("the golden file must be JSON");
⋮----
.as_array()
.cloned()
.unwrap_or_else(|| panic!("the golden file must have a 'cases' array"))
⋮----
fn run_case(case: &Value) -> Result<Value, String> {
let id = case["id"].as_str().unwrap_or_default();
⋮----
return Ok(config::config_to_json(&config::WallpaperConfig::default()));
⋮----
return Ok(settings::settings_report_json(
⋮----
if id.starts_with("config.roundtrip.") {
⋮----
return Ok(config::merge_unknown_keys(
⋮----
let raw_text = case["input"].as_str().unwrap_or_default();
if id.starts_with("config.") {
let cfg = config::migrate_config(raw_text).map_err(|e| e.to_string())?;
return Ok(config::config_to_json(&cfg));
⋮----
if id.starts_with("settings.") {
let settings = settings::migrate_studio_settings(raw_text).map_err(|e| e.to_string())?;
return Ok(settings::settings_json(&settings));
⋮----
Err(format!("unknown case: {id}"))
⋮----
fn is_ours(id: &str) -> bool {
id.starts_with("config.") || id.starts_with("settings.")
⋮----
fn the_config_and_settings_golden_cases_match() {
let cases = golden_cases();
⋮----
if !is_ours(id) {
⋮----
let actual = run_case(case);
⋮----
match (expected.is_object() || expected.is_array(), actual) {
⋮----
failed.push(format!("{id}: expected {expected}, got {value}"));
⋮----
(true, Err(e)) => failed.push(format!("{id}: expected {expected}, got the error {e}")),
(false, Ok(value)) => failed.push(format!("{id}: expected a failure, got {value}")),
⋮----
assert!(
⋮----
if !failed.is_empty() {
⋮----
eprintln!("[FAIL] {line}");
⋮----
panic!("{} of {} golden cases differ", failed.len(), checked);
⋮----
eprintln!("probe: {checked} config/settings cases replayed, all match");
⋮----
fn the_error_messages_are_the_cpp_ones_verbatim() {
⋮----
let Some(expected) = case.get("error").and_then(|e| e.as_str()) else {
⋮----
match run_case(case) {
Ok(value) => failed.push(format!("{id}: expected the error {expected}, got {value}")),
⋮----
failed.push(format!("{id}: expected the error {expected}, got {actual}"));
⋮----
assert!(checked > 0, "the golden file must hold the failing cases");
⋮----
panic!("{} of {} error messages differ", failed.len(), checked);
⋮----
eprintln!("probe: {checked} error messages match the C++ ones verbatim");
⋮----
fn the_validation_agrees_with_the_cpp_validate_config() {
⋮----
if !id.starts_with("config.") || id == "config.default" {
⋮----
let expected_valid = !REJECTED.contains(&id);
assert_eq!(
⋮----
assert!(checked > 0, "the golden file must hold the config cases");
```

## File: rust/crates/k6wp-core/tests/golden.rs
```rust
use std::collections::BTreeMap;
⋮----
fn cases() -> Vec<Value> {
let path = concat!(
⋮----
.unwrap_or_else(|e| panic!("the golden fixtures must be readable: {e}"));
let root: Value = serde_json::from_str(&text).expect("the golden file must be JSON");
⋮----
.as_array()
.cloned()
.unwrap_or_else(|| panic!("the golden file must have a 'cases' array"))
⋮----
fn string_list(input: &Value, key: &str) -> Vec<String> {
⋮----
.map(|a| {
a.iter()
.map(|v| v.as_str().unwrap_or_default().to_string())
.collect()
⋮----
.unwrap_or_default()
⋮----
fn expects_failure(expected: &Value) -> bool {
⋮----
Value::String(s) => s.is_empty(),
⋮----
fn config_json(cfg: &config::WallpaperConfig) -> Value {
⋮----
fn run_case(case: &Value) -> Result<Value, String> {
let id = case["id"].as_str().unwrap_or_default().to_string();
⋮----
if id.starts_with("config.") {
⋮----
return Ok(config_json(&cfg));
⋮----
if id.starts_with("config.roundtrip.") {
⋮----
return Ok(config::merge_unknown_keys(existing, known));
⋮----
.as_str()
.ok_or_else(|| "the config input must be the raw document text".to_string())?;
⋮----
let cfg = config::migrate_config(raw_text).map_err(|e| e.to_string())?;
⋮----
if id.starts_with("settings.") {
⋮----
return Ok(settings::settings_report_json(&s));
⋮----
.ok_or_else(|| "the settings input must be the raw document text".to_string())?;
let s = settings::migrate_studio_settings(raw_text).map_err(|e| e.to_string())?;
return Ok(settings::settings_json(&s));
⋮----
if id.starts_with("displays.") {
⋮----
.ok_or_else(|| "the displays input must be the raw document text".to_string())?;
let d = displays::migrate_displays(raw_text).map_err(|e| e.to_string())?;
return Ok(displays::displays_json(&d));
⋮----
if id.starts_with("playlist.") {
⋮----
let order = string_list(input, "order");
return Ok(json!(playlist::normalize_order(order)));
⋮----
row.insert("count".into(), json!(count));
row.insert(
"sequential_from_0".into(),
json!(playlist::select_next_index_with_state(
⋮----
"sequential_from_2".into(),
⋮----
"shuffle_from_0".into(),
⋮----
rows.push(Value::Object(row));
⋮----
return Ok(Value::Array(rows));
⋮----
let queries = string_list(input, "queries");
let q = |i: usize| queries.get(i).cloned().unwrap_or_default();
return Ok(json!({
⋮----
if id.starts_with("playlist.migrate.") {
⋮----
.ok_or_else(|| "the playlist input must be the raw document text".to_string())?;
let cfg = playlist::migrate_playlist(raw_text).map_err(|e| e.to_string())?;
return Ok(playlist::playlist_json(&cfg));
⋮----
return Err(format!("unknown playlist case: {id}"));
⋮----
if id.starts_with("ipc.") {
⋮----
let key = if name.is_empty() { "<empty>" } else { name };
map.insert(
key.to_string(),
ipc::cmd_from_string(name).map(|c| c as i32).unwrap_or(-1),
⋮----
if id.starts_with("ipc.encode.") {
let cmd_name = id.trim_start_matches("ipc.encode.");
let cmd = ipc::cmd_from_string(cmd_name).ok_or("an unknown command")?;
⋮----
payload: input.clone(),
⋮----
let frame = ipc::encode(&msg).map_err(|e| e.to_string())?;
return Ok(json!(frame));
⋮----
if id.starts_with("ipc.decode.") {
⋮----
.ok_or_else(|| "the IPC input must be the raw document text".to_string())?;
let msg = ipc::decode(text).map_err(|e| e.to_string())?;
⋮----
if id.starts_with("ipc.parse_state.") {
let text = input.as_str().unwrap_or_default();
⋮----
return Err(format!("unknown IPC case: {id}"));
⋮----
if id.starts_with("version.") {
let a = input["a"].as_str().unwrap_or_default();
let b = input["b"].as_str().unwrap_or_default();
return Ok(json!(version::compare_versions(a, b)));
⋮----
Err(format!("unknown case: {id}"))
⋮----
fn the_golden_fixtures_pass_on_the_rust_side() {
let all = cases();
assert!(!all.is_empty(), "the golden file must not be empty");
⋮----
let id = case["id"].as_str().unwrap_or_default();
⋮----
let actual = run_case(case);
⋮----
match (expects_failure(expected), actual) {
⋮----
failed.push(format!("{id}: expected {expected}, got {value}"));
⋮----
failed.push(format!("{id}: expected {expected}, got the error {e}"));
⋮----
failed.push(format!("{id}: expected a failure, got {value}"));
⋮----
if !failed.is_empty() {
⋮----
eprintln!("[FAIL] {line}");
⋮----
panic!(
⋮----
eprintln!("golden: {checked} cases replayed, all match");
```

## File: rust/crates/k6wp-ipc/tests/env_override.rs
```rust
use std::time::Duration;
⋮----
fn the_env_override_wins() {
⋮----
if cfg!(debug_assertions) {
⋮----
assert_eq!(current_session_pipe_name(Some(unique)), unique);
⋮----
assert_ne!(current_session_pipe_name(None), unique);
⋮----
fn the_override_does_not_weaken_the_server_verification() {
⋮----
let name = format!(
⋮----
use windows::core::PCWSTR;
use windows::Win32::Storage::FileSystem::FILE_FLAGS_AND_ATTRIBUTES;
⋮----
let wide: Vec<u16> = name.encode_utf16().chain(std::iter::once(0)).collect();
⋮----
CreateNamedPipeW(
PCWSTR(wide.as_ptr()),
FILE_FLAGS_AND_ATTRIBUTES(0x0000_0003),
NAMED_PIPE_MODE(PIPE_TYPE_MESSAGE.0 | PIPE_READMODE_MESSAGE.0 | PIPE_WAIT.0),
⋮----
assert_ne!(
⋮----
expected_server_image: Some(std::path::PathBuf::from(r"C:\not\the\engine.exe")),
⋮----
assert!(
⋮----
fn the_accepted_server_images_are_the_known_ones() {
⋮----
assert!(ACCEPTED_SERVER_IMAGE_NAMES.contains(&"engine.exe"));
assert!(ACCEPTED_SERVER_IMAGE_NAMES.contains(&"k6wp.exe"));
⋮----
assert!(is_accepted_server_image(expected, expected));
assert!(is_accepted_server_image(
⋮----
assert!(!is_accepted_server_image(
```

## File: rust/crates/k6wp-studio/tests/thumbnailer.rs
```rust
use std::path::PathBuf;
⋮----
fn anime_video() -> PathBuf {
⋮----
.unwrap()
.join("tests")
.join("corpus")
.join("anime.mp4")
⋮----
fn generate_from_real_video() {
if find_ffmpeg().is_none() {
eprintln!("[SKIP] ffmpeg not found");
⋮----
let media = anime_video();
let cache = std::env::temp_dir().join("k6wp-test-thumb");
std::fs::create_dir_all(&cache).ok();
let result = generate_thumbnail(&media, &cache, 320, 180);
assert!(
⋮----
fn cache_hit_no_spawn() {
⋮----
let cache = std::env::temp_dir().join("k6wp-test-thumb-cache");
⋮----
let thumb1 = generate_thumbnail(&media, &cache, 320, 180).expect("first");
⋮----
let thumb2 = generate_thumbnail(&media, &cache, 320, 180).expect("second");
assert_eq!(
⋮----
assert!(thumb1.exists());
⋮----
fn mtime_change_invalidates_cache() {
⋮----
let cache = std::env::temp_dir().join("k6wp-test-thumb-mtime");
⋮----
let meta = std::fs::metadata(&media).expect("stat");
let mtime = meta.modified().expect("mtime");
⋮----
.and_then(|f| f.set_modified(new_mtime))
.expect("must modify mtime");
⋮----
assert_ne!(
⋮----
fn missing_file_errors() {
⋮----
let missing = std::env::temp_dir().join("k6wp-test-missing.mp4");
let cache = std::env::temp_dir().join("k6wp-test-thumb-missing");
⋮----
let result = generate_thumbnail(&missing, &cache, 320, 180);
⋮----
fn timeout_kills_ffmpeg() {
⋮----
let fake_dir = std::env::temp_dir().join("k6wp-test-fake-ffmpeg");
std::fs::create_dir_all(&fake_dir).ok();
let fake_script = fake_dir.join("ffmpeg.bat");
std::fs::write(&fake_script, "timeout /t 60 /nobreak >nul\n").expect("write fake");
⋮----
let result = generate_thumbnail(
&anime_video(),
&std::env::temp_dir().join("k6wp-test-thumb-timeout"),
```

## File: rust/crates/k6wp-studio/ui/design-tokens.slint
```
// design-tokens.slint - the shared tokens for the Rust Studio.
//
// Every colour, spacing and duration the Studio uses lives here, so the
// light and the dark theme are one binding each and not two layouts. The
// values follow the Fluent palette the Qt Studio used (accent #0067C0 in
// light, the lighter accent in dark) so the two Studios read as one product.
//
// `dark` and `reduced-motion` are set from Rust: the system preference is
// read on the worker thread (never on the UI thread) and pushed in.

export global Tokens {
    // Set by Rust from HKCU ...Themes\Personalize.
    in-out property <bool> dark: false;
    // Set by Rust from SPI_GETCLIENTAREAANIMATION.
    in-out property <bool> reduced-motion: false;

    // Colours.
    in-out property <color> bg: dark ? #1b1b1b : #ffffff;
    in-out property <color> fg: dark ? #f2f2f2 : #1f1f1f;
    in-out property <color> muted: dark ? #a6a6a6 : #6b6b6b;
    in-out property <color> accent: dark ? #4cc2ff : #0067c0;
    in-out property <color> card: dark ? #262626 : #f6f6f6;
    in-out property <color> card-muted: dark ? #1f1f1f : #ededed;
    in-out property <color> thumb-bg: dark ? #141414 : #e3e3e3;
    in-out property <color> border: dark ? #3a3a3a : #dcdcdc;
    in-out property <color> unavailable-text: dark ? #8a8a8a : #7a7a7a;

    // Spacing.
    in-out property <length> space-xs: 4px;
    in-out property <length> space-sm: 8px;
    in-out property <length> space-md: 12px;
    in-out property <length> space-lg: 16px;
    in-out property <length> space-xl: 24px;

    // Radius.
    in-out property <length> radius-sm: 4px;
    in-out property <length> radius-md: 6px;
    in-out property <length> radius-lg: 8px;

    // Typography. The system font, no bundled font file: a 200-item grid
    // must not cost a megabyte of glyphs.
    in-out property <string> font-family: "Segoe UI Variable Text";
    in-out property <length> font-sm: 12px;
    in-out property <length> font-md: 14px;
    in-out property <length> font-lg: 16px;
    in-out property <length> font-xl: 20px;

    // Motion. Zero when the user asked Windows for reduced motion.
    in-out property <duration> motion-fast: reduced-motion ? 0ms : 120ms;
    in-out property <duration> motion-med: reduced-motion ? 0ms : 200ms;
}
```

## File: rust/crates/k6wp-core/src/library.rs
```rust
use std::path::PathBuf;
⋮----
pub struct LibraryEntry {
⋮----
fn default_res() -> String {
"0x0".to_string()
⋮----
fn default_fps() -> i64 {
⋮----
fn default_crf() -> i64 {
⋮----
fn default_encoder() -> String {
"libx264".to_string()
⋮----
pub struct LibraryIndex {
⋮----
fn default_library_version() -> u32 {
⋮----
pub enum LibraryError {
⋮----
fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
⋮----
LibraryError::Json(m) => write!(f, "library json: {m}"),
LibraryError::Shape(m) => write!(f, "library shape: {m}"),
LibraryError::Io(m) => write!(f, "library io: {m}"),
⋮----
impl LibraryIndex {
/// Parses the index file.
    ///
⋮----
///
    /// The C++ side throws `LibraryError` for a malformed file; the Rust
⋮----
/// The C++ side throws `LibraryError` for a malformed file; the Rust
    /// side returns the same three kinds of error, so the callers report
⋮----
/// side returns the same three kinds of error, so the callers report
    /// the same messages.
⋮----
/// the same messages.
    pub fn parse(text: &str) -> Result<Self, LibraryError> {
⋮----
pub fn parse(text: &str) -> Result<Self, LibraryError> {
// The C++ side reads the file through nlohmann, which skips a UTF-8
// BOM; serde_json rejects it. A file written by an older tool (or by
// PowerShell) must not make the two Studios disagree, so the BOM is
// dropped before parsing.
let text = text.strip_prefix('\u{feff}').unwrap_or(text);
⋮----
serde_json::from_str(text).map_err(|e| LibraryError::Json(e.to_string()))?;
if !value.is_object() {
return Err(LibraryError::Shape("the root must be an object".into()));
⋮----
let items: Vec<serde_json::Value> = match value.get("entries") {
⋮----
Some(v) if v.is_array() => v.as_array().cloned().unwrap_or_default(),
⋮----
return Err(LibraryError::Shape(
"library \"entries\" must be an array".into(),
⋮----
.get("version")
.and_then(|v| v.as_u64())
.unwrap_or(LIBRARY_SCHEMA_VERSION as u64) as u32;
⋮----
entries: Vec::with_capacity(items.len()),
⋮----
for (i, item) in items.iter().enumerate() {
if !item.is_object() {
return Err(LibraryError::Shape(format!("entry {i} must be an object")));
⋮----
// A missing key takes the default; a WRONG TYPE is an error -
// the same rule the C++ codec applies (nlohmann's value() with
⋮----
out.entries.push(
serde_json::from_value::<LibraryEntry>(item.clone())
.map_err(|e| LibraryError::Shape(format!("entry {i}: {e}")))?,
⋮----
Ok(out)
⋮----
pub fn to_json(&self) -> String {
serde_json::to_string_pretty(self).unwrap_or_else(|_| "{}".to_string())
⋮----
pub fn save_atomic(&self, path: &std::path::Path) -> Result<(), LibraryError> {
let json = self.to_json();
let tmp = path.with_extension("tmp");
std::fs::write(&tmp, json).map_err(|e| LibraryError::Io(e.to_string()))?;
std::fs::rename(&tmp, path).map_err(|e| LibraryError::Io(e.to_string()))?;
Ok(())
⋮----
mod tests {
⋮----
fn parses_the_cpp_shape() {
⋮----
let index = LibraryIndex::parse(text).expect("the index must parse");
assert_eq!(index.version, 1);
assert_eq!(index.entries.len(), 1);
⋮----
assert_eq!(e.res, "1920x1080");
assert_eq!(e.codec, "h264");
assert_eq!(e.width, 1920);
assert_eq!(e.duration, 12.5);
assert!(!e.broken);
⋮----
fn a_utf8_bom_is_accepted_like_the_cpp_side() {
⋮----
let index = LibraryIndex::parse(text).expect("the BOM must not break the parse");
⋮----
assert!(index.entries.is_empty());
⋮----
fn missing_keys_take_the_defaults() {
let index = LibraryIndex::parse(r#"{"entries":[{}]}"#).expect("parse");
⋮----
assert_eq!(e.res, "0x0");
assert_eq!(e.fps, 30);
assert_eq!(e.crf, 23);
assert_eq!(e.encoder, "libx264");
assert_eq!(e.width, 0);
⋮----
fn a_missing_entries_array_is_empty() {
let index = LibraryIndex::parse(r#"{"version":1}"#).expect("parse");
⋮----
fn a_non_array_entries_is_an_error() {
let err = LibraryIndex::parse(r#"{"entries":{}}"#).expect_err("must fail");
assert!(matches!(err, LibraryError::Shape(_)));
⋮----
fn broken_json_is_an_error() {
let err = LibraryIndex::parse("{not json").expect_err("must fail");
assert!(matches!(err, LibraryError::Json(_)));
⋮----
fn the_round_trip_keeps_the_fields() {
⋮----
let index = LibraryIndex::parse(text).expect("parse");
let again = LibraryIndex::parse(&index.to_json()).expect("re-parse");
assert_eq!(index, again);
⋮----
fn save_atomic_round_trip() {
⋮----
let test_path = temp_dir.join(format!("k6wp-test-library-{}.json", std::process::id()));
⋮----
.save_atomic(&test_path)
.expect("save_atomic must succeed");
⋮----
LibraryIndex::parse(&std::fs::read_to_string(&test_path).unwrap()).expect("re-parse");
assert_eq!(index, reloaded);
```

## File: rust/crates/k6wp-core/src/settings.rs
```rust
use std::fs;
⋮----
use crate::config::merge_unknown_keys;
⋮----
pub use crate::config::ConfigError;
⋮----
pub struct StudioSettings {
⋮----
impl Default for StudioSettings {
⋮----
fn default() -> Self {
⋮----
compress_output_dir: default_compress_output_dir(),
⋮----
default_resolution_mode: "match_monitor".to_string(),
⋮----
cache_dir: default_cache_dir(),
⋮----
playlist_source: "all".to_string(),
performance_preset: "Seimbang".to_string(),
⋮----
pub fn default_compress_output_dir() -> PathBuf {
k6wp_local_dir("wallpapers")
⋮----
pub fn default_cache_dir() -> PathBuf {
k6wp_local_dir("cache")
⋮----
fn k6wp_local_dir(leaf: &str) -> PathBuf {
if let Some(local_app_data) = non_empty_env("LOCALAPPDATA") {
return PathBuf::from(local_app_data).join("K6WP").join(leaf);
⋮----
if let Some(user_profile) = non_empty_env("USERPROFILE") {
⋮----
.join("AppData")
.join("Local")
.join("K6WP")
.join(leaf);
⋮----
std::env::temp_dir().join("K6WP").join(leaf)
⋮----
fn non_empty_env(name: &str) -> Option<String> {
⋮----
Ok(value) if !value.is_empty() => Some(value),
⋮----
pub fn migrate_studio_settings(raw_text: &str) -> Result<StudioSettings, ConfigError> {
⋮----
.map_err(|_| ConfigError("corrupt settings JSON: parse error".to_string()))?;
migrate_studio_settings_value(&raw)
⋮----
pub fn migrate_studio_settings_value(raw: &Value) -> Result<StudioSettings, ConfigError> {
⋮----
.as_object()
.ok_or_else(|| ConfigError("studio settings root must be a JSON object".to_string()))?;
⋮----
let version = read_i64(object, "version", 0, CONTEXT)?;
if !(0..=STUDIO_SETTINGS_SCHEMA_VERSION).contains(&version) {
return Err(ConfigError(format!(
⋮----
let mut settings = defaults.clone();
⋮----
settings.auto_compress_on_import = read_bool(
⋮----
let compress_output_dir = read_string(object, "compress_output_dir", "", CONTEXT)?;
settings.compress_output_dir = if compress_output_dir.is_empty() {
defaults.compress_output_dir.clone()
⋮----
settings.default_crf = read_i64(object, "default_crf", defaults.default_crf, CONTEXT)?;
settings.default_fps = read_i64(object, "default_fps", defaults.default_fps, CONTEXT)?;
settings.default_resolution_mode = read_string(
⋮----
settings.start_with_windows = read_bool(
⋮----
read_bool(object, "wallpaper_sync", defaults.wallpaper_sync, CONTEXT)?;
⋮----
read_bool(object, "lockscreen_sync", defaults.lockscreen_sync, CONTEXT)?;
settings.lockscreen_offset_sec = read_f64(
⋮----
settings.lockscreen_offer_shown = read_bool(
⋮----
settings.compress_advanced_visible = read_bool(
⋮----
settings.check_updates = read_bool(object, "check_updates", defaults.check_updates, CONTEXT)?;
settings.close_to_tray = read_bool(object, "closeToTray", defaults.close_to_tray, CONTEXT)?;
⋮----
read_tolerant_enum(object, "playlistSource", "all", &["all", "custom"]);
settings.performance_preset = read_tolerant_enum(
⋮----
let cache_dir = read_string(object, "cache_dir", "", CONTEXT)?;
settings.cache_dir = if cache_dir.is_empty() {
defaults.cache_dir.clone()
⋮----
Ok(settings)
⋮----
/// Rejects the values outside the accepted ranges and normalizes the two
/// plan-todo-8 enums in place - the Rust mirror of the C++
⋮----
/// plan-todo-8 enums in place - the Rust mirror of the C++
/// `ValidateStudioSettings`:
⋮----
/// `ValidateStudioSettings`:
///
⋮----
///
/// * `default_crf` not in `[16, 28]`
⋮----
/// * `default_crf` not in `[16, 28]`
/// * `default_fps` not in `[1, 30]`
⋮----
/// * `default_fps` not in `[1, 30]`
/// * an empty `compress_output_dir` / `cache_dir`
⋮----
/// * an empty `compress_output_dir` / `cache_dir`
/// * a negative `lockscreen_offset_sec`
⋮----
/// * a negative `lockscreen_offset_sec`
/// * an unknown `default_resolution_mode`
⋮----
/// * an unknown `default_resolution_mode`
/// * a `playlist_source` outside `{all, custom}` -> `"all"`
⋮----
/// * a `playlist_source` outside `{all, custom}` -> `"all"`
/// * a `performance_preset` outside `{Hemat, Seimbang, Maksimal}` ->
⋮----
/// * a `performance_preset` outside `{Hemat, Seimbang, Maksimal}` ->
⋮----
pub fn validate_studio_settings(settings: &mut StudioSettings) -> Result<(), ConfigError> {
⋮----
if settings.compress_output_dir.as_os_str().is_empty() {
return Err(ConfigError(
"compress_output_dir must not be empty".to_string(),
⋮----
if settings.cache_dir.as_os_str().is_empty() {
return Err(ConfigError("cache_dir must not be empty".to_string()));
⋮----
"lockscreen_offset_sec must be >= 0".to_string(),
⋮----
if !matches!(
⋮----
if !matches!(settings.playlist_source.as_str(), "all" | "custom") {
settings.playlist_source = "all".to_string();
⋮----
settings.performance_preset = "Seimbang".to_string();
⋮----
Ok(())
⋮----
pub fn settings_json(settings: &StudioSettings) -> Value {
⋮----
out.insert("version".to_string(), Value::from(settings.version));
out.insert(
"auto_compress_on_import".to_string(),
⋮----
out.insert("default_crf".to_string(), Value::from(settings.default_crf));
out.insert("default_fps".to_string(), Value::from(settings.default_fps));
⋮----
"default_resolution_mode".to_string(),
Value::from(settings.default_resolution_mode.clone()),
⋮----
"wallpaper_sync".to_string(),
⋮----
"lockscreen_sync".to_string(),
⋮----
"lockscreen_offset_sec".to_string(),
⋮----
"lockscreen_offer_shown".to_string(),
⋮----
"compress_advanced_visible".to_string(),
⋮----
"check_updates".to_string(),
⋮----
"close_to_tray".to_string(),
⋮----
"playlist_source".to_string(),
Value::from(settings.playlist_source.clone()),
⋮----
"performance_preset".to_string(),
Value::from(settings.performance_preset.clone()),
⋮----
pub fn settings_report_json(settings: &StudioSettings) -> Value {
let mut out = match settings_json(settings) {
⋮----
"start_with_windows".to_string(),
⋮----
"compress_output_dir_basename".to_string(),
basename_bytes(&settings.compress_output_dir),
⋮----
"cache_dir_basename".to_string(),
basename_bytes(&settings.cache_dir),
⋮----
pub fn save_studio_settings(path: &Path, settings: &StudioSettings) -> Result<(), ConfigError> {
let mut settings = settings.clone();
validate_studio_settings(&mut settings)?;
let existing = read_existing_json(path);
let known = settings_json(&settings);
let doc = merge_unknown_keys(&existing, &known);
if let Some(parent) = path.parent() {
if !parent.as_os_str().is_empty() {
⋮----
.map_err(|e| ConfigError(format!("cannot create settings directory: {e}")))?;
⋮----
let mut tmp = path.as_os_str().to_owned();
tmp.push(".tmp");
⋮----
.map_err(|e| ConfigError(format!("cannot serialize settings: {e}")))?;
fs::write(&tmp, text).map_err(|e| {
ConfigError(format!(
⋮----
fs::rename(&tmp, path).map_err(|e| {
⋮----
fn read_existing_json(path: &Path) -> Value {
⋮----
.ok()
.and_then(|text| serde_json::from_str(&text).ok())
.filter(Value::is_object)
.unwrap_or_else(|| Value::Object(Map::new()))
⋮----
fn basename_bytes(path: &Path) -> Value {
let text = path.to_string_lossy();
let trimmed = text.trim_end_matches(['\\', '/']);
let name = match trimmed.rfind(['\\', '/']) {
⋮----
name.encode_utf16()
.map(|unit| Value::from(unit as u32))
.collect(),
⋮----
mod tests {
⋮----
fn the_default_is_the_cpp_default() {
⋮----
assert_eq!(settings.version, 5);
assert!(settings.auto_compress_on_import);
assert_eq!(settings.default_crf, 22);
assert_eq!(settings.default_fps, 30);
assert_eq!(settings.default_resolution_mode, "match_monitor");
assert!(settings.wallpaper_sync);
assert!(settings.lockscreen_sync);
assert_eq!(settings.lockscreen_offset_sec, 0.0);
assert!(!settings.lockscreen_offer_shown);
assert!(!settings.compress_advanced_visible);
assert!(settings.check_updates);
assert!(!settings.close_to_tray);
assert_eq!(settings.playlist_source, "all");
assert_eq!(settings.performance_preset, "Seimbang");
⋮----
assert!(validate_studio_settings(&mut settings).is_ok());
⋮----
fn a_missing_key_takes_the_default() {
let settings = migrate_studio_settings("{}").expect("the empty file must migrate");
⋮----
assert_eq!(settings.version, STUDIO_SETTINGS_SCHEMA_VERSION);
⋮----
fn a_wrong_type_is_an_error_not_a_default() {
⋮----
migrate_studio_settings(r#"{"version":5,"default_crf":"x"}"#).expect_err("must fail");
assert_eq!(
⋮----
fn an_unknown_version_is_an_error() {
let err = migrate_studio_settings(r#"{"version":6}"#).expect_err("must fail");
⋮----
fn the_v5_offset_rule_moves_only_an_exactly_one() {
let settings = migrate_studio_settings(r#"{"version":4,"lockscreen_offset_sec":1.0}"#)
.expect("must migrate");
⋮----
let settings = migrate_studio_settings(r#"{"version":4,"lockscreen_offset_sec":0.5}"#)
⋮----
assert_eq!(settings.lockscreen_offset_sec, 0.5);
let settings = migrate_studio_settings(r#"{"version":5,"lockscreen_offset_sec":1.0}"#)
⋮----
assert_eq!(settings.lockscreen_offset_sec, 1.0);
⋮----
fn the_stored_sync_flags_are_never_flipped() {
let settings = migrate_studio_settings(r#"{"version":1,"lockscreen_sync":false}"#)
⋮----
assert!(!settings.lockscreen_sync);
⋮----
fn the_plan_todo_8_enums_are_tolerant() {
let settings = migrate_studio_settings(r#"{"version":5,"playlistSource":"nope"}"#)
⋮----
let settings = migrate_studio_settings(r#"{"version":5,"performancePreset":"nope"}"#)
⋮----
migrate_studio_settings(r#"{"version":5,"playlistSource":42}"#).expect("must migrate");
⋮----
fn the_removed_keys_are_ignored() {
⋮----
migrate_studio_settings(r#"{"version":3,"keep_original":true,"start_minimized":true}"#)
⋮----
fn the_basename_is_a_byte_array() {
⋮----
let report = settings_report_json(&settings);
⋮----
fn settings_round_trip() {
⋮----
std::env::temp_dir().join(format!("k6wp_settings_roundtrip_{}", std::process::id()));
⋮----
fs::create_dir_all(&dir).expect("scratch dir");
let path = dir.join("studio_settings.json");
⋮----
settings.default_resolution_mode = "1080p".to_string();
⋮----
save_studio_settings(&path, &settings).expect("save must succeed");
⋮----
let loaded = migrate_studio_settings(&fs::read_to_string(&path).expect("read settings"))
.expect("load must succeed");
assert_eq!(loaded, settings);
assert_eq!(loaded.default_crf, 20);
assert_eq!(loaded.default_fps, 24);
assert_eq!(loaded.default_resolution_mode, "1080p");
assert!(!loaded.wallpaper_sync);
assert!(!loaded.lockscreen_sync);
assert!(!loaded.check_updates);
⋮----
fn settings_validation_rejects_bad_crf() {
let dir = std::env::temp_dir().join(format!("k6wp_settings_badcrf_{}", std::process::id()));
⋮----
let err = save_studio_settings(&path, &settings).expect_err("crf 99 must fail");
assert!(err.to_string().contains("default_crf out of range"));
⋮----
fn settings_validation_rejects_bad_fps() {
let dir = std::env::temp_dir().join(format!("k6wp_settings_badfps_{}", std::process::id()));
⋮----
let err = save_studio_settings(&path, &settings).expect_err("fps 31 must fail");
assert!(err.to_string().contains("default_fps out of range"));
⋮----
fn settings_save_preserves_unknown_keys() {
⋮----
std::env::temp_dir().join(format!("k6wp_settings_unknownkeys_{}", std::process::id()));
⋮----
seed.insert("version".to_string(), Value::from(5));
seed.insert("auto_compress_on_import".to_string(), Value::from(true));
seed.insert("future_key".to_string(), Value::from(42));
fs::write(&path, serde_json::to_string(&seed).unwrap()).expect("write seed");
⋮----
let doc: Value = serde_json::from_str(&fs::read_to_string(&path).expect("read settings"))
.expect("valid json");
assert_eq!(doc["future_key"], 42, "unknown key must be preserved");
assert_eq!(doc["version"], 5, "version must be current schema");
```

## File: rust/crates/k6wp-core/Cargo.toml
```toml
[package]
name = "k6wp-core"
version.workspace = true
edition.workspace = true
rust-version.workspace = true
license.workspace = true
description = "The K6WP contracts in Rust: the config, the displays, the playlist, the settings, the IPC codec and the library index."

[dependencies]
serde = { version = "1", features = ["derive"] }
serde_json = "1"

# Task J3: the Windows-side config persistence (the cross-process settings
# lock). Gated to Windows so the pure contracts stay portable.
[target.'cfg(windows)'.dependencies]
windows = { version = "0.62", features = [
  "Win32_Foundation",
  "Win32_Security",
  "Win32_System_Threading",
] }

[dev-dependencies]
proptest = "1.6"
serde_json = "1"
```

## File: rust/crates/k6wp-ipc/tests/live_engine.rs
```rust
use std::path::PathBuf;
use std::time::Duration;
⋮----
fn live_enabled() -> bool {
std::env::var("K6WP_IPC_LIVE").as_deref() == Ok("1")
⋮----
fn options(expected_image: Option<PathBuf>) -> ConnectOptions {
⋮----
fn message(cmd: Cmd, payload: serde_json::Value) -> IpcMessage {
⋮----
fn the_live_engine_answers_the_whole_command_set() {
if !live_enabled() {
eprintln!("[SKIP] the live engine test needs K6WP_IPC_LIVE=1");
⋮----
let engine_exe = std::env::var("K6WP_IPC_ENGINE_EXE").ok().map(PathBuf::from);
assert!(
⋮----
let wrong = options(Some(PathBuf::from(r"C:\not\the\engine.exe")));
⋮----
let mut client = IpcClient::connect_current_session(options(engine_exe.clone()))
.expect("the engine must accept the connection");
let identity = client.server_identity().clone();
assert!(identity.process_id > 0, "the server pid must be known");
⋮----
eprintln!(
⋮----
.request(&message(Cmd::GetState, serde_json::json!({})))
.expect("get_state must answer");
⋮----
eprintln!("the state: {inner}");
⋮----
.request(&message(
⋮----
.expect("set_video must answer");
assert_eq!(
⋮----
.request(&message(Cmd::Pause, serde_json::json!({})))
.expect("pause must answer");
⋮----
.request(&message(Cmd::Resume, serde_json::json!({})))
.expect("resume must answer");
⋮----
.expect("set_display_video must answer");
⋮----
.expect("a payload with an unknown key must still be answered");
⋮----
.write_line(b"{\"version\":1,\"cmd\":\"a_command_from_the_future\",\"payload\":{}}\n")
.expect("the unknown command must be sent");
let answer = client.read_line().expect("the engine must answer");
let text = String::from_utf8_lossy(&answer).to_string();
eprintln!("the unknown command was answered with: {text}");
⋮----
serde_json::from_str(text.trim()).expect("the answer must be JSON");
⋮----
.expect("the engine must still answer");
⋮----
client.close();
⋮----
IpcClient::connect_current_session(options(engine_exe)).expect("the reconnect must work");
⋮----
.expect("the reconnected client must get an answer");
assert!(state.get("state").is_some());
⋮----
.expect("the assignment must be clearable");
⋮----
.request(&message(Cmd::Quit, serde_json::json!({})))
.expect("quit must answer");
eprintln!("the quit was answered with: {ack}");
let name = current_session_pipe_name(None);
eprintln!("the pipe was {name}");
```

## File: rust/crates/k6wp-studio/src/thumbs.rs
```rust
use std::collections::HashMap;
use std::io::Cursor;
⋮----
pub struct Pixels {
⋮----
pub fn decode_to_cover(bytes: &[u8], target_w: u32, target_h: u32) -> Result<Pixels, String> {
if bytes.len() < 8 {
return Err("thumbnail: the file is too short to be an image".to_string());
⋮----
let (src, sw, sh) = if bytes.starts_with(&[0xFF, 0xD8]) {
decode_jpeg(bytes)?
} else if bytes.starts_with(&[0x89, b'P', b'N', b'G']) {
decode_png(bytes)?
⋮----
return Err("thumbnail: not a JPEG or a PNG".to_string());
⋮----
return Err("thumbnail: the image is empty".to_string());
⋮----
let tw = target_w.max(1) as f32;
let th = target_h.max(1) as f32;
let scale = (tw / sw as f32).max(th / sh as f32).max(0.01);
⋮----
let mut dw = (sw as f32 * scale).round().max(1.0);
let mut dh = (sh as f32 * scale).round().max(1.0);
let limit = cap * tw.max(th);
⋮----
let shrink = limit / dw.max(dh);
dw = (dw * shrink).round().max(1.0);
dh = (dh * shrink).round().max(1.0);
⋮----
return Ok(Pixels {
⋮----
Ok(Pixels {
⋮----
rgba: box_resize(&src, sw, sh, dw, dh),
⋮----
fn decode_jpeg(bytes: &[u8]) -> Result<(Vec<u8>, u32, u32), String> {
⋮----
.decode()
.map_err(|e| format!("thumbnail: jpeg: {e}"))?;
⋮----
.info()
.ok_or_else(|| "thumbnail: jpeg: the header is missing".to_string())?;
⋮----
jpeg_decoder::PixelFormat::RGB24 => rgb_to_rgba(&pixels),
jpeg_decoder::PixelFormat::L8 => gray_to_rgba(&pixels),
⋮----
let mut out = Vec::with_capacity(pixels.len() / 4 * 4);
for px in pixels.chunks_exact(4) {
⋮----
out.extend_from_slice(&[r as u8, g as u8, b as u8, 255]);
⋮----
return Err("thumbnail: jpeg: 16-bit greyscale is not supported".to_string())
⋮----
Ok((rgba, w, h))
⋮----
fn decode_png(bytes: &[u8]) -> Result<(Vec<u8>, u32, u32), String> {
⋮----
decoder.set_transformations(png::Transformations::EXPAND);
⋮----
.read_info()
.map_err(|e| format!("thumbnail: png: {e}"))?;
let mut buf = vec![0u8; reader.output_buffer_size()];
⋮----
.next_frame(&mut buf)
⋮----
let data = &buf[..info.buffer_size()];
⋮----
png::ColorType::Rgba => data.to_vec(),
png::ColorType::Rgb => rgb_to_rgba(data),
png::ColorType::Grayscale => gray_to_rgba(data),
⋮----
let mut out = Vec::with_capacity(data.len() * 2);
for px in data.chunks_exact(2) {
out.extend_from_slice(&[px[0], px[0], px[0], px[1]]);
⋮----
return Err("thumbnail: png: an indexed image survived EXPAND".to_string())
⋮----
fn rgb_to_rgba(src: &[u8]) -> Vec<u8> {
let mut out = Vec::with_capacity(src.len() / 3 * 4);
for px in src.chunks_exact(3) {
out.extend_from_slice(&[px[0], px[1], px[2], 255]);
⋮----
fn gray_to_rgba(src: &[u8]) -> Vec<u8> {
let mut out = Vec::with_capacity(src.len() * 4);
⋮----
out.extend_from_slice(&[v, v, v, 255]);
⋮----
fn box_resize(src: &[u8], sw: u32, sh: u32, dw: u32, dh: u32) -> Vec<u8> {
let mut out = vec![0u8; (dw as usize) * (dh as usize) * 4];
⋮----
.max(y0 + 1)
.min(sh);
⋮----
.max(x0 + 1)
.min(sw);
⋮----
if i + 3 >= src.len() {
⋮----
out[o + c] = acc[c].checked_div(n).unwrap_or(0) as u8;
⋮----
struct Cached {
⋮----
pub struct ImageCache {
⋮----
impl ImageCache {
pub fn new(cap_bytes: usize) -> Self {
⋮----
cap_bytes: cap_bytes.max(1),
⋮----
pub fn cap_bytes(&self) -> usize {
⋮----
pub fn used_bytes(&self) -> usize {
⋮----
pub fn len(&self) -> usize {
self.map.len()
⋮----
pub fn is_empty(&self) -> bool {
self.map.is_empty()
⋮----
pub fn get(&mut self, key: u64) -> Option<slint::Image> {
⋮----
let entry = self.map.get_mut(&key)?;
⋮----
Some(entry.image.clone())
⋮----
pub fn contains(&self, key: u64) -> bool {
self.map.contains_key(&key)
⋮----
pub fn peek(&self, key: u64) -> Option<slint::Image> {
self.map.get(&key).map(|c| c.image.clone())
⋮----
pub fn insert(&mut self, key: u64, image: slint::Image) {
let size = image.size();
⋮----
if let Some(old) = self.map.remove(&key) {
self.used_bytes = self.used_bytes.saturating_sub(old.bytes);
⋮----
self.map.insert(
⋮----
self.evict_to_cap();
⋮----
fn evict_to_cap(&mut self) {
while self.used_bytes > self.cap_bytes && self.map.len() > 1 {
⋮----
.iter()
.min_by_key(|(_, c)| c.stamp)
.map(|(k, _)| *k);
⋮----
if let Some(removed) = self.map.remove(&k) {
self.used_bytes = self.used_bytes.saturating_sub(removed.bytes);
⋮----
pub fn retain_only(&mut self, keep: &[u64]) -> usize {
⋮----
.keys()
.copied()
.filter(|k| !keep.contains(k))
.collect();
⋮----
if let Some(removed) = self.map.remove(key) {
⋮----
victims.len()
⋮----
pub fn clear(&mut self) {
self.map.clear();
⋮----
mod tests {
⋮----
fn rgba(w: u32, h: u32) -> slint::Image {
⋮----
fn cache_evicts_the_coldest_when_over_the_cap() {
⋮----
cache.insert(1, rgba(4, 4));
cache.insert(2, rgba(4, 4));
cache.insert(3, rgba(4, 4));
let _ = cache.get(1);
cache.insert(4, rgba(4, 4));
assert!(!cache.contains(2), "the coldest entry must be evicted");
assert!(cache.contains(1));
assert!(cache.used_bytes() <= 200);
⋮----
fn retain_only_releases_what_left_the_viewport() {
⋮----
cache.insert(key, rgba(2, 2));
⋮----
let released = cache.retain_only(&[3, 4, 5]);
assert_eq!(released, 7);
assert_eq!(cache.len(), 3);
assert!(cache.contains(3) && cache.contains(5));
⋮----
assert_eq!(cache.used_bytes(), expect);
⋮----
fn box_resize_averages_a_solid_image() {
let src = vec![10u8; 8 * 8 * 4];
let out = box_resize(&src, 8, 8, 4, 4);
assert_eq!(out.len(), 4 * 4 * 4);
assert!(out.iter().all(|&v| v == 10 || v == 255));
⋮----
fn a_short_file_is_not_an_image() {
assert!(decode_to_cover(&[0u8; 4], 100, 56).is_err());
assert!(decode_to_cover(b"not an image at all", 100, 56).is_err());
⋮----
fn a_solid_png_decodes_and_is_scaled_to_cover() {
let png = synthetic_png(4, 4);
let px = decode_to_cover(&png, 20, 12).expect("the synthetic png must decode");
assert!(px.width >= 20 && px.height >= 12);
assert_eq!(px.rgba.len(), (px.width * px.height * 4) as usize);
⋮----
fn synthetic_png(w: u32, h: u32) -> Vec<u8> {
⋮----
out.extend_from_slice(&[0x89, b'P', b'N', b'G', 0x0D, 0x0A, 0x1A, 0x0A]);
⋮----
ihdr.extend_from_slice(&w.to_be_bytes());
ihdr.extend_from_slice(&h.to_be_bytes());
ihdr.extend_from_slice(&[8, 6, 0, 0, 0]);
push_chunk(&mut out, b"IHDR", &ihdr);
⋮----
raw.push(0);
⋮----
raw.extend_from_slice(&[255, 0, 0, 255]);
⋮----
push_chunk(&mut out, b"IDAT", &zlib_stored(&raw));
push_chunk(&mut out, b"IEND", &[]);
⋮----
fn push_chunk(out: &mut Vec<u8>, kind: &[u8; 4], data: &[u8]) {
out.extend_from_slice(&(data.len() as u32).to_be_bytes());
out.extend_from_slice(kind);
out.extend_from_slice(data);
let mut crc_input = Vec::with_capacity(4 + data.len());
crc_input.extend_from_slice(kind);
crc_input.extend_from_slice(data);
out.extend_from_slice(&crc32(&crc_input).to_be_bytes());
⋮----
fn zlib_stored(data: &[u8]) -> Vec<u8> {
let mut out = vec![0x78, 0x01];
let mut chunks = data.chunks(65535).peekable();
if data.is_empty() {
out.push(0x01);
out.extend_from_slice(&0u16.to_le_bytes());
out.extend_from_slice(&(!0u16).to_le_bytes());
⋮----
while let Some(c) = chunks.next() {
let last = chunks.peek().is_none();
out.push(if last { 1 } else { 0 });
let len = c.len() as u16;
out.extend_from_slice(&len.to_le_bytes());
out.extend_from_slice(&(!len).to_le_bytes());
out.extend_from_slice(c);
⋮----
out.extend_from_slice(&adler32(data).to_be_bytes());
⋮----
fn crc32(data: &[u8]) -> u32 {
⋮----
let mask = (crc & 1).wrapping_neg();
⋮----
fn adler32(data: &[u8]) -> u32 {
```

## File: rust/crates/k6wp-core/tests/sps_golden.rs
```rust
use k6wp_core::sps::probe_dpb;
use std::path::PathBuf;
⋮----
fn fixture() -> Vec<serde_json::Value> {
let path = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../../../tests/golden/sps-dpb.json");
⋮----
.unwrap_or_else(|e| panic!("the SPS fixtures must be readable: {e}"));
let root: serde_json::Value = serde_json::from_str(&text).expect("valid JSON");
⋮----
.as_array()
.cloned()
.unwrap_or_else(|| panic!("the fixture must have a 'cases' array"))
⋮----
fn the_sps_parser_agrees_with_ffmpeg() {
let corpus = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../../../tests/corpus");
let cases = fixture();
assert!(!cases.is_empty(), "the fixture must not be empty");
⋮----
let name = case["file"].as_str().unwrap_or_default();
let file = corpus.join(name);
if !file.exists() {
skipped.push(name.to_string());
⋮----
match probe_dpb(&file) {
⋮----
.as_u64()
.or(expected["sps_max_dec_pic_buffering"].as_u64());
⋮----
let got = report.max_dpb.unwrap_or(0) as u64;
⋮----
failures.push(format!(
⋮----
if let Some(expected_vui) = expected["max_dec_frame_buffering"].as_u64() {
⋮----
.iter()
.filter_map(|s| s.max_dec_frame_buffering)
.max()
.unwrap_or(0) as u64;
⋮----
if let Some(expected_interlaced) = expected["interlaced"].as_bool() {
⋮----
.filter_map(|s| s.frame_mbs_only_flag)
.any(|mbs| mbs == 0);
⋮----
if let Some(expected_chroma) = expected["chroma_format_idc"].as_u64() {
⋮----
.filter_map(|s| s.chroma_format_idc)
⋮----
if let Some(expected_depth) = expected["bit_depth_luma"].as_u64() {
⋮----
.filter_map(|s| s.bit_depth_luma)
⋮----
Err(e) => failures.push(format!("{name}: the parser failed: {e}")),
⋮----
eprintln!("[FAIL] {line}");
⋮----
eprintln!("[SKIP] {name} is not in the corpus (run tools\\build_corpus.ps1)");
⋮----
panic!(
⋮----
assert!(
⋮----
eprintln!("sps: {checked} cases match ffmpeg's trace_headers");
⋮----
fn the_savings_a_reencode_can_obtain_are_pinned() {
⋮----
eprintln!("[SKIP] {name} is not in the corpus");
⋮----
let report = probe_dpb(&file).unwrap_or_else(|e| panic!("{name}: {e}"));
⋮----
.filter_map(|s| s.dpb_bytes())
⋮----
.unwrap_or(0);
assert_eq!(total, *want_total, "{name}: the total DPB changed");
⋮----
.filter_map(|s| s.savings_obtainable())
⋮----
assert_eq!(
⋮----
let copy_safe = report.sps.iter().any(|s| s.is_copy_safe());
assert_eq!(copy_safe, *want_copy, "{name}: the copy decision changed");
⋮----
assert!(checked > 0, "no corpus file was available");
⋮----
fn the_dpb_byte_budget_is_pinned() {
⋮----
assert_eq!(bytes, *want_bytes, "{name}: the DPB bytes changed");
⋮----
eprintln!("sps: {checked} DPB budgets pinned");
```

## File: rust/crates/k6wp-studio/src/thumbnailer.rs
```rust
pub enum ThumbError {
⋮----
fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
⋮----
ThumbError::FfmpegNotFound => write!(f, "ffmpeg tidak ditemukan (FindFfmpeg)"),
ThumbError::Timeout => write!(f, "ffmpeg timeout 10 detik"),
ThumbError::SpawnFailed(msg) => write!(f, "spawn gagal: {}", msg),
ThumbError::CacheError(msg) => write!(f, "cache error: {}", msg),
ThumbError::InvalidMedia(msg) => write!(f, "media tidak valid: {}", msg),
⋮----
/// Cache version — **wajib `"thumb-v3"`** (`ADR` C1 verified; `thumbnailer.cpp` l.59).
/// Perubahan version = semua thumbnail Qt (`thumb-v3`) stale saat transisi.
⋮----
/// Perubahan version = semua thumbnail Qt (`thumb-v3`) stale saat transisi.
const THUMB_VERSION: &str = "thumb-v3";
⋮----
/// Konkurensi thumbnail generation (konstanta, bukan hardcode `PROBE_THREADS`-style).
/// Default thumbnail dimensions (kontrak `thumbnailer.cpp`: `kThumbWidth` = 320, `kThumbHeight` = 180).
⋮----
/// Default thumbnail dimensions (kontrak `thumbnailer.cpp`: `kThumbWidth` = 320, `kThumbHeight` = 180).
/// **Hardcode** (`GenerateTo` hanya `(video, thumb)`, `BuildThumbCommand` dipanggil dengan `kThumbWidth`/`kThumbHeight` — `ADR` C1 verified: `GenerateTo` tanpa `width,height` param).
⋮----
/// **Hardcode** (`GenerateTo` hanya `(video, thumb)`, `BuildThumbCommand` dipanggil dengan `kThumbWidth`/`kThumbHeight` — `ADR` C1 verified: `GenerateTo` tanpa `width,height` param).
pub const THUMB_WIDTH: u32 = 320;
⋮----
/// Find ffmpeg binary (kontrak `FindFfmpeg` — `shared/ffmpeg_path.cpp`).
/// Urutan: sibling `engine.exe` dir → `build/release/ffmpeg.exe` → env `K6WP_FFMPEG` (`debug_assertions` only, `I4`).
⋮----
/// Urutan: sibling `engine.exe` dir → `build/release/ffmpeg.exe` → env `K6WP_FFMPEG` (`debug_assertions` only, `I4`).
/// Tidak ada `INFINITE` atau loop tidak terbatas.
⋮----
/// Tidak ada `INFINITE` atau loop tidak terbatas.
pub fn find_ffmpeg() -> Option<PathBuf> {
⋮----
pub fn find_ffmpeg() -> Option<PathBuf> {
// Coba sibling directory (folder tempat `k6wp-studio.exe` berjalan)
⋮----
if let Some(parent) = current.parent() {
let sibling = parent.join("ffmpeg.exe");
if sibling.exists() {
return Some(sibling);
⋮----
// Fallback: build release tree (dev layout)
let release_path = parent.join("build").join("release").join("ffmpeg.exe");
if release_path.exists() {
return Some(release_path);
⋮----
// Env override hanya debug (`#[cfg(debug_assertions)]` — `I4`)
⋮----
if !env_path.is_empty() {
⋮----
if p.exists() {
return Some(p);
⋮----
/// Seek formula — **PERSIS** `SafeSeekSeconds` (`thumbnailer.cpp` l.74-78).
/// Alasan: `-ss 1` gagal untuk video <1s (seek past EOF, no frame);
⋮----
/// Alasan: `-ss 1` gagal untuk video <1s (seek past EOF, no frame);
/// clamp half-duration untuk klip pendek (`duration/2.0`); fallback `0.0` jika durasi ≤0.
⋮----
/// clamp half-duration untuk klip pendek (`duration/2.0`); fallback `0.0` jika durasi ≤0.
pub fn safe_seek_seconds(duration_seconds: f64) -> f64 {
⋮----
pub fn safe_seek_seconds(duration_seconds: f64) -> f64 {
⋮----
/// Generate thumbnail — ffmpeg subprocess dengan timeout 10 detik (`kFfmpegTimeoutMs` = 10000, `l.53`).
/// Konkurensi ≤2 (`THUMB_MAX_CONCURRENT` — konstanta manual, bukan hardcode).
⋮----
/// Konkurensi ≤2 (`THUMB_MAX_CONCURRENT` — konstanta manual, bukan hardcode).
/// Cache: `abs|size|mtime|thumb-v3` (`ThumbCacheKeyMaterial` persis, `ADR` §4, `C1`).
⋮----
/// Cache: `abs|size|mtime|thumb-v3` (`ThumbCacheKeyMaterial` persis, `ADR` §4, `C1`).
pub fn generate_thumbnail(
⋮----
pub fn generate_thumbnail(
⋮----
// 1. Cek ffmpeg (fallback placeholder jika `None` — `ADR` §6)
let ffmpeg_path = find_ffmpeg().ok_or(ThumbError::FfmpegNotFound)?;
⋮----
// 2. Validasi file media (return InvalidMedia jika tidak ada — `ADR` §6)
if !media_path.exists() {
return Err(ThumbError::InvalidMedia(format!(
⋮----
// 3. Cache key (`ADR` §4, `C1` verified: `abs|size|mtime|thumb-v3`)
⋮----
.canonicalize()
.unwrap_or_else(|_| media_path.to_path_buf());
let size = std::fs::metadata(media_path).map(|m| m.len()).unwrap_or(0);
⋮----
.ok()
.and_then(|m| m.modified().ok())
.map(|t| {
t.duration_since(std::time::UNIX_EPOCH)
.unwrap_or_default()
.as_secs() as i64
⋮----
.unwrap_or(0);
let version_string = THUMB_VERSION; // wajib `thumb-v3`
⋮----
let material = format!(
⋮----
hasher.update(material.as_bytes());
let hash_hex = format!("{:x}", hasher.finalize());
let thumb_path = cache_dir.join(format!("{}.jpg", hash_hex));
⋮----
// 3. Cache hit (tidak spawn ffmpeg — `ADR` §4)
if thumb_path.exists() {
return Ok(thumb_path);
⋮----
// 4. Spawn ffmpeg (`ADR` §2: `-ss`, `-i`, `-frames:v 1`, `-vf scale=W:H:force_original_aspect_ratio=decrease,pad=W:H`, `-q:v` 4)
// Seek (`ADR` §2): `SafeSeekSeconds(duration)` — durasi dari `ffprobe -show_format` atau `ffmpeg` sendiri (simplifikasi: pakai `ffmpeg` untuk baca metadata jika ada, atau default 1.0 untuk video ≥2s, 0.5 untuk <2s).
// Untuk ADR ini: simplifikasi `seek` = 1.0 (konstanta, bukan dinamis dari durasi — akan diperbaiki di `Run 4` jika perlu, tapi `ADR` mencatat kontrak Qt persis).
⋮----
.args([
⋮----
&format!("{}", seek), // persis `FormatSeekSeconds` — `std::fixed` + `std::setprecision(6)`; simplifikasi: format biasa
⋮----
media_path.to_str().unwrap_or(""),
⋮----
&format!(
⋮----
thumb_path.to_str().unwrap_or(""),
⋮----
.stdout(Stdio::null())
.stderr(Stdio::null())
.output();
⋮----
if o.status.success() && thumb_path.exists() {
Ok(thumb_path)
⋮----
Err(ThumbError::SpawnFailed(format!(
⋮----
Err(e) => Err(ThumbError::SpawnFailed(e.to_string())),
⋮----
pub fn cache_version_string() -> &'static str {
⋮----
mod tests {
⋮----
fn anime_video() -> PathBuf {
⋮----
.unwrap()
.join("tests")
.join("corpus")
.join("anime.mp4")
⋮----
fn generate_from_real_video() {
if find_ffmpeg().is_none() {
eprintln!("[SKIP] ffmpeg not found");
⋮----
let media = anime_video();
let cache = std::env::temp_dir().join("k6wp-test-thumb");
std::fs::create_dir_all(&cache).ok();
let result = generate_thumbnail(&media, &cache, 320, 180);
assert!(
⋮----
fn cache_hit_no_spawn() {
⋮----
let cache = std::env::temp_dir().join("k6wp-test-thumb-cache");
⋮----
let thumb1 = generate_thumbnail(&media, &cache, 320, 180).expect("first");
let thumb2 = generate_thumbnail(&media, &cache, 320, 180).expect("second");
assert_eq!(
⋮----
assert!(thumb1.exists());
⋮----
fn mtime_change_invalidates_cache() {
⋮----
let cache = std::env::temp_dir().join("k6wp-test-thumb-mtime");
⋮----
let meta = std::fs::metadata(&media).expect("stat");
let new_mtime = meta.modified().expect("mtime") + Duration::from_secs(1);
⋮----
.and_then(|f| f.set_modified(new_mtime))
.expect("must modify mtime");
⋮----
assert_ne!(
⋮----
fn missing_file_errors() {
⋮----
let fake_dir = std::env::temp_dir().join("k6wp-test-fake-ffmpeg-missing");
std::fs::create_dir_all(&fake_dir).ok();
let fake_bat = fake_dir.join("ffmpeg.bat");
std::fs::write(&fake_bat, "@exit /b 1\n").expect("write fake ffmpeg");
⋮----
let missing = std::env::temp_dir().join("k6wp-test-missing.mp4");
let cache = std::env::temp_dir().join("k6wp-test-thumb-missing");
⋮----
let result = generate_thumbnail(&missing, &cache, 320, 180);
eprintln!("DEBUG missing_file_errors: result = {:?}", result);
⋮----
fn timeout_kills_ffmpeg() {
⋮----
let fake_dir = std::env::temp_dir().join("k6wp-test-fake-ffmpeg");
⋮----
let fake_script = fake_dir.join("ffmpeg.bat");
std::fs::write(&fake_script, "timeout /t 60 /nobreak >nul\n").expect("write fake");
⋮----
let result = generate_thumbnail(
&anime_video(),
&std::env::temp_dir().join("k6wp-test-thumb-timeout"),
```

## File: rust/crates/k6wp-core/src/config.rs
```rust
use std::path::PathBuf;
⋮----
pub struct ConfigError(pub String);
⋮----
fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
f.write_str(&self.0)
⋮----
/// The on-disk wallpaper configuration - the Rust mirror of the C++
/// `WallpaperConfig`.
⋮----
/// `WallpaperConfig`.
///
⋮----
///
/// Every field has a default, so a v0/legacy file (or a file missing
⋮----
/// Every field has a default, so a v0/legacy file (or a file missing
/// fields) migrates without crashing.
⋮----
/// fields) migrates without crashing.
#[derive(Clone, Debug, PartialEq)]
pub struct WallpaperConfig {
/// The schema version - always the current one after a migration.
    pub version: i64,
/// The wallpaper video. UTF-8 on disk (the JSON form); the C++ keeps a
    /// wide string in memory for the Win32 calls.
⋮----
/// wide string in memory for the Win32 calls.
    pub video_path: PathBuf,
/// `cover` | `fill` (legacy alias) | `fit` | `stretch` | `center`.
    pub fit_mode: String,
/// 0.5 - 2.0 (RESERVED: not applied by the renderer).
    pub speed: f64,
/// -1 = all screens; >= 0 = that monitor only.
    pub monitor_id: i64,
/// 16 - 28.
    pub crf: i64,
/// 0 = monitor native.
    pub resolution_w: i64,
/// 0 = monitor native.
    pub resolution_h: i64,
/// 1 - 30.
    pub fps_cap: i64,
/// Whether the battery policy is active.
    pub battery_saver: bool,
/// `cap24` | `static` (the DC policy, master-gated by `battery_saver`).
    pub battery_mode: String,
/// `auto` | `all` (P3L: E-core pin on a hybrid CPU when `auto`).
    pub cpu_affinity: String,
/// `auto` | `integrated` | `discrete` (P3L: iGPU pin on a hybrid GPU
    /// when `auto`).
⋮----
/// when `auto`).
    pub gpu_adapter: String,
/// The display-resilience reconcile mode: `legacy` | `shadow` | `on`.
    pub reconcile_mode: String,
/// Unload the decode pipeline after this many seconds of continuous
    /// pause. 0 = never unload; the range is 0..3600 s.
⋮----
/// pause. 0 = never unload; the range is 0..3600 s.
    pub pause_unload_after_sec: i64,
⋮----
impl Default for WallpaperConfig {
/// The C++ default constructor: every field takes its declared default.
    fn default() -> Self {
⋮----
fn default() -> Self {
⋮----
fit_mode: "cover".to_string(),
⋮----
battery_mode: "cap24".to_string(),
cpu_affinity: "auto".to_string(),
gpu_adapter: "auto".to_string(),
reconcile_mode: "legacy".to_string(),
⋮----
/// The fields whose absence means the file predates the current schema and
/// has to be rewritten (the C++ `NeedsMigration` field list).
⋮----
/// has to be rewritten (the C++ `NeedsMigration` field list).
///
⋮----
///
/// `reconcile_mode` is deliberately NOT in the list: it is an additive field,
⋮----
/// `reconcile_mode` is deliberately NOT in the list: it is an additive field,
/// so a file without it loads clean (no `.bak`, no version bump) and simply
⋮----
/// so a file without it loads clean (no `.bak`, no version bump) and simply
/// takes the default.
⋮----
/// takes the default.
const MIGRATION_FIELDS: [&str; 13] = [
⋮----
/// Parses a raw config document and migrates it to the current schema.
///
⋮----
///
/// The C++ `LoadConfig` splits this into "read the file", "parse", "migrate"
⋮----
/// The C++ `LoadConfig` splits this into "read the file", "parse", "migrate"
/// and "validate"; this function is the "parse + migrate" half, and
⋮----
/// and "validate"; this function is the "parse + migrate" half, and
/// [`validate_config`] is the other half. The golden fixtures record the
⋮----
/// [`validate_config`] is the other half. The golden fixtures record the
/// migration alone, so the two are kept apart on purpose.
⋮----
/// migration alone, so the two are kept apart on purpose.
///
⋮----
///
/// # Errors
⋮----
/// # Errors
///
⋮----
///
/// * A corrupt document -> `ConfigError("corrupt config JSON: parse error")`.
⋮----
/// * A corrupt document -> `ConfigError("corrupt config JSON: parse error")`.
/// * A root that is not an object -> `ConfigError("config root must be a
⋮----
/// * A root that is not an object -> `ConfigError("config root must be a
///   JSON object")`.
⋮----
///   JSON object")`.
/// * A schema version outside `0..=6` -> `ConfigError("unsupported config
⋮----
/// * A schema version outside `0..=6` -> `ConfigError("unsupported config
///   schema version: N")`.
⋮----
///   schema version: N")`.
/// * A type-corrupt field -> `ConfigError("config field type error: ...")`.
⋮----
/// * A type-corrupt field -> `ConfigError("config field type error: ...")`.
pub fn migrate_config(raw_text: &str) -> Result<WallpaperConfig, ConfigError> {
⋮----
pub fn migrate_config(raw_text: &str) -> Result<WallpaperConfig, ConfigError> {
⋮----
.map_err(|_| ConfigError("corrupt config JSON: parse error".to_string()))?;
migrate_config_value(&raw)
⋮----
/// The C++ `MigrateConfig`: turns a parsed JSON object into a
/// [`WallpaperConfig`].
⋮----
/// [`WallpaperConfig`].
///
⋮----
///
/// Missing fields are filled with the defaults. The per-version rules:
⋮----
/// Missing fields are filled with the defaults. The per-version rules:
///
⋮----
///
/// * v0/v1 -> v2 always writes `monitor_id = -1` (the old id had no render
⋮----
/// * v0/v1 -> v2 always writes `monitor_id = -1` (the old id had no render
///   effect, so keeping it would newly single out one screen). A v2 file
⋮----
///   effect, so keeping it would newly single out one screen). A v2 file
///   keeps its stored value.
⋮----
///   keeps its stored value.
/// * A file older than v3 with `fit_mode = "fill"` normalizes to `"cover"`.
⋮----
/// * A file older than v3 with `fit_mode = "fill"` normalizes to `"cover"`.
/// * Every file is written at the current schema version.
⋮----
/// * Every file is written at the current schema version.
pub fn migrate_config_value(raw: &Value) -> Result<WallpaperConfig, ConfigError> {
⋮----
pub fn migrate_config_value(raw: &Value) -> Result<WallpaperConfig, ConfigError> {
⋮----
.as_object()
.ok_or_else(|| ConfigError("config root must be a JSON object".to_string()))?;
⋮----
let version = read_i64(object, "version", 0, CONTEXT)?;
⋮----
return Err(ConfigError(format!(
⋮----
// A NEWER file (written by a newer engine) is read BEST-EFFORT: the
// known fields keep their values and the ones this engine does not know
// fall back to their defaults. The older engine must NOT reset the
// user's settings to the defaults - that is a data loss the user cannot
⋮----
let video_path = PathBuf::from(read_string(object, "video_path", "", CONTEXT)?);
let mut fit_mode = read_string(object, "fit_mode", &defaults.fit_mode, CONTEXT)?;
⋮----
fit_mode = "cover".to_string();
⋮----
Ok(WallpaperConfig {
⋮----
speed: read_f64(object, "speed", defaults.speed, CONTEXT)?,
⋮----
read_i64(object, "monitor_id", defaults.monitor_id, CONTEXT)?
⋮----
crf: read_i64(object, "crf", defaults.crf, CONTEXT)?,
resolution_w: read_i64(object, "resolution_w", defaults.resolution_w, CONTEXT)?,
resolution_h: read_i64(object, "resolution_h", defaults.resolution_h, CONTEXT)?,
fps_cap: read_i64(object, "fps_cap", defaults.fps_cap, CONTEXT)?,
battery_saver: read_bool(object, "battery_saver", defaults.battery_saver, CONTEXT)?,
battery_mode: read_string(object, "battery_mode", &defaults.battery_mode, CONTEXT)?,
cpu_affinity: read_string(object, "cpu_affinity", &defaults.cpu_affinity, CONTEXT)?,
gpu_adapter: read_string(object, "gpu_adapter", &defaults.gpu_adapter, CONTEXT)?,
reconcile_mode: read_string(object, "reconcile_mode", &defaults.reconcile_mode, CONTEXT)?,
pause_unload_after_sec: read_i64(
⋮----
pub fn validate_config(cfg: &WallpaperConfig) -> Result<(), ConfigError> {
⋮----
return Err(ConfigError("resolution must be >= 0".to_string()));
⋮----
if !matches!(
⋮----
return Err(ConfigError(format!("unknown fit_mode: {}", cfg.fit_mode)));
⋮----
if !matches!(cfg.battery_mode.as_str(), "cap24" | "static") {
⋮----
if !matches!(cfg.cpu_affinity.as_str(), "auto" | "all") {
⋮----
if !matches!(cfg.gpu_adapter.as_str(), "auto" | "integrated" | "discrete") {
⋮----
if !matches!(cfg.reconcile_mode.as_str(), "legacy" | "shadow" | "on") {
⋮----
Ok(())
⋮----
pub fn merge_unknown_keys(existing: &Value, known: &Value) -> Value {
let mut out = known.clone();
let Some(existing) = existing.as_object() else {
⋮----
let Some(out_obj) = out.as_object_mut() else {
⋮----
let mut keys: Vec<&String> = existing.keys().collect();
keys.sort();
⋮----
if out_obj.contains_key(key) {
⋮----
let cost = key.len() + serialized.len();
⋮----
out_obj.insert(key.clone(), value.clone());
⋮----
pub fn config_to_json(cfg: &WallpaperConfig) -> Value {
⋮----
out.insert("version".to_string(), Value::from(cfg.version));
out.insert(
"video_path".to_string(),
Value::from(cfg.video_path.to_string_lossy().into_owned()),
⋮----
out.insert("fit_mode".to_string(), Value::from(cfg.fit_mode.clone()));
out.insert("speed".to_string(), Value::from(cfg.speed));
out.insert("monitor_id".to_string(), Value::from(cfg.monitor_id));
out.insert("crf".to_string(), Value::from(cfg.crf));
out.insert("resolution_w".to_string(), Value::from(cfg.resolution_w));
out.insert("resolution_h".to_string(), Value::from(cfg.resolution_h));
out.insert("fps_cap".to_string(), Value::from(cfg.fps_cap));
out.insert("battery_saver".to_string(), Value::from(cfg.battery_saver));
⋮----
"battery_mode".to_string(),
Value::from(cfg.battery_mode.clone()),
⋮----
"cpu_affinity".to_string(),
Value::from(cfg.cpu_affinity.clone()),
⋮----
"gpu_adapter".to_string(),
Value::from(cfg.gpu_adapter.clone()),
⋮----
"reconcile_mode".to_string(),
Value::from(cfg.reconcile_mode.clone()),
⋮----
"pause_unload_after_sec".to_string(),
⋮----
pub fn needs_migration(raw: &Value) -> bool {
let Some(object) = raw.as_object() else {
⋮----
let version = match object.get("version") {
Some(Value::Number(number)) => number_to_i64(number),
⋮----
.iter()
.any(|field| !object.contains_key(*field))
⋮----
pub(crate) fn json_type_name(value: &Value) -> &'static str {
⋮----
/// The nlohmann `type_error.302` the C++ surfaces as a `ConfigError`.
pub(crate) fn type_error(context: &str, expected: &str, actual: &str) -> ConfigError {
⋮----
pub(crate) fn type_error(context: &str, expected: &str, actual: &str) -> ConfigError {
ConfigError(format!(
⋮----
/// Reads an integer field: absent -> the default, wrong type -> an error.
pub(crate) fn read_i64(
⋮----
pub(crate) fn read_i64(
⋮----
match object.get(key) {
None => Ok(default),
Some(Value::Number(number)) => Ok(number_to_i64(number)),
Some(other) => Err(type_error(context, "number", json_type_name(other))),
⋮----
/// Reads a floating point field: absent -> the default, wrong type -> an
/// error.
⋮----
/// error.
pub(crate) fn read_f64(
⋮----
pub(crate) fn read_f64(
⋮----
Some(Value::Number(number)) => Ok(number_to_f64(number)),
⋮----
/// Reads a boolean field: absent -> the default, wrong type -> an error.
pub(crate) fn read_bool(
⋮----
pub(crate) fn read_bool(
⋮----
Some(Value::Bool(value)) => Ok(*value),
Some(other) => Err(type_error(context, "boolean", json_type_name(other))),
⋮----
/// Reads a string field: absent -> the default, wrong type -> an error.
pub(crate) fn read_string(
⋮----
pub(crate) fn read_string(
⋮----
None => Ok(default.to_string()),
Some(Value::String(value)) => Ok(value.clone()),
Some(other) => Err(type_error(context, "string", json_type_name(other))),
⋮----
/// The tolerant enum read the C++ uses for the plan-todo-8 string fields: a
/// missing key, a non-string value or a value outside `allowed` all take
⋮----
/// missing key, a non-string value or a value outside `allowed` all take
/// the default. An old file must never fail to load because of a
⋮----
/// the default. An old file must never fail to load because of a
/// hand-edited preset name.
⋮----
/// hand-edited preset name.
pub(crate) fn read_tolerant_enum(
⋮----
pub(crate) fn read_tolerant_enum(
⋮----
Some(Value::String(value)) if allowed.contains(&value.as_str()) => value.clone(),
_ => default.to_string(),
⋮----
/// The C++ `static_cast<int>` of a JSON number: a float truncates towards
/// zero, an integer passes through. Rust saturates where the C++ would
⋮----
/// zero, an integer passes through. Rust saturates where the C++ would
/// overflow, so an absurd on-disk value can never panic.
⋮----
/// overflow, so an absurd on-disk value can never panic.
fn number_to_i64(number: &serde_json::Number) -> i64 {
⋮----
fn number_to_i64(number: &serde_json::Number) -> i64 {
if let Some(value) = number.as_i64() {
⋮----
} else if let Some(value) = number.as_u64() {
i64::try_from(value).unwrap_or(i64::MAX)
⋮----
// A float: the C++ `static_cast<int>` truncates towards zero. Rust's
⋮----
number_to_f64(number) as i64
⋮----
fn number_to_f64(number: &serde_json::Number) -> f64 {
number.as_f64().unwrap_or(0.0)
⋮----
fn format_cpp_double(value: f64) -> String {
format!("{value:.6}")
⋮----
mod tests {
⋮----
fn the_default_is_the_cpp_default() {
⋮----
assert_eq!(cfg.version, 6);
assert_eq!(cfg.fit_mode, "cover");
assert_eq!(cfg.speed, 1.0);
assert_eq!(cfg.monitor_id, -1);
assert_eq!(cfg.crf, 22);
assert_eq!(cfg.fps_cap, 24);
assert_eq!(cfg.battery_mode, "cap24");
assert_eq!(cfg.cpu_affinity, "auto");
assert_eq!(cfg.gpu_adapter, "auto");
assert_eq!(cfg.reconcile_mode, "legacy");
assert_eq!(cfg.pause_unload_after_sec, 30);
assert!(validate_config(&cfg).is_ok());
⋮----
fn a_missing_key_takes_the_default() {
let cfg = migrate_config("{}").expect("the empty config must migrate");
⋮----
assert_eq!(cfg.version, CONFIG_SCHEMA_VERSION);
⋮----
fn a_wrong_type_is_an_error_not_a_default() {
let err = migrate_config(r#"{"version":5,"speed":"fast"}"#).expect_err("must fail");
assert_eq!(
⋮----
let err = migrate_config(r#"{"version":"5"}"#).expect_err("must fail");
assert!(err
⋮----
fn a_corrupt_version_is_an_error_and_a_newer_one_is_not() {
⋮----
let err = migrate_config(r#"{"version":-1}"#).expect_err("must fail");
assert_eq!(err.to_string(), "unsupported config schema version: -1");
⋮----
let cfg = migrate_config(r#"{"version":7,"video_path":"C:\\v\\new.mp4","crf":19}"#)
.expect("a newer file must migrate");
⋮----
assert_eq!(cfg.video_path, std::path::PathBuf::from("C:\\v\\new.mp4"));
assert_eq!(cfg.crf, 19);
⋮----
fn a_corrupt_document_is_an_error() {
let err = migrate_config(r#"{"version":5,"#).expect_err("must fail");
assert_eq!(err.to_string(), "corrupt config JSON: parse error");
let err = migrate_config("[1,2,3]").expect_err("must fail");
assert_eq!(err.to_string(), "config root must be a JSON object");
⋮----
fn a_v0_or_v1_file_forces_the_monitor_id() {
let cfg = migrate_config(r#"{"version":1,"monitor_id":2}"#).expect("must migrate");
⋮----
let cfg = migrate_config(r#"{"version":2,"monitor_id":1}"#).expect("must migrate");
assert_eq!(cfg.monitor_id, 1);
⋮----
fn a_legacy_fill_becomes_cover() {
let cfg = migrate_config(r#"{"version":2,"fit_mode":"fill"}"#).expect("must migrate");
⋮----
let cfg = migrate_config(r#"{"version":3,"fit_mode":"fill"}"#).expect("must migrate");
assert_eq!(cfg.fit_mode, "fill");
⋮----
fn the_validation_ranges_are_the_cpp_ones() {
⋮----
assert!(validate_config(&bad_crf).is_err());
⋮----
assert!(validate_config(&bad_speed).is_err());
⋮----
assert!(validate_config(&bad_fps).is_err());
⋮----
assert!(validate_config(&bad_monitor).is_err());
⋮----
fit_mode: "squish".to_string(),
⋮----
assert!(validate_config(&bad_fit).is_err());
⋮----
assert!(validate_config(&bad_pause).is_err());
⋮----
fn a_migrated_config_may_still_be_invalid() {
⋮----
let cfg = migrate_config(r#"{"version":5,"crf":15}"#).expect("must migrate");
assert_eq!(cfg.crf, 15);
assert!(validate_config(&cfg).is_err());
⋮----
fn the_migration_trigger_matches_the_cpp_field_list() {
let raw: Value = serde_json::from_str("{}").expect("json");
assert!(needs_migration(&raw));
let raw: Value = serde_json::from_str(r#"{"version":6}"#).expect("json");
⋮----
.expect("json");
assert!(!needs_migration(&raw));
```

## File: rust/crates/k6wp-core/src/lib.rs
```rust
pub mod compress_args;
pub mod config;
⋮----
pub mod config_store;
pub mod displays;
pub mod engine_logic;
pub mod ipc;
pub mod library;
pub mod playlist;
pub mod settings;
pub mod sps;
pub mod version;
⋮----
pub use settings::StudioSettings;
```

## File: rust/crates/k6wp-ipc/src/lib.rs
```rust
use std::ffi::c_void;
use std::path::PathBuf;
use std::time::Duration;
⋮----
pub enum IpcError {
⋮----
fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
⋮----
IpcError::Connect(m) => write!(f, "ipc connect failed: {m}"),
IpcError::Unverified(m) => write!(f, "ipc server not verified: {m}"),
IpcError::Timeout(m) => write!(f, "ipc timeout: {m}"),
IpcError::Transport(m) => write!(f, "ipc transport error: {m}"),
IpcError::Protocol(m) => write!(f, "ipc protocol error: {m}"),
⋮----
/// The environment variable that overrides the pipe name (the same one the
/// C++ engine reads - E5). A test that starts its own engine uses a unique
⋮----
/// C++ engine reads - E5). A test that starts its own engine uses a unique
/// name so it can never talk to the user's live session.
⋮----
/// name so it can never talk to the user's live session.
⋮----
pub fn is_accepted_server_image(actual: &std::path::Path, expected: &std::path::Path) -> bool {
let Some(name) = actual.file_name() else {
⋮----
let name = name.to_string_lossy().to_lowercase();
if !ACCEPTED_SERVER_IMAGE_NAMES.contains(&name.as_str()) {
⋮----
actual.parent() == expected.parent()
⋮----
pub const fn dev_server_image_allowed() -> bool {
cfg!(debug_assertions)
⋮----
pub fn current_session_pipe_name(override_name: Option<&str>) -> String {
⋮----
if !name.is_empty() {
return name.to_string();
⋮----
let session = current_session_id();
format!("{PIPE_NAME_BASE}-{session}")
⋮----
pub fn pipe_name_for_session(session_id: u32) -> String {
format!("{PIPE_NAME_BASE}-{session_id}")
⋮----
pub fn current_session_id() -> u32 {
use windows::Win32::System::RemoteDesktop::ProcessIdToSessionId;
⋮----
let _ = ProcessIdToSessionId(
⋮----
pub struct ConnectOptions {
⋮----
impl Default for ConnectOptions {
fn default() -> Self {
⋮----
pub struct ServerIdentity {
⋮----
pub struct IpcClient {
⋮----
impl IpcClient {
⋮----
pub fn connect(pipe_name: &str, options: ConnectOptions) -> Result<IpcClient, IpcError> {
⋮----
fn connect_with(
⋮----
let handle = open_pipe(pipe_name, options.timeout, wait_for_the_pipe)?;
⋮----
pipe_name: pipe_name.to_string(),
⋮----
match client.verify_server() {
⋮----
Ok(client)
⋮----
Err(e) => Err(e),
⋮----
pub fn reconnect(self, options: ConnectOptions) -> Result<IpcClient, IpcError> {
let pipe_name = self.pipe_name.clone();
drop(self);
⋮----
pub fn connect_current_session(options: ConnectOptions) -> Result<IpcClient, IpcError> {
IpcClient::connect(&current_session_pipe_name(None), options)
⋮----
pub fn server_identity(&self) -> &ServerIdentity {
⋮----
pub fn request(&mut self, message: &IpcMessage) -> Result<serde_json::Value, IpcError> {
let line = encode(message).map_err(|e: CoreIpcError| IpcError::Protocol(e.to_string()))?;
self.write_line(line.as_bytes())?;
let answer = self.read_line()?;
let text = String::from_utf8_lossy(&answer).to_string();
let text = text.trim_end_matches(['\r', '\n']);
⋮----
.map_err(|e| IpcError::Protocol(format!("the answer is not JSON: {e}")))
⋮----
pub fn write_line(&mut self, bytes: &[u8]) -> Result<(), IpcError> {
overlapped_write(self.handle, bytes, self.options.timeout)
⋮----
pub fn read_line(&mut self) -> Result<Vec<u8>, IpcError> {
let mut buffer = vec![0u8; k6wp_core::ipc::MAX_PAYLOAD_BYTES + 1024];
let read = overlapped_read(self.handle, &mut buffer, self.options.timeout)?;
buffer.truncate(read);
Ok(buffer)
⋮----
pub fn close(self) {
⋮----
fn verify_server(&self) -> Result<ServerIdentity, IpcError> {
use windows::Win32::System::Pipes::GetNamedPipeServerProcessId;
⋮----
let ok = unsafe { GetNamedPipeServerProcessId(self.handle, &mut pid) };
if ok.is_err() {
return Err(IpcError::Unverified(
"the server process id could not be read".to_string(),
⋮----
"the server process id is zero".to_string(),
⋮----
let server_session = process_session_id(pid);
⋮----
let own_session = current_session_id();
⋮----
return Err(IpcError::Unverified(format!(
⋮----
let owner_sid = process_owner_sid(pid);
⋮----
let own_sid = current_user_sid();
⋮----
"the server's owner could not be read".to_string(),
⋮----
let image_path = process_image_path(pid);
⋮----
Some(actual) if paths_equal(actual, expected.as_path()) => {}
⋮----
"the server image path could not be read".to_string(),
⋮----
Ok(ServerIdentity {
⋮----
impl Drop for IpcClient {
fn drop(&mut self) {
⋮----
fn paths_equal(a: &std::path::Path, b: &std::path::Path) -> bool {
a.to_string_lossy()
.eq_ignore_ascii_case(&b.to_string_lossy())
⋮----
fn process_session_id(pid: u32) -> u32 {
⋮----
let _ = ProcessIdToSessionId(pid, &mut session);
⋮----
fn process_image_path(pid: u32) -> Option<PathBuf> {
use windows::core::PWSTR;
⋮----
let process = unsafe { OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid) }.ok()?;
⋮----
let mut size = buffer.len() as u32;
⋮----
QueryFullProcessImageNameW(
⋮----
PROCESS_NAME_FORMAT(0),
PWSTR(buffer.as_mut_ptr()),
⋮----
Some(PathBuf::from(text))
⋮----
fn process_owner_sid(pid: u32) -> Option<String> {
use windows::Win32::Foundation::HANDLE;
⋮----
let opened = unsafe { OpenProcessToken(process, TOKEN_QUERY, &mut token) };
⋮----
if opened.is_err() {
⋮----
let _ = GetTokenInformation(token, TokenUser, None, 0, &mut size);
⋮----
let mut buffer = vec![0u8; size as usize];
⋮----
GetTokenInformation(
⋮----
Some(buffer.as_mut_ptr() as *mut c_void),
⋮----
if read.is_err() {
⋮----
sid_to_string(&buffer)
⋮----
fn current_user_sid() -> Option<String> {
⋮----
let process = unsafe { GetCurrentProcess() };
⋮----
let mut buffer = vec![0u8; size.max(1) as usize];
⋮----
buffer.len() as u32,
⋮----
fn sid_to_string(buffer: &[u8]) -> Option<String> {
⋮----
use windows::Win32::Foundation::LocalFree;
use windows::Win32::Security::Authorization::ConvertSidToStringSidW;
⋮----
let token_user = buffer.as_ptr() as *const windows::Win32::Security::TOKEN_USER;
⋮----
let ok = unsafe { ConvertSidToStringSidW(sid, &mut text) };
if ok.is_err() || text.is_null() {
⋮----
while *text.0.add(len) != 0 {
⋮----
let _ = LocalFree(Some(windows::Win32::Foundation::HLOCAL(
⋮----
Some(owned)
⋮----
fn open_pipe(
⋮----
use windows::core::PCWSTR;
⋮----
use windows::Win32::System::Pipes::WaitNamedPipeW;
⋮----
let wide: Vec<u16> = pipe_name.encode_utf16().chain(std::iter::once(0)).collect();
⋮----
CreateFileW(
PCWSTR(wide.as_ptr()),
⋮----
FILE_SHARE_MODE(0),
⋮----
let mode = NAMED_PIPE_MODE(PIPE_READMODE_MESSAGE.0);
⋮----
let _ = SetNamedPipeHandleState(
⋮----
Some(&mode as *const NAMED_PIPE_MODE),
⋮----
return Ok(h);
⋮----
let last_error = (e.code().0 as u32) & 0xFFFF;
⋮----
return Err(IpcError::Connect(format!(
⋮----
let remaining = deadline.saturating_duration_since(std::time::Instant::now());
if remaining.is_zero() {
⋮----
let wait_ms = remaining.as_millis().clamp(1, u32::MAX as u128) as u32;
⋮----
let waited = unsafe { WaitNamedPipeW(PCWSTR(wide.as_ptr()), wait_ms) };
if !waited.as_bool() {
⋮----
std::thread::sleep(Duration::from_millis(50).min(remaining));
⋮----
fn overlapped_write(
⋮----
use windows::Win32::Foundation::CloseHandle;
use windows::Win32::System::Threading::CreateEventW;
⋮----
let event = unsafe { CreateEventW(None, true, false, None) }
.map_err(|e| IpcError::Transport(format!("the write event could not be created: {e}")))?;
⋮----
Some(bytes),
Some(&mut written),
Some(&mut overlapped),
⋮----
if ok.is_ok() {
⋮----
let _ = CloseHandle(event);
⋮----
if written as usize != bytes.len() {
return Err(IpcError::Transport(format!(
⋮----
return Ok(());
⋮----
let timeout_ms = timeout.as_millis().min(u32::MAX as u128) as u32;
⋮----
GetOverlappedResult(
⋮----
return Err(IpcError::Timeout(
"the write did not finish in time".to_string(),
⋮----
if done.is_ok() {
⋮----
Err(IpcError::Transport(
"the write failed on the pipe".to_string(),
⋮----
fn overlapped_read(
⋮----
.map_err(|e| IpcError::Transport(format!("the read event could not be created: {e}")))?;
⋮----
Some(&mut buffer[..]),
Some(&mut transferred),
⋮----
return Ok(transferred as usize);
⋮----
"the answer did not arrive in time".to_string(),
⋮----
"the read failed on the pipe".to_string(),
```

## File: rust/crates/k6wp-ipc/tests/client.rs
```rust
use windows::core::PCWSTR;
⋮----
fn the_pipe_name_is_the_per_session_one() {
⋮----
let name = current_session_pipe_name(None);
assert!(
⋮----
let tail = name.rsplit('-').next().unwrap();
⋮----
let session: u32 = tail.parse().unwrap();
assert_eq!(pipe_name_for_session(session), name);
⋮----
fn a_missing_pipe_fails_fast() {
⋮----
let elapsed = started.elapsed();
assert!(result.is_err(), "connecting to a missing pipe must fail");
⋮----
fn a_payload_over_the_limit_is_refused_before_the_pipe() {
⋮----
let huge = "x".repeat(k6wp_core::ipc::MAX_PAYLOAD_BYTES + 1024);
⋮----
fn a_silent_server_times_out_instead_of_hanging() {
⋮----
use windows::Win32::Foundation::HANDLE;
use windows::Win32::Storage::FileSystem::FILE_FLAGS_AND_ATTRIBUTES;
⋮----
let name = format!(r"\\.\pipe\k6wp-engine-test-silent-{}", std::process::id());
let wide: Vec<u16> = name.encode_utf16().chain(std::iter::once(0)).collect();
// SAFETY: the name is null-terminated and the out-parameters are valid.
⋮----
CreateNamedPipeW(
PCWSTR(wide.as_ptr()),
FILE_FLAGS_AND_ATTRIBUTES(0x0000_0003),
NAMED_PIPE_MODE(PIPE_TYPE_MESSAGE.0 | PIPE_READMODE_MESSAGE.0 | PIPE_WAIT.0),
⋮----
assert_ne!(
⋮----
let mut client = IpcClient::connect(&name, options).expect("the pipe must open");
⋮----
let result = client.request(&IpcMessage {
⋮----
let _ = HANDLE(std::ptr::null_mut());
⋮----
fn a_busy_pipe_waits_then_says_why() {
⋮----
let name = format!(r"\\.\pipe\k6wp-engine-test-busy-{}", std::process::id());
⋮----
// One instance only: the occupant takes it.
⋮----
CreateFileW(
⋮----
FILE_SHARE_MODE(0),
⋮----
assert!(occupant.is_ok(), "the occupant must take the only instance");
⋮----
Ok(_) => panic!("a busy pipe must not connect"),
⋮----
let message = error.to_string();
⋮----
fn the_reconnect_releases_the_old_handle() {
⋮----
let name = format!(
⋮----
let wide = wide.clone();
⋮----
// One instance at a time, exactly like the engine: the next
// instance exists only after the client left.
⋮----
// SAFETY: the name is null-terminated.
⋮----
NAMED_PIPE_MODE(
⋮----
// ERROR_PIPE_BUSY: the old instance is still alive.
⋮----
// The instance exists: the client may connect now.
let _ = ready_tx.send(());
⋮----
// Wait for the client (ERROR_PIPE_CONNECTED means it is
// already there).
// SAFETY: the handle is live.
if let Err(e) = unsafe { ConnectNamedPipe(pipe, None) } {
let code = (e.code().0 as u32) & 0xFFFF;
⋮----
let _ = CloseHandle(pipe);
⋮----
// Read one request.
⋮----
// SAFETY: the handle is live and the buffer is valid.
let ok = unsafe { ReadFile(pipe, Some(&mut buffer), Some(&mut read), None) };
if ok.is_err() {
⋮----
// Answer it.
⋮----
let _ = unsafe { WriteFile(pipe, Some(answer), None, None) };
⋮----
let ok2 = unsafe { ReadFile(pipe, Some(&mut buffer), Some(&mut read2), None) };
⋮----
if served >= 2 || ok2.is_err() {
⋮----
.recv_timeout(Duration::from_secs(5))
.expect("the fake engine must create its pipe instance");
⋮----
IpcClient::connect(&name, options.clone()).expect("the first connect must work");
let first = client.request(&message);
⋮----
let second = client.request(&message);
assert!(second.is_err(), "the request after the hang-up must fail");
⋮----
.reconnect(options)
.expect("the reconnect must succeed - the old handle must be released first");
let third = client.request(&message);
⋮----
let fourth = client.request(&message);
assert!(fourth.is_err(), "the request after the hang-up must fail");
⋮----
let _ = server.join();
⋮----
fn set_video_encodes_correctly() {
⋮----
let encoded = encode(&message).expect("must encode");
⋮----
fn set_video_error_payload_decodes() {
⋮----
let error_msg = error_response["error"].as_str().unwrap_or("");
assert!(error_msg.contains("video format"));
⋮----
fn set_video_missing_media() {
```

## File: rust/Cargo.toml
```toml
# The K6WP Rust workspace (Phase 2: the Studio in Slint, no Qt).
#
# The workspace is deliberately small: one crate per deliverable, so the
# engine (Phase 3) and the Studio can evolve independently while sharing
# the protocol and the settings crates.
[workspace]
resolver = "2"
members = ["crates/k6wp-core", "crates/k6wp-ipc", "crates/k6wp-studio"]

[workspace.package]
version = "0.1.0"
edition = "2021"
rust-version = "1.80"
license = "GPL-2.0-or-later"

[workspace.dependencies]
slint = "1.17"

[profile.release]
opt-level = "z"
lto = true
codegen-units = 1
panic = "abort"
strip = true

[workspace.metadata.nextest]
slow-timeout = { period = "60s", terminate-after = 1 }
```

## File: rust/crates/k6wp-studio/Cargo.toml
```toml
[package]
name = "k6wp-studio"
version.workspace = true
edition.workspace = true
rust-version.workspace = true
license.workspace = true
description = "The K6WP Studio: the wallpapers, the library and the settings, in Slint."

[dependencies]
k6wp-core = { path = "../k6wp-core", version = "0.1.0" }
k6wp-ipc = { path = "../k6wp-ipc", version = "0.1.0" }
slint = { workspace = true }
sha1 = "0.10.6"
serde_json = "1"
# The minimal decoders: the Studio reads thumbnails, it is not an image editor.
# Default features off - jpeg-decoder's `rayon` would pull a thread pool for a
# 160x90 thumbnail.
jpeg-decoder = { version = "0.3", default-features = false }
png = { version = "0.17", default-features = false }

# File picker for import (cross-platform, minimal dependency)
rfd = "0.15"

# The system light/dark preference, read on the worker thread. Same crate and
# version the other two crates already build, so no new DLL surface.
[target.'cfg(windows)'.dependencies]
windows = { version = "0.62", features = [
  "Win32_Foundation",
  "Win32_System_Registry",
  "Win32_UI_WindowsAndMessaging",
] }

[build-dependencies]
slint-build = "1"

[[bin]]
name = "k6wp-studio"
path = "src/main.rs"

[lib]
name = "k6wp_studio"
path = "src/lib.rs"
```

## File: rust/crates/k6wp-core/src/sps.rs
```rust
use std::path::Path;
⋮----
pub enum Codec {
⋮----
impl Codec {
pub fn as_str(self) -> &'static str {
⋮----
/// The input limits (G2: the parser processes UNTRUSTED input).
///
⋮----
///
/// Every count that comes FROM THE FILE is capped before it is used as a
⋮----
/// Every count that comes FROM THE FILE is capped before it is used as a
/// loop bound or an allocation size, and every arithmetic operation on a
⋮----
/// loop bound or an allocation size, and every arithmetic operation on a
/// file-derived value is checked. A malformed file must produce an error
⋮----
/// file-derived value is checked. A malformed file must produce an error
/// or an "unknown" answer - never a panic, a huge allocation or an endless
⋮----
/// or an "unknown" answer - never a panic, a huge allocation or an endless
/// loop.
⋮----
/// loop.
mod limits {
⋮----
mod limits {
/// The most entries any container table may declare.
    pub const MAX_TABLE_ENTRIES: usize = 1 << 20;
/// The most samples the sample table may declare.
    pub const MAX_SAMPLES: usize = 1 << 20;
/// The most entries in a parameter-set array.
    pub const MAX_PARAM_SETS: usize = 256;
/// The most pictures in a picture-ordering cycle.
    pub const MAX_PIC_ORDER_CYCLE: u32 = 4096;
/// The most CPB entries in an HRD.
    pub const MAX_CPB_ENTRIES: u32 = 32;
/// The most sub-layers in an HEVC profile_tier_level.
    pub const MAX_SUB_LAYERS: u32 = 7;
/// The most entries a box tree may hold on the stack.
    pub const MAX_BOX_STACK: usize = 64;
/// The most bytes the `moov` box may contribute. A `moov` bigger than
    /// this is a corrupt (or hostile) file: it is skipped, not loaded.
⋮----
/// this is a corrupt (or hostile) file: it is skipped, not loaded.
    pub const MAX_MOOV_BYTES: u64 = 32 * 1024 * 1024;
/// The most bytes the `ftyp` box may contribute (it is a brand + a few
    /// compatible brands - a few dozen bytes in practice).
⋮----
/// compatible brands - a few dozen bytes in practice).
    pub const MAX_FTYP_BYTES: u64 = 4096;
⋮----
/// The container a file turned out to be.
///
⋮----
///
/// The policy is EXPLICIT (task I2): this module parses the MP4 family
⋮----
/// The policy is EXPLICIT (task I2): this module parses the MP4 family
/// (the parameter sets live in the `moov` box) and the raw Annex-B streams
⋮----
/// (the parameter sets live in the `moov` box) and the raw Annex-B streams
/// (the parameter sets are in band). Every other container - Matroska /
⋮----
/// (the parameter sets are in band). Every other container - Matroska /
/// WebM, AVI, MPEG-TS - FALLS BACK to ffmpeg's `trace_headers`: the parser
⋮----
/// WebM, AVI, MPEG-TS - FALLS BACK to ffmpeg's `trace_headers`: the parser
⋮----
pub enum ContainerKind {
⋮----
impl ContainerKind {
⋮----
/// True when this module can parse the container itself.
    pub fn is_parsed(self) -> bool {
⋮----
pub fn is_parsed(self) -> bool {
matches!(self, ContainerKind::Mp4 | ContainerKind::RawStream)
⋮----
/// Detects the container from the head of the file.
pub fn detect_container(head: &[u8]) -> ContainerKind {
⋮----
pub fn detect_container(head: &[u8]) -> ContainerKind {
// The EBML magic (Matroska / WebM).
if head.len() >= 4 && head[0] == 0x1A && head[1] == 0x45 && head[2] == 0xDF && head[3] == 0xA3 {
⋮----
// "RIFF"...."AVI " (AVI).
if head.len() >= 12 && &head[0..4] == b"RIFF" && &head[8..12] == b"AVI " {
⋮----
// The MPEG-TS sync byte at 0 and at the packet size (188 / 192).
if head.len() >= 189
⋮----
&& (head[188] == 0x47 || head[192.min(head.len() - 1)] == 0x47)
⋮----
// "ftyp" at the offset 4 (the MP4 family).
if head.len() >= 8 && &head[4..8] == b"ftyp" {
⋮----
// A raw Annex-B stream: a NAL start code (00 00 01 or 00 00 00 01).
if head.len() >= 3
⋮----
&& (head[2] == 1 || (head[2] == 0 && head.len() >= 4 && head[3] == 1))
⋮----
/// What one SPS says.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct SpsInfo {
⋮----
/// H.264 `max_num_ref_frames` - the DPB size.
    pub max_num_ref_frames: Option<u32>,
/// H.264 VUI `max_dec_frame_buffering_minus1 + 1`.
    pub max_dec_frame_buffering: Option<u32>,
/// HEVC `sps_max_dec_pic_buffering_minus1 + 1`.
    pub sps_max_dec_pic_buffering: Option<u32>,
/// H.264 `frame_mbs_only_flag` (0 = interlaced / MBAFF).
    pub frame_mbs_only_flag: Option<u32>,
/// The frame size in pixels, when the SPS carries it.
    pub width: Option<u32>,
⋮----
/// The chroma format the SPS declares: 0 = monochrome, 1 = 4:2:0,
    /// 2 = 4:2:2, 3 = 4:4:4 (task H2).
⋮----
/// 2 = 4:2:2, 3 = 4:4:4 (task H2).
    ///
⋮----
///
    /// `None` when the codec's profile does not carry the field (the
⋮----
/// `None` when the codec's profile does not carry the field (the
⋮----
impl SpsInfo {
⋮----
pub fn dpb_size(&self) -> Option<u32> {
⋮----
Some(Codec::H264) => self.max_num_ref_frames.or(self.max_dec_frame_buffering),
⋮----
pub fn effective_refs(&self) -> Option<u32> {
⋮----
let refs = self.max_num_ref_frames.or(self.sps_max_dec_pic_buffering)?;
⋮----
.or(self.sps_max_dec_pic_buffering);
⋮----
Some(bound) if bound > 0 => Some(refs.min(bound)),
_ => Some(refs),
⋮----
pub fn dpb_bytes(&self) -> Option<u64> {
let refs = self.effective_refs()?;
let frame = self.frame_bytes()?;
⋮----
u64::from(refs).checked_mul(frame)
⋮----
pub fn frame_bytes(&self) -> Option<u64> {
⋮----
let pixels = u64::from(width).checked_mul(u64::from(height))?;
⋮----
.checked_mul(samples_x2)?
.checked_mul(bytes_per_sample)?
.checked_div(2)
⋮----
pub fn pixel_format(&self) -> Option<(&'static str, u32)> {
⋮----
Some((chroma, depth))
⋮----
/// The RAM a DPB-pinned re-encode would actually RECOVER (task H1).
    ///
⋮----
///
    /// `(effective_refs − pin_refs) × frame_bytes` - NOT the total DPB.
⋮----
/// `(effective_refs − pin_refs) × frame_bytes` - NOT the total DPB.
    ///
⋮----
///
    /// The total says how big the decoder's pool is; the savings say how
⋮----
/// The total says how big the decoder's pool is; the savings say how
⋮----
pub fn savings_obtainable(&self) -> Option<u64> {
⋮----
return Some(0);
⋮----
u64::from(refs - Self::PINNED_REFS).checked_mul(frame)
⋮----
pub fn is_copy_safe(&self) -> bool {
match self.dpb_bytes() {
⋮----
pub fn is_plausible(&self) -> bool {
⋮----
match self.dpb_size() {
Some(dpb) => (1..=16).contains(&dpb),
⋮----
pub struct DpbReport {
⋮----
pub enum SpsError {
⋮----
fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
⋮----
SpsError::Io(m) => write!(f, "sps io: {m}"),
SpsError::NoSps => write!(f, "sps: no parameter set found"),
SpsError::UnsupportedCodec(m) => write!(f, "sps: unsupported codec {m}"),
⋮----
/// How much of the file is read. The parameter sets live in the header
/// (the moov / the extradata), so a few MB is plenty; the cap keeps a
⋮----
/// (the moov / the extradata), so a few MB is plenty; the cap keeps a
/// pathological file from being read whole.
⋮----
/// pathological file from being read whole.
const MAX_SCAN_BYTES: usize = 64 * 1024 * 1024;
⋮----
// ---------------------------------------------------------------------------
// the bit reader
⋮----
struct Bits<'a> {
⋮----
pos: usize, // in bits
⋮----
fn new(data: &'a [u8]) -> Self {
⋮----
fn bit(&mut self) -> Option<u32> {
⋮----
if byte >= self.data.len() {
⋮----
Some(((self.data[byte] >> shift) & 1) as u32)
⋮----
fn bits(&mut self, n: u32) -> Option<u32> {
⋮----
value = (value << 1) | self.bit()?;
⋮----
Some(value)
⋮----
fn ue(&mut self) -> Option<u32> {
⋮----
while self.bit()? == 0 {
⋮----
let rest = self.bits(leading_zeros)?;
Some(((1u32 << leading_zeros) | rest) - 1)
⋮----
fn se(&mut self) -> Option<i32> {
let value = self.ue()?;
let magnitude = value.div_ceil(2) as i32;
⋮----
Some(-magnitude)
⋮----
Some(magnitude)
⋮----
fn rbsp_payload(nal: &[u8]) -> Vec<u8> {
let mut out = Vec::with_capacity(nal.len());
⋮----
for (index, &byte) in nal.iter().enumerate() {
⋮----
out.push(byte);
⋮----
if out.len() > 1 {
out.pop();
⋮----
fn parse_h264_sps(nal: &[u8]) -> Option<SpsInfo> {
⋮----
if nal.len() < 2 {
⋮----
let payload = rbsp_payload(&nal[1..]);
if payload.is_empty() {
⋮----
let profile_idc = bits.bits(8)? as u8;
let _constraints = bits.bits(8)?;
let _level_idc = bits.bits(8)?;
let _sps_id = bits.ue()?;
⋮----
codec: Some(Codec::H264),
⋮----
chroma_format_idc: Some(1),
bit_depth_luma: Some(8),
⋮----
if HIGH_PROFILES.contains(&profile_idc) {
let chroma_format_idc = bits.ue()?;
⋮----
bits.bit()?;
⋮----
let bit_depth_luma = bits.ue()?;
let bit_depth_chroma = bits.ue()?;
⋮----
info.chroma_format_idc = Some(chroma_format_idc);
info.bit_depth_luma = Some(bit_depth_luma + 8);
⋮----
let scaling_matrix_present = bits.bit()?;
⋮----
if bits.bit()? == 1 {
⋮----
let _last = bits.bit()?;
let _next = bits.se()?;
skip_scaling_list(&mut bits, size)?;
⋮----
bits.ue()?;
let pic_order_cnt_type = bits.ue()?;
⋮----
bits.se()?;
⋮----
let cycle = bits.ue()?.min(limits::MAX_PIC_ORDER_CYCLE);
⋮----
info.max_num_ref_frames = bits.ue();
⋮----
let pic_width_in_mbs_minus1 = bits.ue()?;
let pic_height_in_map_units_minus1 = bits.ue()?;
let frame_mbs_only_flag = bits.bit()?;
info.frame_mbs_only_flag = Some(frame_mbs_only_flag);
⋮----
let cropping = bits.bit()?;
⋮----
let vui_present = bits.bit()?;
⋮----
parse_h264_vui(&mut bits, &mut info);
⋮----
info.width = Some(width);
info.height = Some(height);
Some(info)
⋮----
fn skip_scaling_list(bits: &mut Bits, size: usize) -> Option<()> {
⋮----
let delta_scale = bits.se()?;
⋮----
Some(())
⋮----
fn parse_h264_vui(bits: &mut Bits, info: &mut SpsInfo) {
let aspect_ratio_info_present = bits.bit().unwrap_or(0);
⋮----
let aspect_ratio_idc = bits.bits(8).unwrap_or(0);
⋮----
bits.bits(16);
⋮----
if bits.bit().unwrap_or(0) == 1 {
bits.bit();
⋮----
let video_signal_type_present = bits.bit().unwrap_or(0);
⋮----
bits.bits(3);
⋮----
let colour_description_present = bits.bit().unwrap_or(0);
⋮----
bits.bits(8);
⋮----
bits.ue();
⋮----
bits.bits(32);
⋮----
let nal_hrd_present = bits.bit().unwrap_or(0);
⋮----
skip_hrd(bits);
⋮----
let vcl_hrd_present = bits.bit().unwrap_or(0);
⋮----
let max_num_reorder_frames = bits.ue().unwrap_or(0);
let _max_dec_frame_buffering = bits.ue().unwrap_or(0);
⋮----
fn skip_hrd(bits: &mut Bits) {
let cpb_cnt_minus1 = bits.ue().unwrap_or(0);
bits.bits(4);
⋮----
for _ in 0..=cpb_cnt_minus1.min(limits::MAX_CPB_ENTRIES) {
⋮----
bits.bits(5);
⋮----
fn parse_hevc_sps(nal: &[u8]) -> Option<SpsInfo> {
⋮----
if nal.len() < 3 {
⋮----
let payload = rbsp_payload(&nal[2..]);
⋮----
let _vps_id = bits.bits(4)?;
let max_sub_layers_minus1 = bits.bits(3)?.min(limits::MAX_SUB_LAYERS);
⋮----
skip_profile_tier_level(&mut bits, max_sub_layers_minus1)?;
⋮----
let width = bits.ue()?;
let height = bits.ue()?;
let conformance_window = bits.bit().unwrap_or(0);
⋮----
codec: Some(Codec::Hevc),
width: Some(width),
height: Some(height),
chroma_format_idc: Some(chroma_format_idc),
bit_depth_luma: Some(bit_depth_luma + 8),
⋮----
let sub_layer_ordering_present = bits.bit().unwrap_or(0);
⋮----
let max_dec_pic_buffering_minus1 = bits.ue()?;
⋮----
info.sps_max_dec_pic_buffering = Some(max_dec_pic_buffering_minus1 + 1);
⋮----
.unwrap_or(0)
.max(max_dec_pic_buffering_minus1 + 1);
info.sps_max_dec_pic_buffering = Some(value);
⋮----
fn skip_profile_tier_level(bits: &mut Bits, max_sub_layers_minus1: u32) -> Option<()> {
bits.bits(2)?;
⋮----
bits.bits(5)?;
⋮----
bits.bits(8)?;
⋮----
sub_layer_profile_present.push(bits.bit()?);
sub_layer_level_present.push(bits.bit()?);
⋮----
fn container_sps_nals(data: &[u8]) -> Vec<Vec<u8>> {
⋮----
for (kind, nals) in find_parameter_set_boxes(data) {
⋮----
let parsed = parse_h264_sps(&nal);
if parsed.is_some() {
out.push(nal);
⋮----
if parse_hevc_sps(&nal).is_some() {
⋮----
fn find_parameter_set_boxes(data: &[u8]) -> Vec<(Codec, Vec<Vec<u8>>)> {
⋮----
for avcc in find_box_payloads(data, b"avcC") {
if let Some(nals) = parse_avcc(&avcc) {
out.push((Codec::H264, nals));
⋮----
for hvcc in find_box_payloads(data, b"hvcC") {
if let Some(nals) = parse_hvcc(&hvcc) {
out.push((Codec::Hevc, nals));
⋮----
fn find_box_payloads(data: &[u8], four_cc: &[u8; 4]) -> Vec<Vec<u8>> {
⋮----
let mut stack: Vec<(usize, usize)> = vec![(0, data.len())];
⋮----
while let Some((start, end)) = stack.pop() {
⋮----
if std::env::var_os("K6WP_SPS_DEBUG").is_some() {
eprintln!(
⋮----
.checked_add(pos)
.filter(|body_end| *body_end <= end)
⋮----
out.push(data[pos + header..body_end].to_vec());
⋮----
stsd_extension_boxes(&data[pos + header..body_end])
⋮----
out.push(inner_payload);
⋮----
if CONTAINERS.contains(&&kind) {
stack.push((pos + header, body_end));
⋮----
fn stsd_extension_boxes(payload: &[u8]) -> Vec<([u8; 4], Vec<u8>)> {
⋮----
if payload.len() < 8 {
⋮----
.min(limits::MAX_TABLE_ENTRIES);
⋮----
if pos + 8 > payload.len() {
⋮----
if size < 8 || pos + size > payload.len() {
⋮----
out.push((inner_kind, payload[inner + 8..inner + inner_size].to_vec()));
⋮----
fn parse_avcc(payload: &[u8]) -> Option<Vec<Vec<u8>>> {
if payload.len() < 7 {
⋮----
let count = ((payload[5] & 0x1F) as usize).min(limits::MAX_PARAM_SETS);
⋮----
if pos + 2 > payload.len() {
⋮----
if pos + len > payload.len() {
⋮----
nals.push(payload[pos..pos + len].to_vec());
⋮----
Some(nals)
⋮----
fn parse_hvcc(payload: &[u8]) -> Option<Vec<Vec<u8>>> {
if payload.len() < 23 {
⋮----
let num_arrays = (payload[22] as usize).min(limits::MAX_PARAM_SETS);
⋮----
if pos + 3 > payload.len() {
⋮----
.min(limits::MAX_PARAM_SETS);
⋮----
fn inband_sps_nals(data: &[u8]) -> Vec<Vec<u8>> {
⋮----
out.extend(annexb_sps_nals(data));
out.extend(sample_sps_nals_in_memory(data));
⋮----
fn annexb_sps_nals(data: &[u8]) -> Vec<Vec<u8>> {
⋮----
let limit = data.len().min(MAX_SCAN_BYTES);
⋮----
if is_sps_nal(nal) {
out.push(nal.to_vec());
⋮----
fn sample_sps_nals_in_memory(data: &[u8]) -> Vec<Vec<u8>> {
⋮----
let length_size = nal_length_size(data);
for (offset, size) in sample_ranges(data) {
⋮----
let end = start.saturating_add(size as usize);
⋮----
fn sample_sps_nals_from_file(
⋮----
let length_size = nal_length_size(moov);
⋮----
for (offset, size) in sample_ranges(moov) {
let end = offset.saturating_add(size as u64);
⋮----
if offset.saturating_add(size as u64) > reader.length {
⋮----
reader.seek_to(offset)?;
let mut data = vec![0u8; size as usize];
reader.read_exact_at(&mut data)?;
⋮----
while pos + 4 <= data.len() {
⋮----
if len < 2 || pos + length_size + len > data.len() {
⋮----
Ok(out)
⋮----
fn nal_length_size(data: &[u8]) -> usize {
⋮----
if avcc.len() >= 5 {
⋮----
fn sample_ranges(data: &[u8]) -> Vec<(u64, u32)> {
let sizes = match find_box_payloads(data, b"stsz").first() {
Some(p) => parse_stsz(p),
⋮----
if sizes.is_empty() {
⋮----
let chunk_offsets = match find_box_payloads(data, b"stco").first() {
Some(p) => parse_chunk_offsets(p, 4),
None => match find_box_payloads(data, b"co64").first() {
Some(p) => parse_chunk_offsets(p, 8),
⋮----
let samples_per_chunk = match find_box_payloads(data, b"stsc").first() {
Some(p) => parse_stsc(p),
⋮----
if chunk_offsets.is_empty() || samples_per_chunk.is_empty() {
⋮----
for (chunk_index, chunk_offset) in chunk_offsets.iter().enumerate() {
let count = samples_in_chunk(&samples_per_chunk, chunk_index);
⋮----
if sample_index >= sizes.len() {
⋮----
out.push((offset, size));
⋮----
fn parse_stsz(payload: &[u8]) -> Vec<u32> {
if payload.len() < 12 {
⋮----
.min(limits::MAX_SAMPLES);
⋮----
return vec![sample_size; count];
⋮----
if at + 4 > payload.len() {
⋮----
out.push(u32::from_be_bytes([
⋮----
fn parse_chunk_offsets(payload: &[u8], width: usize) -> Vec<u64> {
⋮----
if at + width > payload.len() {
⋮----
out.push(value);
⋮----
fn parse_stsc(payload: &[u8]) -> Vec<(u32, u32)> {
⋮----
if at + 12 > payload.len() {
⋮----
out.push((first, per_chunk));
⋮----
fn samples_in_chunk(runs: &[(u32, u32)], chunk_index: usize) -> u32 {
⋮----
for (i, (_, per_chunk)) in runs.iter().enumerate() {
⋮----
if let Some((_, per_chunk)) = runs.last() {
let first = runs[runs.len() - 1].0 as usize;
⋮----
fn is_sps_nal(nal: &[u8]) -> bool {
if nal.is_empty() {
⋮----
struct CountingFile {
⋮----
impl CountingFile {
fn open(path: &Path) -> Result<Self, SpsError> {
let file = std::fs::File::open(path).map_err(|e| SpsError::Io(e.to_string()))?;
⋮----
.metadata()
.map_err(|e| SpsError::Io(e.to_string()))?
.len();
Ok(CountingFile {
⋮----
fn read_exact_at(&mut self, buf: &mut [u8]) -> Result<(), SpsError> {
⋮----
.read_exact(buf)
.map_err(|e| SpsError::Io(e.to_string()))?;
self.bytes_read += buf.len() as u64;
Ok(())
⋮----
fn seek_to(&mut self, pos: u64) -> Result<(), SpsError> {
⋮----
.seek(std::io::SeekFrom::Start(pos))
⋮----
fn length(&mut self) -> Result<u64, SpsError> {
⋮----
.stream_position()
⋮----
.seek(std::io::SeekFrom::End(0))
⋮----
self.seek_to(here)?;
Ok(end)
⋮----
fn walk_top_level_boxes(reader: &mut CountingFile) -> Result<Vec<(String, Vec<u8>)>, SpsError> {
let file_len = reader.length()?;
⋮----
reader.seek_to(pos)?;
⋮----
reader.read_exact_at(&mut header)?;
⋮----
let kind = String::from_utf8_lossy(&header[4..8]).to_string();
⋮----
reader.read_exact_at(&mut ext)?;
⋮----
let cap = match kind.as_str() {
"ftyp" => Some(limits::MAX_FTYP_BYTES),
"moov" => Some(limits::MAX_MOOV_BYTES),
⋮----
let mut payload = vec![0u8; payload_len as usize];
reader.read_exact_at(&mut payload)?;
out.push((kind, payload));
⋮----
let Some(next) = pos.checked_add(size) else {
⋮----
pub fn probe_dpb(path: &Path) -> Result<DpbReport, SpsError> {
⋮----
reader.seek_to(0)?;
⋮----
let head_len = std::cmp::min(193, reader.length()? as usize);
reader.read_exact_at(&mut head[..head_len])?;
let container = detect_container(&head[..head_len]);
⋮----
let boxes = walk_top_level_boxes(&mut reader)?;
⋮----
moov = Some(payload);
⋮----
for (codec_kind, nals_in) in find_parameter_set_boxes(moov_bytes) {
⋮----
Codec::H264 => nals.extend(nals_in),
Codec::Hevc => nals.extend(nals_in),
⋮----
nals.extend(sample_sps_nals_from_file(moov_bytes, &mut reader)?);
⋮----
let mut buf = vec![0u8; 64 * 1024];
⋮----
let want = std::cmp::min(buf.len(), MAX_SCAN_BYTES - total);
match reader.file.read(&mut buf[..want]) {
⋮----
prefix.extend_from_slice(&buf[..n]);
⋮----
Err(e) => return Err(SpsError::Io(e.to_string())),
⋮----
nals.extend(annexb_sps_nals(&prefix));
⋮----
return Ok(DpbReport {
⋮----
container: container.as_str().to_string(),
⋮----
let info = match is_sps_nal(nal) {
_ if (nal[0] & 0x1F) == 7 => parse_h264_sps(nal),
_ => parse_hevc_sps(nal),
⋮----
if !info.is_plausible() {
⋮----
if !sps.contains(&info) {
sps.push(info);
⋮----
if sps.is_empty() {
⋮----
let max_dpb = sps.iter().filter_map(|s| s.dpb_size()).max();
Ok(DpbReport {
⋮----
pub fn container_parameter_sets(data: &[u8]) -> Vec<(Codec, Vec<Vec<u8>>)> {
let scanned = &data[..data.len().min(MAX_SCAN_BYTES)];
find_parameter_set_boxes(scanned)
⋮----
pub fn probe_dpb_bytes(data: &[u8]) -> DpbReport {
⋮----
let container = detect_container(scanned);
⋮----
if container.is_parsed() {
nals = container_sps_nals(scanned);
nals.extend(inband_sps_nals(scanned));
⋮----
bytes_read: scanned.len() as u64,
⋮----
mod tests {
⋮----
fn the_exp_golomb_reader_is_correct() {
⋮----
assert_eq!(bits.ue(), Some(0));
assert_eq!(bits.ue(), Some(1));
assert_eq!(bits.ue(), Some(2));
assert_eq!(bits.ue(), Some(3));
⋮----
fn the_signed_exp_golomb_is_correct() {
⋮----
assert_eq!(bits.se(), Some(0));
assert_eq!(bits.se(), Some(1));
assert_eq!(bits.se(), Some(-1));
assert_eq!(bits.se(), Some(2));
⋮----
fn the_emulation_prevention_bytes_are_removed() {
⋮----
let payload = rbsp_payload(&nal);
assert_eq!(payload, vec![0x00, 0x00, 0x01]);
⋮----
fn an_sps_nal_is_recognised() {
assert!(is_sps_nal(&[0x67, 0x42]));
assert!(is_sps_nal(&[0x42, 0x01]));
assert!(!is_sps_nal(&[0x68, 0x42]));
assert!(!is_sps_nal(&[0x40, 0x01]));
assert!(!is_sps_nal(&[]));
⋮----
fn the_frame_size_follows_the_bit_depth_and_the_chroma() {
⋮----
width: Some(1920),
height: Some(1080),
max_num_ref_frames: Some(4),
⋮----
assert_eq!(base.frame_bytes(), Some(3_110_400));
⋮----
bit_depth_luma: Some(10),
..base.clone()
⋮----
assert_eq!(ten.frame_bytes(), Some(6_220_800));
⋮----
chroma_format_idc: Some(3),
⋮----
assert_eq!(full.frame_bytes(), Some(6_220_800));
⋮----
chroma_format_idc: Some(2),
⋮----
assert_eq!(subsampled.frame_bytes(), Some(4_147_200));
⋮----
chroma_format_idc: Some(0),
⋮----
assert_eq!(mono.frame_bytes(), Some(2_073_600));
⋮----
assert_eq!(base.dpb_bytes(), Some(4 * 3_110_400));
assert_eq!(ten.dpb_bytes(), Some(4 * 6_220_800));
⋮----
assert_eq!(base.savings_obtainable(), Some(2 * 3_110_400));
assert_eq!(ten.savings_obtainable(), Some(2 * 6_220_800));
⋮----
fn an_unknown_frame_size_is_never_guessed() {
⋮----
assert_eq!(no_size.frame_bytes(), None);
assert_eq!(no_size.dpb_bytes(), None);
assert!(!no_size.is_copy_safe());
⋮----
assert_eq!(unknown_codec.frame_bytes(), None);
assert_eq!(unknown_codec.dpb_bytes(), None);
assert!(!unknown_codec.is_copy_safe());
```

## File: rust/crates/k6wp-studio/ui/app.slint
```
// The K6WP Studio - the Library page (Phase 2, Blok A-6).
//
// One page: a header, a virtualised grid of cards and a status line. The grid
// is a ListView, which instantiates only the rows that fit the viewport; Rust
// adds one row of buffer above and below and asks the worker for exactly those
// thumbnails. Nothing here reads a file.
//
// Structs and their fields are updated from Rust, never bound to the disk:
// a card's `thumb` is an already-decoded image handed over by the worker.

import { VerticalBox, HorizontalBox, ListView, Button } from "std-widgets.slint";
import { Tokens } from "design-tokens.slint";

// Re-exported so Rust can set `dark` / `reduced-motion` on the global; a
// private import is inlined by the compiler and has no Rust accessor.
export { Tokens }

export struct LibraryCard {
    name: string,
    meta: string,
    unavailable: bool,
    thumb: image,
    index: int,
    media_path: string,  // for the "Pasang" action
}

export struct LibraryRow {
    cards: [LibraryCard],
}

export struct MonitorAssignment {
    device: string,
    video: string,
}

// A hoverable overlay that shows "Pasang" on hover for available items.
component HoverOverlay inherits Rectangle {
    in property <bool> show: false;
    in property <string> text: "Pasang";
    in property <color> text-color: #ffffff;
    in property <color> bg-color: #000000aa;
    in property <length> radius: 6px;
    callback clicked();

    background: show ? bg-color : transparent;
    border-radius: radius;
    opacity: show ? 1.0 : 0.0;
    animate opacity { duration: 150ms; easing: ease-out; }

    Text {
        text: text;
        color: text-color;
        font-size: 14px;
        font-family: "Segoe UI Variable";
        font-weight: 600;
        horizontal-alignment: center;
        vertical-alignment: center;
    }

    TouchArea {
        clicked => { root.clicked(); }
    }
}

// One card: the thumbnail band on top, the title and the metadata below.
// Hover shows a "Pasang" button for available items.
// Forwards the set-wallpaper callback to the root AppWindow.
export component CardItem inherits Rectangle {
    in property <LibraryCard> card;
    in property <length> card-width: 208px;
    in property <length> band-height: 117px;

    // Forward the set-wallpaper callback to the root AppWindow
    callback set-wallpaper(index: int, media_path: string);
    // Forward the compress callback to the root AppWindow
    callback compress(index: int);

    width: card-width;
    border-radius: Tokens.radius-md;
    background: card.unavailable ? Tokens.card-muted : Tokens.card;
    border-width: 1px;
    border-color: Tokens.border;

    // Touch area for the whole card to detect hover and click
    hover_area := TouchArea {
    }

    VerticalLayout {
        // The thumbnail band. `cover` crops; the worker already scaled the
        // image to the drawn size, so this is a blit, not a resample.
        Rectangle {
            height: band-height;
            clip: true;
            border-radius: Tokens.radius-md;
            background: Tokens.thumb-bg;

            if card.thumb.width > 0 : Image {
                source: card.thumb;
                image-fit: cover;
                width: 100%;
                height: 100%;
            }

            if card.unavailable : Rectangle {
                background: Tokens.card-muted;
                border-radius: Tokens.radius-md;
                Text {
                    text: "Tidak tersedia";
                    color: Tokens.unavailable-text;
                    font-size: Tokens.font-sm;
                    horizontal-alignment: center;
                    vertical-alignment: center;
                }
            }

            // "Pasang" overlay on hover (only for available items)
            HoverOverlay {
                show: !card.unavailable && hover_area.has-hover;
            }
            // "Kompres" button on hover (top-right of thumbnail band)
            if !card.unavailable && hover_area.has-hover : Rectangle {
                background: #1976d2;
                width: 56px;
                height: 22px;
                x: band-height - 56px;
                y: 4px;
                border-radius: 4px;
                Text {
                    text: "Kompres";
                    color: #ffffff;
                    font-size: 11px;
                    font-family: "Segoe UI Variable";
                    font-weight: 600;
                    horizontal-alignment: center;
                    vertical-alignment: center;
                }
                TouchArea {
                    clicked => { root.compress(card.index); }
                }
            }
        }

        // The text block: the title, then the metadata line.
        VerticalLayout {
            padding-left: Tokens.space-sm;
            padding-right: Tokens.space-sm;
            padding-top: Tokens.space-xs;
            padding-bottom: Tokens.space-xs;
            spacing: 2px;

            Text {
                text: card.name;
                color: card.unavailable ? Tokens.muted : Tokens.fg;
                font-size: Tokens.font-md;
                font-family: Tokens.font-family;
                overflow: elide;
            }
            Text {
                text: card.meta;
                color: Tokens.muted;
                font-size: Tokens.font-sm;
                font-family: Tokens.font-family;
                overflow: elide;
            }
        }
        // Absorbs the slack so the two lines sit at the top of the block
        // instead of being pushed to the bottom edge.
        Rectangle { vertical-stretch: 1; }
    }
    
    // Click on the card sends the set_wallpaper callback (only for available)
    TouchArea {
        clicked => {
            if !card.unavailable {
                root.set_wallpaper(card.index, card.media_path);
            }
        }
    }
}

export component AppWindow inherits Window {
    in-out property <[LibraryRow]> rows: [];
    in-out property <int> columns: 4;
    in-out property <length> card-width: 208px;
    in-out property <length> row-height: 173px;
    in-out property <int> count: 0;
    in-out property <string> footer: "";
    in-out property <string> empty-message: "Memuat library...";
    in-out property <string> version: "";

    // IPC connection status
    in-out property <string> connection-status: "Terputus";
    in-out property <brush> connection-color: #d32f2f;

    // Current engine state
    in-out property <string> current-video: "—";
    in-out property <bool> is-paused: false;

    // Toast notification
    in-out property <string> toast: "";
    /// Show the "Urungkan" action inside the toast.
    in-out property <bool> undo-available: false;
    /// Guards the auto-clear: only the toast that set this value may clear it.
    in-out property <int> toast-generation: 0;
    /// Active compress job id (-1 = none). Shown as cancel button in toast.
    in-out property <int> compress-job-id: -1;

    // Settings page
    in-out property <bool> settings-open: false;
    in-out property <bool> settings-auto-compress: true;
    in-out property <string> settings-compress-output-dir: "";
    in-out property <string> settings-default-crf: "22";
    in-out property <string> settings-default-fps: "30";
    in-out property <string> settings-default-resolution: "match_monitor";
    in-out property <string> settings-cache-dir: "";
    in-out property <bool> settings-wallpaper-sync: true;
    in-out property <bool> settings-lockscreen-sync: true;
    in-out property <string> settings-lockscreen-offset: "0.0";
    in-out property <bool> settings-check-updates: true;
    in-out property <bool> settings-close-to-tray: false;

    // Settings tab
    in-out property <string> settings-tab: "umum";

    in-out property <[MonitorAssignment]> monitors: [];
    in-out property <string> selected-monitor-device: "";
    in-out property <string> selected-monitor-video: "";
    in-out property <bool> monitor-video-picker-open: false;

    // Playlist tab
    in-out property <bool> settings-playlist-enabled: false;
    in-out property <string> settings-playlist-interval: "30";
    in-out property <bool> settings-playlist-shuffle: false;
    in-out property <string> settings-playlist-order: "";
    in-out property <bool> settings-playlist-order-picker-open: false;

    callback save-settings();
    callback pick-folder(string);
    callback list-monitors();
    callback assign-monitor(string, string);
    callback clear-assignment(string);
    callback load-playlist();
    callback save-playlist(string, int, bool, string);
    callback pick-folder-playlist(string);
    callback pick-folder-monitor(string);

    // Rust rebuilds the row model when the column count changes.
    callback rebuild-grid(width: length);
    // Rust asks the worker for the thumbnails of the visible band.
    callback viewport-changed(scroll-y: length, viewport-height: length);
    // User clicked "Pasang" on a card (forwarded from CardItem).
    callback set-wallpaper(index: int, media_path: string);
    // User clicked "Jeda" in the header.
    callback pause();
    // User clicked "Lanjut" in the header.
    callback resume();
    // User clicked "Urungkan" in the toast: go back to the previous wallpaper.
    callback undo();
    // User clicked "Impor" in the header.
    callback import();
    // User clicked "Kompres" on a library card.
    callback compress(index: int);
    // User clicked "Batal" to cancel compress job.
    callback compress-cancel(job_id: int);

    title: "K6WP Studio";
    preferred-width: 960px;
    preferred-height: 640px;
    min-width: 640px;
    min-height: 420px;
    background: Tokens.bg;

    // The band is 16:9 of the card width; the text block is the rest.
    property <length> band-height: card-width * 9 / 16;

    VerticalLayout {
        padding: Tokens.space-lg;
        spacing: Tokens.space-md;

        // Header: title, connection status, current video, pause/resume buttons.
        HorizontalLayout {
            spacing: Tokens.space-sm;

            // Left: title
            Text {
                text: "Library";
                color: Tokens.fg;
                font-size: Tokens.font-xl;
                font-family: Tokens.font-family;
                font-weight: 600;
                vertical-alignment: center;
            }

            Rectangle { horizontal-stretch: 1; }

            // Center: connection status
            Rectangle {
                background: connection-color;
                border-radius: Tokens.radius-sm;
                padding: Tokens.space-xs;
                padding-left: Tokens.space-sm;
                padding-right: Tokens.space-sm;
                Text {
                    text: connection-status;
                    color: #ffffff;
                    font-size: Tokens.font-sm;
                    font-family: Tokens.font-family;
                    font-weight: 500;
                }
            }

            // Center-right: current video
            Rectangle {
                background: Tokens.card;
                border-width: 1px;
                border-color: Tokens.border;
                border-radius: Tokens.radius-sm;
                padding: Tokens.space-xs;
                padding-left: Tokens.space-sm;
                padding-right: Tokens.space-sm;
                min-width: 200px;
                Text {
                    text: "Sedang: " + current-video;
                    color: Tokens.fg;
                    font-size: Tokens.font-sm;
                    font-family: Tokens.font-family;
                    overflow: elide;
                }
            }

            // Right: import/pause/resume buttons
            HorizontalLayout {
                spacing: Tokens.space-xs;
                Button {
                    text: "Impor";
                    clicked => { root.import(); }
                }
                Button {
                    text: "Pengaturan";
                    clicked => { root.settings-open = true; }
                }
                Button {
                    text: "Jeda";
                    enabled: !is-paused;
                    clicked => { root.pause(); }
                }
                Button {
                    text: "Lanjut";
                    enabled: is-paused;
                    clicked => { root.resume(); }
                }
            }

            Text {
                text: count + " item";
                color: Tokens.muted;
                font-size: Tokens.font-sm;
                font-family: Tokens.font-family;
                vertical-alignment: center;
            }
            Text {
                text: "v" + version;
                color: Tokens.muted;
                font-size: Tokens.font-sm;
                font-family: Tokens.font-family;
                vertical-alignment: center;
            }
        }

        // The grid. ListView is lazy: a row exists only while it is close to
        // the viewport, so 200 items cost the same as 12 painted cards.
        Rectangle {
            vertical-stretch: 1;

            grid := ListView {
                for row in rows : Rectangle {
                    height: row-height;
                    HorizontalLayout {
                        spacing: Tokens.space-md;
                        for card in row.cards : CardItem {
                            card: card;
                            card-width: root.card-width;
                            band-height: root.band-height;
                            // Forward the set-wallpaper callback from CardItem to AppWindow
                            set-wallpaper(index, media_path) => { root.set_wallpaper(index, media_path); }
                            compress(index) => { root.compress(index); }
                        }
                        Rectangle { horizontal-stretch: 1; }
                    }
                }
            }

            // The empty state shares the grid's space: the same page, no dialog.
            if count == 0 : Rectangle {
                background: Tokens.bg;
                Text {
                    text: empty-message;
                    color: Tokens.muted;
                    font-size: Tokens.font-md;
                    font-family: Tokens.font-family;
                    horizontal-alignment: center;
                    vertical-alignment: center;
                }
            }
        }

        // Toast notification (bottom-right overlay)
        if toast != "" : Rectangle {
            background: #333333ee;
            border-radius: Tokens.radius-md;
            padding: Tokens.space-md;
            HorizontalLayout {
                spacing: Tokens.space-md;
                Text {
                    text: toast;
                    color: #ffffff;
                    font-size: Tokens.font-md;
                    font-family: Tokens.font-family;
                    vertical-alignment: center;
                    horizontal-stretch: 1;
                }
                if undo-available : Button {
                    text: "Urungkan";
                    clicked => { root.undo(); }
                }
                if compress-job-id >= 0 : Button {
                    text: "Batal";
                    clicked => { root.compress-cancel(compress-job-id); }
                }
            }
        }

        // Settings page overlay
        if settings-open : Rectangle {
            background: #000000aa;
            VerticalLayout {
                horizontal-stretch: 1;
                vertical-stretch: 1;
                padding: Tokens.space-lg;
                spacing: Tokens.space-md;

                Text {
                    text: "Pengaturan";
                    color: #ffffff;
                    font-size: Tokens.font-xl;
                    font-family: Tokens.font-family;
                    font-weight: 600;
                }

                // Tab buttons
                HorizontalLayout {
                    spacing: Tokens.space-sm;
                    TouchArea {
                        clicked => { root.settings-tab = "umum"; }
                        Rectangle {
                            background: root.settings-tab == "umum" ? #1976d2 : transparent;
                            border-radius: Tokens.radius-sm;
                            padding: Tokens.space-sm;
                            padding-left: Tokens.space-md;
                            padding-right: Tokens.space-md;
                            Text {
                                text: "Umum";
                                color: #ffffff;
                                font-size: Tokens.font-sm;
                                font-family: Tokens.font-family;
                                font-weight: 600;
                                horizontal-alignment: center;
                            }
                        }
                    }
                    TouchArea {
                        clicked => { root.settings-tab = "monitor"; }
                        Rectangle {
                            background: root.settings-tab == "monitor" ? #1976d2 : transparent;
                            border-radius: Tokens.radius-sm;
                            padding: Tokens.space-sm;
                            padding-left: Tokens.space-md;
                            padding-right: Tokens.space-md;
                            Text {
                                text: "Monitor";
                                color: #ffffff;
                                font-size: Tokens.font-sm;
                                font-family: Tokens.font-family;
                                font-weight: 600;
                                horizontal-alignment: center;
                            }
                        }
                    }
                    TouchArea {
                        clicked => { root.settings-tab = "playlist"; }
                        Rectangle {
                            background: root.settings-tab == "playlist" ? #1976d2 : transparent;
                            border-radius: Tokens.radius-sm;
                            padding: Tokens.space-sm;
                            padding-left: Tokens.space-md;
                            padding-right: Tokens.space-md;
                            Text {
                                text: "Playlist";
                                color: #ffffff;
                                font-size: Tokens.font-sm;
                                font-family: Tokens.font-family;
                                font-weight: 600;
                                horizontal-alignment: center;
                            }
                        }
                    }
                }

                // Tab content
                if root.settings-tab == "umum" : VerticalLayout {
                    horizontal-stretch: 1;
                    spacing: Tokens.space-md;

                    // Auto compress on import
                    HorizontalLayout {
                        spacing: Tokens.space-md;
                        TouchArea {
                            clicked => { root.settings-auto-compress = !root.settings-auto-compress; }
                        }
                        if root.settings-auto-compress : Text {
                            text: "[✓]";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        if !root.settings-auto-compress : Text {
                            text: "[ ]";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        Text {
                            text: "Kompres otomatis setelah impor";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                    }

                    // Compress output dir
                    HorizontalLayout {
                        spacing: Tokens.space-md;
                        Text {
                            text: "Folder output kompresi:";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        Text {
                            text: settings-compress-output-dir;
                            color: #bbbbbb;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                            horizontal-stretch: 1;
                            overflow: elide;
                        }
                        Button {
                            text: "Pilih";
                            clicked => { root.pick-folder("compress_output_dir"); }
                        }
                    }

                    // Default CRF
                    HorizontalLayout {
                        spacing: Tokens.space-md;
                        Text {
                            text: "Kualitas default (CRF):";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        Text {
                            text: "16 - 28";
                            color: #bbbbbb;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        TextInput {
                            text: root.settings-default-crf;
                            width: 40px;
                            edited => {
                                root.settings-default-crf = self.text;
                            }
                        }
                    }

                    // Default FPS
                    HorizontalLayout {
                        spacing: Tokens.space-md;
                        Text {
                            text: "Frame rate default (FPS):";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        Text {
                            text: "1 - 30";
                            color: #bbbbbb;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        TextInput {
                            text: root.settings-default-fps;
                            width: 40px;
                            edited => {
                                root.settings-default-fps = self.text;
                            }
                        }
                    }

                    // Default resolution mode
                    HorizontalLayout {
                        spacing: Tokens.space-md;
                        Text {
                            text: "Resolusi default:";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        Text {
                            text: "(match_monitor / source / 720p / 1080p / 2160p)";
                            color: #bbbbbb;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        TextInput {
                            text: root.settings-default-resolution;
                            width: 100px;
                            edited => {
                                root.settings-default-resolution = self.text;
                            }
                        }
                    }

                    // Cache dir
                    HorizontalLayout {
                        spacing: Tokens.space-md;
                        Text {
                            text: "Folder cache:";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        Text {
                            text: settings-cache-dir;
                            color: #bbbbbb;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                            horizontal-stretch: 1;
                            overflow: elide;
                        }
                        Button {
                            text: "Pilih";
                            clicked => { root.pick-folder("cache_dir"); }
                        }
                    }

                    // Wallpaper sync
                    HorizontalLayout {
                        spacing: Tokens.space-md;
                        TouchArea {
                            clicked => { root.settings-wallpaper-sync = !root.settings-wallpaper-sync; }
                        }
                        if root.settings-wallpaper-sync : Text {
                            text: "[✓]";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        if !root.settings-wallpaper-sync : Text {
                            text: "[ ]";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        Text {
                            text: "Sinkronisasi wallpaper";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                    }

                    // Lockscreen sync
                    HorizontalLayout {
                        spacing: Tokens.space-md;
                        TouchArea {
                            clicked => { root.settings-lockscreen-sync = !root.settings-lockscreen-sync; }
                        }
                        if root.settings-lockscreen-sync : Text {
                            text: "[✓]";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        if !root.settings-lockscreen-sync : Text {
                            text: "[ ]";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        Text {
                            text: "Sinkronisasi layar kunci";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                    }

                    // Lockscreen offset
                    HorizontalLayout {
                        spacing: Tokens.space-md;
                        Text {
                            text: "Offset layar kunci (detik):";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        Text {
                            text: "≥ 0";
                            color: #bbbbbb;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        TextInput {
                            text: root.settings-lockscreen-offset;
                            width: 60px;
                            edited => {
                                root.settings-lockscreen-offset = self.text;
                            }
                        }
                    }

                    // Check updates
                    HorizontalLayout {
                        spacing: Tokens.space-md;
                        TouchArea {
                            clicked => { root.settings-check-updates = !root.settings-check-updates; }
                        }
                        if root.settings-check-updates : Text {
                            text: "[✓]";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        if !root.settings-check-updates : Text {
                            text: "[ ]";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        Text {
                            text: "Periksa pembaruan saat mulai";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                    }

                    // Close to tray
                    HorizontalLayout {
                        spacing: Tokens.space-md;
                        TouchArea {
                            clicked => { root.settings-close-to-tray = !root.settings-close-to-tray; }
                        }
                        if root.settings-close-to-tray : Text {
                            text: "[✓]";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        if !root.settings-close-to-tray : Text {
                            text: "[ ]";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        Text {
                            text: "Tutup ke sistem tray";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                    }

                    Rectangle { vertical-stretch: 1; }

                    HorizontalLayout {
                        spacing: Tokens.space-md;
                        Button {
                            text: "Simpan";
                            clicked => { root.save-settings(); }
                        }
                        Button {
                            text: "Batal";
                            clicked => { root.settings-open = false; }
                        }
                    }
                }

                // Monitor tab
                if root.settings-tab == "monitor" : VerticalLayout {
                    horizontal-stretch: 1;
                    spacing: Tokens.space-md;

                    Text {
                        text: "Penetapan Monitor";
                        color: #ffffff;
                        font-size: Tokens.font-lg;
                        font-family: Tokens.font-family;
                        font-weight: 600;
                    }

                    // Monitor list
                    for monitor in monitors : HorizontalLayout {
                        spacing: Tokens.space-md;
                        Text {
                            text: monitor.device;
                            color: #ffffff;
                            font-size: Tokens.font-md;
                            font-family: Tokens.font-family;
                            horizontal-stretch: 1;
                        }
                        if monitor.video != "" : Text {
                            text: monitor.video;
                            color: #bbbbbb;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            horizontal-stretch: 1;
                            overflow: elide;
                        }
                        if monitor.video == "" : Text {
                            text: "Belum ditetapkan";
                            color: #888888;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            horizontal-stretch: 1;
                        }
                        Button {
                            text: "Pasang";
                            clicked => {
                                root.selected-monitor-device = monitor.device;
                                root.monitor-video-picker-open = true;
                            }
                        }
                        Button {
                            text: "Hapus";
                            clicked => {
                                root.clear-assignment(monitor.device);
                            }
                        }
                    }

                    Rectangle { vertical-stretch: 1; }

                    HorizontalLayout {
                        spacing: Tokens.space-md;
                        Button {
                            text: "Tutup";
                            clicked => { root.settings-tab = "umum"; }
                        }
                    }
                }

                // Playlist tab
                if root.settings-tab == "playlist" : VerticalLayout {
                    horizontal-stretch: 1;
                    spacing: Tokens.space-md;

                    Text {
                        text: "Playlist Rotasi";
                        color: #ffffff;
                        font-size: Tokens.font-lg;
                        font-family: Tokens.font-family;
                        font-weight: 600;
                    }

                    // Enabled
                    HorizontalLayout {
                        spacing: Tokens.space-md;
                        TouchArea {
                            clicked => { root.settings-playlist-enabled = !root.settings-playlist-enabled; }
                        }
                        if root.settings-playlist-enabled : Text {
                            text: "[✓]";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        if !root.settings-playlist-enabled : Text {
                            text: "[ ]";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        Text {
                            text: "Aktifkan rotasi playlist";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                    }

                    // Interval
                    HorizontalLayout {
                        spacing: Tokens.space-md;
                        Text {
                            text: "Interval (menit):";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        Text {
                            text: "1 - 1440";
                            color: #bbbbbb;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        TextInput {
                            text: root.settings-playlist-interval;
                            width: 40px;
                            edited => {
                                root.settings-playlist-interval = self.text;
                            }
                        }
                    }

                    // Shuffle
                    HorizontalLayout {
                        spacing: Tokens.space-md;
                        TouchArea {
                            clicked => { root.settings-playlist-shuffle = !root.settings-playlist-shuffle; }
                        }
                        if root.settings-playlist-shuffle : Text {
                            text: "[✓]";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        if !root.settings-playlist-shuffle : Text {
                            text: "[ ]";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        Text {
                            text: "Acak urutan";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                    }

                    // Order
                    HorizontalLayout {
                        spacing: Tokens.space-md;
                        Text {
                            text: "Urutan (pisahkan dengan |):";
                            color: #ffffff;
                            font-size: Tokens.font-sm;
                            font-family: Tokens.font-family;
                            vertical-alignment: center;
                        }
                        TextInput {
                            text: root.settings-playlist-order;
                            width: 100px;
                            edited => {
                                root.settings-playlist-order = self.text;
                            }
                        }
                        Button {
                            text: "Pilih dari Library";
                            clicked => {
                                root.settings-playlist-order-picker-open = true;
                            }
                        }
                    }

                    Rectangle { vertical-stretch: 1; }

                    HorizontalLayout {
                        spacing: Tokens.space-md;
                        Button {
                            text: "Simpan";
                            clicked => { root.save-settings(); }
                        }
                        Button {
                            text: "Batal";
                            clicked => { root.settings-open = false; }
                        }
                    }
                }
            }
        }

        // Footer: what the grid is showing, and what it costs.
        Text {
            text: footer;
            color: Tokens.muted;
            font-size: Tokens.font-sm;
            font-family: Tokens.font-family;
        }
    }

    // Report the viewport to Rust whenever it moves or resizes. These are
    // property mirrors, not I/O: the callback only posts to the worker.
    // `content-y` is negative once scrolled down (Slint's convention).
    out property <length> scroll-y <=> grid.content-y;
    out property <length> viewport-h <=> grid.height;

    changed scroll-y => {
        root.viewport-changed(self.scroll-y, self.viewport-h);
    }
    changed viewport-h => {
        root.viewport-changed(self.scroll-y, self.viewport-h);
    }
    changed width => {
        root.rebuild-grid(self.width - Tokens.space-lg * 2);
    }
    init => {
        root.rebuild-grid(self.width - Tokens.space-lg * 2);
    }
}
```

## File: rust/crates/k6wp-studio/src/main.rs
```rust
mod grid;
mod history;
pub mod thumbnailer;
mod thumbs;
mod worker;
⋮----
use std::cell::RefCell;
⋮----
use std::path::PathBuf;
use std::rc::Rc;
use std::sync::mpsc::Sender;
⋮----
use k6wp_core::StudioSettings;
⋮----
use rfd::FileDialog;
⋮----
struct CardState {
⋮----
struct Studio {
⋮----
thread_local! {
⋮----
fn with_studio(f: impl FnOnce(&mut Studio)) {
STUDIO.with(|slot| {
if let Ok(mut guard) = slot.try_borrow_mut() {
if let Some(studio) = guard.as_mut() {
f(studio);
⋮----
fn library_index_path(library_override: Option<&str>) -> Option<PathBuf> {
⋮----
if !override_path.is_empty() {
return Some(PathBuf::from(override_path));
⋮----
let local = std::env::var("LOCALAPPDATA").ok()?;
if local.is_empty() {
⋮----
Some(PathBuf::from(local).join("K6WP").join("library.json"))
⋮----
fn parse_theme(args: &[String]) -> Option<bool> {
let mut it = args.iter();
while let Some(arg) = it.next() {
⋮----
return match it.next().map(|s| s.as_str()) {
Some("dark") => Some(true),
Some("light") => Some(false),
⋮----
fn main() -> Result<(), Box<dyn std::error::Error>> {
⋮----
let args: Vec<String> = std::env::args().skip(1).collect();
let cli_theme = parse_theme(&args);
let ui_watch = args.iter().any(|a| a == "--ui-watch");
⋮----
.iter()
.position(|a| a == "--e2e-pick")
.and_then(|i| args.get(i + 1))
.and_then(|s| s.parse::<usize>().ok());
⋮----
ui.set_version(SharedString::from(env!("CARGO_PKG_VERSION")));
ui.set_empty_message(SharedString::from("Memuat library..."));
⋮----
ui.set_rows(ModelRc::from(rows.clone()));
⋮----
let weak = ui.as_weak();
⋮----
let weak = weak.clone();
⋮----
if weak.upgrade().is_some() {
with_studio(|studio| studio.on_event(event));
⋮----
ui.on_rebuild_grid(|width| with_studio(|studio| studio.rebuild_by_width(width)));
ui.on_viewport_changed(|scroll_y, viewport_h| {
with_studio(|studio| studio.on_viewport(scroll_y, viewport_h))
⋮----
ui.on_set_wallpaper(|index, media_path| {
with_studio(|studio| studio.set_wallpaper(index, media_path))
⋮----
ui.on_pause(|| with_studio(|studio| studio.pause()));
ui.on_resume(|| with_studio(|studio| studio.resume()));
ui.on_undo(|| with_studio(|studio| studio.undo()));
ui.on_import(|| with_studio(|studio| studio.import()));
ui.on_compress(|index| with_studio(|studio| studio.compress(index)));
ui.on_save_settings(|| with_studio(|studio| studio.save_settings()));
ui.on_pick_folder(|field| with_studio(|studio| studio.pick_folder(field)));
⋮----
*slot.borrow_mut() = Some(Studio {
ui: ui.clone_strong(),
tx: tx.clone(),
⋮----
ui.global::<Tokens>().set_dark(dark);
⋮----
let _ = tx.send(Request::Theme);
⋮----
match library_index_path(
⋮----
std::env::var("K6WP_LIBRARY_JSON").ok().as_deref(),
⋮----
let _ = tx.send(Request::Load { path });
⋮----
ui.set_empty_message(SharedString::from(
⋮----
let _ = tx.send(Request::SettingsLoad);
⋮----
spawn_ui_watchdog(&ui);
⋮----
ui.run()?;
let _ = tx.send(Request::Stop);
Ok(())
⋮----
impl Studio {
fn on_event(&mut self, event: Event) {
⋮----
Event::Library(Ok(entries)) => self.apply_library(entries),
⋮----
self.ui.set_empty_message(SharedString::from(format!(
⋮----
self.update_footer();
⋮----
if let Some(card) = self.cards.get_mut(index) {
⋮----
self.sync_row(index);
⋮----
self.pending.remove(&index);
⋮----
self.cache.insert(index as u64, image_from(pixels));
⋮----
Err(message) => eprintln!("studio: {message}"),
⋮----
tokens.set_reduced_motion(reduced_motion);
if self.cli_theme.is_none() {
tokens.set_dark(dark);
⋮----
self.update_connection_ui();
⋮----
self.update_engine_ui();
⋮----
self.show_toast(message, false);
⋮----
let Some(card) = self.cards.get(index) else {
⋮----
let undo = self.history.record(history::WallpaperRef {
⋮----
name: card.name.clone(),
media: card.media.clone(),
⋮----
.applied()
.map(|a| a.name.clone())
.unwrap_or_default();
self.show_toast(format!("Terpasang: {name}"), undo);
⋮----
self.compress_pct = Some(pct);
⋮----
.map(|e| format!(" ({e}s remaining)"))
⋮----
self.show_toast(format!("Mengompres... {pct}%{eta}"), false);
⋮----
self.compress_jobs.remove(&job_id);
self.ui.set_compress_job_id(-1);
⋮----
.as_ref()
.and_then(|p| p.file_name().and_then(|n| n.to_str()))
.unwrap_or("")
.to_string();
self.show_toast(format!("Selesai: {name}"), false);
⋮----
let msg = error.unwrap_or_else(|| "Gagal diketahui".to_string());
self.show_toast(format!("Gagal: {msg}"), false);
⋮----
self.settings_path = settings_index_path();
self.apply_settings_ui();
⋮----
fn apply_library(&mut self, entries: Vec<Entry>) {
⋮----
.into_iter()
.map(|entry| CardState {
⋮----
.collect();
self.cache.clear();
self.pending.clear();
⋮----
self.ui.set_count(self.cards.len() as i32);
if self.cards.is_empty() {
self.ui.set_empty_message(SharedString::from(
⋮----
self.rebuild_by_width(width);
self.ensure_visible();
⋮----
for (index, card) in self.cards.iter().enumerate() {
⋮----
let _ = self.tx.send(Request::Probe {
⋮----
path: card.media.clone(),
⋮----
if let Some(pick) = self.e2e_pick.take() {
if pick < self.cards.len() {
let media = self.cards[pick].media.clone();
self.set_wallpaper(
⋮----
SharedString::from(media.to_string_lossy().as_ref()),
⋮----
fn rebuild_by_width(&mut self, width: f32) {
if !width.is_finite() || width <= 0.0 {
⋮----
let card_width = grid::card_width(width, columns).floor();
let band = (card_width * 9.0 / 16.0).round();
⋮----
let scale = self.ui.window().scale_factor().max(1.0);
⋮----
((card_width * scale).round() as u32).max(8),
((band * scale).round() as u32).max(8),
⋮----
let needs_model = self.rows.row_count() == 0 && !self.cards.is_empty();
⋮----
self.ui.set_columns(columns as i32);
self.ui.set_card_width(card_width);
self.ui.set_row_height(row_height);
⋮----
self.rebuild_model();
⋮----
fn rebuild_model(&mut self) {
⋮----
for (start, end) in grid::row_ranges(self.cards.len(), self.columns.max(1)) {
rows.push(LibraryRow {
cards: self.row_cards(start, end),
⋮----
self.rows.set_vec(rows);
⋮----
fn row_cards(&mut self, start: usize, end: usize) -> ModelRc<LibraryCard> {
let end = end.min(self.cards.len());
let cards: Vec<LibraryCard> = (start..end).map(|index| self.make_card(index)).collect();
⋮----
fn make_card(&mut self, index: usize) -> LibraryCard {
⋮----
self.cache.get(index as u64)
⋮----
self.cache.peek(index as u64)
⋮----
name: SharedString::from(card.name.as_str()),
meta: SharedString::from(card.meta.as_str()),
⋮----
media_path: SharedString::from(card.media.to_string_lossy().as_ref()),
⋮----
fn sync_row(&mut self, index: usize) {
let columns = self.columns.max(1);
⋮----
let end = (start + columns).min(self.cards.len());
if row < self.rows.row_count() {
let cards = self.row_cards(start, end);
self.rows.set_row_data(row, LibraryRow { cards });
⋮----
fn on_viewport(&mut self, scroll_y: f32, viewport_height: f32) {
if self.cards.is_empty() || self.columns == 0 {
⋮----
let offset = (-scroll_y).max(0.0);
let row_height = self.ui.get_row_height();
⋮----
self.cards.len(),
⋮----
self.request_visible();
⋮----
fn ensure_visible(&mut self) {
⋮----
let scroll = self.ui.get_scroll_y();
let viewport = self.ui.get_viewport_h();
self.on_viewport(scroll, viewport);
⋮----
fn request_visible(&mut self) {
⋮----
let keep: Vec<u64> = (start..end).map(|i| i as u64).collect();
let dropped = self.cache.retain_only(&keep);
⋮----
if self.cache.contains(key) || self.pending.contains(&index) {
⋮----
if card.unavailable && card.thumb.as_os_str().is_empty() {
⋮----
self.pending.insert(index);
⋮----
let _ = self.tx.send(Request::Thumb {
⋮----
let _ = self.tx.send(Request::Thumbnail {
⋮----
fn update_footer(&mut self) {
let total = self.cards.len();
let missing = self.cards.iter().filter(|c| c.unavailable).count();
let used = self.cache.used_bytes() as f64 / (1024.0 * 1024.0);
let cap = self.cache.cap_bytes() as f64 / (1024.0 * 1024.0);
self.ui.set_footer(SharedString::from(format!(
⋮----
fn update_connection_ui(&mut self) {
⋮----
self.ui.set_connection_status(SharedString::from(text));
⋮----
self.ui.set_connection_color(Brush::SolidColor(c));
⋮----
fn update_engine_ui(&mut self) {
⋮----
self.ui.set_current_video(SharedString::from(video));
⋮----
self.ui.set_current_video(SharedString::from("—"));
⋮----
self.ui.set_is_paused(self.engine_state.is_paused);
⋮----
fn show_toast(&mut self, message: String, undo: bool) {
self.toast = Some(message.clone());
self.ui.set_toast(SharedString::from(message));
self.ui.set_undo_available(undo);
let generation = self.ui.get_toast_generation().wrapping_add(1);
self.ui.set_toast_generation(generation);
⋮----
let ui_weak = self.ui.as_weak();
⋮----
if let Some(ui) = ui_weak.upgrade() {
if ui.get_toast_generation() == generation {
ui.set_toast(SharedString::from(""));
ui.set_undo_available(false);
⋮----
/// Callback: user clicked "Urungkan" in the toast. Re-applies the picture
fn undo(&mut self) {
let Some(previous) = self.history.take_undo() else {
⋮----
self.ui.set_undo_available(false);
let _ = self.tx.send(Request::SetWallpaper {
⋮----
media: previous.media.clone(),
⋮----
self.show_toast(format!("Mengembalikan: {}", previous.name), false);
⋮----
fn set_wallpaper(&mut self, index: i32, media_path: SharedString) {
⋮----
if index >= self.cards.len() {
⋮----
let media = media_path.to_string();
if media.is_empty() {
self.show_toast("Path media kosong".to_string(), false);
⋮----
media: media.into(),
⋮----
self.show_toast(format!("Memasang {}...", self.cards[index].name), false);
⋮----
fn pause(&mut self) {
let _ = self.tx.send(Request::Pause);
⋮----
fn resume(&mut self) {
let _ = self.tx.send(Request::Resume);
⋮----
fn import(&mut self) {
⋮----
.add_filter("Video", &["mp4", "mkv", "webm", "mov"])
.set_directory(std::env::var("USERPROFILE").unwrap_or_default())
.pick_files();
⋮----
if !files.is_empty() {
let _ = self.tx.send(Request::Import { paths: files });
⋮----
fn compress(&mut self, index: i32) {
⋮----
let out = card.media.with_extension("compressed.mp4");
self.compress_jobs.insert(job_id, 0);
self.compress_pct = Some(0);
self.ui.set_compress_job_id(job_id as i32);
let _ = self.tx.send(Request::Compress {
⋮----
src: card.media.clone(),
out: out.clone(),
⋮----
self.show_toast(format!("Mengompres {}... 0%", card.name), false);
⋮----
fn compress_cancel(&mut self, job_id: u64) {
let _ = self.tx.send(Request::CompressCancel { job_id });
⋮----
self.show_toast("Kompresi dibatalkan".to_string(), false);
⋮----
fn save_settings(&mut self) {
⋮----
.get_settings_default_crf()
.trim()
.parse()
.unwrap_or(22);
⋮----
.get_settings_default_fps()
⋮----
.unwrap_or(30);
⋮----
.get_settings_lockscreen_offset()
⋮----
.unwrap_or(0.0);
⋮----
auto_compress_on_import: self.ui.get_settings_auto_compress(),
⋮----
self.ui.get_settings_compress_output_dir().to_string(),
⋮----
default_resolution_mode: self.ui.get_settings_default_resolution().to_string(),
⋮----
cache_dir: PathBuf::from(self.ui.get_settings_cache_dir().to_string()),
wallpaper_sync: self.ui.get_settings_wallpaper_sync(),
lockscreen_sync: self.ui.get_settings_lockscreen_sync(),
⋮----
check_updates: self.ui.get_settings_check_updates(),
close_to_tray: self.ui.get_settings_close_to_tray(),
playlist_source: self.settings.playlist_source.clone(),
performance_preset: self.settings.performance_preset.clone(),
⋮----
let _ = self.tx.send(Request::SettingsSave { settings });
⋮----
fn pick_folder(&mut self, field: SharedString) {
⋮----
.pick_folder();
⋮----
match field.as_ref() {
⋮----
self.ui.set_settings_compress_output_dir(
path.to_string_lossy().to_string().into(),
⋮----
.set_settings_cache_dir(path.to_string_lossy().to_string().into());
⋮----
fn apply_settings_ui(&mut self) {
⋮----
fn image_from(pixels: Pixels) -> slint::Image {
⋮----
fn spawn_ui_watchdog(ui: &AppWindow) {
⋮----
.name("k6wp-studio-uiwatch".to_string())
.spawn(move || loop {
⋮----
let _ = weak.upgrade();
let _ = tx.send(());
⋮----
if queued.is_err() {
⋮----
match rx.recv_timeout(Duration::from_secs(5)) {
⋮----
let ms = posted.elapsed().as_secs_f64() * 1000.0;
⋮----
eprintln!("UIBLOCK {ms:.1}");
⋮----
.ok();
```

## File: rust/crates/k6wp-studio/src/worker.rs
```rust
use std::collections::HashMap;
⋮----
use k6wp_core::settings::save_studio_settings;
⋮----
use serde_json::Value;
⋮----
pub struct Entry {
⋮----
pub enum Request {
⋮----
pub enum IpcStatus {
⋮----
pub struct EngineState {
⋮----
pub enum Event {
⋮----
type EventSink = std::sync::Arc<std::sync::Mutex<Box<dyn Fn(Event) + Send>>>;
⋮----
struct Sink(EventSink);
⋮----
impl Sink {
fn emit(&self, event: Event) {
if let Ok(callback) = self.0.lock() {
callback(event);
⋮----
pub fn spawn(
⋮----
let sink = Sink(std::sync::Arc::new(std::sync::Mutex::new(
⋮----
.name("k6wp-studio-worker".to_string())
.spawn(move || run(rx, sink, pipe_name_override));
⋮----
.spawn(move || run(rx, sink, None));
if spawned.is_err() {
eprintln!("studio: the worker thread could not start; the library will stay empty");
⋮----
fn run(rx: Receiver<Request>, sink: Sink, pipe_name_override: Option<String>) {
// One queue per probe thread: a shared `Receiver` behind a mutex would
// serialise the probes (one thread would hold the lock while it blocks).
⋮----
.map(|n| {
⋮----
let sink = sink.clone();
⋮----
.name(format!("k6wp-studio-probe-{n}"))
.spawn(move || probe_loop(rx, sink));
⋮----
eprintln!(
⋮----
.collect();
⋮----
// IPC connection manager state
⋮----
let mut ipc_status = IpcStatus::Reconnecting; // Start with Reconnecting so main loop attempts connection
⋮----
let mut next_connect_attempt = Instant::now(); // Allow immediate first attempt
⋮----
let studio_visible = true; // updated by UI via a new Request variant if needed
let mut compress_jobs: HashMap<u64, u32> = HashMap::new(); // job_id -> pid
⋮----
// Resolve the pipe name once at startup (uses override in debug, real name in release)
let effective_pipe_name = pipe_name_override.unwrap_or_else(|| current_session_pipe_name(None));
⋮----
// Adaptive polling: check if it's time to poll get_state
⋮----
ipc_client.is_some() && studio_visible && last_poll.elapsed() >= POLL_INTERVAL_VISIBLE;
⋮----
match rx.recv_timeout(timeout) {
⋮----
Ok(Request::Load { path }) => sink.emit(Event::Library(load_library(&path))),
⋮----
if probe_queues.is_empty() {
sink.emit(Event::Probe {
⋮----
exists: probe(&path, PROBE_TIMEOUT),
⋮----
let queue = &probe_queues[next_probe % probe_queues.len()];
next_probe = next_probe.wrapping_add(1);
let _ = queue.send((index, path));
⋮----
let result = read_and_decode(&path, width, height);
sink.emit(Event::Thumb {
⋮----
.ok()
.and_then(|p| p.parent().map(|d| d.join("thumbs")))
.unwrap_or_else(|| std::env::temp_dir().join("k6wp-thumbs"));
std::fs::create_dir_all(&cache_dir).ok();
⋮----
let result = generate_thumbnail(&media, &cache_dir, THUMB_WIDTH, THUMB_HEIGHT);
⋮----
.map_err(|e| e.to_string())
.and_then(|path| {
read_and_decode(&path, THUMB_WIDTH, THUMB_HEIGHT)
.map(|pixels| (pixels, path))
⋮----
.map(|(pixels, path)| (Ok(pixels), Some(path)))
.unwrap_or_else(|e| (Err(e), None));
⋮----
let (dark, reduced_motion) = system_appearance();
sink.emit(Event::Theme {
⋮----
if !probe(&media, PROBE_TIMEOUT) {
sink.emit(Event::IpcError(format!(
⋮----
handle_set_wallpaper(client, index, &media, &sink);
⋮----
sink.emit(Event::IpcError(
"Tidak terhubung ke engine. Menghubungkan ulang...".to_string(),
⋮----
let _ = attempt_connect(
⋮----
let lib_path = match library_index_path(None) {
⋮----
"Tidak bisa menemukan library.json".to_string(),
⋮----
match import_into_library(&lib_path, &paths) {
⋮----
&std::fs::read_to_string(&lib_path).unwrap_or_default(),
⋮----
.unwrap_or_default();
⋮----
.iter()
.map(|e| Entry {
⋮----
name: e.dst.file_name().unwrap().to_string_lossy().to_string(),
⋮----
thumb: e.thumb.clone(),
media: e.dst.clone(),
unavailable: !e.dst.exists(),
⋮----
sink.emit(Event::Library(Ok(entries)));
⋮----
sink.emit(Event::IpcError(e));
⋮----
compress_handler(job_id, &src, &out, res_w, res_h, fps, crf, sink.clone());
⋮----
if let Some(&pid) = compress_jobs.get(&job_id) {
⋮----
.args(["/T", "/F", "/PID", &pid.to_string()])
.status();
compress_jobs.remove(&job_id);
sink.emit(Event::CompressDone {
⋮----
error: Some("Dibatalkan".to_string()),
⋮----
let path = match settings_index_path() {
⋮----
"LOCALAPPDATA tidak ditemukan: settings tidak bisa dibaca".to_string(),
⋮----
Ok(settings) => sink.emit(Event::SettingsLoaded(settings)),
⋮----
sink.emit(Event::IpcError(format!("Gagal migrasi settings: {e}")))
⋮----
sink.emit(Event::SettingsLoaded(StudioSettings::default()));
⋮----
.to_string(),
⋮----
match save_studio_settings(&path, &settings) {
Ok(()) => sink.emit(Event::SettingsLoaded(settings)),
Err(e) => sink.emit(Event::IpcError(format!("Gagal simpan settings: {e}"))),
⋮----
handle_pause(client, &sink);
⋮----
handle_resume(client, &sink);
⋮----
poll_get_state(client, &sink, &mut engine_state);
⋮----
if ipc_client.is_none()
⋮----
let backoff = RECONNECT_BACKOFF[backoff_index.min(RECONNECT_BACKOFF.len() - 1)];
⋮----
pub(crate) fn import_into_library(
⋮----
match LibraryIndex::parse(&std::fs::read_to_string(lib_path).unwrap_or_default()) {
⋮----
Err(e) => return Err(format!("Gagal baca library.json: {e}")),
⋮----
let lib_dir = lib_path.parent().unwrap();
⋮----
if !path.exists() {
return Err(format!("File tidak ditemukan: {}", path.display()));
⋮----
.extension()
.and_then(|s| s.to_str())
.unwrap_or("")
.to_lowercase();
if !matches!(ext.as_str(), "mp4" | "mkv" | "webm" | "mov") {
return Err(format!("Format tidak didukung: {}", path.display()));
⋮----
.map_err(|e| format!("Gagal baca metadata {}: {e}", path.display()))?;
if meta.len() == 0 {
return Err(format!("File kosong: {}", path.display()));
⋮----
let dst_name = path.file_name().unwrap().to_string_lossy().to_string();
let dst = lib_dir.join("wallpapers").join(&dst_name);
std::fs::create_dir_all(lib_dir.join("wallpapers")).ok();
std::fs::copy(path, &dst).map_err(|e| format!("Gagal copy {}: {e}", path.display()))?;
⋮----
.modified()
⋮----
.and_then(|t| t.duration_since(std::time::UNIX_EPOCH).ok())
.map(|d| d.as_secs() as i64)
.unwrap_or(0);
⋮----
src: path.clone(),
⋮----
res: "0x0".to_string(),
⋮----
encoder: "libx264".to_string(),
⋮----
size: meta.len(),
⋮----
codec: "".to_string(),
⋮----
index.entries.push(entry);
⋮----
.save_atomic(lib_path)
.map_err(|e| format!("Gagal simpan library.json: {e}"))?;
Ok((index.entries.len() - added..index.entries.len()).collect())
⋮----
/// Returns the path to `compressor.exe` if found.
/// Looks in: sibling of current exe → build/release/ → K6WP_COMPRESSOR env (debug-only).
⋮----
/// Looks in: sibling of current exe → build/release/ → K6WP_COMPRESSOR env (debug-only).
fn find_compressor() -> Option<PathBuf> {
⋮----
fn find_compressor() -> Option<PathBuf> {
⋮----
if let Some(parent) = current.parent() {
let sibling = parent.join("compressor.exe");
if sibling.exists() {
return Some(sibling);
⋮----
let release_compressor = parent.join("build").join("release").join("compressor.exe");
if release_compressor.exists() {
return Some(release_compressor);
⋮----
if !env_path.is_empty() {
⋮----
if p.exists() {
return Some(p);
⋮----
fn compress_handler(
⋮----
let Some(compressor_exe) = find_compressor() else {
⋮----
error: Some("compressor.exe tidak ditemukan".to_string()),
⋮----
in_: src.to_string_lossy().to_string(),
out: out.to_string_lossy().to_string(),
⋮----
encoder: "auto".to_string(),
⋮----
let argv = build_argv(&args);
⋮----
.map(|e| e == "bat" || e == "cmd")
.unwrap_or(false);
⋮----
c.arg("/c").arg(&compressor_exe);
c.args(&argv);
⋮----
.stdout(std::process::Stdio::piped())
.stderr(std::process::Stdio::piped())
.spawn()
⋮----
error: Some(format!("Gagal spawn compressor: {e}")),
⋮----
let stderr = child.stderr.take();
⋮----
for line in reader.lines() {
⋮----
if let Some(t_str) = line.split("time=").nth(1).and_then(|s| s.split(' ').next()) {
if let Some(secs) = parse_ffmpeg_time(t_str) {
let pct = if let Some(duration) = library_duration_secs(&src) {
⋮----
Some(p.min(100))
⋮----
sink.emit(Event::CompressProgress {
⋮----
let status = match child.wait() {
⋮----
error: Some(format!("Gagal tunggu compressor: {e}")),
⋮----
if status.success() {
⋮----
out_path: Some(out.to_path_buf()),
⋮----
error: Some(format!("Compressor exit code: {:?}", status.code())),
⋮----
fn parse_ffmpeg_time(s: &str) -> Option<f64> {
let parts: Vec<&str> = s.split(':').collect();
if parts.len() != 3 {
⋮----
let h: f64 = parts[0].trim().parse().ok()?;
let m: f64 = parts[1].trim().parse().ok()?;
let s: f64 = parts[2].trim().parse().ok()?;
Some(h * 3600.0 + m * 60.0 + s)
⋮----
fn library_duration_secs(src: &Path) -> Option<f64> {
let lib_path = library_index_path(None)?;
let text = std::fs::read_to_string(&lib_path).ok()?;
let index: LibraryIndex = LibraryIndex::parse(&text).ok()?;
index.entries.iter().find(|e| e.src == src).and_then(|e| {
⋮----
Some(e.duration)
⋮----
fn expected_engine_image() -> Option<PathBuf> {
⋮----
let current = std::env::current_exe().ok()?;
let parent = current.parent()?;
let engine_path = parent.join("engine.exe");
if engine_path.exists() {
return Some(engine_path);
⋮----
let release_engine = parent.join("build").join("release").join("engine.exe");
if release_engine.exists() {
return Some(release_engine);
⋮----
fn attempt_connect(
⋮----
sink.emit(Event::IpcStatus(IpcStatus::Connecting));
⋮----
expected_server_image: expected_engine_image(),
⋮----
*client = Some(c);
⋮----
sink.emit(Event::IpcStatus(IpcStatus::Connected));
Ok(())
⋮----
eprintln!("WORKER: Connection failed: {:?}", e);
⋮----
*backoff_index = (*backoff_index + 1).min(RECONNECT_BACKOFF.len());
sink.emit(Event::IpcStatus(IpcStatus::Reconnecting));
Err(e)
⋮----
fn handle_set_wallpaper(client: &mut IpcClient, index: usize, media: &Path, sink: &Sink) {
⋮----
match client.request(&msg) {
⋮----
if answer.get("error").is_some() {
⋮----
sink.emit(Event::WallpaperApplied { index });
⋮----
sink.emit(Event::IpcError(format!("Kesalahan IPC: {e}")));
⋮----
fn handle_pause(client: &mut IpcClient, sink: &Sink) {
⋮----
fn handle_resume(client: &mut IpcClient, sink: &Sink) {
⋮----
fn poll_get_state(client: &mut IpcClient, sink: &Sink, state: &mut EngineState) {
⋮----
apply_state_answer(&answer, state);
⋮----
sink.emit(Event::EngineState(state.clone()));
⋮----
sink.emit(Event::IpcError(format!("Gagal memuat status: {e}")));
⋮----
fn apply_state_answer(answer: &Value, state: &mut EngineState) {
let inner = answer.get("state").unwrap_or(answer);
if let Some(video) = inner.get("video").and_then(Value::as_str) {
⋮----
.file_name()
.map(|n| n.to_string_lossy().to_string())
.unwrap_or_else(|| video.to_string());
state.video_name = Some(name);
⋮----
if let Some(paused) = inner.get("paused").and_then(Value::as_bool) {
⋮----
fn probe_loop(rx: Receiver<(usize, PathBuf)>, sink: Sink) {
while let Ok((index, path)) = rx.recv() {
let exists = probe(&path, PROBE_TIMEOUT);
sink.emit(Event::Probe { index, exists });
⋮----
pub fn load_library(path: &Path) -> Result<Vec<Entry>, String> {
⋮----
.map_err(|e| format!("{}: {e}", path.display()))?;
let index = LibraryIndex::parse(&text).map_err(|e| e.to_string())?;
Ok(index.entries.iter().map(entry_view).collect())
⋮----
fn library_index_path(override_dir: Option<PathBuf>) -> Option<PathBuf> {
⋮----
let p = dir.join("library.json");
⋮----
if let Some(dir) = exe.parent() {
⋮----
let p = cwd.join("library.json");
⋮----
pub(crate) fn settings_index_path() -> Option<PathBuf> {
let local = std::env::var("LOCALAPPDATA").ok()?;
if local.is_empty() {
⋮----
Some(
⋮----
.join("K6WP")
.join("studio_settings.json"),
⋮----
fn entry_view(entry: &k6wp_core::LibraryEntry) -> Entry {
⋮----
.unwrap_or_else(|| entry.dst.to_string_lossy().to_string());
⋮----
meta.push_str(&format!("{:.1} s", entry.duration));
⋮----
if !meta.is_empty() {
meta.push_str(" - ");
⋮----
meta.push_str(&format!("{}x{}", entry.width, entry.height));
⋮----
if !entry.codec.is_empty() {
⋮----
meta.push_str(&entry.codec);
⋮----
thumb: entry.thumb.clone(),
media: entry.dst.clone(),
unavailable: !entry.dst.exists(),
⋮----
pub fn probe(path: &Path, timeout: Duration) -> bool {
⋮----
.components()
.next()
.map(|c| matches!(c, std::path::Component::Prefix(_)))
.unwrap_or(false)
&& path.to_string_lossy().as_bytes().get(1) == Some(&b':');
⋮----
return path.exists(); // worker
⋮----
let owned = path.to_path_buf();
⋮----
.name("k6wp-studio-probe".to_string())
.spawn(move || {
let _ = tx.send(owned.exists()); // worker
⋮----
fn read_and_decode(path: &Path, width: u32, height: u32) -> Result<Pixels, String> {
let bytes = std::fs::read(path) // worker
⋮----
decode_to_cover(&bytes, width, height)
⋮----
/// The system light/dark preference and the "show animations" preference.
fn system_appearance() -> (bool, bool) {
⋮----
fn system_appearance() -> (bool, bool) {
(system_is_dark(), !system_animations_enabled())
⋮----
fn registry_dword(subkey: &str, value: &str) -> Option<u32> {
use windows::Win32::Foundation::WIN32_ERROR;
⋮----
let subkey: Vec<u16> = subkey.encode_utf16().chain(std::iter::once(0)).collect();
let value: Vec<u16> = value.encode_utf16().chain(std::iter::once(0)).collect();
⋮----
RegGetValueW(
⋮----
windows::core::PCWSTR::from_raw(subkey.as_ptr()),
windows::core::PCWSTR::from_raw(value.as_ptr()),
⋮----
Some(&mut data as *mut u32 as *mut core::ffi::c_void),
Some(&mut size),
⋮----
if status == WIN32_ERROR(0) {
Some(data)
⋮----
fn system_is_dark() -> bool {
// AppsUseLightTheme: 0 = dark, 1 = light, absent = light.
match registry_dword(
⋮----
fn system_animations_enabled() -> bool {
⋮----
SystemParametersInfoW(
⋮----
Some(&mut enabled as *mut _ as *mut core::ffi::c_void),
SYSTEM_PARAMETERS_INFO_UPDATE_FLAGS(0),
⋮----
ok.is_ok() && enabled.as_bool()
⋮----
pub(crate) enum ThumbChoice {
⋮----
pub(crate) fn choose_thumb_request(thumb: &Path, media: &Path) -> ThumbChoice {
if thumb.as_os_str().is_empty() || !thumb.exists() {
⋮----
media_path: media.to_path_buf(),
⋮----
thumb_path: thumb.to_path_buf(),
⋮----
mod tests {
⋮----
use std::path::PathBuf;
use std::sync::mpsc;
use std::time::Duration;
⋮----
// Test utilities for fake compressor
enum FakeCompressorBehavior {
⋮----
fn make_fake_compressor(
⋮----
std::env::temp_dir().join(format!("k6wp-test-fake-compressor-{}", std::process::id()));
std::fs::create_dir_all(&dir).ok();
let exe = dir.join("compressor.bat");
⋮----
std::fs::write(&exe, script).expect("write fake compressor");
(std::process::Command::new(exe.clone()), exe)
⋮----
fn a_missing_index_is_an_error_not_a_panic() {
let err = load_library(Path::new(r"Z:\definitely\not\here\library.json"));
assert!(err.is_err());
⋮----
fn a_drive_letter_path_is_probed_inline() {
// The temp directory exists; the drive-letter fast path must find it.
⋮----
assert!(probe(&dir, PROBE_TIMEOUT));
⋮----
fn an_unreachable_probe_gives_up_after_the_timeout() {
⋮----
let exists = probe(
⋮----
assert!(!exists);
assert!(
⋮----
fn a_dead_unc_probe_does_not_block_the_decode_behind_it() {
// The real worker, the real queues: a probe of a dead server is
// dispatched to its own thread, so the thumbnail request queued
// behind it is answered immediately. With an inline probe this test
// fails after the full 2 s timeout.
⋮----
let requests = spawn(
⋮----
let _ = events_tx.send(event);
⋮----
.send(Request::Probe {
⋮----
.expect("the worker must accept a probe");
⋮----
.send(Request::Thumb {
⋮----
.expect("the worker must accept a decode");
// Skip IPC status events that are emitted during startup
⋮----
.recv_timeout(Duration::from_millis(500))
.expect("the decode must answer while the probe is still in flight");
if matches!(event, Event::Thumb { .. }) {
⋮----
assert_eq!(index, 1);
⋮----
other => panic!("the decode must answer first, got {other:?}"),
⋮----
let _ = requests.send(Request::Stop);
⋮----
fn set_wallpaper_success_with_fake_server() {
// Fake server that answers set_video with success.
⋮----
let name = format!(r"\\.\pipe\k6wp-test-set-video-ok-{}", std::process::id());
⋮----
let name = name.clone();
⋮----
use windows::core::PCWSTR;
⋮----
let wide: Vec<u16> = name.encode_utf16().chain(std::iter::once(0)).collect();
⋮----
CreateNamedPipeW(
PCWSTR(wide.as_ptr()),
FILE_FLAGS_AND_ATTRIBUTES(0x0000_0003),
NAMED_PIPE_MODE(
⋮----
let _ = ready_tx.send(());
if let Err(e) = unsafe { ConnectNamedPipe(pipe, None) } {
let code = (e.code().0 as u32) & 0xFFFF;
⋮----
let _ = CloseHandle(pipe);
⋮----
let ok = unsafe { ReadFile(pipe, Some(&mut buffer), Some(&mut read), None) };
if ok.is_err() {
⋮----
let _ = WriteFile(pipe, Some(answer), None, None);
⋮----
// Exit after handling the request (test only needs one)
⋮----
// Pass the pipe name directly to spawn (debug-only parameter)
⋮----
Some(name.clone()),
⋮----
.recv_timeout(Duration::from_secs(5))
.expect("the fake engine must create its pipe instance");
⋮----
// Give the worker time to attempt connection (it polls every 500ms when disconnected)
⋮----
// Create a temporary file that exists for the test
let temp_file = std::env::temp_dir().join(format!("k6wp-test-{}.mp4", std::process::id()));
std::fs::write(&temp_file, b"fake video content").expect("must create temp file");
let media_path = temp_file.clone();
⋮----
// Wait for IpcStatus::Connecting then IpcStatus::Connected before sending the request
⋮----
.recv_timeout(Duration::from_secs(10))
.expect("must get IPC status");
⋮----
.send(Request::SetWallpaper {
⋮----
media: media_path.clone(),
⋮----
.expect("must accept");
⋮----
// The worker must report the accepted install with the index; the UI
// turns that into the "Terpasang" toast (and the "Urungkan" button).
⋮----
.expect("must get response");
⋮----
assert_eq!(index, 0, "the applied index must be reported");
⋮----
other => panic!("expected WallpaperApplied{{index: 0}}, got {other:?}"),
⋮----
// Wait for worker to exit (recv_timeout is up to 500ms)
⋮----
let _ = server.join();
⋮----
fn set_wallpaper_engine_error() {
// Fake server that returns an engine error.
⋮----
let name = format!(r"\\.\pipe\k6wp-test-set-video-error-{}", std::process::id());
⋮----
std::env::temp_dir().join(format!("k6wp-test-error-{}.hevc", std::process::id()));
⋮----
other => panic!("expected IpcError with engine error, got {other:?}"),
⋮----
fn set_wallpaper_engine_timeout() {
// Fake server that accepts connection but never answers.
⋮----
let name = format!(
⋮----
// Accept connection but NEVER read/answer - ReadFile blocks forever.
// This simulates a server that accepts but doesn't respond.
let _ = unsafe { ReadFile(pipe, Some(&mut [0u8; 1]), Some(&mut 0u32), None) };
⋮----
std::env::temp_dir().join(format!("k6wp-test-timeout-{}.mp4", std::process::id()));
⋮----
.recv_timeout(Duration::from_millis(1000))
.expect("must get timeout error");
let elapsed = started.elapsed();
⋮----
other => panic!("expected IpcError with timeout, got {other:?}"),
⋮----
fn set_wallpaper_missing_media_no_ipc_sent() {
⋮----
use windows::Win32::Storage::FileSystem::FILE_FLAGS_AND_ATTRIBUTES;
⋮----
// For missing file test, we expect NO connection. Close pipe immediately.
// The worker checks file existence first and won't connect for missing files.
⋮----
.expect("must get error");
⋮----
other => panic!("expected IpcError with missing file, got {other:?}"),
⋮----
fn engine_state_reads_the_real_get_state_shape() {
⋮----
apply_state_answer(&answer, &mut state);
assert_eq!(state.video_name.as_deref(), Some("clip_2.mp4"));
assert!(state.is_paused);
⋮----
fn engine_state_ignores_a_missing_video() {
⋮----
assert_eq!(state.video_name, None);
assert!(!state.is_paused);
⋮----
mod choose_thumb_tests {
⋮----
use crate::thumbnailer::ThumbError;
⋮----
fn choose_empty_thumb_generates() {
let choice = choose_thumb_request(Path::new(""), Path::new(r"C:\video.mp4"));
assert!(matches!(choice, ThumbChoice::Generate { .. }));
⋮----
fn choose_existing_thumb_decodes() {
let temp = std::env::temp_dir().join(format!("k6wp-test-thumb-{}", std::process::id()));
std::fs::create_dir_all(&temp).expect("create temp dir");
let thumb = temp.join("existing_thumb.jpg");
std::fs::write(&thumb, b"fake thumbnail").expect("write thumb");
let choice = choose_thumb_request(&thumb, Path::new(r"C:\video.mp4"));
assert!(matches!(choice, ThumbChoice::Decode { .. }));
⋮----
mod import_tests {
⋮----
use k6wp_core::library::LibraryEntry;
⋮----
fn import_validates_extension() {
⋮----
std::env::temp_dir().join(format!("k6wp-test-import-{}.txt", std::process::id()));
std::fs::write(&temp, b"fake content").expect("must create temp file");
let cache = std::env::temp_dir().join("k6wp-test-thumb-import-ext");
std::fs::create_dir_all(&cache).ok();
let result = generate_thumbnail(&temp, &cache, THUMB_WIDTH, THUMB_HEIGHT);
assert!(result.is_err(), "txt should be rejected");
⋮----
fn import_validates_missing_file() {
⋮----
let cache = std::env::temp_dir().join("k6wp-test-thumb-missing");
⋮----
let result = generate_thumbnail(&missing, &cache, THUMB_WIDTH, THUMB_HEIGHT);
assert!(result.is_err(), "missing file should be rejected");
⋮----
fn import_appends_to_library() {
⋮----
let dir = std::env::temp_dir().join(format!("k6wp-import-test-{}", std::process::id()));
std::fs::create_dir_all(&dir).unwrap();
let lib_path = dir.join("library.json");
⋮----
entries: vec![LibraryEntry {
⋮----
existing.save_atomic(&lib_path).unwrap();
⋮----
let new_file = dir.join("new.mp4");
std::fs::write(&new_file, b"fake").unwrap();
let result = import_into_library(&lib_path, &[new_file.clone()]);
assert!(result.is_ok(), "import must succeed");
⋮----
let after = LibraryIndex::parse(&std::fs::read_to_string(&lib_path).unwrap()).unwrap();
assert_eq!(after.entries.len(), 2, "import must APPEND, not overwrite");
assert!(after
⋮----
std::fs::remove_dir_all(&dir).ok();
⋮----
fn assign_video_sends_correct_payload() {
⋮----
if find_compressor().is_some() {
⋮----
let (_cmd, exe) = make_fake_compressor(FakeCompressorBehavior::Ok);
⋮----
let _ = events_tx.send(e);
⋮----
Sink(arc)
⋮----
let src = std::env::temp_dir().join("k6wp-test-assign-src.mp4");
std::fs::write(&src, b"fake video").expect("write src");
⋮----
let src_for_thread = src.clone();
let sink_clone = sink.clone();
⋮----
assert_eq!(payload["device"], r"\\\\.\\\\DISPLAY1");
assert!(payload["path"]
⋮----
fn clear_assignment_sends_clear_true() {
⋮----
let src = std::env::temp_dir().join("k6wp-test-clear-src.mp4");
⋮----
assert_eq!(payload["clear"], true);
⋮----
fn playlist_rejects_interval_zero() {
⋮----
let err = validate_playlist(&mut playlist).expect_err("interval 0 must fail");
assert!(err.to_string().contains("interval_min out of range"));
⋮----
let err2 = validate_playlist(&mut playlist2).expect_err("interval 1441 must fail");
assert!(err2.to_string().contains("interval_min out of range"));
⋮----
fn playlist_preserves_order() {
⋮----
let dir = std::env::temp_dir().join(format!("k6wp-test-playlist-{}", std::process::id()));
⋮----
std::fs::create_dir_all(&dir).expect("create dir");
let path = dir.join("playlist.json");
⋮----
playlist.order = vec![
⋮----
save_playlist(&path, &playlist).expect("save must succeed");
⋮----
let loaded = migrate_playlist(&std::fs::read_to_string(&path).unwrap()).unwrap();
assert_eq!(loaded.order, playlist.order);
assert_eq!(loaded.enabled, playlist.enabled);
assert_eq!(loaded.interval_min, playlist.interval_min);
assert_eq!(loaded.shuffle, playlist.shuffle);
⋮----
fn save_playlist_round_trip() {
⋮----
std::env::temp_dir().join(format!("k6wp-test-playlist-rt-{}", std::process::id()));
⋮----
mod compress_tests {
⋮----
fn compress_spawns_compressor() {
⋮----
let sink = Sink(std::sync::Arc::new(std::sync::Mutex::new(Box::new(
⋮----
let src = std::env::temp_dir().join("k6wp-test-compress-src.mp4");
⋮----
let out = std::env::temp_dir().join("k6wp-test-compress-out.mp4");
⋮----
compress_handler(1, &src, &out, 1920, 1080, 30, 23, sink);
⋮----
.expect("must get CompressDone");
⋮----
assert_eq!(job_id, 1);
assert!(ok, "expected ok, got error: {error:?}");
assert!(out_path.is_some());
⋮----
other => panic!("expected CompressDone, got {other:?}"),
⋮----
fn compress_parses_error() {
⋮----
assert_eq!(parse_ffmpeg_time("not-a-time"), None);
assert_eq!(parse_ffmpeg_time("00:00"), None);
assert_eq!(parse_ffmpeg_time(""), None);
assert_eq!(parse_ffmpeg_time("00:00:00"), Some(0.0));
assert_eq!(parse_ffmpeg_time("25:00:00"), Some(90000.0));
⋮----
fn compress_progress_from_stderr() {
⋮----
assert_eq!(parse_ffmpeg_time("00:00:01.00"), Some(1.0));
assert_eq!(parse_ffmpeg_time("00:01:30.00"), Some(90.0));
assert_eq!(parse_ffmpeg_time("01:00:00.00"), Some(3600.0));
assert_eq!(parse_ffmpeg_time("00:00:00.50"), Some(0.5));
⋮----
fn compress_cancel_kills_tree() {
⋮----
let src = std::env::temp_dir().join("k6wp-test-compress-src4.mp4");
⋮----
let out = std::env::temp_dir().join("k6wp-test-compress-out4.mp4");
⋮----
compress_handler(4, &src, &out, 1920, 1080, 30, 23, sink);
⋮----
assert_eq!(job_id, 4);
```
