# Repo hygiene (Phase 0.8)

Status: applied on `refactor/rust` after tag `baseline-pre-refactor`.
No historical evidence was deleted; root-level scratch evidence moved to
`docs/archive/`.

## Moved to `docs/archive/`

| File | Why |
|---|---|
| `test_output.txt` … `test_output5.txt` | repeated ctest logs (identical content) |
| `05-driver-vetting.md` | planning/evidence note, not a build input |
| `05-shadow.md` | planning/evidence note, not a build input |
| `rtm.md` | release task matrix snapshot |
| `install_guide.txt` | superseded by `docs/release-build.md` + README |

## Ignored, not deleted

- `video/` — Remotion promo project, ~1.2 GB with `node_modules/`. It is
  untracked and now ignored. Proposal: move it to a separate marketing repo
  (or `marketing/video/` in a future marketing branch) so the product repo
  stays source-only. Owner decision pending; nothing was removed.
- `shadow-evidence/karantina/` — third-party driver package (VDD Control).
  It was never tracked (already covered by `.git/info/exclude`). Only hashes
  are recorded below; the binaries stay local for audit and must never be
  committed (rule §12: no driver binaries in git).

## Quarantined driver hashes (SHA256)

```
A701F2272E9FCF382849B24F913C6DD07597B3B1116525F2E90182F019609154  shadow-evidence/karantina/VDD.Control.25.7.23.zip
CC6FF00C23E0E62A45EFB7ED59FC83AB0E7F8B0FEAB3E4C76203F360E5DE2F0F  shadow-evidence/karantina/VDD.Control.25.7.23/VDD Control.exe
10F2F1BB5905C0B4B09C2D6D8EFA7D8C94F4ABA0FC34988598A4DD0028F6B2A5  shadow-evidence/karantina/VDD.Control.25.7.23/VDD Control.pdb
77E87E7C4E23D6B9A4F5D86D15ED9EC61A16A242FB299C390AF6F179225BFA05  shadow-evidence/karantina/VDD.Control.25.7.23/Dependencies/devcon.exe
EDB2501D6D5DA17F66D15D4B97A6F4A3F0D8963165AC4A6A6259D95118288020  shadow-evidence/karantina/VDD.Control.25.7.23/Dependencies/vdd_settings.xml
9E62DD01DE5D80773819E6C92C76DACB175B4632E0D551026975DA5F815B5A97  shadow-evidence/karantina/VDD.Control.25.7.23/SignedDrivers/ARM64/VDD/mttvdd.cat
9BD5429A71F2103309B291C18D97DAA5E2D8E1A35794F375AE718F1F1EB27C4F  shadow-evidence/karantina/VDD.Control.25.7.23/SignedDrivers/ARM64/VDD/MttVDD.dll
A8B8D1C939EE49BB44B4CC3EF9A820B9943B1DBFA456D5B6880B6F106B51011B  shadow-evidence/karantina/VDD.Control.25.7.23/SignedDrivers/ARM64/VDD/MttVDD.inf
C023E471DD760B1F2E17E38E1EB6139B05B6E5FAD2952F9F34DE678FDA981723  shadow-evidence/karantina/VDD.Control.25.7.23/SignedDrivers/ARM64/VDD/vdd_settings.xml
75366382F49CD744A717E6FAC675495D6B634FE78F439F1FBBDDCF716ED65199  shadow-evidence/karantina/VDD.Control.25.7.23/SignedDrivers/x86/VAD/virtualaudiodriver.cat
6602076C5691A702E1EF0D11D44930282B9E9A5249B66F03A2E093A31E1C74CF  shadow-evidence/karantina/VDD.Control.25.7.23/SignedDrivers/x86/VAD/VirtualAudioDriver.inf
8F2DC680B0D218A18923F56D0B51D43729105072D4676436EB4B697FDFBB9AF3  shadow-evidence/karantina/VDD.Control.25.7.23/SignedDrivers/x86/VAD/VirtualAudioDriver.sys
08A0093FC9B2E32B287A6F8A77CA4DE0A31830D29FC33D2B13A918DC859468F6  shadow-evidence/karantina/VDD.Control.25.7.23/SignedDrivers/x86/VDD/mttvdd.cat
C9CA837F57A98FBD43BC416A7F535A95843626E7759EAF85CF0CD7CE334DBB05  shadow-evidence/karantina/VDD.Control.25.7.23/SignedDrivers/x86/VDD/MttVDD.dll
550D211FE481E74DFE3F9D724ED78BE48B3A9113405965D683D9373E8D672F5D  shadow-evidence/karantina/VDD.Control.25.7.23/SignedDrivers/x86/VDD/MttVDD.inf
EDB2501D6D5DA17F66D15D4B97A6F4A3F0D8963165AC4A6A6259D95118288020  shadow-evidence/karantina/VDD.Control.25.7.23/SignedDrivers/x86/VDD/vdd_settings.xml
```

## Bench JSON consolidation (0.8, follow-up)

`docs/bench_*.json` remains in place for now: the historical reports and
`docs/bench/phase*.json` files are referenced by existing docs/tests. The new
measurement output lives in `docs/bench/` (Phase 0 onward). A future move of
the historical files must update every link; it is tracked as a low-priority
cleanup, not a code change.
