#!/usr/bin/env python3
"""Todo 31 pipe ACL + fuzz: 1000 random messages at a live engine.

Reuses the ctypes + kernel32 pipe pattern from tests/send_test.py
(stdlib only, no pywin32). Spawns engine.exe (its overlapped pipe
server listens on \\\\.\\pipe\\k6wp-engine with a current-user-only
DACL from MakeCurrentUserOnlySA()), fires 1000 fuzz datagrams across
9 classes, then verifies post-fuzz liveness (5 valid cmds -> 5 acks)
and that the engine process is still alive.

Fuzz classes (total exactly 1000):
  - truncated   (120): valid NDJSON cut at a random offset, re-framed
  - oversize    (120): payload dump > 64 KiB (incl. >128 KiB pipe buf)
  - json_bad    (120): garbage bytes (no valid JSON)
  - wrong_ver   (110): version 0 / 2 / "1" / 1.5 / -1 / null / true
  - unknown_cmd (110): version ok, cmd not in the allowlist
  - wrong_types (110): cmd as int/null, payload as string/array
  - frameless   (110): valid JSON with NO trailing \\n -> no reply
  - empty       (100): zero-byte datagram -> no reply
  - near_limit  (100): payload dump exactly 64 KiB - 1 / 64 KiB (valid)

Expected server policy (Todo 28 HandleMessage):
  - framed + Decode-false -> {"error":...} ack (never throws/crashes)
  - frameless / empty    -> NO reply (client read times out)
  - framed + valid       -> {"ok":...} ack

PASS iff: engine survives all 1000 (0 crash), every reply parses as
a JSON object with an "ok" or "error" key (0 outside-allowlist
responses), no-reply occurs ONLY for frameless/empty, and post-fuzz
liveness gives 5/5 valid acks with the engine process still alive.
Anything else -> FAIL + stop-gate (do NOT fake counts).

ACL note: MakeCurrentUserOnlySA() (shared/ipc_protocol.cpp, Todo 6)
builds a DACL with exactly ONE ACCESS_ALLOWED_ACE (GENERIC_ALL) for
the current-user SID -- no Everyone / Authenticated Users ACE. A
cross-user denial check ("Access denied" for another user) is not
possible headless on this box; it is documented as manual_pending
(see learnings.md Todo 31 entry for the exact procedure).

Usage:
  python tests/pipe_fuzz.py [--engine PATH] [--seed N]

Exit 0 on PASS, 1 on FAIL.
"""

import argparse
import ctypes
import json
import random
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
MAX_PAYLOAD = 64 * 1024  # must match kMaxPayloadBytes

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


def read_with_timeout(h, timeout_s):
    """Blocking ReadFile on a worker thread.

    Returns (got_data, text, thread, out): the thread + out dict are
    returned so the caller can CancelIoEx + join + re-check for a late
    reply instead of abandoning a pending ReadFile on the handle (a
    stale reader would steal the NEXT message's ack -- Todo 28 trap).
    """
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
    return (not t.is_alive() and out.get("ok"), out.get("text", ""), t, out)


def send_raw(h, payload: bytes, reply_timeout_s):
    """Write one datagram, wait for one ack. Returns (status, text).

    status is one of "ok" (valid {"ok"} ack), "error" ({"error"} ack),
    "noreply" (read timed out), "io_error" (WriteFile failed).
    """
    if len(payload) == 0:
        # Zero-byte datagram: WriteFile(h, b"", 0) is a no-op status
        # check; the server's message-mode ReadFile yields n=0 and, by
        # policy, sends no reply.
        written = ctypes.c_uint32(0)
        empty_buf = ctypes.create_string_buffer(1)
        if not k32.WriteFile(h, empty_buf, 0, ctypes.byref(written), None):
            return ("io_error", f"zero-byte WriteFile failed: {ctypes.get_last_error()}")
    else:
        written = ctypes.c_uint32(0)
        if not k32.WriteFile(h, payload, len(payload), ctypes.byref(written), None):
            return ("io_error", f"WriteFile failed: {ctypes.get_last_error()}")
        if written.value != len(payload):
            return ("io_error", f"short write {written.value}/{len(payload)}")
    got, text, reader, out = read_with_timeout(h, reply_timeout_s)
    if not got:
        # A timed-out ReadFile stays pending on this synchronous handle;
        # reusing the handle without cancelling would let the stale reader
        # steal the NEXT datagram's ack (false missing-reply cascade).
        # Cancel + join first (Todo 28 CancelIoEx pattern); the cancel only
        # drops the pending read, the connection stays healthy.
        try:
            k32.CancelIoEx(h, None)
        except Exception:  # noqa: BLE001 - best effort
            pass
        reader.join(5.0)
        if not reader.is_alive() and out.get("ok"):
            # Reply landed between the timeout and the cancel: honor it.
            got, text = True, out.get("text", "")
        else:
            return ("noreply", "")
    try:
        ack = json.loads(text)
    except json.JSONDecodeError:
        return ("nonjson", text)
    if isinstance(ack, dict) and ack.get("ok") is True:
        return ("ok", text)
    if isinstance(ack, dict) and isinstance(ack.get("error"), str):
        return ("error", text)
    return ("allowlist_violation", text)


# --- Fuzz corpus -----------------------------------------------------------

VALID_CMDS = ["set_video", "pause", "resume", "set_monitor", "get_state"]

# Classes that must NOT receive a reply (frameless/empty policy).
NOREPLY_CLASSES = frozenset({"frameless", "empty"})


def gen_truncated(rng):
    base = json.dumps({"version": 1, "cmd": rng.choice(VALID_CMDS),
                       "payload": {"path": "C:\\Videos\\demo.mp4", "n": rng.randrange(9999)}},
                      separators=(",", ":")) + "\n"
    cut = rng.randrange(1, len(base))  # cut anywhere incl. the framing \n
    frag = base[:cut]
    if not frag.endswith("\n"):
        frag += "\n"  # keep it framed: malformed JSON -> error ack
    return frag.encode("utf-8")


def gen_oversize(rng):
    # Half: 64 KiB + slack (single datagram, Decode refuses) ;
    # half: >128 KiB pipe buffer (server ERROR_MORE_DATA drain path).
    if rng.randrange(2) == 0:
        n = MAX_PAYLOAD + rng.randrange(1, 4096)
    else:
        n = BUF_SIZE + rng.randrange(1, 96 * 1024)
    # payload.dump() == n  ->  {"blob":"xxx..."} overhead is 12 bytes.
    blob = "x" * max(0, n - 12)
    return (json.dumps({"version": 1, "cmd": "get_state",
                        "payload": {"blob": blob}},
                       separators=(",", ":")) + "\n").encode("utf-8")


def gen_json_bad(rng):
    choices = [
        b"{not json at all}\n",
        b"[[[{{{\n",
        b"\x00\x01\x02\xff\xfe binary garbage \x80\x81\n",
        b'{"version":1,"cmd":"get_state","payload":{}',  # missing brace + framed
        b'{"version":1,"cmd":unquoted,"payload":{}}\n',
        b"just some words 12345 !@#$%\n",
    ]
    pick = rng.choice(choices)
    if rng.randrange(4) == 0:
        # random byte soup (no \n inside, framed at the end)
        soup = bytes(rng.randrange(1, 256) for _ in range(rng.randrange(1, 200)))
        soup = soup.replace(b"\n", b" ").replace(b"\r", b" ")
        pick = soup + b"\n"
    if not pick.endswith(b"\n"):
        pick += b"\n"
    return pick


def gen_wrong_ver(rng):
    ver = rng.choice([0, 2, -1, 99, "1", 1.5, None, True,
                      [1], {"v": 1}])
    return (json.dumps({"version": ver, "cmd": "get_state",
                        "payload": {}}, separators=(",", ":")) + "\n").encode("utf-8")


def gen_unknown_cmd(rng):
    cmd = rng.choice(["delete_everything", "SET_VIDEO", "Get_State", "",
                      "pause ", "exec", "shutdown", "get_state\x00"])
    return (json.dumps({"version": 1, "cmd": cmd, "payload": {}},
                       separators=(",", ":")) + "\n").encode("utf-8")


def gen_wrong_types(rng):
    variant = rng.randrange(4)
    if variant == 0:
        root = {"version": 1, "cmd": 123, "payload": {}}
    elif variant == 1:
        root = {"version": 1, "cmd": None, "payload": {}}
    elif variant == 2:
        root = {"version": 1, "cmd": "get_state",
                "payload": rng.choice(["oops", [1, 2], 42, None])}
    else:
        root = {"version": "1", "cmd": "pause", "payload": {}}
    return (json.dumps(root, separators=(",", ":")) + "\n").encode("utf-8")


def gen_frameless(rng):
    # Valid JSON, NO trailing \n -> server sends no reply by policy.
    return json.dumps({"version": 1, "cmd": rng.choice(VALID_CMDS),
                       "payload": {}}, separators=(",", ":")).encode("utf-8")


def gen_empty(rng):  # noqa: ARG001 - signature kept uniform
    return b""


def gen_near_limit(rng):
    # payload.dump() exactly 64 KiB - 1 or exactly 64 KiB: both legal.
    target = rng.choice([MAX_PAYLOAD - 1, MAX_PAYLOAD])
    blob = "y" * max(0, target - 12)
    return (json.dumps({"version": 1, "cmd": "get_state",
                        "payload": {"blob": blob}},
                       separators=(",", ":")) + "\n").encode("utf-8")


GENERATORS = [
    ("truncated", 120, gen_truncated),
    ("oversize", 120, gen_oversize),
    ("json_bad", 120, gen_json_bad),
    ("wrong_ver", 110, gen_wrong_ver),
    ("unknown_cmd", 110, gen_unknown_cmd),
    ("wrong_types", 110, gen_wrong_types),
    ("frameless", 110, gen_frameless),
    ("empty", 100, gen_empty),
    ("near_limit", 100, gen_near_limit),
]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--engine", default="",
                    help="path to engine.exe (default: build/msvc-dev/engine.exe)")
    ap.add_argument("--seed", type=int, default=31)
    args = ap.parse_args()

    root = Path(__file__).resolve().parent.parent
    engine = Path(args.engine) if args.engine else root / "build" / "msvc-dev" / "engine.exe"
    if not engine.exists():
        print(f"FAIL: engine.exe not found: {engine}", flush=True)
        return 1

    rng = random.Random(args.seed)
    # Build the full 1000-message corpus up front (deterministic).
    corpus = []
    for cls, count, gen in GENERATORS:
        for _ in range(count):
            corpus.append((cls, gen(rng)))
    rng.shuffle(corpus)
    total = len(corpus)
    assert total == 1000, f"corpus size {total} != 1000"
    print(f"fuzz: {total} messages (seed={args.seed}), "
          + ", ".join(f"{cls}={n}" for cls, n, _ in GENERATORS), flush=True)

    stats = {cls: {"sent": 0, "ok": 0, "error": 0, "noreply": 0,
                   "bad": 0} for cls, _, _ in GENERATORS}
    violations = []  # (index, class, status, excerpt) outside the allowlist
    io_errors = []
    ok = True

    log_path = root / "build" / "pipe_fuzz_engine.log"
    log_path.parent.mkdir(parents=True, exist_ok=True)
    print(f"engine: {engine}", flush=True)
    with open(log_path, "w", encoding="utf-8") as logf:
        proc = subprocess.Popen([str(engine), "--exit-after-ms=300000"],
                                stdout=logf, stderr=subprocess.STDOUT)
        print(f"engine pid: {proc.pid}", flush=True)
        h = None
        try:
            try:
                h = connect_pipe()
            except RuntimeError as e:
                print(f"FAIL: initial connect: {e}", flush=True)
                return 1
            for i, (cls, payload) in enumerate(corpus):
                # Framed classes must answer promptly; frameless/empty get
                # a short window in which any reply would be a violation.
                timeout = 0.4 if cls in NOREPLY_CLASSES else 5.0
                try:
                    status, text = send_raw(h, payload, timeout)
                except Exception as e:  # noqa: BLE001 - harness must not crash
                    status, text = "io_error", f"<harness exc: {e}>"
                if status == "io_error":
                    # Possible crash: try one reconnect; if the server is
                    # gone, that IS the stop-gate crash signal.
                    io_errors.append((i, cls, text))
                    try:
                        if h is not None:
                            try:
                                k32.CancelIoEx(h, None)
                            except Exception:  # noqa: BLE001, S110 - best effort
                                pass
                            k32.CloseHandle(h)
                    except Exception:  # noqa: BLE001 - best effort teardown
                        pass
                    try:
                        h = connect_pipe(timeout_ms=5000)
                        status, text = send_raw(h, payload, timeout)
                    except Exception as e:  # noqa: BLE001
                        print(f"FAIL: crash suspected at msg {i} ({cls}): "
                              f"{text} / reconnect: {e}", flush=True)
                        ok = False
                        break
                    if status == "io_error":
                        print(f"FAIL: crash suspected at msg {i} ({cls}): {text}",
                              flush=True)
                        ok = False
                        break
                stats[cls]["sent"] += 1
                if status in ("ok", "error", "noreply"):
                    stats[cls][status] += 1
                else:  # nonjson / allowlist_violation
                    stats[cls]["bad"] += 1
                    violations.append((i, cls, status, text[:160]))
                # No-reply is legal ONLY for frameless/empty; a reply to
                # frameless/empty, or silence on a framed message, is a
                # policy violation (not a crash -- keep fuzzing, fail later).
                if cls in NOREPLY_CLASSES and status != "noreply":
                    violations.append((i, cls, f"unexpected-reply:{status}",
                                       text[:160]))
                    stats[cls]["bad"] += 1
                elif cls not in NOREPLY_CLASSES and status == "noreply":
                    # Framed message got no ack within 5s. Refine the
                    # diagnosis with a same-handle ping: a dead handle
                    # (server re-listening after a sync-path disconnect)
                    # fails fast with ERROR_PIPE_NOT_CONNECTED, while a
                    # live-but-silent server answers the ping. Without
                    # this probe a dropped connection masquerades as a
                    # plain timeout (seen on seed-31 msg 0: >128 KiB
                    # datagram arriving before the server parks its read
                    # takes the synchronous ERROR_MORE_DATA path, which
                    # disconnects instead of draining + error-acking).
                    ping = (b'{"version":1,"cmd":"get_state",'
                            b'"payload":{}}\n')
                    try:
                        pstatus, _ = send_raw(h, ping, 5.0)
                    except Exception:  # noqa: BLE001 - harness must not crash
                        pstatus = "io_error"
                    if pstatus == "io_error":
                        kind = "missing-reply+conn-dropped"
                        # Server re-listened; reconnect eagerly so the
                        # run continues on a live instance (the normal
                        # io_error path below would do it one msg later).
                        try:
                            try:
                                k32.CancelIoEx(h, None)
                            except Exception:  # noqa: BLE001 - best effort
                                pass
                            k32.CloseHandle(h)
                        except Exception:  # noqa: BLE001 - best effort
                            pass
                        try:
                            h = connect_pipe(timeout_ms=5000)
                        except Exception as e:  # noqa: BLE001
                            print(f"FAIL: reconnect after drop at msg {i} "
                                  f"({cls}): {e}", flush=True)
                            ok = False
                            break
                    elif pstatus == "noreply":
                        kind = "missing-reply+ping-noreply"
                    else:
                        kind = "missing-reply-conn-alive"
                    violations.append((i, cls, kind, ""))
                    stats[cls]["bad"] += 1
                if (i + 1) % 250 == 0:
                    alive = proc.poll() is None
                    print(f"progress: {i + 1}/{total} alive={alive}", flush=True)
                    if not alive:
                        print(f"FAIL: engine died mid-fuzz at msg {i}", flush=True)
                        ok = False
                        break

            # --- Post-fuzz liveness: 5 valid commands -> 5 valid acks ---
            live_ok = 0
            if ok and h is not None:
                # Fresh connection: proves re-listen still works after fuzz.
                try:
                    k32.CloseHandle(h)
                except Exception:  # noqa: BLE001 - best effort
                    pass
                try:
                    h = connect_pipe()
                except RuntimeError as e:
                    print(f"FAIL: post-fuzz reconnect: {e}", flush=True)
                    ok = False
                    h = None
            if ok and h is not None:
                for j in range(5):
                    line = ('{"version":1,"cmd":"get_state","payload":{}}\n'
                            if j % 2 == 0 else
                            '{"version":1,"cmd":"pause","payload":{}}\n')
                    status, text = send_raw(h, line.encode("utf-8"), 5.0)
                    if status == "ok":
                        live_ok += 1
                    else:
                        print(f"FAIL: post-fuzz liveness cmd {j}: {status} {text[:160]}",
                              flush=True)
                        ok = False
                        break
                print(f"liveness: {live_ok}/5 valid acks", flush=True)
            if h is not None:
                try:
                    k32.CloseHandle(h)
                except Exception:  # noqa: BLE001 - best effort
                    pass

            engine_alive = proc.poll() is None
            print(f"engine_alive_at_end: {engine_alive}", flush=True)
            if not engine_alive:
                print("FAIL: engine process died during fuzz", flush=True)
                ok = False
        finally:
            proc.terminate()
            try:
                proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                proc.kill()

    # --- Report ---
    print("---- fuzz report (per class) ----", flush=True)
    for cls, _, _ in GENERATORS:
        s = stats[cls]
        print(f"  {cls:11s} sent={s['sent']:3d} ok={s['ok']:3d} "
              f"error={s['error']:3d} noreply={s['noreply']:3d} bad={s['bad']}",
              flush=True)
    n_bad = sum(s["bad"] for s in stats.values())
    n_reply_outside = len([v for v in violations if "reply" in v[2]
                           or v[2] in ("nonjson", "allowlist_violation")])
    print(f"total={total} bad={n_bad} io_errors={len(io_errors)} "
          f"violations={len(violations)}", flush=True)
    for v in violations[:20]:
        print(f"  violation msg={v[0]} class={v[1]} kind={v[2]} "
              f"excerpt={v[3]!r}", flush=True)
    for e in io_errors[:10]:
        print(f"  io_error msg={e[0]} class={e[1]} detail={e[2][:160]!r}",
              flush=True)
    if violations or n_bad or not ok:
        ok = False

    print("ACL: MakeCurrentUserOnlySA single-ACE current-user DACL "
          "(code-reviewed, no change); cross-user denial = manual_pending",
          flush=True)
    print("RESULT: PASS" if ok else "RESULT: FAIL", flush=True)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
