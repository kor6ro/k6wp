// Mini unit tests for the P2a tiered monitor identity (§R4.1/§R4.2/§R4.3).
// Drives the pure AssignStableKeys seam with synthetic inputs: no hardware,
// no CCD, no registry. Plain pass/fail counter; exit 0 = all pass.
//
// Contract under test:
//   - tier precedence ccd -> edid -> gdi, applied per monitor;
//   - a tier is used only when its candidate key is UNIQUE in the snapshot;
//   - Tier 3 (gdi) fallback flags the monitor degraded (ambiguous);
//   - deterministic, index-preserving output and pinned key encoding.
#include <iostream>
#include <string>
#include <vector>

#include "monitor_util.hpp"

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

// Convenience builders.
k6wp::MonitorIdentityInputs Ccd(const std::string& path,
                                const std::wstring& name) {
  k6wp::MonitorIdentityInputs in;
  in.tier1_path = path;
  in.device_name = name;
  return in;
}

k6wp::MonitorIdentityInputs Edid(const std::string& mfg,
                                 const std::string& product,
                                 const std::string& serial_hex,
                                 const std::wstring& name) {
  k6wp::MonitorIdentityInputs in;
  in.tier2_usable = true;
  in.edid_mfg = mfg;
  in.edid_product = product;
  in.edid_serial_hex = serial_hex;
  in.device_name = name;
  return in;
}

std::string GdiKey(const wchar_t* name) {
  // Mirror the production Tier 3 key for the ASCII device names used here.
  std::string narrow;
  for (const wchar_t* p = name; *p; ++p) narrow.push_back(static_cast<char>(*p));
  return "gdi:" + narrow;
}

}  // namespace

int main() {
  using k6wp::AssignStableKeys;
  using k6wp::AssignedIdentity;

  // 1. Tier 1 unique -> ccd:<lower path>, not degraded.
  {
    const auto out = AssignStableKeys({
        Ccd("\\\\?\\DISPLAY#AAA#1", L"\\\\.\\DISPLAY1"),
        Ccd("\\\\?\\DISPLAY#BBB#2", L"\\\\.\\DISPLAY2"),
    });
    Check(out.size() == 2, "1. one identity per input");
    Check(out[0].stable_key == "ccd:\\\\?\\display#aaa#1",
          "1. tier1 key lowercased");
    Check(out[1].stable_key == "ccd:\\\\?\\display#bbb#2",
          "1. tier1 key lowercased (2)");
    Check(!out[0].degraded && !out[1].degraded, "1. tier1 not degraded");
  }

  // 2. Tier 1 duplicate -> fall to Tier 2 when the EDID is unique.
  {
    const auto out = AssignStableKeys({
        Ccd("\\\\?\\DISPLAY#SAME#1", L"\\\\.\\DISPLAY1"),
        Ccd("\\\\?\\DISPLAY#same#1", L"\\\\.\\DISPLAY2"),  // same after lower()
    });
    // No EDID -> both fall to GDI + degraded.
    Check(out[0].stable_key == GdiKey(L"\\\\.\\DISPLAY1"),
          "2. dup tier1 -> gdi (1)");
    Check(out[1].stable_key == GdiKey(L"\\\\.\\DISPLAY2"),
          "2. dup tier1 -> gdi (2)");
    Check(out[0].degraded && out[1].degraded, "2. dup tier1 -> degraded");
  }

  // 3. Tier 2 unique (serial != 0) -> pinned edid:<MFG>:<PPPP>:<hex>.
  {
    const auto out = AssignStableKeys({
        Edid("DEL", "4098", "1a2b", L"\\\\.\\DISPLAY1"),
    });
    Check(out[0].stable_key == "edid:DEL:4098:1a2b",
          "3. edid key exact format");
    Check(!out[0].degraded, "3. unique edid not degraded");
  }

  // 4. Tier 2 with serial 0 is NOT usable -> gdi + degraded.
  {
    const auto out = AssignStableKeys({
        Edid("DEL", "4098", "0", L"\\\\.\\DISPLAY1"),
    });
    Check(out[0].stable_key == GdiKey(L"\\\\.\\DISPLAY1"),
          "4. serial 0 -> gdi");
    Check(out[0].degraded, "4. serial 0 -> degraded");
  }

  // 5. Tier 2 duplicate (identical model + serial) -> gdi + degraded for both.
  {
    const auto out = AssignStableKeys({
        Edid("DEL", "4098", "5", L"\\\\.\\DISPLAY1"),
        Edid("DEL", "4098", "5", L"\\\\.\\DISPLAY2"),
    });
    Check(out[0].stable_key == GdiKey(L"\\\\.\\DISPLAY1"),
          "5. ambiguous -> gdi (1)");
    Check(out[1].stable_key == GdiKey(L"\\\\.\\DISPLAY2"),
          "5. ambiguous -> gdi (2)");
    Check(out[0].degraded && out[1].degraded, "5. ambiguous -> degraded");
  }

  // 6. Distinct-usable Tier 2 serials each get their own edid key.
  {
    const auto out = AssignStableKeys({
        Edid("DEL", "4098", "1", L"\\\\.\\DISPLAY1"),
        Edid("DEL", "4098", "2", L"\\\\.\\DISPLAY2"),
    });
    Check(out[0].stable_key == "edid:DEL:4098:1", "6. unique serial (1)");
    Check(out[1].stable_key == "edid:DEL:4098:2", "6. unique serial (2)");
    Check(!out[0].degraded && !out[1].degraded, "6. unique serial not degraded");
  }

  // 7. Tier 1 present & unique wins even when Tier 2 is also available.
  {
    k6wp::MonitorIdentityInputs in = Edid("DEL", "4098", "9", L"\\\\.\\DISPLAY1");
    in.tier1_path = "\\\\?\\DISPLAY#CCC#3";
    const auto out = AssignStableKeys({in});
    Check(out[0].stable_key == "ccd:\\\\?\\display#ccc#3",
          "7. tier1 precedence over tier2");
    Check(!out[0].degraded, "7. tier1 precedence not degraded");
  }

  // 8. No tier1/tier2 -> gdi + degraded; index order preserved.
  {
    k6wp::MonitorIdentityInputs a;
    a.device_name = L"\\\\.\\DISPLAY3";
    k6wp::MonitorIdentityInputs b;
    b.device_name = L"\\\\.\\DISPLAY1";
    const auto out = AssignStableKeys({a, b});
    Check(out[0].stable_key == GdiKey(L"\\\\.\\DISPLAY3"),
          "8. index order preserved (0)");
    Check(out[1].stable_key == GdiKey(L"\\\\.\\DISPLAY1"),
          "8. index order preserved (1)");
    Check(out[0].degraded && out[1].degraded, "8. bare -> degraded");
  }

  // 9. Empty input -> empty output (no crash).
  {
    const auto out = AssignStableKeys({});
    Check(out.empty(), "9. empty input -> empty output");
  }

  // 10. Serial hex is lowercase + minimal (0x00001a2b -> "1a2b" via 4-byte
  //     little-endian is encoded upstream; here we assert the key is verbatim).
  {
    const auto out = AssignStableKeys({
        Edid("SAM", "0ABC", "deadbeef", L"\\\\.\\DISPLAY1"),
    });
    Check(out[0].stable_key == "edid:SAM:0ABC:deadbeef",
          "10. hex/product verbatim in key");
  }

  std::cout << "stable_key_test: " << g_checks << " checks, " << g_failures
            << " failures\n";
  return g_failures == 0 ? 0 : 1;
}
