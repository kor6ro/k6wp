// engine/src/ipc_marshal.hpp
// CRIT-2 (audit-remediation todo 11): worker-side payload validation for the
// PostMessage-marshaled set_video / set_monitor paths.
//
// The IPC server runs every handler on its single worker thread, but window
// creation (DesktopInjector::Attach via MultiMonitor::SetActiveMonitor /
// ApplyActiveFilter) and renderer teardown are main-loop-thread-only. So the
// IpcHandlers installed by EngineApp do NOT execute here: they only
// VALIDATE the payload with the helpers below, stash the pending target
// under EngineApp::marshal_mutex_, PostMessageW a private UINT to the hidden
// window, and ack {"ok":true} ("diterima" = accepted for execution, NOT
// "selesai" = applied — the client verifies via get_state). The main thread
// pops the pending value in HandleMessage and runs the real executor
// (HandleSetVideo / HandleSetMonitor).
//
// windows.h-free (std types only) so the unit test (tests/ipc_marshal_test.cpp)
// compiles this TU standalone without pulling Win32 or mpv.
#pragma once

#include <optional>
#include <string>

namespace k6wp {

// Parses a set_monitor payload ({"monitor": N}, alias {"monitor_id": N}).
// Returns the target on success; std::nullopt unless the payload is an
// object with an integer >= -1 (-1 = all screens). Pure: no filesystem, no
// window, no global state — safe on any thread.
std::optional<int> ParseSetMonitorPayload(const std::string& payload_json);

// Validates a set_video payload ({"path": "<utf8>"}). Returns the UTF-8 path
// on success; std::nullopt when the path is missing, empty, or not an
// existing regular file (same reject policy as the old inline handler: a bad
// path acks {"error"} while the old video keeps playing). Filesystem probe
// only — safe on any thread.
std::optional<std::string> ValidateSetVideoPayload(
    const std::string& payload_json);

// Row 13: per-monitor assignment for set_display_video.
//
// Accepts {"device":"\\\\.\\DISPLAY1","path":"C:/.../a.mp4"} (non-empty
// device matching a \\.\DISPLAYn shape, non-empty UTF-8 path) or
// {"device":"\\\\.\\DISPLAY1","clear":true} (drop that device's assignment).
// The device shape matches the displays.json assignment keys (GDI device
// name, MONITORINFOEXW.szDevice). Returns nullopt + caller logs an ipc:
// reject on any invalid shape (mirror ParseSetMonitorPayload). Rejects
// payloads over kMaxPayloadBytes the way Decode does. Pure, no side effects.
struct DisplayVideoCommand {
  std::string device;  // GDI device name, e.g. \\.\DISPLAY1
  std::string path;    // UTF-8 video path; empty when clear == true
  bool clear = false;  // true -> drop the assignment for `device`
};
std::optional<DisplayVideoCommand> ParseSetDisplayVideoPayload(
    const std::string& payload_json);

}  // namespace k6wp
