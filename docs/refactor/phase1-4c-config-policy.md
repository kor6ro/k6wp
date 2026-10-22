# The config policy: the round trip, the writers, the downgrade

Task I3. This file states the three things the task asks for:

1. **the policy when the config is saved** (the unknown keys are not
   dropped);
2. **who writes each JSON file**, and the proof that the writes are atomic
   and that there is no double writer;
3. **the downgrade policy** (also in
   `packaging/known-limitations.md`, section 9).

## 1. The policy: the unknown keys are PRESERVED

**The decision (one of the two the task allows):** when the config is
written, the keys this engine does not know are **kept**. The known keys
always win - they are what this engine understands.

**Why this one and not the other.** The alternative was "refuse to
overwrite a file of a newer version without a `.bak`". That protects the
newer file, but it also means: an older engine cannot write its settings
at all while a newer file is on disk (the user would lose the ability to
change anything until the file is deleted by hand). The round-trip keeps
**both** worlds:

- the newer engine's fields survive (nothing is lost);
- the older engine's own fields are written normally (the user can still
  change the settings).

**The merge is pure**, so it is a golden fixture on both sides:

```cpp
// shared/config_schema.cpp
nlohmann::json MergeUnknownKeys(const nlohmann::json& existing,
                                 const nlohmann::json& known);
```

```rust
// rust/crates/k6wp-core/src/config.rs
pub fn merge_unknown_keys(existing: &Value, known: &Value) -> Value
```

The fixture cases (`config.roundtrip.*` in
`tests/golden/shared_behavior.json`, **6 cases**):

| case | the existing file | the known document | the result |
|---|---|---|---|
| `unknown_keys_survive` | `{"version":7,"crf":19,"a_key_from_the_future":42}` | `{"version":6,"crf":19}` | the unknown key is **kept** |
| `the_known_keys_win` | `{"version":7,"crf":19}` | `{"version":6,"crf":22}` | the known key wins (`crf` 22) |
| `an_empty_existing` | `{}` | `{"version":6,"crf":22}` | the known document |
| `a_non_object_existing` | `[1,2,3]` | `{"version":6,"crf":22}` | the known document (a corrupt file must not break a save) |
| `a_nested_unknown_key` | `{"version":7,"future":{"a":1}}` | `{"version":6}` | the nested key is kept **as it is** |
| `the_version_is_known` | `{"version":7}` | `{"version":6}` | the version is a known key: the current one wins |

**The C++ side** applies the merge in `SaveConfig` (the whole-document
write) and in `PersistConfigField` (the single-field write already merged
at the JSON level, so it preserved the unknown keys for a current-schema
file - it now does so for every file).

**The Rust side** mirrors it (`k6wp_core::config::merge_unknown_keys`) and
the replayer (`tests/golden.rs`) runs the same 6 cases.

**The live proof** (the C++ test, `tests/config_test.cpp`, case 73): a
file with `version = current + 1` and two unknown keys is loaded; the
self-heal rewrite happens at the current schema; **both unknown keys are
still there**, and an explicit `SaveConfig` keeps them too.

## 2. The writers

| file | who writes it | how | the atomic publish |
|---|---|---|---|
| `%LOCALAPPDATA%\K6WP\config.json` | the **engine** (the config watcher's self-heal after a migration, and `PersistConfigField` for a single field) and the **Studio** (the settings bridge, through `PersistConfigField`) | `k6wp::SaveConfig` / `k6wp::PersistConfigField` | `AtomicWriteJson` |
| `%LOCALAPPDATA%\K6WP\playlist.json` | the **Studio** (the playlist editor) | `k6wp::SavePlaylist` | `AtomicWriteJson` |
| `%LOCALAPPDATA%\K6WP\displays.json` | the **engine** (the per-display assignments) | `k6wp::SaveDisplays` | `AtomicWriteJson` |
| `%LOCALAPPDATA%\K6WP\studio_settings.json` | the **Studio** (the settings dialog) | `k6wp::SaveStudioSettings` | `AtomicWriteJson` |

**There is ONE writer path.** All four go through `AtomicWriteJson`
(`shared/config_schema.cpp`):

1. the payload goes to `<path>.tmp` (flushed, closed, the stream state
   checked);
2. `MoveFileExW(MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)`
   replaces the main file **atomically**;
3. a failure removes the `.tmp` and throws (the old file stays intact).

So a crash (or a force-kill) can never leave a half-written JSON file, and
a leftover `.tmp` is ignored by the loaders and truncated by the next save.

**The double-writer question.** `config.json` has **two** writers (the
engine and the Studio) - that is by design (they are two processes), and
it is safe because **both** write through the JSON-level merge
(`PersistConfigField` merges one field into the document it just read) and
**both** publish atomically. The whole-document write (`SaveConfig`) is
used only by the engine's self-heal (right after a load, so it writes what
it just read) and it now carries the unknown keys forward ✓.

**The proof** (the C++ test, cases 74 and 75): after a load+save cycle of
each of the four writers, the file exists, it is readable, and **no `.tmp`
file is left behind**.

## 3. The downgrade policy

The full text is in `packaging/known-limitations.md` (section 9). The
summary:

- the current schema is **v6**;
- an engine reads a **newer** file best-effort (the known fields keep
  their values) and rewrites it at the current schema **with the unknown
  keys preserved** ✓ (section 1);
- an **older** engine (measured with the `baseline-pre-refactor` build,
  schema v5) refuses a v6 file, keeps its last-valid config, **leaves the
  file byte-identical** and writes a `.bak` with the same bytes - so the
  file is never damaged, but the old engine does not apply the settings
  it does not know.
