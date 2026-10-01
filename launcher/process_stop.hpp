#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <string>

namespace k6wp::launcher {

// Pipe base used for THIS process's session (mirrors
// k6wp::CurrentSessionPipeName; the launcher links no k6wp_shared).
std::wstring SessionPipeName();

inline constexpr DWORD kPipeProbeTimeoutMs = 2000;

// One NDJSON v1 request frame ({"version":1,"cmd":"<name>","payload":{}}\n),
// byte-identical to shared/ipc_protocol.hpp Encode().
std::string BuildEngineCommandLine(const wchar_t* cmd_name);

// True when an ack frame is "{\"ok\":true...}\n".
bool AckIsOk(const std::string& ack);

// Extracts the engine PID from a get_state ack; 0 when absent/malformed.
DWORD PidFromStateJson(const std::string& ack);

// --stop: stop every K6WP process in this K6WP.exe's own folder (never by
// image name). 0 = all gone, 3 = something is still running.
int RunStop();

}  // namespace k6wp::launcher
