# Run 11b: A10 Engine Build + A11 Stability Investigation — BLOCKED

## Status: BLOCKED — libmpv vendor download failing (404)

## A10: Engine Build — BLOCKED

### Issue
`tools/fetch_vendor.ps1` fails to download libmpv 0.41.0-dev:
- URL: `https://github.com/shinchiro/mpv-winbuild-cmake/releases/download/20260903/mpv-dev-x86_64-20260903-git-69e63f425a.7z`
- Error: `FAIL The remote server returned an error: (404) Not Found.`
- The specific release (20260903) doesn't exist on GitHub.

### Current Vendor State
```
vendor/libmpv/
├── lib/
│   └── mpv.lib (13,578 bytes)
└── include/  (empty - headers missing)
```

Missing: `libmpv-2.dll`, `include/mpv/*.h`

### Attempted Fixes
1. Re-run `fetch_vendor.ps1` → 404 Not Found (release URL invalid)
2. libmpv release 20260903 doesn't exist on GitHub

### Workaround Needed
- Find alternative libmpv 0.41.0-dev x64 build with headers
- Or update fetch_vendor.ps1 to use a valid release URL
- Or manually download libmpv from alternative source

### Current Blocker
**Cannot build `build/release/engine.exe` without `vendor\libmpv\bin\libmpv-2.dll` and headers.**
A10 live test cannot proceed until engine builds.

---

## A11: Bimodal Variance Investigation — PENDING

### Per-Run Analysis (5 runs, 200-item library)

| Run | first_frame_ms | private_first (MB) | private_after (MB) | Δ (after-first) | gpu_shared_after (MB) | stalls |
|-----|---------------|-------------------|-------------------|-----------------|----------------------|--------|
| 1   | 639           | 40.9              | 40.8              | -0.1            | 43.2                 | 1      |
| 2   | 620           | 40.7              | 94.0              | +53.3           | 68.7                 | 2      |
| 3   | 726           | 73.1              | 97.1              | +24.0           | 57.1                 | 1      |
| 4   | 713           | 74.1              | 98.1              | +24.0           | 57.2                 | 2      |
| 5   | 703           | 40.6              | 40.5              | -0.1            | 43.2                 | 1      |

### Analysis
- **BIMODAL distribution**: 3 runs "low" (first ~40MB), 2 runs "high" (first ~73MB)
- **Not monotonic leak**: Run 1 & 5 show Δ ≈ 0, Run 2 shows +53MB, Run 3/4 show +24MB
- **Interpretation**: 3/5 runs = thumbnails NOT loading (Δ ≈ 0), 3/5 runs = thumbnails loading (Δ > 24MB)
- **Thumbnail load rate**: 60% (3/5 runs load, 2/5 don't load at all)
- **Run 2 anomaly**: starts low (40.7) but loads all (+53MB) — cache miss then full load

### Root Cause Hypotheses
1. **`generate_thumbnail` race/fail**: ffmpeg spawn fails silently, returns early
2. **`request_visible()` miss**: UI doesn't request thumbnails for all visible cards
3. **`Thumbnail` event timing**: events arrive after measurement window
4. **ffmpeg path resolution**: `find_ffmpeg()` returns None intermittently

### Required Investigation
1. Run Studio with `K6WP_THUMBS_DIR=<fresh>` + `--ui-watch`
2. Scroll 200 items, capture stderr + `--ui-watch` output
3. Count `Thumb` events vs 200 items (should be 200)
4. Check `generate_thumbnail` errors in stderr
5. Check `request_visible` triggers for all visible cards

---

## Blocker Summary

| Item | Status | Blocker |
|------|--------|---------|
| **A10** | **UNPROVEN** | libmpv vendor download 404 → engine.exe missing → live test skipped |
| **A11** | **PARTIAL** | Bimodal 40-98MB = 60% thumbnail load rate → bug, not noise |

### Next Steps
1. **Fix libmpv vendor** → build engine.exe → Run 11b A10
2. **Investigate bimodal** → fix thumbnail load bug → re-measure A11
3. **Only then** → Run 12 (Gate A verify)

---

## Files
- `docs/bench/studio-e1-final-detailed.json` — latest measurement
- `docs/refactor/RUN-11b.md` — this file
- `docs/refactor/parity-checklist.md` — A10 UNPROVEN, A11 PARTIAL