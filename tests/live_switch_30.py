#!/usr/bin/env python3
"""Todo 30 live-switch check: Apply -> IPC set_video, PID stability, bad-path.

Starts engine.exe, notes its PID, then:
  phase1: 5x set_video (alternating two real files) -> {"ok":true} each,
          records send->ack ms (budget: each <<500ms; the 500ms budget
          covers the render swap, the ack is the measurable proxy + the
          PID-unchanged proof that no restart happened).
  phase2: get_state -> same PID as spawn + video == last set path.
  phase3 (QA-fail): set_video bad path -> {"error":...} ack, then get_state
          -> still running, same PID, video still the last-valid path.
  phase4 (studio-crash survival): brutal client close + reconnect -> ack,
          same PID (engine never depends on the client process).

Usage: python tests/live_switch_30.py [--engine PATH]
Exit 0 on PASS, 1 on FAIL.
"""

import argparse
import ctypes
import json
import subprocess
import sys
import time
from pathlib import Path

PIPE_NAME = r"\\.\pipe\k6wp-engine"
GENERIC_READ = 0x80000000
GENERIC_WRITE = 0x40000000
OPEN_EXISTING = 3
INVALID_HANDLE_VALUE = ctypes.c_void_p(-1).value
BUF_SIZE = 128 * 1024

k32 = ctypes.WinDLL("kernel32", use_last_error=True)

k32.WaitNamedPipeW.argtypes = [ctypes.c_wchar_p, ctypes.c_uint32]
k32.WaitNamedPipeW.restype = ctypes.c_bool
k32.CreateFileW.argtypes = [
    ctypes.c_wchar_p, ctypes.c_uint32, ctypes.c_uint32,
    ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_void_p,
]
k32.CreateFileW.restype = ctypes.c_void_p
k32.WriteFile.argtypes = [
    ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint32,
    ctypes.POINTER(ctypes.c_uint32), ctypes.c_void_p,
]
k32.WriteFile.restype = ctypes.c_bool
k32.ReadFile.argtypes = [
    ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint32,
    ctypes.POINTER(ctypes.c_uint32), ctypes.c_void_p,
]
k32.ReadFile.restype = ctypes.c_bool
k32.CloseHandle.argtypes = [ctypes.c_void_p]
k32.CloseHandle.restype = ctypes.c_bool
k32.CancelIoEx.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
k32.CancelIoEx.restype = ctypes.c_bool


def fail(msg):
    print(f"FAIL: {msg}", flush=True)
    return False


def connect_pipe(timeout_ms=15000):
    deadline = time.perf_counter() + timeout_ms / 1000.0
    last_err = 0
    while True:
        if k32.WaitNamedPipeW(PIPE_NAME, 500):
            break
        last_err = ctypes.get_last_error()
        if time.perf_counter() >= deadline:
            raise RuntimeError(
                f"pipe not available after {timeout_ms}ms (err {last_err})")
        time.sleep(0.1)
    for _ in range(50):
        h = k32.CreateFileW(PIPE_NAME, GENERIC_READ | GENERIC_WRITE,
                            0, None, OPEN_EXISTING, 0, None)
        if h != INVALID_HANDLE_VALUE and h is not None:
            return h
        time.sleep(0.1)
    raise RuntimeError("CreateFileW failed")


def transact(h, line):
    data = line.encode("utf-8")
    written = ctypes.c_uint32(0)
    t0 = time.perf_counter()
    if not k32.WriteFile(h, data, len(data), ctypes.byref(written), None):
        raise RuntimeError(f"WriteFile failed: {ctypes.get_last_error()}")
    buf = ctypes.create_string_buffer(BUF_SIZE)
    nread = ctypes.c_uint32(0)
    if not k32.ReadFile(h, buf, BUF_SIZE, ctypes.byref(nread), None):
        raise RuntimeError(f"ReadFile failed: {ctypes.get_last_error()}")
    dt_ms = (time.perf_counter() - t0) * 1000.0
    return buf.raw[:nread.value].decode("utf-8"), dt_ms


def cmd_set_video(path):
    return json.dumps({"version": 1, "cmd": "set_video",
                       "payload": {"path": path}},
                      separators=(",", ":")) + "\n"


def cmd_get_state():
    return '{"version":1,"cmd":"get_state","payload":{}}\n'


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--engine", default="",
                    help="path to engine.exe (default: build/msvc-dev/engine.exe)")
    args = ap.parse_args()

    root = Path(__file__).resolve().parent.parent
    engine = Path(args.engine) if args.engine else root / "build" / "msvc-dev" / "engine.exe"
    if not engine.exists():
        print(f"FAIL: engine.exe not found: {engine}", flush=True)
        return 1
    corpus = root / "tests" / "corpus"
    vid_a = corpus / "slideshow.mp4"
    vid_b = corpus / "anime.mp4"
    if not vid_a.exists() or not vid_b.exists():
        print(f"FAIL: corpus videos missing in {corpus}", flush=True)
        return 1

    ok = True
    log_path = root / "build" / "live_switch_30_engine.log"
    log_path.parent.mkdir(parents=True, exist_ok=True)
    print(f"engine: {engine}", flush=True)
    with open(log_path, "w", encoding="utf-8") as logf:
        proc = subprocess.Popen([str(engine), "--exit-after-ms=60000"],
                                stdout=logf, stderr=subprocess.STDOUT)
        spawn_pid = proc.pid
        print(f"engine spawn pid: {spawn_pid}", flush=True)
        try:
            h = connect_pipe()
            try:
                # --- Phase 1: 5x live-switch, time each send->ack ---
                lat = []
                vids = [str(vid_a), str(vid_b)] * 3
                for i in range(5):
                    ack_text, dt = transact(h, cmd_set_video(vids[i]))
                    lat.append(dt)
                    ack = json.loads(ack_text)
                    if not ack.get("ok"):
                        ok = fail(f"apply {i}: ack not ok: {ack_text!r}")
                        break
                    print(f"apply {i}: set_video ok in {dt:.2f}ms", flush=True)
                else:
                    over = sum(1 for v in lat if v > 500.0)
                    print(f"phase1: 5/5 live-switch acks, "
                          f"min={min(lat):.2f} max={max(lat):.2f}ms "
                          f"over_500ms={over}", flush=True)
                    if over:
                        ok = fail(f"{over} live-switches exceeded 500ms")
                    last_valid = vids[4]

                # --- Phase 2: PID unchanged + state video == last path ---
                if ok:
                    ack_text, _ = transact(h, cmd_get_state())
                    state = json.loads(ack_text)["state"]
                    print(f"phase2: state pid={state['pid']} "
                          f"video={state.get('video')!r}", flush=True)
                    if state["pid"] != spawn_pid:
                        ok = fail(f"engine PID changed "
                                  f"({spawn_pid} -> {state['pid']}) = restart!")
                    elif state.get("video") != last_valid:
                        ok = fail(f"state video mismatch: "
                                  f"{state.get('video')!r} != {last_valid!r}")
                    else:
                        print("phase2: PID stable, video tracks last apply",
                              flush=True)
            finally:
                k32.CloseHandle(h)

            # --- Phase 3 (QA-fail): bad path -> error ack, old video stays ---
            if ok:
                h = connect_pipe()
                try:
                    bad = "C:\\Videos\\does-not-exist-k6wp30.mp4"
                    ack_text, dt = transact(h, cmd_set_video(bad))
                    ack = json.loads(ack_text)
                    if ack.get("ok") or "error" not in ack:
                        ok = fail(f"bad path should error-ack: {ack_text!r}")
                    else:
                        print(f"phase3: bad path -> error ack "
                              f"({dt:.2f}ms, {ack['error']!r})", flush=True)
                    ack_text, _ = transact(h, cmd_get_state())
                    state = json.loads(ack_text)["state"]
                    if state["pid"] != spawn_pid:
                        ok = fail("engine PID changed after bad path!")
                    elif state.get("video") != last_valid:
                        ok = fail(f"bad path clobbered video: "
                                  f"{state.get('video')!r}")
                    else:
                        print("phase3: old wallpaper kept, PID stable",
                              flush=True)
                finally:
                    # Brutal close: CancelIoEx first (pending-IO trap, Todo 28).
                    k32.CancelIoEx(h, None)
                    k32.CloseHandle(h)

            # --- Phase 4 (studio-crash survival): reconnect -> ack, same PID ---
            if ok:
                time.sleep(0.5)
                h = connect_pipe()
                try:
                    ack_text, dt = transact(h, cmd_get_state())
                    state = json.loads(ack_text)["state"]
                    assert state["pid"] == spawn_pid, state
                    print(f"phase4: reconnect after brutal close -> ack "
                          f"({dt:.2f}ms), PID {state['pid']} stable "
                          f"(engine survives client death)", flush=True)
                except (RuntimeError, AssertionError) as e:
                    ok = fail(f"survival reconnect: {e}")
                finally:
                    k32.CloseHandle(h)
        finally:
            proc.terminate()
            try:
                proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                proc.kill()

    print("RESULT: PASS" if ok else "RESULT: FAIL", flush=True)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
