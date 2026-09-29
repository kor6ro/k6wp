// Unit tests for the PostMessage-marshal payload validators (CRIT-2,
// audit-remediation todo 11). Build: ipc_marshal_test target in
// engine/CMakeLists.txt (compiles ../engine/src/ipc_marshal.cpp standalone:
// windows.h-free, only nlohmann/json + <filesystem>). Exit 0 = all pass.
//
// What this locks: the worker-side accept/reject policy must stay identical
// to the old inline handlers — a bad payload acks {"error"} ("ditolak") and
// never reaches the main-loop queue, while a good payload yields exactly the
// pending value the main thread will execute.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "ipc_marshal.hpp"

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

// --- set_monitor: accepted shapes -------------------------------------------
void TestMonitorAccepted() {
  auto a = k6wp::ParseSetMonitorPayload(R"({"monitor":-1})");
  CHECK(a.has_value() && *a == -1);  // all screens
  auto b = k6wp::ParseSetMonitorPayload(R"({"monitor":0})");
  CHECK(b.has_value() && *b == 0);
  auto c = k6wp::ParseSetMonitorPayload(R"({"monitor_id":2})");
  CHECK(c.has_value() && *c == 2);  // alias honored
  auto d = k6wp::ParseSetMonitorPayload(R"({"monitor":1,"extra":true})");
  CHECK(d.has_value() && *d == 1);  // unknown fields ignored
}

// --- set_monitor: rejected shapes (ack {"error"}, nothing queued) -----------
void TestMonitorRejected() {
  CHECK(!k6wp::ParseSetMonitorPayload(R"({"monitor":-2})").has_value());
  CHECK(!k6wp::ParseSetMonitorPayload(R"({"monitor":"0"})").has_value());
  CHECK(!k6wp::ParseSetMonitorPayload(R"({"monitor":1.5})").has_value());
  CHECK(!k6wp::ParseSetMonitorPayload(R"({})").has_value());
  CHECK(!k6wp::ParseSetMonitorPayload(R"([])").has_value());
  CHECK(!k6wp::ParseSetMonitorPayload("not json").has_value());
  CHECK(!k6wp::ParseSetMonitorPayload("").has_value());
}

// --- set_video: accepted shape echoes the path back --------------------------
void TestVideoAccepted() {
  // Fixture: a real temp file (CWD-independent — __FILE__ may be relative
  // to the build dir, so it cannot serve as the "existing file" probe).
  const std::filesystem::path fixture =
      std::filesystem::temp_directory_path() / "k6wp-ipc-marshal-fixture.mp4";
  {
    std::ofstream out(fixture, std::ios::binary);
    out << "fixture";
  }
  const std::string self = fixture.u8string();
  // JSON-encode the path (a Windows temp path carries backslashes, which
  // must be escaped to survive the payload parse).
  std::string encoded;
  for (char c : self) {
    if (c == '\\') {
      encoded += "\\\\";
    } else {
      encoded += c;
    }
  }
  const std::string payload = std::string(R"({"path":")") + encoded + R"("})";
  auto v = k6wp::ValidateSetVideoPayload(payload);
  CHECK(v.has_value() && *v == self);
  std::error_code ec;
  std::filesystem::remove(fixture, ec);
}

// --- set_video: rejected shapes ----------------------------------------------
void TestVideoRejected() {
  CHECK(!k6wp::ValidateSetVideoPayload(R"({})").has_value());
  CHECK(!k6wp::ValidateSetVideoPayload(R"({"path":""})").has_value());
  CHECK(!k6wp::ValidateSetVideoPayload(R"({"path":42})").has_value());
  // Missing file: rejected, old video keeps playing.
  CHECK(!k6wp::ValidateSetVideoPayload(
                 R"({"path":"C:\\no\\such\\k6wp-test-file.mp4"})")
             .has_value());
  // A directory is not a playable file.
  CHECK(!k6wp::ValidateSetVideoPayload(R"({"path":"C:\\Windows"})")
             .has_value());
  CHECK(!k6wp::ValidateSetVideoPayload("garbage").has_value());
}

}  // namespace

int main() {
  TestMonitorAccepted();
  TestMonitorRejected();
  TestVideoAccepted();
  TestVideoRejected();
  std::printf("ipc_marshal_test: %d checks, %d failures\n", g_checks,
              g_failures);
  return g_failures == 0 ? 0 : 1;
}
