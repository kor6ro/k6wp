#pragma once

// The build flags of the C++ side (task I4).
//
// THE ENVIRONMENT OVERRIDES ARE COMPILED OUT OF THE RELEASE BUILD.
//
// The overrides (K6WP_ENGINE_PIPE, K6WP_ENGINE_MUTEX, K6WP_LIBRARY_JSON)
// exist so a TEST can talk to its own engine instead of the user's live
// session. A shipped binary must not let an environment variable redirect
// the IPC endpoint, the singleton mutex or the library path: whoever can
// set an environment variable for the process would then be able to point
// the app at a pipe it controls.
//
// The rule: the overrides are compiled in a DEBUG build only (`_DEBUG` is
// defined by MSVC in the Debug configuration and NOT in Release). The
// release binaries are checked by tools/check_release_overrides.ps1, which
// scans them for the three variable names: a release binary that still
// contains one of them FAILS the check.

#if defined(_DEBUG)
#define K6WP_ALLOW_ENV_OVERRIDES 1
#else
#define K6WP_ALLOW_ENV_OVERRIDES 0
#endif
