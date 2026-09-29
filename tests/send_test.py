#!/usr/bin/env python3
"""Todo 28 loop test: 100 IPC commands -> 100 acks, latency stats.

Spawns engine.exe (its overlapped pipe server listens on the per-session
\\\\.\\pipe\\k6wp-engine-<session_id>, see MED-12 / shared/ipc_protocol.hpp),
sends 100 sequential NDJSON commands over a
single pipe connection using ctypes + kernel32 only (no pywin32), asserts
100 acks each <=100ms, then verifies:
  - clean disconnect + reconnect -> ack (re-listen),
  - frameless message (no trailing \\n) -> 2s read timeout, no crash,
  - brutal close after the timeout + reconnect -> ack (server survived).

Usage:
  python tests/send_test.py [--engine PATH] [--count N]

Exit 0 on PASS, 1 on FAIL.
"""

import argparse
import ctypes
import json
import os
import statistics
import subprocess
import sys
import threading
import time
from pathlib import Path

PIPE_BASE = r"\\.\pipe\k6wp-engine"


def _current_session_pipe():
    """Per-session pipe name (MED-12): \\.\pipe\k6wp-engine-<session_id>.

    Mirrors k6wp::CurrentSessionPipeName() (shared/ipc_protocol.*): the
    session of THIS process via ProcessIdToSessionId(GetCurrentProcessId()).
    The engine serves the same name, so client and server rendezvous without
    a hardcoded literal. Falls back to session 0 when the lookup fails.
    """
    _proc_id_to_session = ctypes.WinDLL("kernel32", use_last_error=True) \
        .ProcessIdToSessionId
    _proc_id_to_session.argtypes = [ctypes.c_uint32,
                                    ctypes.POINTER(ctypes.c_uint32)]
    _proc_id_to_session.restype = ctypes.c_bool
    _get_pid = ctypes.WinDLL("kernel32", use_last_error=True).GetCurrentProcessId
    _get_pid.restype = ctypes.c_uint32
    session = ctypes.c_uint32(0)
    if _proc_id_to_session(_get_pid(), ctypes.byref(session)):
        return f"{PIPE_BASE}-{session.value}"
    return f"{PIPE_BASE}-0"


PIPE_NAME = _current_session_pipe()
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
    """Wait for the pipe and open a synchronous R/W handle.

    NOTE: WaitNamedPipeW only honors its timeout when at least one pipe
    instance exists but all are busy; with zero instances it fails
    immediately (ERROR_FILE_NOT_FOUND). Hence the poll loop.
    """
    deadline = time.perf_counter() + timeout_ms / 1000.0
    last_err = 0
    while True:
        if k32.WaitNamedPipeW(PIPE_NAME, 500):
            break
        last_err = ctypes.get_last_error()
        if time.perf_counter() >= deadline:
            raise RuntimeError(
                f"pipe not available after {timeout_ms}ms (err {last_err}): {PIPE_NAME}")
        time.sleep(0.1)
    last_err = 0
    for _ in range(50):
        h = k32.CreateFileW(PIPE_NAME, GENERIC_READ | GENERIC_WRITE,
                            0, None, OPEN_EXISTING, 0, None)
        if h != INVALID_HANDLE_VALUE and h is not None:
            return h
        last_err = ctypes.get_last_error()
        time.sleep(0.1)
    raise RuntimeError(f"CreateFileW failed (err {last_err}): {PIPE_NAME}")


def transact(h, line):
    """Write one NDJSON line, read one ack. Returns (ack_str, latency_ms)."""
    data = line.encode("utf-8")
    written = ctypes.c_uint32(0)
    t0 = time.perf_counter()
    if not k32.WriteFile(h, data, len(data), ctypes.byref(written), None):
        raise RuntimeError(f"WriteFile failed: {ctypes.get_last_error()}")
    if written.value != len(data):
        raise RuntimeError(f"short write {written.value}/{len(data)}")
    buf = ctypes.create_string_buffer(BUF_SIZE)
    nread = ctypes.c_uint32(0)
    if not k32.ReadFile(h, buf, BUF_SIZE, ctypes.byref(nread), None):
        raise RuntimeError(f"ReadFile failed: {ctypes.get_last_error()}")
    dt_ms = (time.perf_counter() - t0) * 1000.0
    return buf.raw[:nread.value].decode("utf-8"), dt_ms


def build_commands(count, vids):
    """Alternate across all five commands so every dispatch path is hit.

    set_video payloads use real corpus files (vids): since Todo 30 the
    engine validates path existence and error-acks fake paths.
    """
    cmds = []
    for i in range(count):
        slot = i % 5
        if slot == 0:
            cmds.append({"version": 1, "cmd": "set_video",
                         "payload": {"path": vids[(i // 5) % len(vids)]}})
        elif slot == 1:
            cmds.append({"version": 1, "cmd": "pause", "payload": {}})
        elif slot == 2:
            cmds.append({"version": 1, "cmd": "resume", "payload": {}})
        elif slot == 3:
            cmds.append({"version": 1, "cmd": "set_monitor",
                         "payload": {"monitor": 0}})
        else:
            cmds.append({"version": 1, "cmd": "get_state", "payload": {}})
    return cmds


def read_with_timeout(h, timeout_s):
    """Blocking ReadFile on a worker thread; returns (got_data, text)."""
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


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--engine", default="",
                    help="path to engine.exe (default: build/msvc-dev/engine.exe)")
    ap.add_argument("--count", type=int, default=100)
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

    ok = True
    log_path = root / "build" / "send_test_engine.log"
    log_path.parent.mkdir(parents=True, exist_ok=True)
    # The engine mirrors every line to %LOCALAPPDATA%/K6WP/engine.log
    # (K6WP_VERBOSE=0 -> stdout stays empty), so phase 4 reads THAT file.
    # Record its length before spawn and read only the appended portion so
    # markers from earlier runs can never satisfy the ordering check.
    engine_log = Path(os.environ.get("LOCALAPPDATA", "")) / "K6WP" / "engine.log"
    log_offset = engine_log.stat().st_size if engine_log.exists() else 0
    print(f"engine: {engine}", flush=True)
    with open(log_path, "w", encoding="utf-8") as logf:
        proc = subprocess.Popen([str(engine), "--exit-after-ms=90000"],
                                stdout=logf, stderr=subprocess.STDOUT)
        print(f"engine pid: {proc.pid}", flush=True)
        try:
            # --- Phase 1: N commands -> N acks on one connection ---
            h = connect_pipe()
            try:
                lat = []
                for i, cmd in enumerate(build_commands(args.count, vids)):
                    line = json.dumps(cmd, separators=(",", ":")) + "\n"
                    try:
                        ack_text, dt = transact(h, line)
                    except RuntimeError as e:
                        ok = fail(f"cmd {i} ({cmd['cmd']}): {e}") and False
                        break
                    try:
                        ack = json.loads(ack_text)
                    except json.JSONDecodeError:
                        ok = fail(f"cmd {i}: ack not JSON: {ack_text!r}")
                        break
                    if not ack.get("ok"):
                        ok = fail(f"cmd {i}: ack not ok: {ack_text!r}")
                        break
                    if cmd["cmd"] == "get_state" and "state" not in ack:
                        ok = fail(f"cmd {i}: get_state ack missing state")
                        break
                    lat.append(dt)
                else:
                    print(f"phase1: {len(lat)}/{args.count} acks", flush=True)

                if lat and len(lat) == args.count:
                    lat_sorted = sorted(lat)
                    lo, med, hi = lat_sorted[0], statistics.median(lat_sorted), lat_sorted[-1]
                    over = sum(1 for v in lat if v > 100.0)
                    print(f"latency_ms: min={lo:.2f} med={med:.2f} max={hi:.2f} "
                          f"count={len(lat)} over_100ms={over}", flush=True)
                    if over:
                        ok = fail(f"{over} acks exceeded 100ms (max {hi:.2f}ms)")
            finally:
                k32.CloseHandle(h)

            # --- Phase 2: clean disconnect + reconnect ---
            if ok:
                h = connect_pipe()
                try:
                    ack_text, dt = transact(
                        h, '{"version":1,"cmd":"get_state","payload":{}}\n')
                    ack = json.loads(ack_text)
                    assert ack.get("ok") and "state" in ack, ack_text
                    print(f"phase2: reconnect after clean close -> ack "
                          f"({dt:.2f}ms, state keys: {sorted(ack['state'])})",
                          flush=True)
                except (RuntimeError, AssertionError) as e:
                    ok = fail(f"reconnect after clean close: {e}")
                finally:
                    k32.CloseHandle(h)

            # --- Phase 3 (QA-fail): frameless message -> 2s timeout, no crash ---
            if ok:
                h = connect_pipe()
                reader = None
                try:
                    raw = b'{"version":1,"cmd":"get_state","payload":{}}'  # no \n
                    written = ctypes.c_uint32(0)
                    if not k32.WriteFile(h, raw, len(raw),
                                         ctypes.byref(written), None):
                        ok = fail(f"frameless write failed: {ctypes.get_last_error()}")
                    else:
                        got, text, reader = read_with_timeout(h, 2.0)
                        if got:
                            ok = fail(f"frameless message got a reply: {text!r}")
                        else:
                            print("phase3: frameless (no \\n) -> 2s timeout, "
                                  "no reply, no crash", flush=True)
                finally:
                    # Brutal close while the server is parked on this
                    # instance. CancelIoEx FIRST: the reader thread still
                    # has a ReadFile pending on h, and CloseHandle alone
                    # does not tear down a handle with in-flight I/O from
                    # another thread — the server would (correctly) keep
                    # waiting instead of seeing the disconnect.
                    k32.CancelIoEx(h, None)
                    if reader is not None:
                        reader.join(5.0)
                    k32.CloseHandle(h)
                # Give the server a moment to re-listen, then verify
                # survival with a fresh connect. (The reconnect ack IS the
                # brutal-disconnect survival proof: a dead server refuses
                # the connection, and connect_pipe raises after 15s.)
                time.sleep(0.5)
                try:
                    h = connect_pipe()
                    try:
                        ack_text, dt = transact(
                            h, '{"version":1,"cmd":"get_state","payload":{}}\n')
                        ack = json.loads(ack_text)
                        assert ack.get("ok"), ack_text
                        print(f"phase3: reconnect after brutal close -> ack "
                              f"({dt:.2f}ms)", flush=True)
                    except (RuntimeError, AssertionError) as e:
                        ok = fail(f"brutal-close reconnect transaction: {e}")
                    finally:
                        k32.CloseHandle(h)
                except RuntimeError as e:
                    ok = fail(f"reconnect after brutal close: {e}")

            # --- Phase 4 (HIGH-1 repro): quit over an active connection ---
            # The engine must exit 0 (graceful teardown) and the log must
            # show the IPC worker stopped BEFORE the headless renderer was
            # reset — the ordering that prevents a use-after-free when an
            # in-flight handler touches renderer_/multi_monitor_ mid-shutdown.
            if ok:
                h = connect_pipe()
                try:
                    ack_text, _ = transact(
                        h, '{"version":1,"cmd":"quit","payload":{}}\n')
                    ack = json.loads(ack_text)
                    assert ack.get("ok"), ack_text
                    print("phase4: quit acked, waiting for engine exit 0",
                          flush=True)
                except (RuntimeError, AssertionError) as e:
                    ok = fail(f"quit transaction: {e}")
                finally:
                    k32.CloseHandle(h)
                try:
                    proc.wait(timeout=15)
                except subprocess.TimeoutExpired:
                    ok = fail("engine did not exit within 15s of quit")
                if ok and proc.returncode != 0:
                    ok = fail(f"engine exit code {proc.returncode}, expected 0")
                if ok:
                    with open(engine_log, "r", encoding="utf-8",
                              errors="replace") as ef:
                        ef.seek(log_offset)
                        log_text = ef.read()
                    ipc_stopped = "ipc: server thread stopped" in log_text
                    headless_stopped = (
                        "engine shutdown: headless renderer event thread stopped"
                        in log_text)
                    if not ipc_stopped or not headless_stopped:
                        ok = fail(
                            f"log markers missing (ipc_stopped={ipc_stopped}, "
                            f"headless_stopped={headless_stopped})")
                    else:
                        ipc_pos = log_text.index("ipc: server thread stopped")
                        headless_pos = log_text.index(
                            "engine shutdown: headless renderer event thread stopped")
                        if ipc_pos > headless_pos:
                            ok = fail(
                                "log order WRONG: IPC stopped AFTER headless "
                                "renderer stopped (use-after-free window)")
                        else:
                            print("phase4: exit 0, log order OK (IPC stopped "
                                  "before headless renderer stopped)",
                                  flush=True)
        finally:
            if proc.poll() is None:
                proc.terminate()
            try:
                proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                proc.kill()

    print("RESULT: PASS" if ok else "RESULT: FAIL", flush=True)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
