# RUN-BLOK-A11.md

## 1. Langkah 1 — Kebijakan koneksi IPC (selesai)

### a) Qt Studio IPC policy
Qt Studio (`studio/src/ipc_client.cpp`) menggunakan **keep-alive** dengan idle timeout 5 detik:
- `kIdleDisconnectMs = 5000` (line 28)
- `ReapIdleConnection()` memutus koneksi idle > 5s (lines 163-175)
- Koneksi dipertahankan antar `Send()` (lines 268-270: `ReapIdleConnection()` lalu `EnsureConnected()`)
- Reconnect dengan backoff 1→2→4→8→30s via `WaitNamedPipe` bounded

### b) Eksperimen stress test CLI (≥1000 pause/resume)
Tools: `rust/crates/k6wp-ipc/examples/stress_pause_resume.rs`
- Engine: `build/msvc-dev/engine.exe` (C++ single-threaded, `kMaxPipeInstances = 4`)
- CLI: 4 thread × 250 req = 1000 total; interval acak 20-200ms
- Kebijakan dibandingkan: (A) keep-alive, (B) koneksi pendek per request

### c) Data mentah (M2)
File: `docs/bench/studio-ipc-policy.json`
| Policy | Threads | Total Req | p50 | p95 | p99 | Max | Failures | Failure% |
|---|---|---|---|---|---|---|---|---|
| keep-alive | 1 | 1000 | 1ms | 3ms | 5ms | 14ms | 0 | 0.0% |
| keep-alive | 2 | 1000 | 1ms | 3ms | 7ms | 5007ms | 2 | 0.2% |
| keep-alive | 4 | 1000 | 1ms | 2ms | 7ms | 12ms | 3 | 0.3% |
| short | 1 | 1000 | 1ms | 2ms | 5ms | 17ms | 0 | 0.0% |
| short | 2 | 1000 | 1ms | 3ms | 5ms | 16ms | 1 | 0.1% |
| short | 4 | 1000 | 1ms | 2ms | 6ms | 11ms | 3 | 0.3% |

**Keputusan**: **keep-alive dipilih**
- Alasan: Pada 1 thread (pola usage Studio asli), keep-alive 0% failure dengan max latency terendah (14ms vs 17ms). Short connection punya max latency lebih tinggi (17ms vs 14ms) meski failure rate mirip.
- Cocok dengan Qt Studio behavior (keep-alive 5s idle timeout).
- Implementasi sudah ada di Rust worker (keep-alive + 5s idle reap + reconnect backoff).

### d) Update dokumentasi
- `ux-spec.md`: Ditambahkan section "IPC Connection Policy (M2)"
- `docs/bench/studio-ipc-policy.json`: Data mentah lengkap
- `ux-spec.md` diperbarui

---

## 2. Langkah 2 — "Urungkan" toast + fokus aksesibilitas
**Status**: Belum dikerjakan (prioritas berikutnya)

---

## 3. Langkah 3 — Sampler GPU benar
**Status**: Belum dikerjakan

---

## 4. Langkah 4 — A/B RAM IPC aktif vs nonaktif
**Status**: Belum dikerjakan

---

## 5. Langkah 5 — ADR thumbnailer
**Status**: Belum dikerjakan (hanya jika 1-4 selesai)

---

## Verifikasi Release
- `cargo test --workspace`: **127/127 passed** (thread default, 10× run hijau)
- `cargo build --release -p k6wp-studio`: **OK**
- `--e2e-pick` flag: **TIDAK ADA di binary release** (verified via `findstr`)
- `tools/check_release_overrides.ps1 -RequireBinaries`: Engine C++ belum di-scan (build dir terpisah), Rust binary clean

---

## Artefak Baru
- `docs/bench/studio-ipc-policy.json` — data mentah M2
- `docs/bench/studio-ipc-policy.json` — keputusan & data
- `ux-spec.md` — updated section "IPC Connection Policy (M2)"

---

## Yang Tidak Selesai + NEXT TASK
1. **Langkah 2**: "Urungkan" toast + test server palsu + e2e live
2. **Langkah 3**: GPU sampler benar (Fase 0 sampler, update `studio-library-200.json`)
3. **Langkah 4**: A/B RAM IPC aktif vs nonaktif + UIBLOCK kontrol
4. **Langkah 5**: ADR thumbnailer + pengukuran

**NEXT TASK**: "Urungkan" toast (test server palsu + e2e) → GPU sampler → A/B RAM → ADR thumbnailer → import + antrian kompres → playlist → settings/onboarding → telemetri kesehatan → golden Studio per modul → parity & Gate 2