#!/usr/bin/env python3
"""window.py - how much slack a winning run has (tools/solve/solve.c).

  window.py WIN.keys NSCENES

For each jump (and the stop) in the script, moves that press earlier and
later, a step at a time (with the jumps chained straight after it, which
would otherwise come while he is still in the air), everything else where it
was, and replays the
script on the native model (build/ref/ref): the press's window is the run of
offsets that still clears NSCENES scenes without a crash.  Prints each press
with its window in steps (1 step = 20 ms) and the smallest window."""
import os, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
REF = os.path.join(ROOT, "build", "ref", "ref")


def parse(path):
    lines = []
    for ln in open(path):
        ln = ln.split("#")[0].split()
        if not ln:
            continue
        a, _, b = ln[0].partition("-")
        lines.append([int(a), int(b or a), ln[1:]])
    return lines


def trace(lines, steps):
    with tempfile.NamedTemporaryFile("w", suffix=".keys", delete=False) as f:
        for a, b, t in lines:
            f.write(f"{a}-{b} {' '.join(t)}\n" if a != b else f"{a} {' '.join(t)}\n")
        fn = f.name
    out = subprocess.run([REF, fn, str(steps)], capture_output=True, text=True).stdout.splitlines()
    os.unlink(fn)
    return out


def ok(lines, nsc, steps):
    with tempfile.NamedTemporaryFile("w", suffix=".keys", delete=False) as f:
        for a, b, t in lines:
            f.write(f"{a}-{b} {' '.join(t)}\n" if a != b else f"{a} {' '.join(t)}\n")
        fn = f.name
    out = subprocess.run([REF, fn, str(steps)], capture_output=True, text=True).stdout.splitlines()
    os.unlink(fn)
    scenes = 0
    for ln in out:
        f = ln.split()
        ev = int(f[7], 16)
        if ev & 0x80:
            return False
        if ev & 0x10:
            scenes += 1
    return scenes >= nsc


def windows(path, nsc, reach=12):
    lines = parse(path)
    steps = max(b for a, b, t in lines) + 5
    for ln in open(path):
        if ln.startswith("# ") and ln.strip().endswith("steps"):
            steps = int(ln.split()[1]) + 5
    assert ok(lines, nsc, steps), "the script itself fails"
    modes = [int(ln.split()[14], 16) & 15 for ln in trace(lines, steps)]
    res = []
    for i, (a, b, t) in enumerate(lines):
        if a != b or t not in (["jump"], ["stop"]):
            continue
        if a < 2 or modes[a - 2] != 2:      # a press on a card, not in play
            continue
        chain, last = [i], a                 # jumps taken as soon as he lands
        if t == ["jump"]:
            for j in sorted((j for j, (a2, b2, t2) in enumerate(lines)
                             if t2 == ["jump"] and a2 == b2 and a2 > a), key=lambda j: lines[j][0]):
                if lines[j][0] - last > 90:
                    break
                chain.append(j)
                last = lines[j][0]

        def moved(d):
            l2 = [list(x) for x in lines]
            for j in chain:
                l2[j][0] = l2[j][1] = lines[j][0] + d
            return l2
        lo = hi = 0
        while lo > -reach and ok(moved(lo - 1), nsc, steps):
            lo -= 1
        while hi < reach and ok(moved(hi + 1), nsc, steps):
            hi += 1
        res.append((a, t[0], lo, hi, hi - lo + 1))
    return res


if __name__ == "__main__":
    res = windows(sys.argv[1], int(sys.argv[2]))
    for a, tok, lo, hi, w in res:
        print(f"{tok:5} at {a:5}: {lo:+3} .. {hi:+3}  window {w:2} steps")
    print("smallest window:", min(r[4] for r in res) if res else None)
