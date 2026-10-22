//! Windows-side config persistence (task J3).
//!
//! This is the Rust mirror of the C++ `SaveConfig` / `PersistConfigField`
//! contract in `shared/config_schema.cpp`, plus the cross-process lock that
//! makes it safe when the engine and the Studio write the same file.
//!
//! # Why a lock
//!
//! Both processes rewrite `config.json` (the engine through a single-field
//! update, the Studio through a whole-config save). Each write is a
//! read-merge-write: read the file, merge the fields this writer owns,
//! publish atomically. Two writers that read the same old file and publish in
//! turn lose the first writer's change - the classic lost update. The lock
//! makes the whole read-merge-write one critical section, so the second
//! writer reads what the first wrote.
//!
//! The lock also makes the `"<file>.tmp"` side file safe: every writer
//! publishes through it, and two processes writing that one name at the same
//! time can truncate each other's payload. Measured on the C++ side: without
//! the lock, 5 out of 5 two-process scenes ended with a lost field, a failed
//! rename (`ERROR_ACCESS_DENIED` / `ERROR_SHARING_VIOLATION`) or a corrupt
//! JSON file. With the lock, 5 out of 5 scenes were clean.
//!
//! # The name
//!
//! `Local\K6WP-Config-Lock`, the same name the C++ side uses, so the two
//! implementations contend with each other during the Phase 3 cut-over.
//! `Local\` scopes it to the session: a second user's session writes its own
//! `%LOCALAPPDATA%` and must not contend with ours.
//!
//! # The bounded wait
//!
//! A writer waits at most [`LOCK_TIMEOUT`] and then returns a [`ConfigError`].
//! Waiting forever would turn a wedged writer into a hung UI; a bounded wait
//! turns it into a visible error. A lock whose previous owner died is
//! reported as abandoned by Windows and taken over immediately, so a crash
//! cannot wedge the next write.

use std::fs;
use std::path::{Path, PathBuf};
use std::time::Duration;

use serde_json::{Map, Value};
use windows::core::w;
use windows::Win32::Foundation::{CloseHandle, WAIT_ABANDONED, WAIT_OBJECT_0, WAIT_TIMEOUT};
use windows::Win32::System::Threading::{CreateMutexW, ReleaseMutex, WaitForSingleObject};

use crate::config::{
    config_to_json, merge_unknown_keys, migrate_config_value, validate_config, ConfigError,
    WallpaperConfig, CONFIG_SCHEMA_VERSION,
};

/// The C++ `kMaxConfigBytes`: refuse to slurp a hostile multi-gigabyte file.
pub const MAX_CONFIG_BYTES: u64 = 1024 * 1024;

/// The default bounded wait - the same 2 s the C++ `ConfigFileLock` uses.
pub const LOCK_TIMEOUT: Duration = Duration::from_millis(2000);

/// RAII holder of the settings-family mutex.
///
/// Acquisition is recursive for the same thread, exactly like the Win32
/// mutex the C++ side holds through `AtomicWriteJson`, so a function that
/// already holds it can call another one that acquires it again.
pub struct ConfigFileLock {
    handle: windows::Win32::Foundation::HANDLE,
}

impl ConfigFileLock {
    /// Acquire the lock, waiting at most `timeout`.
    ///
    /// # Errors
    ///
    /// [`ConfigError`] when the mutex cannot be created, when the wait times
    /// out, or when the wait fails for another reason.
    pub fn acquire(timeout: Duration) -> Result<Self, ConfigError> {
        // SAFETY: the name is a NUL-terminated wide literal with static
        // lifetime; the returned handle is closed by Drop.
        unsafe {
            let handle = CreateMutexW(None, false, w!("Local\\K6WP-Config-Lock"))
                .map_err(|e| ConfigError(format!("config lock: CreateMutexW failed ({e})")))?;
            let wait_ms = timeout.as_millis().min(u128::from(u32::MAX)) as u32;
            let state = WaitForSingleObject(handle, wait_ms);
            if state == WAIT_OBJECT_0 || state == WAIT_ABANDONED {
                // WAIT_ABANDONED: the previous owner died while holding the
                // lock. The file is either the old bytes or the new bytes
                // (the publish is a rename), never a mixture.
                return Ok(Self { handle });
            }
            let _ = CloseHandle(handle);
            if state == WAIT_TIMEOUT {
                return Err(ConfigError(format!(
                    "config lock timeout after {wait_ms} ms"
                )));
            }
            Err(ConfigError(format!(
                "config lock: WaitForSingleObject failed (state={state:?})"
            )))
        }
    }
}

impl Drop for ConfigFileLock {
    fn drop(&mut self) {
        // SAFETY: the handle was returned by CreateMutexW and is released and
        // closed exactly once, here.
        unsafe {
            let _ = ReleaseMutex(self.handle);
            let _ = CloseHandle(self.handle);
        }
    }
}

/// Write the whole config under the lock - the Rust `SaveConfig`.
///
/// The unknown keys of the file being replaced are preserved (task I3): an
/// older engine must not drop the fields a newer engine wrote. The known keys
/// always win.
///
/// # Errors
///
/// [`ConfigError`] when the config is invalid, the lock times out or the write
/// fails.
pub fn save_config(path: &Path, cfg: &WallpaperConfig) -> Result<(), ConfigError> {
    let _lock = ConfigFileLock::acquire(LOCK_TIMEOUT)?;
    validate_config(cfg)?;
    let existing = read_existing_json(path);
    let doc = merge_unknown_keys(&existing, &config_to_json(cfg));
    atomic_write_json(path, &doc)
}

/// Update one field, preserving everything else - the Rust
/// `PersistConfigField`.
///
/// Returns `true` when the file changed, `false` when the field already held
/// `value` (the C++ contract: a no-op write must not touch the mtime).
///
/// A missing, corrupt or invalid file is preserved as `<file>.bak` and
/// rebuilt from `fallback`, exactly like the C++ side. A pre-current-schema
/// file is written back at the current schema.
///
/// # Errors
///
/// [`ConfigError`] when the merged document fails validation (the on-disk
/// file stays untouched), the lock times out or the write fails.
pub fn persist_config_field(
    path: &Path,
    field: &str,
    value: Value,
    fallback: &WallpaperConfig,
) -> Result<bool, ConfigError> {
    let _lock = ConfigFileLock::acquire(LOCK_TIMEOUT)?;

    let mut have_valid = false;
    let mut raw = Value::Object(Map::new());
    if let Some(parsed) = read_existing_json_checked(path) {
        if parsed.is_object() {
            let probe = migrate_config_value(&parsed)
                .and_then(|probe| validate_config(&probe).map(|()| probe));
            if probe.is_ok() {
                raw = parsed;
                have_valid = true;
            }
        }
    }
    if !have_valid {
        backup_file(path);
        raw = config_to_json(fallback);
    }

    if have_valid && raw.get(field) == Some(&value) {
        return Ok(false);
    }

    let current_schema =
        have_valid && raw.get("version").and_then(Value::as_i64) == Some(CONFIG_SCHEMA_VERSION);
    if let Value::Object(object) = &mut raw {
        object.insert(field.to_string(), value);
    } else {
        return Err(ConfigError("config root must be a JSON object".to_string()));
    }

    // Validate the merged document BEFORE touching disk: a bad mutation
    // returns an error and the on-disk file stays untouched.
    let merged = migrate_config_value(&raw)?;
    validate_config(&merged)?;

    let doc = if current_schema {
        raw
    } else {
        config_to_json(&merged)
    };
    atomic_write_json(path, &doc)?;
    Ok(true)
}

/// Parse the file when it exists and is a JSON object; `None` otherwise.
fn read_existing_json_checked(path: &Path) -> Option<Value> {
    let meta = fs::metadata(path).ok()?;
    if meta.len() > MAX_CONFIG_BYTES {
        return None;
    }
    let text = fs::read_to_string(path).ok()?;
    serde_json::from_str(&text).ok()
}

/// The C++ `ReadExistingJson`: an empty object when the file is missing,
/// unreadable or not a JSON object.
fn read_existing_json(path: &Path) -> Value {
    read_existing_json_checked(path)
        .filter(Value::is_object)
        .unwrap_or_else(|| Value::Object(Map::new()))
}

/// Best-effort copy of the file for forensics (no-op when it is absent).
fn backup_file(path: &Path) {
    if !path.exists() {
        return;
    }
    let mut bak = path.as_os_str().to_owned();
    bak.push(".bak");
    let _ = fs::copy(path, PathBuf::from(bak));
}

/// Crash-safe publish: write `"<file>.tmp"`, then rename over the target.
///
/// The caller must hold the lock: the `.tmp` name is shared by every writer.
fn atomic_write_json(path: &Path, doc: &Value) -> Result<(), ConfigError> {
    if let Some(parent) = path.parent() {
        if !parent.as_os_str().is_empty() {
            fs::create_dir_all(parent)
                .map_err(|e| ConfigError(format!("cannot create config directory: {e}")))?;
        }
    }
    let mut tmp = path.as_os_str().to_owned();
    tmp.push(".tmp");
    let tmp = PathBuf::from(tmp);
    let text = serde_json::to_string_pretty(doc)
        .map_err(|e| ConfigError(format!("cannot serialize config: {e}")))?;
    fs::write(&tmp, text).map_err(|e| {
        ConfigError(format!(
            "cannot write json temp file: {} ({e})",
            tmp.display()
        ))
    })?;
    // std::fs::rename on Windows uses MoveFileExW with REPLACE_EXISTING, so
    // the publish is atomic and the old file is never half-replaced.
    fs::rename(&tmp, path).map_err(|e| {
        let _ = fs::remove_file(&tmp);
        ConfigError(format!(
            "atomic json replace failed: {} ({e})",
            path.display()
        ))
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::config::MAX_FOREIGN_KEY_BYTES;
    use std::process::{Command, Stdio};

    fn scratch_dir(tag: &str) -> PathBuf {
        let dir =
            std::env::temp_dir().join(format!("k6wp_config_store_{tag}_{}", std::process::id()));
        let _ = fs::remove_dir_all(&dir);
        fs::create_dir_all(&dir).expect("scratch dir");
        dir
    }

    fn seed(path: &Path) {
        let cfg = WallpaperConfig::default();
        assert_eq!(cfg.version, CONFIG_SCHEMA_VERSION);
        save_config(path, &cfg).expect("seed");
    }

    fn read_doc(path: &Path) -> Value {
        serde_json::from_str(&fs::read_to_string(path).expect("read config")).expect("valid json")
    }

    #[test]
    fn persist_preserves_foreign_keys_and_reports_a_no_op() {
        let dir = scratch_dir("foreign");
        let path = dir.join("config.json");
        seed(&path);
        let mut doc = read_doc(&path);
        if let Value::Object(object) = &mut doc {
            object.insert("a_key_from_the_future".to_string(), Value::from(42));
        }
        atomic_write_json(&path, &doc).expect("seed foreign key");

        let fallback = WallpaperConfig::default();
        assert!(persist_config_field(&path, "crf", Value::from(20), &fallback).expect("persist"));
        let after = read_doc(&path);
        assert_eq!(after["crf"], 20);
        assert_eq!(after["a_key_from_the_future"], 42);
        assert_eq!(after["version"], CONFIG_SCHEMA_VERSION);

        // The same value again is a no-op (the C++ contract).
        assert!(!persist_config_field(&path, "crf", Value::from(20), &fallback).expect("no-op"));
        let _ = fs::remove_dir_all(&dir);
    }

    #[test]
    fn persist_rebuilds_a_corrupt_file_and_keeps_a_backup() {
        let dir = scratch_dir("corrupt");
        let path = dir.join("config.json");
        fs::write(&path, "{ this is not json").expect("write corrupt");
        let fallback = WallpaperConfig::default();
        assert!(persist_config_field(&path, "crf", Value::from(21), &fallback).expect("persist"));
        assert_eq!(read_doc(&path)["crf"], 21);
        let mut bak = path.as_os_str().to_owned();
        bak.push(".bak");
        assert!(Path::new(&bak).exists(), "the corrupt file is backed up");
        let _ = fs::remove_dir_all(&dir);
    }

    #[test]
    fn a_bad_value_does_not_touch_the_file() {
        let dir = scratch_dir("badvalue");
        let path = dir.join("config.json");
        seed(&path);
        let before = fs::read_to_string(&path).expect("read");
        let fallback = WallpaperConfig::default();
        // crf 99 is outside [16,28]: the merge must fail before the write.
        assert!(persist_config_field(&path, "crf", Value::from(99), &fallback).is_err());
        assert_eq!(fs::read_to_string(&path).expect("read"), before);
        let _ = fs::remove_dir_all(&dir);
    }

    #[test]
    fn the_lock_is_a_bounded_wait() {
        let lock = ConfigFileLock::acquire(LOCK_TIMEOUT).expect("first");
        // The same THREAD may re-enter (Win32 mutex semantics), so the
        // timeout is tested from another thread.
        let failed =
            std::thread::spawn(|| ConfigFileLock::acquire(Duration::from_millis(0)).is_err())
                .join()
                .expect("thread");
        assert!(failed, "a second thread times out while the lock is held");
        drop(lock);
        assert!(ConfigFileLock::acquire(Duration::from_millis(1000)).is_ok());
    }

    /// The child side of [`two_processes_do_not_lose_updates`]. It does
    /// nothing unless the parent set the environment variables, so the normal
    /// run of the suite is unaffected.
    ///
    /// The parent re-executes this test binary with `--exact
    /// config_store::tests::child_writer_entry`, which is why the name must
    /// stay stable.
    #[test]
    fn child_writer_entry() {
        let Ok(path) = std::env::var("K6WP_RUST_WRITER_PATH") else {
            return;
        };
        let field = std::env::var("K6WP_RUST_WRITER_FIELD").expect("field");
        let iterations: u32 = std::env::var("K6WP_RUST_WRITER_ITERATIONS")
            .expect("iterations")
            .parse()
            .expect("iterations number");
        let fallback = WallpaperConfig::default();
        for i in 1..=iterations {
            persist_config_field(Path::new(&path), &field, Value::from(i), &fallback)
                .expect("child persist");
        }
    }

    #[test]
    fn two_processes_do_not_lose_updates() {
        let dir = scratch_dir("twowriter");
        let path = dir.join("config.json");
        seed(&path);
        let exe = std::env::current_exe().expect("current exe");
        let spawn = |field: &str| {
            Command::new(&exe)
                .arg("--exact")
                .arg("config_store::tests::child_writer_entry")
                .env("K6WP_RUST_WRITER_PATH", &path)
                .env("K6WP_RUST_WRITER_FIELD", field)
                .env("K6WP_RUST_WRITER_ITERATIONS", "120")
                .stdout(Stdio::null())
                .stderr(Stdio::null())
                .spawn()
                .expect("spawn writer")
        };
        let mut a = spawn("field_a");
        let mut b = spawn("field_b");
        assert!(a.wait().expect("wait a").success(), "writer A failed");
        assert!(b.wait().expect("wait b").success(), "writer B failed");

        let doc = read_doc(&path);
        assert_eq!(doc["field_a"], 120, "writer A's last value was lost");
        assert_eq!(doc["field_b"], 120, "writer B's last value was lost");
        assert_eq!(doc["version"], CONFIG_SCHEMA_VERSION);
        let mut tmp = path.as_os_str().to_owned();
        tmp.push(".tmp");
        assert!(!Path::new(&tmp).exists(), "no .tmp file is left behind");
        let _ = fs::remove_dir_all(&dir);
    }

    #[test]
    fn the_foreign_key_budget_matches_the_cpp_side() {
        let known = serde_json::json!({"version": 6, "crf": 22});
        let collide = serde_json::json!({"version": 6, "crf": "high", "note": "kept"});
        let merged = merge_unknown_keys(&collide, &known);
        assert_eq!(merged["crf"], 22);
        assert_eq!(merged["note"], "kept");

        let mut huge = Map::new();
        huge.insert("version".to_string(), Value::from(6));
        huge.insert(
            "a_huge_foreign_key".to_string(),
            Value::from("x".repeat(70 * 1024)),
        );
        let capped = merge_unknown_keys(&Value::Object(huge), &known);
        assert!(capped.get("a_huge_foreign_key").is_none());

        let mut many = Map::new();
        many.insert("version".to_string(), Value::from(6));
        for i in 0..100 {
            many.insert(format!("k{i}"), Value::from("y".repeat(1024)));
        }
        let kept = merge_unknown_keys(&Value::Object(many), &known);
        let carried: usize = kept
            .as_object()
            .expect("object")
            .iter()
            .filter(|(key, _)| known.get(*key).is_none())
            .map(|(key, value)| key.len() + serde_json::to_string(value).expect("serialize").len())
            .sum();
        assert!(carried <= MAX_FOREIGN_KEY_BYTES, "carried {carried} bytes");
        assert!(kept.get("k0").is_some());
        assert!(kept.get("k99").is_none());
    }
}
