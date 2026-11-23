#!/usr/bin/env python3
"""test_f0.py - F0 and F1 acceptance checks (docs/PLAN.md).  Run by `make test`.

Adapted from Oric Chuckie Egg's tools/test_f0.py.  Writes screenshots and
traces to build/test/."""
import os, random, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import run as emu  # noqa: E402

B = os.path.join(ROOT, "build")
OUT = os.path.join(B, "test")
os.makedirs(OUT, exist_ok=True)
TAP = os.path.join(B, "stuntbob.tap")
OTAP = os.path.join(B, "oricutron", "stuntbob.tap")
REF = os.path.join(B, "ref", "ref")
KEYS = os.path.join(ROOT, "tests", "keys")
STEP = 20000
fails = 0


def check(name, ok, detail=""):
    global fails
    print(f"{'PASS' if ok else 'FAIL'}  {name}" + (f"  ({detail})" if detail else ""))
    fails += not ok


def ref_trace(script, steps):
    return subprocess.run([REF, script, str(steps)], capture_output=True, check=True).stdout.decode().splitlines()


def parse(line):
    f = line.split()
    return dict(step=int(f[0], 16), track=int(f[1], 16), x=int(f[2], 16), speed=int(f[3], 16),
                jt=int(f[4], 16), h=int(f[5], 16), flags=int(f[6], 16), ev=int(f[7], 16), inp=int(f[8], 16))


# 1. .tap header
t = open(TAP, "rb").read()
binlen = os.path.getsize(os.path.join(B, "stuntbob.bin"))
end, start = t[9] << 8 | t[10], t[11] << 8 | t[12]
check("tap header: machine code, autorun, $0500, inclusive end",
      t[:5] == b"\x16\x16\x16\x16\x24" and t[7] == 0x80 and t[8] == 0xC7
      and start == 0x0500 and end == start + binlen - 1, f"${start:04X}-${end:04X}")

# 2. CPU core: Klaus Dormann's functional test
os.makedirs(os.path.join(OUT, "klaus"), exist_ok=True)
r = subprocess.run([emu.EMU, "--flat", "--bin", os.path.join(ROOT, "tools/emu/test/6502_functional_test.bin") + "@0",
                    "--start", "0x400", "--clock", "video", "--frames", "100000000", "--out", os.path.join(OUT, "klaus")],
                   capture_output=True, text=True)
check("6502 core passes Klaus Dormann's functional test", "trap $3469" in r.stdout, r.stdout.split("\n")[-1])

# 3. both ROMs and the polling fallback: ROM-free run, 50 Hz steps
syms = emu.symbols(TAP)
worst = 0
for rom, mode in (("atmos", 1), ("oric1", 1), ("none", 0)):
    r = emu.run(TAP, frames=300, rom=rom, out=os.path.join(OUT, "rom-" + rom))
    per, busy = r.frame_stats()
    avg = (r.marks[-1] - r.marks[0]) / (len(r.marks) - 1)
    check(f"{rom}: 300 steps, no ROM code, bad opcode or stray IRQ", r.rc == 0 and "frames=300" in r.summary,
          r.err or r.summary.split("\n")[0])
    check(f"{rom}: irq_mode = {mode} (" + ("IRQ hooked via the RAM vector" if mode else "polling T1") + ")",
          r.mem[syms["_irq_mode"]] == mode)
    check(f"{rom}: step period 20,000 cycles (50 Hz)", abs(avg - STEP) < 1 and max(per) - min(per) < 400,
          f"min {min(per)} max {max(per)} avg {avg:.1f}, {len(r.irq)} IRQs")
    worst = max(worst, max(busy))

# 4. the screen (F1): scene = pixel reference from src/scene.c + tools/mkart.py
import scene_ref  # noqa: E402
r = emu.run(TAP, frames=50, out=os.path.join(OUT, "screen"))
emu.write_png(os.path.join(OUT, "f1-screen.png"), r.rgb, 2)
RED, YEL = (255, 0, 0), (255, 255, 0)
roads = [8 + 48 * b + 44 for b in range(4)]
check("ULA in HIRES mode, mode cell $BFDF = $1E", "hires=1" in r.summary and r.mem[0xBFDF] == 0x1E)
BLK = (0, 0, 0)
check("left edge: cell 0 black on every line, picture from pixel 6 (Bob, BUILD 4)",
      all(r.pixel(x, y) == BLK for y in range(200) for x in range(6)) and r.pixel(6, 0) == RED
      and all(r.pixel(6, y) == YEL for y in roads))
check("title strip: red paper, yellow film name", r.pixel(6, 0) == RED and r.pixel(239, 7) == RED
      and any(r.pixel(x, y) == YEL for x in range(12, 90) for y in range(1, 8)))
def with_rider(want, r):
    """the reference scene plus the rider where the run's last trace line says"""
    s_ = parse(r.trace.decode().splitlines()[-1])
    x = s_["x"] >> 8
    for a_, v in scene_ref.rider_bytes(s_["track"], x, s_["h"], scene_ref.frame_for(x, s_["jt"])).items():
        want[a_ - 0xA000] ^= v
    return want


want = with_rider(scene_ref.scene_bytes(1), r)
got = r.mem[0xA000:0xA000 + 8000]
bad = [i for i in range(320, 8000) if want[i] != got[i]]
check("scene drawn from its data: lines 8-199 = reference + rider (billing 1)", not bad,
      f"{len(bad)} wrong bytes" + (f", first line {bad[0] // 40} cell {bad[0] % 40}" if bad else ""))
check("title strip colour cells", all(got[y * 40:y * 40 + 2] == want[y * 40:y * 40 + 2] for y in range(8)))
roads = [8 + 48 * b + 44 for b in range(4)]
check("four kerbs, yellow from pixel 6 to the right edge (no gaps at billing 1)",
      all(r.pixel(x, y) == YEL for y in roads for x in range(6, 240)), str(roads))
B3 = os.path.join(B, "b3", "stuntbob.tap")
r3 = emu.run(B3, frames=5, out=os.path.join(OUT, "screen-b3"))
emu.write_png(os.path.join(OUT, "f1-screen-billing3.png"), r3.rgb, 2)
want3 = with_rider(scene_ref.scene_bytes(3), r3)
got3 = r3.mem[0xA000:0xA000 + 8000]
bad3 = [i for i in range(320, 8000) if want3[i] != got3[i]]
holes = sum(1 for y in roads for x in range(12, 240) if r3.pixel(x, y) != YEL)
check("billing 3 shows the gaps tagged for it (scene = reference)", not bad3 and holes == 18 + 24,
      f"{len(bad3)} wrong bytes, {holes} kerb pixels missing")
row0 = r.mem[0xBF68:0xBF68 + 40]
row2 = r.mem[0xBF68 + 80:0xBF68 + 120]
ver = int(open(os.path.join(ROOT, "src/version.h")).read().split()[-1])
check("status rows: take, stop lamp lit, fee; BUILD number from src/version.h",
      row0[1:7] == b"TAKE 1" and row0[19] == ord("*") and b"FEE $0" in row0 and row2[1:9] == b"BUILD %02d" % ver,
      row2[1:9].decode())
glyph0 = [r.pixel(7 * 6 + i, 216 + 1) for i in range(6)]
check("status glyphs drawn from our charset at $9800", r.mem[0x9800 + ord("0") * 8] == 0x0E
      and r.mem[0x9800 + ord("$") * 8] == 0x06)

# 5. every key by its Oric name -> input bits (trace column 9)
ak = os.path.join(KEYS, "allkeys.keys")
r = emu.run(TAP, frames=90, keys=ak, out=os.path.join(OUT, "allkeys"))
inp = {parse(l)["step"]: parse(l)["inp"] for l in r.trace.decode().splitlines()}
want = {10: 1, 20: 2, 30: 2, 40: 4, 50: 8, 70: 0x10, 80: 2, 15: 0}
check("keys: Space=jump, either Shift=accel, Return=stop, P=pause, Esc=quit",
      all(inp.get(k) == v for k, v in want.items()), str({k: inp.get(k) for k in want}))

# 6. trace equality: native model vs Oric build, step for step
rnd = os.path.join(OUT, "random.keys")
rg = random.Random(1123)
with open(rnd, "w") as f:
    n = 1
    while n < 6000:
        ln = rg.randint(1, 40)
        f.write(f"{n}-{n + ln - 1} " + " ".join(rg.sample(["jump", "accel", "accel", "stop", "pause", "quit"],
                                                          rg.randint(1, 2))) + "\n")
        n += ln + rg.randint(0, 60)
scripts = [(os.path.join(KEYS, s), n) for s, n in (("basic.keys", 400), ("wrapjump.keys", 300),
                                                    ("pause.keys", 250), ("scene.keys", 800),
                                                    ("airstop.keys", 260))] + [(rnd, 6000)]
total = 0
for path, n in scripts:
    r = emu.run(TAP, frames=n + 1, keys=path, out=os.path.join(OUT, "trace-" + os.path.basename(path)))
    oric = r.trace.decode().splitlines()
    ref = ref_trace(path, n)
    open(os.path.join(OUT, os.path.basename(path) + ".ref.txt"), "w").write("\n".join(ref) + "\n")
    same = oric == ref
    first = next((i for i, (a, b) in enumerate(zip(oric, ref)) if a != b), None)
    check(f"trace {os.path.basename(path)}: Oric == native over {n} steps", same and r.rc == 0,
          f"{len(oric)} vs {len(ref)} lines" + (f", first difference at step {first + 1}" if first is not None else ""))
    worst = max(worst, max(r.frame_stats()[1]))
    total += n
check("step work under the 20,000-cycle step", worst < STEP, f"worst {worst} cycles over {total} scripted steps")

# 7. the model's rules on the native traces (DESIGN "Physics", placeholders)
def rules(lines):
    prev = None
    errs = []
    for l in lines:
        s = parse(l)
        if prev:
            restart = s["ev"] & (0x10 | 0x40)
            if s["speed"] < prev["speed"] and not (s["ev"] & 0x04) and not restart:
                errs.append(f"speed fell at {s['step']}")
            if s["ev"] & 0x04 and not prev["flags"] & 1:
                errs.append(f"second stop at {s['step']}")
            if s["ev"] & 0x08 and not restart and prev["jt"] and s["jt"] != (prev["jt"] + 1) % 60:
                errs.append(f"wrap lost the jump at {s['step']}")
            if s["ev"] & 0x08 and not restart and s["speed"] < prev["speed"]:
                errs.append(f"wrap lost speed at {s['step']}")
        prev = s
    return errs
allr = [ref_trace(p, n) for p, n in scripts]
errs = sum((rules(t) for t in allr), [])
check("rules: speed never falls but for the stop; one stop per scene; wrap keeps arc and speed",
      not errs, "; ".join(errs[:3]))
wj = [parse(l) for l in allr[1]]
wraps = [s for s in wj if s["ev"] & 0x08 and s["jt"]]
check("wrapjump.keys: a jump crosses onto the next track in the air", bool(wraps) and wraps[0]["track"] >= 1,
      f"track {wraps[0]['track'] + 1}, step {wraps[0]['step']}, height {wraps[0]['h']}" if wraps else "no wrap in a jump")
ast = [parse(l) for l in allr[4]]
st_ = next((s for s in ast if s["ev"] & 0x04), None)
after = [s for s in ast if st_ and st_["step"] <= s["step"] < st_["step"] + 65]
check("airstop.keys: the stop works in mid-air; the bike drops straight down, lands, one stop only",
      st_ is not None and st_["jt"] > 0 and all(s["x"] == st_["x"] and s["speed"] == 0 for s in after)
      and any(s["ev"] & 0x02 for s in after) and sum(1 for s in ast if s["ev"] & 0x04) == 1,
      f"stop at step {st_['step']}, jump step {st_['jt']}, height {st_['h']}" if st_ else "no stop")
r = emu.run(TAP, frames=200, keys=os.path.join(KEYS, "airstop.keys"), out=os.path.join(OUT, "lamp"))
r0 = emu.run(TAP, frames=60, keys=os.path.join(KEYS, "airstop.keys"), out=os.path.join(OUT, "lamp0"))
check("stop lamp: lit before the stop, out after it (Bob, BUILD 4)",
      r0.mem[0xBF68 + 19] == ord("*") and r.mem[0xBF68 + 19] == ord("+"),
      f"{chr(r0.mem[0xBF68 + 19])} then {chr(r.mem[0xBF68 + 19])}")
sc = [parse(l) for l in allr[3]]
check("scene.keys: leaving track 4 finishes the scene", any(s["ev"] & 0x10 for s in sc))

# 8. Oricutron build: noise an octave lower (every R6 value doubled)
path = os.path.join(KEYS, "basic.keys")
rel = emu.run(TAP, frames=400, keys=path, out=os.path.join(OUT, "ay-release"))
ori = emu.run(OTAP, frames=400, keys=path, out=os.path.join(OUT, "ay-oricutron"))
r6r = [v for _, reg, v in rel.ay if reg == 6 and v]
r6o = [v for _, reg, v in ori.ay if reg == 6 and v]
check("ORICUTRON=1: every noise period doubled, all within 1-31",
      len(r6r) >= 3 and r6o == [2 * v for v in r6r] and max(r6o) <= 31, f"release {r6r}, Oricutron {r6o}")
check("ORICUTRON=1: same game (traces equal)", rel.trace == ori.trace)

# 9. determinism and size
a, b = emu.run(TAP, frames=200, keys=path), emu.run(TAP, frames=200, keys=path)
check("emulator deterministic (screen, RAM, marks, AY)", (a.rgb, a.mem, a.marks, a.ay) == (b.rgb, b.mem, b.marks, b.ay))
mp = open(os.path.join(B, "stuntbob.map")).read()
seg = mp.split("Segment list:")[1]
st = int(seg.split("STARTUP")[1].split()[2], 16)
check("crt0 + cfg within 256 B", st <= 256, f"STARTUP {st} B")

# 10. the rider (F1): moved in one pass, so no screen byte is written twice in a
#     step; written at the beam's pace, so no video frame shows half a move; and
#     the screen after every script = the scene + the rider where the trace says
import bisect
VF = 19968                                   # Oric video frame: 312 lines x 64 cycles
blank = scene_ref.scene_bytes(1)


def tears(r):
    """(steps in which some video frame showed part old, part new rider, drawing
    steps, slowest burst in cycles a line).  Each step's writes are split into
    bursts (gaps > 300 cycles: the two halves of a wrap); a frame f scans line y,
    cell c at f * VF + y * 64 + c (the emulator's video timing).  A frame is torn
    when it scans some of a burst's bytes before their write and some after."""
    steps = {}
    for a, v, c in r.wlog:
        if c >= r.marks[0]:
            steps.setdefault(bisect.bisect_right(r.marks, c), []).append((a - 0xA000, c))
    bad, pace = 0, 0
    for ws in steps.values():
        bursts, cur = [], [ws[0]]
        for w in ws[1:]:
            if w[1] - cur[-1][1] > 300:
                bursts.append(cur)
                cur = []
            cur.append(w)
        bursts.append(cur)
        torn = False
        for bu in bursts:
            t0, t1 = bu[0][1], bu[-1][1]
            nl = len({o // 40 for o, _ in bu})
            pace = max(pace, (t1 - t0 - 30) / max(1, nl - 1))
            for f in range(t0 // VF - 1, t1 // VF + 1):
                d = [c - (f * VF + (o // 40) * 64 + o % 40) for o, c in bu]
                torn |= min(d) < 0 < max(d)
        bad += torn
    return bad, len(steps), pace


tj = os.path.join(KEYS, "topjumps.keys")
for path, n in [(tj, 1500)] + scripts[:5]:
    r = emu.run(TAP, frames=n, keys=path, wlog=True, out=os.path.join(OUT, "rider-" + os.path.basename(path)))
    quits = {parse(l)["step"] for l in r.trace.decode().splitlines() if parse(l)["ev"] & 0x40}
    last, twice = {}, 0
    for a, v, c in r.wlog:
        if c < r.marks[0]:
            continue
        st = bisect.bisect_right(r.marks, c)
        twice += a in last and last[a] == st and st not in quits
        last[a] = st
    torn, drawn, pace = tears(r)
    s_ = parse(r.trace.decode().splitlines()[-1])
    x = s_["x"] >> 8
    want = bytearray(blank)
    for a, v in scene_ref.rider_bytes(s_["track"], x, s_["h"], scene_ref.frame_for(x, s_["jt"])).items():
        want[a - 0xA000] ^= v
    got = r.mem[0xA000:0xA000 + 8000]
    bad = [i for i in range(320, 8000) if want[i] != got[i]]
    check(f"rider {os.path.basename(path)}: one write per byte per step, under 1% torn frames; "
          "screen = scene + rider", twice == 0 and torn * 100 < drawn and not bad,
          f"{twice} double writes, {torn} torn of {drawn} drawing steps, burst {pace:.0f} cycles a line, "
          f"{len(bad)} wrong bytes")

print(f"\n{'ALL PASS' if not fails else f'{fails} FAILED'}")
sys.exit(1 if fails else 0)
