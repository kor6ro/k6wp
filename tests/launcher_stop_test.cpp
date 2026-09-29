// Unit tests for the launcher's uninstall support paths (1.2.0 release fixes
// for two audited defects):
//
//   A) the uninstallers used to `taskkill /F /IM engine.exe` etc., which kills
//      EVERY process on the machine that happens to share the image name. The
//      replacement is directory-scoped: only images that live directly next to
//      THIS K6WP.exe are eligible (IsK6wpAppImageName + IsK6wpOwnImage), and
//      termination is per-PID (TerminateProcess), never by name.
//   B) `K6WP.exe --elevate-lockscreen off` used to return as soon as
//      ShellExecuteW accepted the runas verb, so the uninstaller deleted
//      K6WP.exe while the elevated worker was still writing HKLM. The parent now
//      waits for the worker with a bounded WaitForSingleObject and reports a
//      distinct exit code per outcome, which the uninstallers surface.
//
// No external test framework: plain asserts with a pass/fail counter, same
// style as tests/lockscreen_backup_test.cpp. Exit code 0 = all pass.
//
// The functions under test live in an anonymous namespace inside
// launcher/main.cpp (single-TU dispatcher by design), so this test includes the
// .cpp directly. wWinMain is compiled but unused (this target is
// console-subsystem, so the test's own main() is the entry point).
//
// NOT unit-tested here, and why (verified by inspection instead):
//   * the actual pipe round-trip (connect / WriteFile / overlapped ack read) and
//     the WM_CLOSE + WaitForSingleObject(process) loop need a live engine /
//     live windows, so they are exercised by the end-to-end uninstall paths
//     and by reading the code against engine/src/ipc_server.cpp;
//   * ShellExecuteExW runas + WaitForSingleObject on the elevated worker needs
//     an interactive UAC prompt, so only its DECISION logic
//     (ClassifyElevatedWait / LockscreenExitCodeFor) is covered here.
#include "../launcher/main.cpp"

#include <iostream>
#include <string>

namespace {

int g_checks = 0;
int g_failures = 0;

void Check(bool cond, const std::string& name) {
  ++g_checks;
  if (cond) {
    std::cout << "[PASS] " << name << "\n";
  } else {
    ++g_failures;
    std::cout << "[FAIL] " << name << "\n";
  }
}

using k6wp::launcher::AckIsOk;
using k6wp::launcher::BuildEngineCommandLine;
using k6wp::launcher::ClassifyElevatedWait;
using k6wp::launcher::DescribeLockscreenRestore;
using k6wp::launcher::ElevatedWaitOutcome;
using k6wp::launcher::IsK6wpAppImageName;
using k6wp::launcher::IsK6wpOwnImage;
using k6wp::launcher::kElevatedWorkerWaitMs;
using k6wp::launcher::kExitElevateDeclined;
using k6wp::launcher::kExitElevateTimeout;
using k6wp::launcher::kExitOk;
using k6wp::launcher::kExitSpawnFailure;
using k6wp::launcher::LockscreenExitCodeFor;
using k6wp::launcher::PidFromStateJson;

const wchar_t* const kOwnDir = L"C:\\Users\\me\\AppData\\Local\\K6WP";

// ---------------------------------------------------------------------------
// Defect A: which processes the uninstaller is allowed to stop.
// ---------------------------------------------------------------------------
void TestImageNameAllowlist() {
  Check(IsK6wpAppImageName(L"engine.exe"), "name: engine.exe");
  Check(IsK6wpAppImageName(L"studio.exe"), "name: studio.exe");
  Check(IsK6wpAppImageName(L"K6WP.exe"), "name: K6WP.exe");
  Check(IsK6wpAppImageName(L"ENGINE.EXE"), "name: ENGINE.EXE (case-insensitive)");
  // Same-base-name traps a name-based kill would fall into.
  Check(!IsK6wpAppImageName(L"engine_x.exe"), "name: engine_x.exe rejected");
  Check(!IsK6wpAppImageName(L"compressor.exe"), "name: compressor.exe rejected");
  Check(!IsK6wpAppImageName(L"ffmpeg.exe"), "name: ffmpeg.exe rejected");
  Check(!IsK6wpAppImageName(L"engine.exe.bak"), "name: engine.exe.bak rejected");
  Check(!IsK6wpAppImageName(L""), "name: empty rejected");
}

void TestOwnImageIsDirectoryScoped() {
  // The own-folder cases: these are the processes the uninstaller must stop.
  Check(IsK6wpOwnImage(std::wstring(kOwnDir) + L"\\engine.exe", kOwnDir),
        "own image: engine.exe in own dir");
  Check(IsK6wpOwnImage(std::wstring(kOwnDir) + L"\\studio.exe", kOwnDir),
        "own image: studio.exe in own dir");
  Check(IsK6wpOwnImage(std::wstring(kOwnDir) + L"\\K6WP.exe", kOwnDir),
        "own image: K6WP.exe in own dir");
  // THE defect: a colliding image name in another folder must NOT match.
  Check(!IsK6wpOwnImage(L"C:\\Tools\\engine.exe", kOwnDir),
        "collision: C:\\Tools\\engine.exe NOT ours");
  Check(!IsK6wpOwnImage(L"C:\\Program Files\\Some App\\studio.exe", kOwnDir),
        "collision: Program Files studio.exe NOT ours");
  Check(!IsK6wpOwnImage(L"C:\\Users\\other\\AppData\\Local\\K6WP\\engine.exe",
                        kOwnDir),
        "collision: another user's install NOT ours");
  // A sibling PREFIX directory is a different folder, not ours.
  Check(!IsK6wpOwnImage(L"C:\\Users\\me\\AppData\\Local\\K6WP-old\\engine.exe",
                        kOwnDir),
        "prefix dir: K6WP-old NOT ours");
  // Not directly in the own dir (vendored / plugin subfolder).
  Check(!IsK6wpOwnImage(std::wstring(kOwnDir) + L"\\plugins\\engine.exe",
                        kOwnDir),
        "subdir: plugins\\engine.exe NOT ours");
  // Right folder, wrong image.
  Check(!IsK6wpOwnImage(std::wstring(kOwnDir) + L"\\ffmpeg.exe", kOwnDir),
        "own dir: ffmpeg.exe NOT ours");
  // Windows paths are case-insensitive; a differently-cased own dir still
  // matches, otherwise the uninstall would silently stop nothing.
  Check(IsK6wpOwnImage(L"c:\\users\\ME\\appdata\\local\\k6wp\\STUDIO.EXE", kOwnDir),
        "own image: case-insensitive dir + name match");
  // A trailing separator on the own dir must not break the comparison.
  Check(IsK6wpOwnImage(std::wstring(kOwnDir) + L"\\engine.exe",
                      std::wstring(kOwnDir) + L"\\"),
        "own image: trailing separator on own dir");
  Check(!IsK6wpOwnImage(L"", kOwnDir), "own image: empty path rejected");
  Check(!IsK6wpOwnImage(std::wstring(kOwnDir) + L"\\engine.exe", L""),
        "own image: empty own dir rejected");
}

// ---------------------------------------------------------------------------
// IPC wire format (must stay byte-identical to shared/ipc_protocol.hpp v1; the
// decode side of the same format is locked by tests/ipc_test.cpp).
// ---------------------------------------------------------------------------
void TestCommandLine() {
  Check(BuildEngineCommandLine(L"quit") ==
            "{\"version\":1,\"cmd\":\"quit\",\"payload\":{}}\n",
        "ipc: quit line is exactly the v1 NDJSON frame");
  Check(BuildEngineCommandLine(L"get_state") ==
            "{\"version\":1,\"cmd\":\"get_state\",\"payload\":{}}\n",
        "ipc: get_state line is exactly the v1 NDJSON frame");
  const std::string quit = BuildEngineCommandLine(L"quit");
  Check(!quit.empty() && quit.back() == '\n',
        "ipc: trailing newline (server ignores frameless datagrams)");
  Check(quit.find("\"version\":1") != std::string::npos,
        "ipc: protocol version stays 1");
}

void TestAckIsOk() {
  Check(AckIsOk("{\"ok\":true}\n"), "ack: quit ack");
  Check(AckIsOk("{\"ok\":true,\"state\":{\"pid\":7}}\n"),
        "ack: get_state ack");
  // An old engine (protocol v1, no quit handler) answers with an error ack.
  Check(!AckIsOk("{\"error\":\"unknown command\"}\n"),
        "ack: old-engine error ack rejected");
  Check(!AckIsOk("{\"ok\":false}\n"), "ack: ok:false rejected");
  Check(!AckIsOk(""), "ack: empty rejected");
  Check(!AckIsOk("not json at all"), "ack: garbage rejected");
  Check(!AckIsOk("{\"ok\":true}"), "ack: frameless ack rejected");
}

void TestPidFromStateJson() {
  Check(PidFromStateJson(
            "{\"ok\":true,\"state\":{\"running\":true,\"pid\":12345,"
            "\"video\":\"C:\\\\v.mp4\"}}\n") == 12345,
        "state pid: extracted");
  Check(PidFromStateJson("{\"ok\":true,\"state\":{\"pid\":1}}\n") == 1,
        "state pid: last field");
  // A video PATH containing the literal text "pid": must not be read as the
  // key (the scanner tracks string literals and escapes). The decoded value
  // here is C:\vid\"pid":9999.jpg.
  Check(PidFromStateJson(
            R"({"ok":true,"state":{"video":"C:\\vid\\\"pid\":9999.jpg","pid":4242}})") ==
            4242,
        "state pid: pid-looking text inside a path ignored");
  // Malformed input must never yield a guessed pid (the "pid" text there is
  // inside an unterminated string, so the key scan must give up).
  Check(PidFromStateJson(
            R"({"ok":true,"state":{"video":"C:\\vid\\\"pid":9999.jpg","pid":4242}})") ==
            0,
        "state pid: malformed frame yields no pid");
  // Wrong nesting depth is not the engine's pid field.
  Check(PidFromStateJson("{\"ok\":true,\"state\":{\"a\":{\"pid\":5}}}\n") == 0,
        "state pid: nested pid ignored");
  Check(PidFromStateJson("{\"ok\":true,\"state\":{}}\n") == 0,
        "state pid: no pid field");
  Check(PidFromStateJson("{\"ok\":true}\n") == 0, "state pid: no state object");
  Check(PidFromStateJson("{\"error\":\"unknown command\"}\n") == 0,
        "state pid: error ack");
  Check(PidFromStateJson("{\"ok\":true,\"state\":{\"pid\":0}}\n") == 0,
        "state pid: 0 rejected");
  Check(PidFromStateJson("{\"ok\":true,\"state\":{\"pid\":\"12\"}}\n") == 0,
        "state pid: string value rejected");
  Check(PidFromStateJson("{\"ok\":true,\"state\":{\"pid\":-3}}\n") == 0,
        "state pid: negative rejected");
  Check(PidFromStateJson("{\"ok\":true,\"state\":{\"pid\":99999999999}}\n") == 0,
        "state pid: out of DWORD range rejected");
  Check(PidFromStateJson("{\"ok\":true,\"state\":{\"pid\":}}\n") == 0,
        "state pid: missing value rejected");
}

// ---------------------------------------------------------------------------
// Defect B: wait/timeout decision + the exit code the uninstallers read.
// ---------------------------------------------------------------------------
void TestClassifyElevatedWait() {
  // ShellExecuteExW returned FALSE + ERROR_CANCELLED == the user dismissed the
  // UAC consent dialog. Must be distinguishable from a launch failure.
  Check(ClassifyElevatedWait(false, ERROR_CANCELLED, WAIT_FAILED, 0) ==
            ElevatedWaitOutcome::kDeclined,
        "elevate: declined UAC");
  Check(ClassifyElevatedWait(false, ERROR_FILE_NOT_FOUND, WAIT_FAILED, 0) ==
            ElevatedWaitOutcome::kLaunchFailed,
        "elevate: launch failure (not a decline)");
  Check(ClassifyElevatedWait(false, ERROR_ACCESS_DENIED, WAIT_FAILED, 0) ==
            ElevatedWaitOutcome::kLaunchFailed,
        "elevate: access denied -> launch failure");
  // Bounded wait: the budget expiring is its own outcome (worker is killed).
  Check(ClassifyElevatedWait(true, ERROR_SUCCESS, WAIT_TIMEOUT, 0) ==
            ElevatedWaitOutcome::kTimedOut,
        "elevate: wait timeout");
  Check(ClassifyElevatedWait(true, ERROR_SUCCESS, WAIT_OBJECT_0, 0) ==
            ElevatedWaitOutcome::kRestored,
        "elevate: worker exited 0");
  Check(ClassifyElevatedWait(true, ERROR_SUCCESS, WAIT_OBJECT_0, 3) ==
            ElevatedWaitOutcome::kWorkerFailed,
        "elevate: worker exited non-zero");
  Check(ClassifyElevatedWait(true, ERROR_SUCCESS, WAIT_FAILED, 0) ==
            ElevatedWaitOutcome::kWaitFailed,
        "elevate: wait itself failed");
  // A launched worker that somehow reports a nonzero wait result must never
  // be reported as success.
  Check(ClassifyElevatedWait(true, ERROR_SUCCESS, WAIT_ABANDONED, 0) !=
            ElevatedWaitOutcome::kRestored,
        "elevate: WAIT_ABANDONED is not success");
}

void TestExitCodes() {
  Check(LockscreenExitCodeFor(ElevatedWaitOutcome::kRestored) == kExitOk,
        "exit code: restored -> 0");
  Check(LockscreenExitCodeFor(ElevatedWaitOutcome::kDeclined) ==
            kExitElevateDeclined,
        "exit code: declined -> distinct decline code");
  Check(LockscreenExitCodeFor(ElevatedWaitOutcome::kTimedOut) ==
            kExitElevateTimeout,
        "exit code: timeout -> distinct timeout code");
  Check(LockscreenExitCodeFor(ElevatedWaitOutcome::kWorkerFailed) ==
            kExitSpawnFailure,
        "exit code: worker failure -> error code");
  Check(LockscreenExitCodeFor(ElevatedWaitOutcome::kLaunchFailed) ==
            kExitSpawnFailure,
        "exit code: launch failure -> error code");
  Check(LockscreenExitCodeFor(ElevatedWaitOutcome::kWaitFailed) ==
            kExitSpawnFailure,
        "exit code: wait failure -> error code");
  // The codes must be distinct, or the uninstaller cannot tell the cases apart.
  Check(kExitOk != kExitElevateDeclined && kExitOk != kExitElevateTimeout &&
            kExitElevateDeclined != kExitElevateTimeout,
        "exit code: 0 / decline / timeout are pairwise distinct");
  // A mandatory, finite bound: the uninstaller must never hang forever.
  Check(kElevatedWorkerWaitMs > 0 && kElevatedWorkerWaitMs <= 10 * 60 * 1000,
        "wait budget: finite and <= 10 min");
}

void TestDescribeLockscreenRestore() {
  const std::wstring ok = DescribeLockscreenRestore(kExitOk);
  Check(ok.empty(), "message: success says nothing to restore");

  // Every failure message must tell the user (a) what is left behind and
  // (b) the exact command that fixes it. The uninstallers print the same
  // wording; this string is the authoritative mapping.
  const int failures[] = {kExitElevateDeclined, kExitElevateTimeout,
                          kExitSpawnFailure, 2, 99};
  for (const int code : failures) {
    const std::wstring msg = DescribeLockscreenRestore(code);
    Check(!msg.empty(),
          "message: code " + std::to_string(code) + " is not silent");
    Check(msg.find(L"--elevate-lockscreen off") != std::wstring::npos,
          "message: code " + std::to_string(code) +
              " names the manual restore command");
    Check(msg.find(L"LockScreenImage") != std::wstring::npos,
          "message: code " + std::to_string(code) +
              " names the stale policy value");
  }
  Check(DescribeLockscreenRestore(kExitElevateDeclined).find(L"declin") !=
            std::wstring::npos,
        "message: decline is described as a decline");
  Check(DescribeLockscreenRestore(kExitElevateTimeout).find(L"still") !=
            std::wstring::npos,
        "message: timeout says the worker is still running");
}

}  // namespace

int main() {
  TestImageNameAllowlist();
  TestOwnImageIsDirectoryScoped();
  TestCommandLine();
  TestAckIsOk();
  TestPidFromStateJson();
  TestClassifyElevatedWait();
  TestExitCodes();
  TestDescribeLockscreenRestore();
  std::cout << (g_failures == 0 ? "ALL PASS" : "FAILURES") << " ("
            << g_checks << " checks, " << g_failures << " failures)\n";
  return g_failures == 0 ? 0 : 1;
}
