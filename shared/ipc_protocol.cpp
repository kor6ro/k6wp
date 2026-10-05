#include "ipc_protocol.hpp"

#include <windows.h>

#include <cstring>
#include <utility>
#include <vector>

namespace k6wp {
namespace {

// RAII guard for Win32 HANDLEs (project rule: no raw new/delete).
class HandleGuard {
 public:
  HandleGuard() = default;
  ~HandleGuard() {
    if (h_ != nullptr && h_ != INVALID_HANDLE_VALUE) {
      CloseHandle(h_);
    }
  }
  HandleGuard(const HandleGuard&) = delete;
  HandleGuard& operator=(const HandleGuard&) = delete;

  HANDLE* ptr() { return &h_; }
  HANDLE get() const { return h_; }

 private:
  HANDLE h_ = nullptr;
};

constexpr const char* kCmdNames[] = {
    "set_video",
    "pause",
    "resume",
    "set_monitor",
    "get_state",
    "quit",
    "set_display_video",
};

}  // namespace

const char* CmdToString(Cmd cmd) noexcept {
  switch (cmd) {
    case Cmd::set_video:
      return "set_video";
    case Cmd::pause:
      return "pause";
    case Cmd::resume:
      return "resume";
    case Cmd::set_monitor:
      return "set_monitor";
    case Cmd::get_state:
      return "get_state";
    case Cmd::quit:
      return "quit";
    case Cmd::set_display_video:
      return "set_display_video";
  }
  return nullptr;
}

bool CmdFromString(const char* s, Cmd& out) noexcept {
  if (s == nullptr) {
    return false;
  }
  constexpr std::size_t kCmdCount = sizeof(kCmdNames) / sizeof(kCmdNames[0]);
  for (std::size_t i = 0; i < kCmdCount; ++i) {
    if (std::strcmp(s, kCmdNames[i]) == 0) {
      out = static_cast<Cmd>(i);
      return true;
    }
  }
  return false;
}

std::string Encode(const IpcMessage& msg) {
  const char* cmd_name = CmdToString(msg.cmd);
  if (cmd_name == nullptr) {
    throw IpcError("Encode: unknown Cmd value");
  }
  const std::string payload_json = msg.payload.dump();
  if (payload_json.size() > kMaxPayloadBytes) {
    throw IpcError("Encode: payload exceeds kMaxPayloadBytes (64 KiB)");
  }
  const nlohmann::json root = {
      {"version", msg.version},
      {"cmd", cmd_name},
      {"payload", msg.payload},
  };
  // Cap the wire FRAME, not just the payload: the documented 64 KiB limit is
  // on the NDJSON line, and the envelope adds key/version overhead.
  const std::string frame = root.dump();
  if (frame.size() > kMaxPayloadBytes) {
    throw IpcError("Encode: frame exceeds kMaxPayloadBytes (64 KiB)");
  }
  return frame + "\n";
}

bool Decode(const std::string& line, IpcMessage& out) {
  try {
    if (line.empty()) {
      return false;
    }
    // One NDJSON line: strip a single trailing '\n' (and optional '\r'), then
    // reject anything that still contains a newline (multi-line input).
    std::string s = line;
    if (!s.empty() && s.back() == '\n') {
      s.pop_back();
    }
    if (!s.empty() && s.back() == '\r') {
      s.pop_back();
    }
    if (s.empty() || s.find('\n') != std::string::npos) {
      return false;
    }

    const nlohmann::json root = nlohmann::json::parse(s);
    if (!root.is_object()) {
      return false;
    }
    if (!root.contains("version") || !root["version"].is_number_integer()) {
      return false;
    }
    const int version = root["version"].get<int>();
    if (version != kProtocolVersion) {
      return false;
    }
    if (!root.contains("cmd") || !root["cmd"].is_string()) {
      return false;
    }
    Cmd cmd = Cmd::get_state;
    if (!CmdFromString(root["cmd"].get<std::string>().c_str(), cmd)) {
      return false;
    }
    nlohmann::json payload = root.value("payload", nlohmann::json::object());
    if (payload.dump().size() > kMaxPayloadBytes) {
      return false;
    }

    out.version = version;
    out.cmd = cmd;
    out.payload = std::move(payload);
    return true;
  } catch (const std::exception&) {
    return false;
  } catch (...) {
    return false;
  }
}

std::wstring PipeNameForSession(unsigned long session_id) {
  return std::wstring(PipeNameBase) + L"-" + std::to_wstring(session_id);
}

std::wstring CurrentSessionPipeName() {
  DWORD session_id = 0;
  if (!ProcessIdToSessionId(GetCurrentProcessId(), &session_id)) {
    session_id = 0;
  }
  return PipeNameForSession(session_id);
}

PSECURITY_ATTRIBUTES MakeCurrentUserOnlySA() {
  // 1. Current user's SID from the process token.
  HandleGuard token;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, token.ptr())) {
    return nullptr;
  }
  DWORD token_len = 0;
  GetTokenInformation(token.get(), TokenUser, nullptr, 0, &token_len);
  if (token_len == 0) {
    return nullptr;
  }
  std::vector<BYTE> token_buf(token_len);
  if (!GetTokenInformation(token.get(), TokenUser, token_buf.data(), token_len,
                           &token_len)) {
    return nullptr;
  }
  const TOKEN_USER* tu = reinterpret_cast<const TOKEN_USER*>(token_buf.data());
  PSID user_sid = tu->User.Sid;
  if (user_sid == nullptr || !IsValidSid(user_sid)) {
    return nullptr;
  }

  // 2. One allocation holding SECURITY_DESCRIPTOR followed by the DACL, so a
  //    single LocalFree(sa->lpSecurityDescriptor) releases both.
  const DWORD sid_len = GetLengthSid(user_sid);
  const DWORD ace_size = sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + sid_len;
  const DWORD dacl_size = sizeof(ACL) + ace_size;
  const DWORD total_size = SECURITY_DESCRIPTOR_MIN_LENGTH + dacl_size;

  PSECURITY_DESCRIPTOR sd =
      static_cast<PSECURITY_DESCRIPTOR>(LocalAlloc(LPTR, total_size));
  if (sd == nullptr) {
    return nullptr;
  }
  PACL dacl = reinterpret_cast<PACL>(reinterpret_cast<BYTE*>(sd) +
                                     SECURITY_DESCRIPTOR_MIN_LENGTH);

  if (!InitializeAcl(dacl, dacl_size, ACL_REVISION) ||
      !AddAccessAllowedAce(dacl, ACL_REVISION, GENERIC_ALL, user_sid) ||
      !InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION) ||
      !SetSecurityDescriptorDacl(sd, TRUE, dacl, FALSE)) {
    LocalFree(sd);
    return nullptr;
  }

  PSECURITY_ATTRIBUTES sa = static_cast<PSECURITY_ATTRIBUTES>(
      LocalAlloc(LPTR, sizeof(SECURITY_ATTRIBUTES)));
  if (sa == nullptr) {
    LocalFree(sd);
    return nullptr;
  }
  sa->nLength = sizeof(SECURITY_ATTRIBUTES);
  sa->lpSecurityDescriptor = sd;
  sa->bInheritHandle = FALSE;
  return sa;
}

EngineState ParseEngineState(const nlohmann::json& raw) {
  EngineState s;
  try {
    const nlohmann::json state = raw.value("state", nlohmann::json::object());
    s.pid = state.value("pid", 0ULL);
    s.video = state.value("video", std::string());
    s.paused = state.value("paused", false);
    s.headless_slots = state.value("headless_slots", 0);
    s.live = state.value("live", true);
    // Row 15, additive: absent keys default safely (old-engine compat —
    // capability 0 + empty maps). Wrong-type present values also fall back
    // to those defaults; parsing never throws out of here.
    if (auto cap = state.find("display_capability");
        cap != state.end() && cap->is_number_integer()) {
      s.display_capability = cap->get<int>();
    }
    if (auto a = state.find("display_assignments");
        a != state.end() && a->is_object()) {
      for (auto it = a->begin(); it != a->end(); ++it) {
        if (it.value().is_string()) {
          s.display_assignments.emplace(it.key(),
                                        it.value().get<std::string>());
        }
      }
    }
    if (auto c = state.find("display_coverage");
        c != state.end() && c->is_object()) {
      for (auto it = c->begin(); it != c->end(); ++it) {
        if (it.value().is_string()) {
          s.display_coverage.emplace(it.key(), it.value().get<std::string>());
        }
      }
    }
  } catch (const std::exception&) {
  }
  return s;
}

}  // namespace k6wp