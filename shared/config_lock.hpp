#pragma once

#include <chrono>

namespace k6wp::detail {

// J3: the cross-process lock that serializes every writer of the settings
// family (config.json, studio_settings.json, playlist.json, displays.json,
// library.json).
//
// WHY A LOCK AT ALL
//   The engine and the Studio both rewrite config.json (the engine through
//   PersistConfigField, the Studio through SaveConfig / the settings
//   bridge). Each write is a read-merge-write: read the file, merge the
//   fields this writer owns, publish atomically. Two writers that read the
//   same old file and then publish in turn lose the first writer's change -
//   the classic lost update. The lock makes the whole read-merge-write one
//   critical section, so the second writer reads what the first wrote.
//
//   The lock also makes the ".tmp" side file safe: every writer publishes
//   through "<file>.tmp", and two processes writing that one name at the
//   same time could truncate each other's payload. Serialized, the name is
//   never shared.
//
// THE NAME
//   "Local\K6WP-Config-Lock": the "Local\" prefix scopes it to the session,
//   which is what we want - a second user's session writes its own
//   %LOCALAPPDATA% and must not contend with ours.
//
// THE WAIT IS BOUNDED
//   A writer waits at most `timeout` (default 2 s) and then throws
//   ConfigError. Waiting forever would turn a wedged writer into a hung
//   message loop; a bounded wait turns it into a visible error the caller
//   already knows how to report. A lock whose previous owner died is
//   reported as abandoned by Windows and is taken over immediately - a
//   crash cannot wedge the next write.
//
// THE TEST ESCAPE
//   K6WP_TEST_NO_CONFIG_LOCK=1 skips the wait. It exists so the two-writer
//   test can prove the lock is what prevents the lost update (the same
//   scene runs with and without it). It is compiled into DEBUG builds only
//   and is listed in tools/check_release_overrides.ps1 with the other
//   overrides.
class ConfigFileLock {
 public:
  explicit ConfigFileLock(
      std::chrono::milliseconds timeout = std::chrono::milliseconds(2000));
  ~ConfigFileLock();

  ConfigFileLock(const ConfigFileLock&) = delete;
  ConfigFileLock& operator=(const ConfigFileLock&) = delete;

 private:
  void* handle_ = nullptr;  // HANDLE, kept void* so this header stays Win32-free
  bool held_ = false;
};

}  // namespace k6wp::detail
