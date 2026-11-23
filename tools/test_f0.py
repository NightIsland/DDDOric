#!/usr/bin/env python3
"""test_f0.py - F0 acceptance checks (docs/PLAN.md F0).  Run by `make test`.

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

# 4. the screen
r = emu.run(TAP, frames=50, out=os.path.join(OUT, "screen"))
emu.write_png(os.path.join(OUT, "f0-screen.png"), r.rgb, 2)
BLK, BLUE, YEL, GRN, CYAN = (0, 0, 0), (0, 0, 255), (255, 255, 0), (0, 255, 0), (0, 255, 255)
check("ULA in HIRES mode, mode cell $BFDF = $1E", "hires=1" in r.summary and r.mem[0xBFDF] == 0x1E)
check("title strip: blue paper, yellow text", r.pixel(0, 0) == BLUE and r.pixel(239, 7) == BLUE
      and any(r.pixel(x, y) == YEL for x in range(12, 90) for y in range(7)))
roads = [8 + 48 * b + 44 for b in range(4)]
check("four road lines, solid green across the play width",
      all(r.pixel(x, y) == GRN for y in roads for x in range(12, 240)), str(roads))
check("attribute cells show paper (play starts at pixel 12)", all(r.pixel(x, y) == BLK for y in roads for x in range(12)))
air = [(x, y) for b in range(4) for y in range(8 + 48 * b, 8 + 48 * b + 28) for x in range(12, 240)]
check("air empty (no rider there at rest)", all(r.pixel(x, y) == BLK for x, y in air))
row0 = r.mem[0xBF68:0xBF68 + 40]
check("status row shows BUILD and the number from src/version.h",
      row0[1:9] == b"BUILD 01" or row0[1:9] == b"BUILD %02d" % int(open(os.path.join(ROOT, "src/version.h")).read().split()[-1]),
      row0[1:9].decode())
glyph0 = [r.pixel(7 * 6 + i, 200 + 1) for i in range(6)]
check("BUILD digit drawn from our charset at $9800", r.mem[0x9800 + ord("0") * 8] == 0x0E and YEL in glyph0)
r = emu.run(TAP, frames=1, out=os.path.join(OUT, "rider"))
ry = 8 + 43
check("rider block on track 1 at x 0-5 (placeholder)", all(r.pixel(12 + i, ry) == YEL for i in range(1, 5)))

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
                                                    ("pause.keys", 250), ("scene.keys", 800))] + [(rnd, 6000)]
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

print(f"\n{'ALL PASS' if not fails else f'{fails} FAILED'}")
sys.exit(1 if fails else 0)
