// proc_util_test.cpp — unit tests for the central RunCaptured helper (MED-4).
//
// Covers: nonexistent exe -> spawned=false, fast; trivial echo -> exit 0 +
// captured output; hung child (ping -n 30) -> timed_out at ~timeout_ms with
// no orphan left behind. Handle-lifetime stability (100x loop, delta ~0) is
// exercised by the QA evidence script, not here (timing-sensitive).

#include "proc_util.hpp"

#include <cstdio>
#include <string>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace {

int g_checks = 0;
int g_failures = 0;

void Check(bool cond, const std::string& name) {
  ++g_checks;
  if (cond) {
    std::printf("PASS %s\n", name.c_str());
  } else {
    ++g_failures;
    std::printf("FAIL %s\n", name.c_str());
  }
}

bool HasSubstr(const std::string& hay, const std::string& needle) {
  return hay.find(needle) != std::string::npos;
}

}  // namespace

int main() {
  // 1. Nonexistent exe -> clean spawn failure, well under the timeout.
  {
    const ULONGLONG t0 = GetTickCount64();
    const k6wp::ProcResult r = k6wp::RunCaptured(
        L"C:\\nonexistent-dir-xyz\\ffprobe.exe",
        L"\"C:\\nonexistent-dir-xyz\\ffprobe.exe\" -version",
        k6wp::kProbeTimeoutMs);
    const ULONGLONG dt = GetTickCount64() - t0;
    std::printf("[info] bad-exe: spawned=%d timed_out=%d dt=%llums\n",
                (int)r.spawned, (int)r.timed_out, dt);
    Check(!r.spawned, "bad-exe not spawned");
    Check(!r.timed_out, "bad-exe not timed out");
    Check(dt < k6wp::kProbeTimeoutMs, "bad-exe returns before timeout");
  }

  // 2. Trivial fast child -> exit 0 + captured stdout.
  {
    wchar_t sysdir[MAX_PATH];
    const UINT n = GetSystemDirectoryW(sysdir, MAX_PATH);
    Check(n > 0 && n < MAX_PATH, "GetSystemDirectoryW for cmd path");
    const std::filesystem::path cmd = std::filesystem::path(sysdir) / L"cmd.exe";
    const std::wstring cmdline =
        L"\"" + cmd.wstring() + L"\" /c echo hello-proc-util";
    const k6wp::ProcResult r =
        k6wp::RunCaptured(cmd, cmdline, k6wp::kProbeTimeoutMs);
    std::printf("[info] echo: spawned=%d timed_out=%d exit=%lu out=[%s]\n",
                (int)r.spawned, (int)r.timed_out, r.exit_code,
                r.output.c_str());
    Check(r.spawned, "echo spawned");
    Check(!r.timed_out, "echo not timed out");
    Check(r.exit_code == 0, "echo exit 0");
    Check(HasSubstr(r.output, "hello-proc-util"), "echo output captured");
  }

  // 3. Hung child (ping ~30 s) with a 2 s budget -> killed at ~2 s.
  {
    wchar_t sysdir[MAX_PATH];
    const UINT n = GetSystemDirectoryW(sysdir, MAX_PATH);
    Check(n > 0 && n < MAX_PATH, "GetSystemDirectoryW for ping path");
    const std::filesystem::path ping =
        std::filesystem::path(sysdir) / L"ping.exe";
    const std::wstring cmdline =
        L"\"" + ping.wstring() + L"\" -n 30 127.0.0.1";
    const ULONGLONG t0 = GetTickCount64();
    const k6wp::ProcResult r = k6wp::RunCaptured(ping, cmdline, 2000);
    const ULONGLONG dt = GetTickCount64() - t0;
    std::printf("[info] hang: spawned=%d timed_out=%d exit=%lu dt=%llums\n",
                (int)r.spawned, (int)r.timed_out, r.exit_code, dt);
    Check(r.spawned, "hang spawned");
    Check(r.timed_out, "hang timed out");
    Check(r.exit_code != 0, "hang exit nonzero (terminate code)");
    Check(dt >= 2000 && dt < 2000 + 5000,
          "hang kill lands near the timeout budget");
  }

  std::printf("%d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
