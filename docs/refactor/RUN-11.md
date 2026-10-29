# Run 11: A11 (E1 Final) + A10 (G6 Test-Proven) — PARTIAL

## Ringkasan
- **A10 (G6 test-proven)**: UNPROVEN — `build/release/engine.exe` tidak ada (mpv deps missing, C++ build gagal)
- **A11 (E1 final)**: DONE — measurement 5 run dengan 200-item library, hasil dicatat di `docs/bench/studio-e1-final.json`

## A10: G6 Image Check — UNPROVEN

### Alasan
C++ release build gagal karena dependency `mpv/client.h` tidak ditemukan:
```
mpv_renderer.cpp(15,10): error C1083: Cannot open include file: 'mpv/client.h'
```

### Rencana
**Run 11b**: Install mpv development dependencies → rebuild `build/release/engine.exe` → jalankan `cargo test -p k6wp-ipc --test live_engine -- K6WP_IPC_LIVE=1`.

### Status di parity-checklist
A10 = `UNPROVEN — engine.exe not built (mpv deps missing)`

## A11: E1 Final Measurement — DONE

### Setup
- Binary: `rust/target/release/k6wp-studio.exe` (9.3 MB, SHA256: a91dcd6cc9879726140ab78e44184a77c83fa72d72a1300f8e11167c495523c6)
- Library: 200 items (15 unavailable) di `C:\Users\axioo\AppData\Local\K6WP\library.json`
- Thumbs: 200 generated
- Runs: 5 (median digunakan)
- Method: `measure_studio_library.ps1` - poll PrintWindow setiap 25ms untuk first frame, scroll 400 notch 8ms apart

### Hasil (median dari 5 run)

| Metric | Target | Hasil | Status |
|--------|--------|-------|--------|
| `first_frame_ms` (median) | ≤ 700 ms | **735 ms** (min 704, max 945) | ✗ +35 ms |
| `private_mb_after_scroll` (median) | ≤ 150 MB | **90.5 MB** | ✓ |
| `private_mb_first` (median) | - | 40.9 MB | - |
| `gpu_shared_mb_after_scroll` (median) | - | 57 MB | - |
| `gpu_dedicated_mb_after_scroll` (median) | - | 0 MB | - |
| `uiblock_count_scroll` (median) | - | 2 | - |
| `idle_cpu_pct` | - | 2.49% | - |

### Analisis
- **Private memory**: 90.5 MB (median after scroll) — **di dalam target ≤150 MB** ✓
- **First frame**: 735 ms median — **di luar target ≤700 ms** (+35 ms) ✗
  - Run 1 outlier: 945 ms (cold start)
  - Run 2-5: 704-786 ms (warm)
- **GPU**: Shared memory 57 MB after scroll, dedicated 0 MB (software rendering / iGPU)
- **UIBlock**: Median 2 events per scroll, 2 per idle second — watchdog noise, bukan blocking nyata

### Files
- Output: `docs/bench/studio-e1-final.json`
- Method detail di JSON: `method.first_frame`, `method.private_bytes`, `method.scroll`, dll.

### Kesimpulan A11
- **Private memory: PASS** (90.5 MB < 150 MB)
- **First frame: FAIL** (735 ms > 700 ms, +35 ms / +5%)
- **Owner decision required**: Terima +35ms overshoot ATAU optimasi (Run 11b)

### Status di parity-checklist
A11 = `done — Run 11 (median first_frame=735ms, private=90.5MB)`

## Files Changed
- `docs/bench/studio-e1-final.json` — measurement output
- `docs/refactor/parity-checklist.md` — A10 UNPROVEN, A11 done
- `docs/refactor/STATE.md` — NEXT TASK updated

## Next Steps
- **Run 11b** (optional): Install mpv deps → build engine.exe → A10 live test
- **Run 11b** (optional): Optimasi first frame jika owner approve
- **Run 12**: Gate A verify (A9) + B3/B4/B5
- **Run 13**: F3 impl (B7) + J5 (B8) + Gate B final

## Owner Decision Required
Apakah +35ms first frame overshoot diterima? Atau perlu Run 11b optimasi?