# Archive — spikes and QA scripts moved to attic/

This directory contains archived spike probes and QA scripts that are no longer
part of the active codebase but are preserved for historical/reference purposes.
All files were renames (git mv) from their original locations, preserving full
git history.

## Origin table

| Attic path                    | Original path (at HEAD 6cc1f4f) | Category      |
|------------------------------|-----------------------------------|---------------|
| attic/spikes/mpv_hwdec.cpp    | spikes/mpv_hwdec.cpp              | Hardware decode probe |
| attic/spikes/qt_embed.cpp     | spikes/qt_embed.cpp               | Qt embed mpv probe |
| attic/spikes/workerw_probe.cpp| spikes/workerw_probe.cpp          | WorkerW injection probe |
| attic/spikes/enc_probe.ps1    | spikes/enc_probe.ps1              | Encoder detection probe |
| attic/tools/bench_compress.ps1| tools/bench_compress.ps1          | Compression benchmark |
| attic/tools/bench_cpu_mem.ps1 | tools/bench_cpu_mem.ps1           | CPU/memory benchmark (archived copy; Step 9 wrote a new generic sampler at tools/bench_cpu_mem.ps1) |
| attic/tests/app_live_apply.py | tests/app_live_apply.py           | Live apply QA |
| attic/tests/gate_fase4_32.py  | tests/gate_fase4_32.py            | Fase 4 QA |
| attic/packaging-icons/app.ico | packaging/app.ico               | Legacy placeholder icon (LOW-22; superseded by k6wp-on.ico) |
| attic/packaging-icons/app_paused.ico | packaging/app_paused.ico   | Legacy placeholder icon (LOW-22; superseded by k6wp-off.ico) |
| attic/spikes/render_shared.cpp | spikes/render_shared.cpp           | GL-WGL shared-surface render probe (P3L spike A; never in CMake) |
| attic/spikes/render_shared_d3d11.cpp | spikes/render_shared_d3d11.cpp | D3D11 probe/verification (P3.6; never in CMake) |

Step 10 promotions (moved back out of the attic, history preserved):
`attic/tests/send_test.py` + `attic/tests/pipe_fuzz.py` → `tests/`
(headless pipe QA beside `config_test`/`ipc_test`);
`attic/tools/bench_startup.ps1` → `tools/` (operator-facing bench);
`live_switch_30.py` also lives at `tests/` (never archived).

## Archive reason

These files are **spike/debug/QA-superseded** artifacts:

- **spikes/** — Throwaway probe programs written during feature exploration
  (Todos 3–4). They are not integrated into the build system (no CMakeLists.txt
  references, no preset references). The actual functionality is housed in the
  engine (`mpv_hwdec` → `engine/src/mpv_renderer`, `qt_embed` → Qt widget
  embedding in Todo 23, `workerw_probe` → desktop injection in Todo 9) and
  compressor (`enc_probe` → `compressor/encoder_detect` in Todo 15).

- **tools/bench_*.ps1** — Benchmark scripts that operate on build output or
  require a running engine instance. Archived 2026-09-16; the archived
  copies are reference-only. Operator-facing benches live in `tools/`
  (`bench_cpu_mem.ps1`, `bench_startup.ps1`); canonical benchmark
  evidence lives in `docs/bench_*.json` + `docs/bench_final.json`.

- **tests/** — QA Python scripts that tested transient or phase-gated scenarios
  (fase 3–4). The active QA surface is covered by the Todo 6–12 test suite;
  these scripts were tied to completed phases and are archived so the test
  directory focuses on current acceptance criteria.

## Restore procedure

To restore any file to its original location (reversing the archive), use `git mv`
with the original path. Example restores:

```bash
# Restore send_test.py from attic to tests/
git mv attic/tests/send_test.py tests/send_test.py

# Restore workerw_probe.cpp from attic to spikes/
git mv attic/spikes/workerw_probe.cpp spikes/workerw_probe.cpp

# Restore bench_startup.ps1 from attic to tools/
git mv attic/tools/bench_startup.ps1 tools/bench_startup.ps1
```

To restore everything at once (from the repo root):

```bash
git mv attic/spikes/ spikes/
git mv attic/tools/ tools/
git mv attic/tests/ tests/
```

After restoring, commit the changes or run `git reset --soft HEAD~1` to undo the
restore if needed.

## Task-25 archive state (2026-09-18)

- Every probe program lives here in `attic/spikes/`; the root `spikes/`
  placeholder directory was removed.
- Main-build exclusion VERIFIED 2026-09-18: root `CMakeLists.txt` lists only
  `shared/engine/compressor/studio/launcher`; repo-wide grep for
  `add_subdirectory` referencing `spikes`/`attic` returns zero matches
  (reported to T24, not edited here).
- `attic/` was removed from `.gitignore` (it sat miscategorized under "# OS")
  so archive additions stay committable; already-tracked files were never
  affected. Build/log/cache/probe outputs stay ignored
  (`build/`, `*.log`, `*verbose-probe*/`, `*cfg-probe-*/`, `*.obj`...).
- `engine/src/mpv_renderer.cpp` references the probe at its archived path
  (`attic/spikes/mpv_hwdec.cpp`); the stale root-`spikes/` pointer was
  corrected.

## Provenances (v1 verification)

The following scripts passed their respective v1 QA gates with 100% success
rates, recorded so future readers know the scripts were proven functional:

- `send_test.py` — 100/100 acks — Todo 6 live-apply QA
- `pipe_fuzz.py` — 1000/1000 — fuzz pipe stress test
- `live_switch_30.py` — 5/5 — live wallpaper switch QA