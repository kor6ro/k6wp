#pragma once

#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>

#include "thirdparty/json.hpp"

// Minimal Win32 forward declarations so consumers of MakeCurrentUserOnlySA()
// do not need windows.h just to call it. _SECURITY_ATTRIBUTES stays opaque
// here; the full definition comes from windows.h in the .cpp (and in any
// consumer that includes windows.h itself — redeclaration is legal).
struct _SECURITY_ATTRIBUTES;
using PSECURITY_ATTRIBUTES = _SECURITY_ATTRIBUTES*;

namespace k6wp {

// Protocol version. Bump when the wire format changes incompatibly.
inline constexpr int kProtocolVersion = 1;

// Named pipe used by Engine <-> Studio.
//
// Per-session naming (MED-12, audit-remediation todo 13): the engine serves
// one pipe per Terminal Services session so concurrent sessions (RDP /
// fast-user-switching) each get an isolated singleton. NEVER use a bare
// literal — always go through PipeNameForSession() / CurrentSessionPipeName()
// below. The singleton mutex stays session-agnostic on purpose:
// Local\K6WP-Engine-Singleton (see engine/src/engine_app.hpp).
inline constexpr const wchar_t* PipeNameBase = L"\\\\.\\pipe\\k6wp-engine";

// Returns L"\\.\pipe\k6wp-engine-<session_id>" for the given session.
// (unsigned long is DWORD on Windows; the header stays windows.h-free by
// design, so the parameter is spelled without the Win32 typedef.)
std::wstring PipeNameForSession(unsigned long session_id);

// Pipe name for THIS process's session: resolves the session via
// ProcessIdToSessionId(GetCurrentProcessId(), ...) and returns
// PipeNameForSession(that session). Falls back to session 0 when the lookup
// fails (never throws, never empty).
std::wstring CurrentSessionPipeName();

// Maximum serialized payload size (64 KiB). Payloads larger than this are
// rejected by Encode (throws IpcError) and by Decode (returns false).
inline constexpr size_t kMaxPayloadBytes = 64 * 1024;

// Commands understood by the engine.
//
// APPEND-ONLY contract (row 13): existing numeric values must never move or
// be reordered — the static_asserts below lock them. kProtocolVersion stays 1
// across additive commands; an older engine that does not know a newer name
// answers with an error ack because CmdFromString returns false for unknown
// names and Decode rejects the frame (shared/ipc_protocol.cpp) — never a
// crash. set_display_video was appended after quit for multi-monitor
// per-slot assignment (GDI device \\.\DISPLAYn + UTF-8 path, or
// {"clear":true} to drop it).
enum class Cmd {
  set_video,
  pause,
  resume,
  set_monitor,
  get_state,
  quit,
  set_display_video,
};

static_assert(static_cast<int>(Cmd::set_video) == 0);
static_assert(static_cast<int>(Cmd::pause) == 1);
static_assert(static_cast<int>(Cmd::resume) == 2);
static_assert(static_cast<int>(Cmd::set_monitor) == 3);
static_assert(static_cast<int>(Cmd::get_state) == 4);
static_assert(static_cast<int>(Cmd::quit) == 5);
static_assert(static_cast<int>(Cmd::set_display_video) == 6);

// One NDJSON message, serialized as:
//   {"version":1,"cmd":"set_video","payload":{...}}\n
struct IpcMessage {
  int version = kProtocolVersion;
  Cmd cmd = Cmd::get_state;
  nlohmann::json payload = nlohmann::json::object();
};

// Fields of a get_state ack's "state" object (engine BuildStateJson). Missing
// keys keep the defaults; parsing never throws. Shared so Studio parses the
// engine state in exactly one place.
struct EngineState {
  unsigned long long pid = 0;
  std::string video;  // utf-8 path
  bool paused = false;
  int headless_slots = 0;
  bool live = true;
  // Row 15, additive: per-monitor assignment map (GDI device name ->
  // utf-8 video path) from displays.json. Missing key -> empty (engines
  // older than row 15 never emit it).
  std::map<std::string, std::string> display_assignments;
  // Row 15, additive: feature-detect Studio reads before trusting the two
  // maps. 0 = engine predates the display get_state fields (or emitted a
  // wrong-type value); 1 = display_assignments / display_coverage are
  // meaningful. Missing key -> 0.
  int display_capability = 0;
  // Row 15, additive: per-device placement verdict ("covered",
  // "clipped-left/top/right/bottom", "headless") from row 4's
  // Slot::coverage_reason chain. Missing key -> empty.
  std::map<std::string, std::string> display_coverage;
  // Plan todo 9, additive: wallpaper playlist snapshot (engine
  // BuildStateJson emits playlist_enabled / playlist_size /
  // playlist_index). Missing keys (engines older than these fields) ->
  // enabled false / size 0 / index -1; wrong-type present values also fall
  // back to those defaults. Parsing never throws.
  bool playlist_enabled = false;
  int playlist_size = 0;
  int playlist_index = -1;
};
EngineState ParseEngineState(const nlohmann::json& raw);

// Structured error thrown by Encode on invalid input (e.g. payload larger
// than kMaxPayloadBytes, or an unknown Cmd value).
class IpcError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

// Serializes msg to a single NDJSON line (JSON + '\n' terminator).
// Throws IpcError if the serialized payload exceeds kMaxPayloadBytes.
std::string Encode(const IpcMessage& msg);

// Parses one NDJSON line into out. Accepts a line with or without a trailing
// '\n' (Encode always emits one); rejects embedded newlines (multi-line
// input). Returns false — never throws — on empty input, malformed JSON,
// wrong protocol version, unknown cmd string, or payload > kMaxPayloadBytes.
bool Decode(const std::string& line, IpcMessage& out);

// Maps a Cmd to its wire string ("set_video", ...). Returns nullptr for
// unknown values.
const char* CmdToString(Cmd cmd) noexcept;

// Parses a wire string back to a Cmd. Returns false if the string is not a
// known command name.
bool CmdFromString(const char* s, Cmd& out) noexcept;

// Builds a SECURITY_ATTRIBUTES whose DACL grants access only to the current
// user's SID (helper for CreateNamedPipeW). Returns nullptr on failure.
// The caller must LocalFree BOTH the returned SA and its lpSecurityDescriptor
// (the descriptor and its DACL live in one allocation, so one LocalFree on
// the descriptor releases both).
PSECURITY_ATTRIBUTES MakeCurrentUserOnlySA();

}  // namespace k6wp