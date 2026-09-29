#!/usr/bin/env python3
"""Todo 32 gate Fase-4: command matrix + latency + error paths, no-restart.

Spawns engine.exe (overlapped pipe server on \\\\.\\pipe\\k6wp-engine),
records spawn PID, then runs the full matrix on ONE connection:

  matrix (3 rounds x 5 commands = 15):
    set_video (real corpus path, alternates slideshow/anime),
    pause, resume, set_monitor 0, get_state
    -> each must {"ok":true} with ack latency <= 100ms.

  PID constancy: get_state `pid` sampled at start/middle/end must equal
    the spawn PID (any change = engine restarted -> FAIL stop-gate).

  error paths (3):
    bad-path set_video  -> {"error":...} ack (Todo 30 validation).
    frameless get_state -> NO reply, 2s client timeout, no crash.
    malformed garbage   -> {"error":...} ack (Decode-false policy).

Writes docs/bench_fase4.json:
  {matrix:[{cmd,ack,latency_ms,pass}...], error_paths:[...],
   pid_constant:bool, verdict}

Usage: python tests/gate_fase4_32.py [--engine PATH]
Exit 0 on PASS (verdict "PASS"), 1 on FAIL.
"""

import argparse
import ctypes
import json
import subprocess
import sys
import threading
import time
from pathlib import Path

PIPE_NAME = r"\\.\pipe\k6wp-engine"
GENERIC_READ = 0x80000000
GENERIC_WRITE = 0x40000000
OPEN_EXISTING = 3
INVALID_HANDLE_VALUE = ctypes.c_void_p(-1).value
BUF_SIZE = 128 * 1024
LAT_BUDGET_MS = 100.0

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


def read_with_timeout(h, timeout_s):
    out = {}

    def _read():
        try:
            buf = ctypes.create_string_buffer(BUF_SIZE)
            nread = ctypes.c_uint32(0)
            ok = k32.ReadFile(h, buf, BUF_SIZE, ctypes.byref(nread), None)
            out["ok"] = bool(ok)
            out["text"] = buf.raw[:nread.value].decode("utf-8", "replace")
        except Exception as e:  # noqa: BLE001 - report via out dict
            out["ok"] = False
            out["text"] = f"<exc: {e}>"

    t = threading.Thread(target=_read, daemon=True)
    t.start()
    t.join(timeout_s)
    return (not t.is_alive() and out.get("ok"), out.get("text", ""), t)


def ffprobe_log(root):
    """Best-effort ffprobe one-liner per corpus file (evidence log)."""
    info = []
    ffprobe = root / "vendor" / "ffmpeg" / "ffprobe.exe"
    for name in ("slideshow.mp4", "anime.mp4"):
        p = root / "tests" / "corpus" / name
        entry = {"file": name, "exists": p.exists(),
                 "size_bytes": p.stat().st_size if p.exists() else 0,
                 "ffprobe": None}
        if ffprobe.exists() and p.exists():
            try:
                r = subprocess.run(
                    [str(ffprobe), "-v", "error", "-show_entries",
                     "stream=codec_name,width,height",
                     "-of", "default=noprint_wrappers=1", str(p)],
                    capture_output=True, text=True, timeout=30)
                entry["ffprobe"] = (r.stdout.strip() or r.stderr.strip())[:300]
            except Exception as e:  # noqa: BLE001 - evidence only
                entry["ffprobe"] = f"<probe failed: {e}>"
        info.append(entry)
    return info


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
    vids = [str(vid_a), str(vid_b)]

    matrix = []       # [{cmd, ack, latency_ms, pass}]
    error_paths = []  # [{name, expectation, got, pass}]
    pid_samples = []
    fail_reason = ""

    def cmd_for(name, round_i):
        if name == "set_video":
            return json.dumps({"version": 1, "cmd": "set_video",
                               "payload": {"path": vids[round_i % 2]}},
                              separators=(",", ":")) + "\n"
        if name == "pause":
            return '{"version":1,"cmd":"pause","payload":{}}\n'
        if name == "resume":
            return '{"version":1,"cmd":"resume","payload":{}}\n'
        if name == "set_monitor":
            return '{"version":1,"cmd":"set_monitor","payload":{"monitor":0}}\n'
        return '{"version":1,"cmd":"get_state","payload":{}}\n'

    ok = True
    log_path = root / "build" / "gate_fase4_32_engine.log"
    log_path.parent.mkdir(parents=True, exist_ok=True)
    print(f"engine: {engine}", flush=True)
    with open(log_path, "w", encoding="utf-8") as logf:
        proc = subprocess.Popen([str(engine), "--exit-after-ms=120000"],
                                stdout=logf, stderr=subprocess.STDOUT)
        spawn_pid = proc.pid
        print(f"engine spawn pid: {spawn_pid}", flush=True)
        try:
            h = connect_pipe()
            try:
                # --- PID sample: start ---
                ack_text, _ = transact(h, cmd_for("get_state", 0))
                st = json.loads(ack_text).get("state", {})
                pid_samples.append(("start", st.get("pid")))
                print(f"pid start: {st.get('pid')}", flush=True)

                # --- Matrix: 3 rounds x 5 commands ---
                order = ["set_video", "pause", "resume",
                         "set_monitor", "get_state"]
                set_idx = 0
                for rnd in range(3):
                    for name in order:
                        line = cmd_for(name, rnd)
                        try:
                            ack_text, dt = transact(h, line)
                        except RuntimeError as e:
                            ok = False
                            fail_reason = f"matrix r{rnd} {name}: io error: {e}"
                            matrix.append({"cmd": name, "ack": None,
                                           "latency_ms": None, "pass": False})
                            print(f"FAIL: {fail_reason}", flush=True)
                            break
                        try:
                            ack = json.loads(ack_text)
                        except json.JSONDecodeError:
                            ok = False
                            fail_reason = (f"matrix r{rnd} {name}: "
                                           f"ack not JSON: {ack_text!r}")
                            matrix.append({"cmd": name, "ack": ack_text,
                                           "latency_ms": round(dt, 2),
                                           "pass": False})
                            print(f"FAIL: {fail_reason}", flush=True)
                            break
                        good = ack.get("ok") is True and dt <= LAT_BUDGET_MS
                        if name == "get_state":
                            good = good and "state" in ack
                            pid_samples.append((f"r{rnd}", ack["state"].get("pid")))
                        if name == "set_video":
                            set_idx += 1
                        matrix.append({"cmd": name, "ack": ack,
                                       "latency_ms": round(dt, 2),
                                       "pass": bool(good)})
                        print(f"matrix r{rnd} {name}: ok={ack.get('ok')} "
                              f"{dt:.2f}ms pass={good}", flush=True)
                        if not good:
                            ok = False
                            fail_reason = (f"matrix r{rnd} {name}: "
                                           f"ack={ack_text!r} dt={dt:.2f}ms")
                            print(f"FAIL: {fail_reason}", flush=True)
                            break
                    if not ok:
                        break

                # --- PID sample: middle ---
                if ok:
                    ack_text, _ = transact(h, cmd_for("get_state", 0))
                    pid_samples.append(("middle",
                                        json.loads(ack_text)["state"].get("pid")))
                    print(f"pid middle: {pid_samples[-1][1]}", flush=True)

                # --- Error path 1: bad path set_video -> error ack ---
                if ok:
                    bad = "C:\\Videos\\does-not-exist-k6wp32.mp4"
                    line = (json.dumps({"version": 1, "cmd": "set_video",
                                        "payload": {"path": bad}},
                                       separators=(",", ":")) + "\n")
                    ack_text, dt = transact(h, line)
                    ack = json.loads(ack_text)
                    passed = (ack.get("ok") is not True
                              and "error" in ack and dt <= LAT_BUDGET_MS)
                    error_paths.append({"name": "bad_path_set_video",
                                        "expectation": "error ack",
                                        "got": ack,
                                        "latency_ms": round(dt, 2),
                                        "pass": bool(passed)})
                    print(f"error-path bad_path: {ack_text!r} "
                          f"({dt:.2f}ms) pass={passed}", flush=True)
                    if not passed:
                        ok = False
                        fail_reason = f"bad path did not error-ack: {ack_text!r}"

                # --- Error path 2: frameless -> timeout, no reply ---
                reader = None
                if ok:
                    raw = b'{"version":1,"cmd":"get_state","payload":{}}'  # no \n
                    written = ctypes.c_uint32(0)
                    if not k32.WriteFile(h, raw, len(raw),
                                         ctypes.byref(written), None):
                        ok = False
                        fail_reason = "frameless write failed"
                        error_paths.append({"name": "frameless",
                                            "expectation": "timeout (no reply)",
                                            "got": fail_reason,
                                            "pass": False})
                    else:
                        got, text, reader = read_with_timeout(h, 2.0)
                        passed = not got
                        error_paths.append({"name": "frameless",
                                            "expectation": "timeout (no reply)",
                                            "got": ("reply: " + text[:120] if got
                                                    else "timeout, no reply"),
                                            "pass": bool(passed)})
                        print(f"error-path frameless: "
                              f"{'FAIL got reply' if got else 'timeout, no reply'} "
                              f"pass={passed}", flush=True)
                        if not passed:
                            ok = False
                            fail_reason = f"frameless got reply: {text!r}"
                    # Settle the pending read before reuse (Todo 28 pattern).
                    try:
                        k32.CancelIoEx(h, None)
                    except Exception:  # noqa: BLE001 - best effort
                        pass
                    if reader is not None:
                        reader.join(5.0)

                # --- Error path 3: malformed -> error ack ---
                if ok:
                    ack_text, dt = transact(h, "{not json at all}\n")
                    try:
                        ack = json.loads(ack_text)
                        passed = ("error" in ack and dt <= LAT_BUDGET_MS)
                    except json.JSONDecodeError:
                        ack, passed = ack_text, False
                    error_paths.append({"name": "malformed",
                                        "expectation": "error ack",
                                        "got": ack,
                                        "latency_ms": round(dt, 2),
                                        "pass": bool(passed)})
                    print(f"error-path malformed: {ack_text!r} "
                          f"({dt:.2f}ms) pass={passed}", flush=True)
                    if not passed:
                        ok = False
                        fail_reason = f"malformed did not error-ack: {ack_text!r}"

                # --- PID sample: end ---
                if ok:
                    ack_text, _ = transact(h, cmd_for("get_state", 0))
                    pid_samples.append(("end",
                                        json.loads(ack_text)["state"].get("pid")))
                    print(f"pid end: {pid_samples[-1][1]}", flush=True)
            finally:
                k32.CloseHandle(h)
        finally:
            proc.terminate()
            try:
                proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                proc.kill()

    pid_constant = (len(pid_samples) > 0
                    and all(p == spawn_pid for _, p in pid_samples))
    if not pid_constant:
        ok = False
        fail_reason = (fail_reason or
                       f"engine PID changed: spawn={spawn_pid} "
                       f"samples={pid_samples}")
        print(f"FAIL: {fail_reason}", flush=True)
    else:
        print(f"pids: spawn={spawn_pid} samples={pid_samples} "
              f"-> constant (no restart)", flush=True)

    verdict = "PASS" if ok else "FAIL"
    doc = {
        "todo": 32,
        "gate": "fase-4",
        "engine_pid": spawn_pid,
        "pid_samples": [{"phase": ph, "pid": p} for ph, p in pid_samples],
        "pid_constant": bool(pid_constant),
        "latency_budget_ms": LAT_BUDGET_MS,
        "matrix": matrix,
        "error_paths": error_paths,
        "ffprobe": ffprobe_log(root),
        "verdict": verdict,
        "fail_reason": fail_reason,
    }
    out_path = root / "docs" / "bench_fase4.json"
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(doc, f, indent=2)
    print(f"wrote {out_path}", flush=True)
    print(f"RESULT: {verdict}", flush=True)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
