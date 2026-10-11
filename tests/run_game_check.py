#!/usr/bin/env python3
"""Gameplay regression check: run the game image headless with scripted pad input for a fixed
time, then check the log.

  board            title -> character select -> board map -> the first minigame (120 s)
  game <Name> [n]  direct launch of one minigame, variation n (default 1), 60 s

Pass: every required line is in the log in order, no forbidden line is, and the host is still running
when the time ends (the game's main never returns, so an earlier exit is a failure).
The log of the last run is kept at <work>/<name>.log for inspection.

Usage: run_game_check.py --host H --image E --iso I --work DIR [--seconds S] board
       run_game_check.py --host H --image E --iso I --work DIR [--seconds S] game NAME [VAR]
"""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile

# The board recipe (build-fix-frame-pacing/run/input_mv.txt): title, menus, 1 human player,
# character select, board; then A every 3 s takes the board turns and the minigame intro.
BOARD_INPUT = ["START@6000", "A@9000", "A@11000", "A@13000", "START@16000", "START@20000",
               "START@24000", "START@28000", "A@32000", "START@36000"]

FORBIDDEN = ["OpenFrenzy fault", "UNIMPLEMENTED", "Out of memory", "assert"]

# Original game behaviour, not a port bug: ScratchMatch 1-3 assert at the outro.
ALLOWED = {"ScratchMatch": ["CScratchmatch_Game.cpp(815)"]}


def a_every_3s(start_ms, end_ms):
    return ["A@%d" % t for t in range(start_ms, end_ms, 3000)]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", required=True)
    ap.add_argument("--image", required=True)
    ap.add_argument("--iso", required=True)
    ap.add_argument("--work", required=True)
    ap.add_argument("--seconds", type=int)
    ap.add_argument("mode", choices=["board", "game"])
    ap.add_argument("game", nargs="?")
    ap.add_argument("variation", nargs="?", default="1")
    args = ap.parse_args()

    env = dict(os.environ)
    for key in [k for k in env if k.startswith("OPENFRENZY_")]:
        del env[key]  # only this script's settings
    env.update({
        "SDL_VIDEO_DRIVER": "dummy",
        "SDL_AUDIO_DRIVER": "dummy",
        "OPENFRENZY_DEBUG_SKIP_VIDEOS": "true",
        "OPENFRENZY_BKLOG": "1",
        "OPENFRENZY_ISO": os.path.abspath(args.iso),
    })

    if args.mode == "board":
        name = "board"
        seconds = args.seconds or 120
        script = BOARD_INPUT + a_every_3s(40000, seconds * 1000)
        required = ["Top of stack now: CharacterSelect", "Top of stack now: Map",
                    "countdown PLAYING"]
        allowed = []
    else:
        if not args.game:
            ap.error("game mode needs a game name")
        name = "%s_%s" % (args.game, args.variation)
        seconds = args.seconds or 60
        script = a_every_3s(3000, seconds * 1000)
        required = ["countdown PLAYING"]
        allowed = ALLOWED.get(args.game, [])
        env["OPENFRENZY_GAME"] = args.game
        env["OPENFRENZY_VARIATION"] = args.variation
    env["OPENFRENZY_DEBUG_TEST_INPUT"] = ",".join(script)

    os.makedirs(args.work, exist_ok=True)
    # A fresh save root and data root for each run (no state from an earlier run).
    root = tempfile.mkdtemp(prefix=name + ".", dir=args.work)
    env["OPENFRENZY_SAVE_ROOT"] = os.path.join(root, "saves")
    env["OPENFRENZY_DATA_ROOT"] = os.path.join(root, "data")
    os.makedirs(env["OPENFRENZY_SAVE_ROOT"])
    os.makedirs(env["OPENFRENZY_DATA_ROOT"])
    log_path = os.path.join(args.work, name + ".log")

    print("run: %s for %d s" % (name, seconds), flush=True)
    with open(log_path, "wb") as log:
        proc = subprocess.Popen([args.host, args.image], stdout=log, stderr=subprocess.STDOUT,
                                stdin=subprocess.DEVNULL, env=env)
        try:
            code = proc.wait(timeout=seconds)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
            code = None  # still running at the end: expected
    shutil.rmtree(root, ignore_errors=True)

    with open(log_path, "r", errors="replace") as f:
        lines = f.read().splitlines()

    errors = []
    if code is not None:
        errors.append("the host exited after less than %d s (status %d)" % (seconds, code))
    # Required lines in order: the title shows the board map once before character select
    # (attract), so "Map" counts only after "CharacterSelect".
    pos = 0
    for want in required:
        found = next((i for i in range(pos, len(lines)) if want in lines[i]), None)
        if found is None:
            errors.append("missing (after line %d): %r" % (pos, want))
        else:
            pos = found + 1
    for i, line in enumerate(lines):
        if any(ok in line for ok in allowed):
            continue
        for bad in FORBIDDEN:
            if bad.lower() in line.lower():
                errors.append("line %d has %r: %s" % (i + 1, bad, line.strip()))
                break

    if errors:
        print("FAIL %s (log: %s)" % (name, log_path))
        for e in errors[:20]:
            print("  " + e)
        print("--- last 40 log lines ---")
        print("\n".join(lines[-40:]))
        return 1
    print("PASS %s (%d log lines; log: %s)" % (name, len(lines), log_path))
    return 0


if __name__ == "__main__":
    sys.exit(main())
