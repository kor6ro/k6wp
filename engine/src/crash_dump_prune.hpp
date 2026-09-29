#pragma once

// Crash-dump directory cap (1.2.0). The unhandled-exception filter in
// main.cpp writes %LOCALAPPDATA%\K6WP\crashes\engine-<ts>.dmp on every crash;
// uncapped, a crash loop (one bad video retried at every boot) grows that
// directory without bound and the user can only reclaim it by hand. This keeps
// the newest kMaxCrashDumps dumps, mirroring engine.log's own cap
// (log_file.cpp: kMaxLogBytes + one engine.log.1 generation, MaybeRotateLog) -
// a small fixed number of generations, older ones dropped, failure non-fatal.
//
// windows.h-free like tray.hpp / log_file.hpp: the directory arrives as UTF-16.
// Only the crash filter calls this, so the implementation stays
// allocation-free (no CRT heap, no std::filesystem, no mutex) with a fixed
// stack footprint - the faulting thread's stack may already be nearly
// exhausted.
namespace k6wp {

// One dump per boot, so 10 consecutive crashes is enough to separate a
// deterministic crash from a transient; a MiniDumpNormal of the engine
// (libmpv + dxgi mapped) runs tens of MiB, which keeps the capped directory
// orders of magnitude below the 5 GiB compress-output cache the app already
// allows itself.
inline constexpr unsigned kMaxCrashDumps = 10;

// Upper bound on enumeration passes (one delete each). A directory holding far
// more than the cap keeps the extra files - the next crash keeps pruning -
// instead of stalling a crashed process on an unbounded loop.
inline constexpr unsigned kMaxCrashPrunePasses = 32;

// Deletes engine-*.dmp files in `crash_dir`, oldest first, until one slot is
// free for the dump the caller is about to write (so the on-disk total never
// exceeds kMaxCrashDumps). "Oldest" is ftLastWriteTime, ties broken by name -
// engine-YYYYMMDD-HHMMSS.dmp is zero-padded, so lexical order is chronological
// order. Anything else in the directory is left alone.
//
// Best-effort and silent: an unreadable directory, a locked file or a denied
// delete just leaves the extra files in place, the same policy as
// MaybeRotateLog keeping the oversized engine.log when the rotate fails
// (log_file.cpp:82, :96-97). Never throws, never reports failure - a crash
// handler must not fail loudly, and leftover .dmp files beat no dump at all.
void PruneOldCrashDumps(const wchar_t* crash_dir);

}  // namespace k6wp
