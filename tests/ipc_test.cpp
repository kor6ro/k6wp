// Mini unit tests for the NDJSON IPC protocol (Todo 6).
// Build: ipc_test target in shared/CMakeLists.txt. Exit 0 = all pass.

#include <windows.h>

#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "ipc_protocol.hpp"

namespace {

int g_checks = 0;
int g_failures = 0;

#define CHECK(cond)                                                       \
  do {                                                                    \
    ++g_checks;                                                           \
    if (!(cond)) {                                                        \
      ++g_failures;                                                       \
      std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);         \
    }                                                                     \
  } while (0)

// --- Test 1: round-trip encode -> decode for all 6 command types ----------
void TestRoundTripAllCmds() {
  const std::vector<std::pair<k6wp::Cmd, nlohmann::json>> cases = {
      {k6wp::Cmd::set_video,
       {{"path", "C:\\videos\\wallpaper.mp4"}, {"loop", true}}},
      {k6wp::Cmd::pause, {{"reason", "user"}}},
      {k6wp::Cmd::resume, nlohmann::json::object()},
      {k6wp::Cmd::set_monitor,
       {{"monitor_id", 0}, {"width", 1920}, {"height", 1080}}},
      {k6wp::Cmd::get_state, nlohmann::json::object()},
      {k6wp::Cmd::quit, nlohmann::json::object()},
  };
  for (const auto& [cmd, payload] : cases) {
    k6wp::IpcMessage msg;
    msg.version = k6wp::kProtocolVersion;
    msg.cmd = cmd;
    msg.payload = payload;

    const std::string wire = k6wp::Encode(msg);
    CHECK(!wire.empty());
    CHECK(wire.back() == '\n');  // NDJSON terminator

    k6wp::IpcMessage out;
    CHECK(k6wp::Decode(wire, out));
    CHECK(out.version == k6wp::kProtocolVersion);
    CHECK(out.cmd == cmd);
    CHECK(out.payload == payload);
  }
}

// --- Test 2: protocol version mismatch -> Decode false ---------------------
void TestVersionMismatch() {
  k6wp::IpcMessage msg;
  msg.version = k6wp::kProtocolVersion + 1;
  msg.cmd = k6wp::Cmd::pause;
  const std::string wire = k6wp::Encode(msg);

  k6wp::IpcMessage out;
  CHECK(!k6wp::Decode(wire, out));
  // Raw JSON with a wrong version field.
  CHECK(!k6wp::Decode("{\"version\":99,\"cmd\":\"pause\",\"payload\":{}}\n",
                      out));
  // Missing version field entirely.
  CHECK(!k6wp::Decode("{\"cmd\":\"pause\",\"payload\":{}}\n", out));
}

// --- Test 3: unknown cmd string -> Decode false ----------------------------
void TestUnknownCmd() {
  k6wp::IpcMessage out;
  CHECK(!k6wp::Decode("{\"version\":1,\"cmd\":\"explode\",\"payload\":{}}\n",
                      out));
  CHECK(!k6wp::Decode("{\"version\":1,\"cmd\":\"\",\"payload\":{}}\n", out));
  CHECK(!k6wp::Decode("{\"version\":1,\"cmd\":123,\"payload\":{}}\n", out));
}

// --- Test 4: payload > 64 KiB -> Encode throws, Decode false ---------------
void TestPayloadTooLarge() {
  const std::string big(k6wp::kMaxPayloadBytes + 1, 'x');

  k6wp::IpcMessage msg;
  msg.cmd = k6wp::Cmd::set_video;
  msg.payload = {{"blob", big}};
  bool threw = false;
  try {
    (void)k6wp::Encode(msg);
  } catch (const k6wp::IpcError&) {
    threw = true;
  }
  CHECK(threw);

  // Decode side: a wire line whose payload exceeds the limit.
  const std::string wire =
      "{\"version\":1,\"cmd\":\"set_video\",\"payload\":{\"blob\":\"" + big +
      "\"}}\n";
  k6wp::IpcMessage out;
  CHECK(!k6wp::Decode(wire, out));
}

// --- Test 5: malformed JSON (no '\n', garbage) -> false, no crash ----------
void TestMalformedJson() {
  k6wp::IpcMessage out;
  CHECK(!k6wp::Decode("this is not json", out));
  CHECK(!k6wp::Decode("{\"version\":1,\"cmd\":\"pa", out));  // truncated
  CHECK(!k6wp::Decode("{\"version\":1,\"cmd\":\"pause\",\"payload\":{}",
                      out));  // unterminated, no '\n'
  CHECK(!k6wp::Decode("{\"version\":1,\"cmd\":\"pause\",\"payload\":{}}\n\n",
                      out));  // embedded newline = multi-line
  CHECK(!k6wp::Decode("[1,2,3]\n", out));  // valid JSON, not an object
}

// --- Test 6: ACL helper returns a current-user-only DACL -------------------
void TestMakeCurrentUserOnlySA() {
  PSECURITY_ATTRIBUTES sa = k6wp::MakeCurrentUserOnlySA();
  CHECK(sa != nullptr);
  if (sa == nullptr) {
    return;
  }
  CHECK(sa->nLength == sizeof(SECURITY_ATTRIBUTES));
  CHECK(sa->lpSecurityDescriptor != nullptr);

  BOOL dacl_present = FALSE;
  BOOL dacl_defaulted = FALSE;
  PACL dacl = nullptr;
  CHECK(GetSecurityDescriptorDacl(sa->lpSecurityDescriptor, &dacl_present,
                                  &dacl, &dacl_defaulted));
  CHECK(dacl_present == TRUE);
  CHECK(dacl != nullptr);

  ACL_SIZE_INFORMATION acl_info = {};
  CHECK(GetAclInformation(dacl, &acl_info, sizeof(acl_info),
                          AclSizeInformation));
  CHECK(acl_info.AceCount >= 1);

  // First ACE must be an access-allowed ACE for the current user's SID.
  LPVOID ace = nullptr;
  CHECK(GetAce(dacl, 0, &ace));
  if (ace != nullptr) {
    const ACCESS_ALLOWED_ACE* aa_ace =
        static_cast<const ACCESS_ALLOWED_ACE*>(ace);
    CHECK(aa_ace->Header.AceType == ACCESS_ALLOWED_ACE_TYPE);
    PSID ace_sid =
        reinterpret_cast<PSID>(&const_cast<ACCESS_ALLOWED_ACE*>(aa_ace)->SidStart);

    // Current user SID via GetTokenInformation.
    HANDLE h_token = nullptr;
    CHECK(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &h_token));
    if (h_token != nullptr) {
      DWORD len = 0;
      GetTokenInformation(h_token, TokenUser, nullptr, 0, &len);
      std::vector<BYTE> buf(len);
      CHECK(GetTokenInformation(h_token, TokenUser, buf.data(), len, &len));
      const TOKEN_USER* tu = reinterpret_cast<const TOKEN_USER*>(buf.data());
      CHECK(EqualSid(ace_sid, tu->User.Sid));
      CloseHandle(h_token);
    }
  }

  // Caller contract: LocalFree both the SA and its descriptor.
  LocalFree(sa->lpSecurityDescriptor);
  LocalFree(sa);
}

// --- Test 7: empty line -> Decode false ------------------------------------
void TestEmptyLine() {
  k6wp::IpcMessage out;
  CHECK(!k6wp::Decode("", out));
  CHECK(!k6wp::Decode("\n", out));
  CHECK(!k6wp::Decode("\r\n", out));
}

// --- Test 8: valid JSON but missing cmd -> Decode false --------------------
void TestMissingCmd() {
  k6wp::IpcMessage out;
  CHECK(!k6wp::Decode("{\"version\":1,\"payload\":{}}\n", out));
  CHECK(!k6wp::Decode("{\"version\":1}\n", out));
}

// --- Bonus: framing leniency + Cmd mapping helpers -------------------------
void TestFramingAndHelpers() {
  // Valid JSON without a trailing '\n' is still one line -> accepted.
  k6wp::IpcMessage out;
  CHECK(k6wp::Decode("{\"version\":1,\"cmd\":\"pause\",\"payload\":{}}", out));
  CHECK(out.cmd == k6wp::Cmd::pause);

  // CmdToString / CmdFromString round-trip.
  for (int i = 0; i < 6; ++i) {
    const k6wp::Cmd cmd = static_cast<k6wp::Cmd>(i);
    const char* name = k6wp::CmdToString(cmd);
    CHECK(name != nullptr);
    k6wp::Cmd back = k6wp::Cmd::get_state;
    CHECK(k6wp::CmdFromString(name, back));
    CHECK(back == cmd);
  }
  CHECK(k6wp::CmdToString(static_cast<k6wp::Cmd>(999)) == nullptr);
  k6wp::Cmd dummy = k6wp::Cmd::get_state;
  CHECK(!k6wp::CmdFromString("nope", dummy));
  CHECK(!k6wp::CmdFromString(nullptr, dummy));
}

// --- Test 9: quit round-trip + dispatch routing ---------------------------
void TestQuitCommand() {
  CHECK(std::string(k6wp::CmdToString(k6wp::Cmd::quit)) == "quit");
  k6wp::Cmd cmd = k6wp::Cmd::get_state;
  CHECK(k6wp::CmdFromString("quit", cmd));
  CHECK(cmd == k6wp::Cmd::quit);

  k6wp::IpcMessage msg;
  msg.version = k6wp::kProtocolVersion;
  msg.cmd = k6wp::Cmd::quit;
  msg.payload = nlohmann::json::object();
  const std::string wire = k6wp::Encode(msg);
  CHECK(!wire.empty());
  CHECK(wire.back() == '\n');

  k6wp::IpcMessage out;
  CHECK(k6wp::Decode(wire, out));
  CHECK(out.cmd == k6wp::Cmd::quit);
  CHECK(out.payload == nlohmann::json::object());

  k6wp::IpcMessage dispatched;
  CHECK(k6wp::Decode("{\"version\":1,\"cmd\":\"quit\",\"payload\":{}}\n",
                     dispatched));
  CHECK(dispatched.cmd == k6wp::Cmd::quit);
}

}  // namespace

int main() {
  TestRoundTripAllCmds();
  TestVersionMismatch();
  TestUnknownCmd();
  TestPayloadTooLarge();
  TestMalformedJson();
  TestMakeCurrentUserOnlySA();
  TestEmptyLine();
  TestMissingCmd();
  TestFramingAndHelpers();
  TestQuitCommand();

  std::printf("ipc_test: %d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}