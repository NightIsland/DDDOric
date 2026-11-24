#!/usr/bin/env python3
"""test_f0.py - F0 to F4 acceptance checks (docs/PLAN.md).  Run by `make test`.

Two game builds: build/stuntbob.tap rides Rush Hour Rumble scene 1 (hazards,
crashes, fees), build/empty/stuntbob.tap the empty test track (-DSCENE=
scene_test), so the physics, sound and rider-drawing checks see no crashes.
F4: the game starts on the title screen; the play scripts press 1 and tap
Space to skip the clapper card (tests/keys/start.keys), so riding starts at
step 3.

Adapted from Oric Chuckie Egg's tools/test_f0.py.  Writes screenshots and
traces to build/test/."""
import os, random, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import run as emu  # noqa: E402

B = os.path.join(ROOT, "build")
OUT = os.path.join(B, "test")
os.makedirs(OUT, exist_ok=True)
TAP = os.path.join(B, "stuntbob.tap")
OTAP = os.path.join(B, "oricutron", "stuntbob.tap")
ETAP = os.path.join(B, "empty", "stuntbob.tap")
REF = os.path.join(B, "ref", "ref")
KEYS = os.path.join(ROOT, "tests", "keys")
STEP = 20000
fails = 0


def check(name, ok, detail=""):
    global fails
    print(f"{'PASS' if ok else 'FAIL'}  {name}" + (f"  ({detail})" if detail else ""))
    fails += not ok


def ref_trace(script, steps, test=False):
    """the native model's trace; test=True: on the empty test track"""
    return subprocess.run([REF, script, str(steps)] + (["test"] if test else []),
                          capture_output=True, check=True).stdout.decode().splitlines()


def parse(line):
    f = line.split()
    return dict(step=int(f[0], 16), track=int(f[1], 16), x=int(f[2], 16), speed=int(f[3], 16),
                jt=int(f[4], 16), h=int(f[5], 16), flags=int(f[6], 16), ev=int(f[7], 16), inp=int(f[8], 16),
                ev2=int(f[9], 16), crash_t=int(f[10], 16), takes=int(f[11], 16) >> 4, take=int(f[11], 16) & 15,
                fee=int(f[12], 16) * 10, hz=int(f[13], 16), mode=int(f[14], 16) & 15, billing=int(f[14], 16) >> 4,
                mt=int(f[15], 16), key=int(f[16], 16))


# F4 modes (src/model.h)
TITLE, CARD, PLAY, CUT, PRINT, FIRED, NAME, WRAP, ASK = range(9)
CUT_SHOW, CARD_SNAP = 75 - 20, 55
START = os.path.join(KEYS, "start.keys")


def steady(lines):
    """the riding steps (play, the crash pause, the quit question) after the
    first three of a new screen (the step that draws it whole overruns, so the
    next one starts late in its 20 ms), less the step "CUT!" pops up in the
    title strip, and the steps a pause or quit popup comes or goes: the steps
    the beam-paced drawing and the step budget are for"""
    out, prev = set(), []
    for l in lines:
        s_ = parse(l)
        rid = s_["mode"] in (PLAY, CUT, ASK)
        if rid and len(prev) == 3 and all(prev) and s_["crash_t"] != CUT_SHOW and not s_["ev"] & 0x60:
            out.add(s_["step"])
        prev = (prev + [rid])[-3:]
    return out


def steady_busy(r):
    """the work of each steady step (frame_stats' busy list: index n - 1 is step n)"""
    busy = r.frame_stats()[1]
    return [busy[n - 1] for n in steady(r.trace.decode().splitlines()) if n - 1 < len(busy)]


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
r = emu.run(TAP, frames=50, keys=START, out=os.path.join(OUT, "screen"))
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
import mkart  # noqa: E402


def run_scene(lines):
    """(scene name, billing) of the scene being played at a trace's last line:
    a run starts at scene (billing - 1) x 12 + 1, each clear moves it on one,
    and film 1's two scenes take turns (src/scene.c scene_at)"""
    start_b, cleared, s_ = 1, 0, None
    for l in lines:
        s_ = parse(l)
        if s_["ev2"] & 0x20:                # a new run
            start_b, cleared = s_["billing"], 0
        if s_["ev"] & 0x10:
            cleared += 1
    return scene_ref.scene_at((start_b - 1) * 12 + 1 + cleared), (s_["billing"] if s_ else 1)


def scene_of(r):
    return run_scene(r.trace.decode().splitlines())


def with_rider(want, r):
    """the reference scene plus the rider where the run's last trace line says"""
    s_ = parse(r.trace.decode().splitlines()[-1])
    x = scene_ref.px(s_["x"])
    veh = scene_ref.load_scene(scene_of(r)[0])["vehicle"]
    for a_, v in scene_ref.rider_bytes(s_["track"], x, s_["h"],
                                       scene_ref.frame_for(x, s_["jt"], s_["crash_t"]), veh).items():
        want[a_ - 0xA000] ^= v
    return want


def reference(r):
    """the screen the run should end on: its scene at its billing, the rider and the moving hazards"""
    name, bill = scene_of(r)
    return with_hazards(with_rider(scene_ref.scene_bytes(bill, name), r), r)


def moving(name="scene_rhr1", bill=1):
    sc = scene_ref.load_scene(name, bill)
    return [(t, k, x, a) for t, recs in enumerate(sc["tracks"]) for k, x, a, b in recs if k in (2, 3, 6, 8)]


SC = scene_ref.load_scene(billing=1)
MOVING = moving()
NSLOT = 8                                   # src/scene.h SC_MAX


def runner_shift(r, sy, i, a, img):
    """the pixel shift of moving hazard i's picture at img: F6a pre-shifts it
    at scene start into src/screen.c mov_buf from mv_img[i], 6 shifts of
    columns x lines"""
    base = r.mem[sy["_mv_img"] + 2 * i] | r.mem[sy["_mv_img"] + 2 * i + 1] << 8
    rows, w, h = mkart.scenery(mkart.SCENERY[a][0])
    return (img - base) // (mkart.sprite_cols(w) * h)


def drawn_pos(r, tap=TAP, name=None):
    """where the moving hazards are drawn (src/draw.s sprite table: sprite
    1 + i), as hazard_bytes positions: a runner's x px, a dangler's drop"""
    sy = emu.symbols(tap)
    name, bill = scene_of(r) if name is None else (name, scene_of(r)[1])
    pos = []
    for i, (t, k, x, a) in enumerate(moving(name, bill)):
        n = 1 + i
        y, c = r.mem[sy["_sp_y"] + n], r.mem[sy["_sp_c"] + n]
        img = r.mem[sy["_sp_lo"] + n] | r.mem[sy["_sp_hi"] + n] << 8
        if k in (2, 8):
            pos.append((c - 2) * 6 + runner_shift(r, sy, i, a, img))
        elif k == 6:                            # a jet: drawn (on) or not
            pos.append(2 if r.mem[sy["_sp_on"] + n] else 0)
        else:
            pos.append(y - (8 + 48 * t + scene_ref.DANGLE_TOP))
    return pos


def lamp_lines(name="scene_rhr1", bill=1):
    """the runners' top two lines: their ink flashes red and blue (Bob, BUILD 13)"""
    return [(8 + 48 * t + 44 - 12, 8 + 48 * t + 45 - 12) for t, k, x, a in moving(name, bill)
            if k == 2 and mkart.SCENERY[a][0] in ("police", "boat")]


def with_hazards(want, r, tap=TAP):
    name, bill = scene_of(r)
    for a_, v in scene_ref.hazard_bytes(name, pos=drawn_pos(r, tap, name), billing=bill).items():
        want[a_ - 0xA000] ^= v
    # a runner whose draw waited for room and then moved far: its old picture
    # shows for one more step, a ghost (sprite 9 + i, src/screen.c hazards_draw)
    sy = emu.symbols(tap)
    for i, (t, k, x, a) in enumerate(moving(name, bill)):
        g = 1 + i + 8
        if k == 6 and r.mem[sy["_mv_hot"] + i]:     # a jet's warning: its vent glows
            w = mkart.scenery(mkart.SCENERY[a][0])[1]
            want[(8 + 48 * t + 44 + 2) * 40 + 2 + (x + w // 2) // 6] ^= scene_ref.VENT_HOT
        if k in (2, 8) and r.mem[sy["_sp_on"] + g]:
            c = r.mem[sy["_sp_c"] + g]
            img = r.mem[sy["_sp_lo"] + g] | r.mem[sy["_sp_hi"] + g] << 8
            gx = (c - 2) * 6 + runner_shift(r, sy, i, a, img)
            art = mkart.SCENERY[a][0]
            for a_, v in scene_ref.sprite_bytes(art, gx, r.mem[sy["_sp_y"] + g]).items():
                want[a_ - 0xA000] ^= v
    for y0, y1 in lamp_lines(name, bill):   # red and blue, either way round
        inks = (r.mem[0xA000 + y0 * 40], r.mem[0xA000 + y1 * 40])
        if inks in ((1, 4), (4, 1)):
            want[y0 * 40], want[y1 * 40] = inks
    return want


want = reference(r)
got = r.mem[0xA000:0xA000 + 8000]
bad = [i for i in range(320, 8000) if want[i] != got[i]]
check("scene drawn from its data: lines 8-199 = reference + rider + moving hazards (billing 1)", not bad,
      f"{len(bad)} wrong bytes" + (f", first line {bad[0] // 40} cell {bad[0] % 40}" if bad else ""))
check("title strip colour cells", all(got[y * 40:y * 40 + 2] == want[y * 40:y * 40 + 2] for y in range(8)))
roads = [8 + 48 * b + 44 for b in range(4)]
check("four kerbs, yellow from pixel 6 to the right edge (no gaps at billing 1)",
      all(r.pixel(x, y) == YEL for y in roads for x in range(6, 240)), str(roads))
B3 = os.path.join(B, "b3", "stuntbob.tap")
r3 = emu.run(B3, frames=10, keys=START, out=os.path.join(OUT, "screen-b3"))
emu.write_png(os.path.join(OUT, "f1-screen-billing3.png"), r3.rgb, 2)
want3 = with_hazards(with_rider(scene_ref.scene_bytes(3), r3), r3, B3)
got3 = r3.mem[0xA000:0xA000 + 8000]
bad3 = [i for i in range(320, 8000) if want3[i] != got3[i]]
holes = sum(1 for y in roads for x in range(12, 240) if r3.pixel(x, y) != YEL)
gw3 = sum(a for recs in scene_ref.load_scene(billing=3)["tracks"] for k, x, a, b in recs if k == 4)
check(f"billing 3 shows the gaps tagged for it (scene = reference; {gw3} px of kerb gone)", not bad3 and holes == gw3,
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
    f.write("1 1\n")
    n = 3
    while n < 6000:
        ln = rg.randint(1, 40)
        f.write(f"{n}-{n + ln - 1} " + " ".join(rg.sample(["jump", "accel", "accel", "stop", "pause", "quit"],
                                                          rg.randint(1, 2))) + "\n")
        n += ln + rg.randint(0, 60)
        if rg.random() < 0.15:              # F4: start, answer Y/N, type a letter
            f.write(f"{n} {rg.choice(['1', '1', 'N', 'Y', 'B'])}\n")
            n += 2
# the screen checks at the end of a random run want a step well into a ride
_rm = [parse(l) for l in ref_trace(rnd, 3000)]
RND_N = max(s_["step"] for i, s_ in enumerate(_rm) if i >= 40 and all(q["mode"] == PLAY for q in _rm[i - 40:i + 1]))
scripts = [(os.path.join(KEYS, s), n) for s, n in (("basic.keys", 400), ("wrapjump.keys", 420),
                                                    ("pause.keys", 250), ("scene.keys", 800),
                                                    ("airstop.keys", 300), ("heldjump.keys", 260),
                                                    ("flow.keys", 1300), ("quit.keys", 200))] + [(rnd, 6000)]
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
    worst = max(worst, max(steady_busy(r)))
    total += n
for path, n in scripts[:2] + scripts[3:4]:
    r = emu.run(ETAP, frames=n + 1, keys=path, out=os.path.join(OUT, "trace-empty-" + os.path.basename(path)))
    check(f"test track {os.path.basename(path)}: Oric == native over {n} steps",
          r.trace.decode().splitlines() == ref_trace(path, n, True) and r.rc == 0)
check("step work under the 20,000-cycle step (riding and the title; a new card's first step runs long)",
      worst < STEP, f"worst {worst} cycles over {total} scripted steps")
S = os.path.join(B, "san")
os.makedirs(S, exist_ok=True)
san = subprocess.run(["cc", "-O1", "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover", "-I", "src",
                      "-I", "build/gen", "-o", os.path.join(S, "ref"), "tools/ref/ref.c", "src/model.c",
                      "src/trace.c", "src/scene.c"], cwd=ROOT, capture_output=True, text=True)
sr = subprocess.run([os.path.join(S, "ref"), rnd, "6000"], capture_output=True, text=True) if not san.returncode else None
check("native model clean under AddressSanitizer + UBSan (random.keys, 6000 steps)",
      sr is not None and sr.returncode == 0 and sr.stdout.splitlines() == ref_trace(rnd, 6000),
      (san.stderr or (sr.stderr if sr else ""))[-200:])

# 7. the model's rules on the native traces (DESIGN "Physics", placeholders)
def rules(lines):
    prev = None
    errs = []
    for l in lines:
        s = parse(l)
        if prev:
            restart = s["ev"] & 0x10 or s["ev2"] & RUNSTART
            if s["speed"] < prev["speed"] and not (s["ev"] & 0x04) and not restart:
                errs.append(f"speed fell at {s['step']}")
            if s["ev"] & 0x04 and not prev["flags"] & 1:
                errs.append(f"second stop at {s['step']}")
            if s["ev"] & 0x08 and not restart and prev["jt"] and s["jt"] != (prev["jt"] + 1) % 85:
                errs.append(f"wrap lost the jump at {s['step']}")
            if s["ev"] & 0x08 and not restart and s["speed"] < prev["speed"]:
                errs.append(f"wrap lost speed at {s['step']}")
        prev = s
    return errs
allr = [ref_trace(p, n, True) for p, n in scripts]
RUNSTART = 0x20     # ev2: a new run from the title
errs = sum((rules(t) for t in allr), [])
check("rules (test track): speed never falls but for the stop; one stop per scene; wrap keeps arc and speed",
      not errs, "; ".join(errs[:3]))
wj = [parse(l) for l in allr[1]]
wraps = [s for s in wj if s["ev"] & 0x08 and s["jt"]]
check("wrapjump.keys: a jump crosses onto the next track in the air", bool(wraps) and wraps[0]["track"] >= 1,
      f"track {wraps[0]['track'] + 1}, step {wraps[0]['step']}, height {wraps[0]['h']}" if wraps else "no wrap in a jump")
ast = [parse(l) for l in allr[4]]
st_ = next((s for s in ast if s["ev"] & 0x04), None)
after = [s for s in ast if st_ and st_["step"] <= s["step"] < st_["step"] + 90]
check("airstop.keys: the stop works in mid-air; the bike drops straight down, lands, one stop only",
      st_ is not None and st_["jt"] > 0 and all(s["x"] == st_["x"] and s["speed"] == 0 for s in after)
      and any(s["ev"] & 0x02 for s in after) and sum(1 for s in ast if s["ev"] & 0x04) == 1,
      f"stop at step {st_['step']}, jump step {st_['jt']}, height {st_['h']}" if st_ else "no stop")
hj = [parse(l) for l in allr[5]]
lands = [s["step"] for s in hj if s["ev"] & 0x02 and s["step"] < 252]
jumps = [s["step"] for s in hj if s["ev"] & 0x01]
gaps = [min(j - l for j in jumps if j > l) for l in lands if any(j > l for j in jumps)]
check("heldjump.keys: Space held jumps again 3 steps after each landing (Bob, BUILD 5)",
      len(jumps) >= 3 and gaps and all(g == 3 for g in gaps), f"jumps at {jumps}, landings at {lands}")
r = emu.run(ETAP, frames=200, keys=os.path.join(KEYS, "airstop.keys"), out=os.path.join(OUT, "lamp"))
r0 = emu.run(ETAP, frames=60, keys=os.path.join(KEYS, "airstop.keys"), out=os.path.join(OUT, "lamp0"))
check("stop lamp: lit before the stop, out after it (Bob, BUILD 4)",
      r0.mem[0xBF68 + 19] == ord("*") and r.mem[0xBF68 + 19] == ord("+"),
      f"{chr(r0.mem[0xBF68 + 19])} then {chr(r.mem[0xBF68 + 19])}")
# F2: the measured feel (docs/M1-MEASUREMENTS.md "In Oric units")
fe = [dict(parse(l), step=parse(l)["step"] - 2) for l in ref_trace(os.path.join(KEYS, "feel.keys"), 402, True)][2:]
sp = {s["step"]: s["speed"] for s in fe}
air = [s for s in fe if s["jt"]]
travel = sum(250 for s in air)                  # x grows by speed each airborne step
lap = [s["step"] for s in fe if s["ev"] & 0x08]
check("feel: standing start, +1 a step to top speed 250 in 250 steps (5.0 s), then it holds",
      fe[0]["speed"] == 1 and sp[249] == 249 and sp[250] == 250 and sp[300] == 250 and sp[399] == 250,
      f"speed {fe[0]['speed']} at step 1, {sp[250]} at 250, {sp[399]} at 399")
check("feel: jump 84 steps (1.68 s) in the air, apex 20 px, held 6 BBC steps (18 of ours)",
      len(air) == 84 and max(s["h"] for s in air) == 20 and sum(1 for s in air if s["h"] == 20) == 18,
      f"{len(air)} steps, apex {max(s['h'] for s in air) if air else 0}")
check("feel: a track in 1.54 s at top speed; a top-speed jump carries 82 BBC bytes (234 px, past a track)",
      any(b - a in (76, 77) for a, b in zip(lap, lap[1:]) if a > 260) and travel * 228 // (80 * 256) == 233,
      f"wraps {lap[-3:]}, jump {travel / 256:.1f} bytes = {travel * 228 / 80 / 256:.0f} px")
r = emu.run(ETAP, frames=300, keys=os.path.join(KEYS, "feel.keys"), out=os.path.join(OUT, "engine"))
per = {}
for _, reg, v in r.ay:
    per[reg] = v
vols = {v for _, reg, v in r.ay[-40:] if reg == 8}
check("engine: tone A period 1,250 - 4 x speed (50 Hz standing, 250 Hz flat out), buzzing volume",
      per.get(0, 0) | per.get(1, 0) << 8 == 250 and vols == {10, 7},
      f"period {per.get(0, 0) | per.get(1, 0) << 8}, volumes {sorted(vols)}")
# Bob on BUILD 7: take-off, landing and the stop must sound different
r = emu.run(ETAP, frames=420, keys=os.path.join(KEYS, "airstop.keys"), out=os.path.join(OUT, "effects"))
tr = {parse(l)["step"]: parse(l)["ev"] for l in r.trace.decode().splitlines()}


def effect(ev):
    """(noise period or None, tone C periods) in the 150 ms after the first step with event ev"""
    st = next(k for k, v in sorted(tr.items()) if v & ev)
    t0 = r.marks[st - 1]
    w = [(reg, v) for c, reg, v in r.ay if t0 <= c < t0 + 150000]
    noise = next((v for reg, v in w if reg == 6), None)
    lo = [v for reg, v in w if reg == 4]
    hi = [v for reg, v in w if reg == 5]
    return noise, [a | b << 8 for a, b in zip(lo, hi)]


fx = {n: effect(e) for n, e in (("jump", 0x01), ("stop", 0x04), ("land", 0x02))}
j, st_, la = fx["jump"], fx["stop"], fx["land"]
check("effects: take-off a rising tone with no noise; landing a low thump; stop a high squeal and hiss",
      j[0] is None and j[1][-1] < j[1][0] and la[0] >= 10 and la[1][-1] > la[1][0]
      and st_[0] is not None and st_[0] <= 2 and max(st_[1]) < 80,
      "; ".join(f"{n}: noise {v[0]}, tone {v[1][0] if v[1] else '-'}->{v[1][-1] if v[1] else '-'}" for n, v in fx.items()))
sc = [parse(l) for l in allr[3]]
check("scene.keys: leaving track 4 finishes the scene", any(s["ev"] & 0x10 for s in sc))

# F3: the scene's hazards, crashes, takes and fees (DESIGN rules 4-5; Bob's
#     decisions: fee per track finished = 10 x the scene's number in the run,
#     paid again on a retake; one-take bonus; a take back every third scene)
def f3_rules(lines):
    prev, errs, crash_at, base = None, [], None, 0
    for l in lines:
        s = parse(l)
        if s["ev2"] & RUNSTART:             # billing b starts at scene (b - 1) x 12 + 1
            base = (s["billing"] - 1) * 12
        if prev:
            restart = s["ev"] & 0x10 or s["ev2"] & (0x04 | 0x08 | RUNSTART)
            if s["speed"] < prev["speed"] and not s["ev"] & (0x04 | 0x80) and not restart:
                errs.append(f"speed fell at {s['step']}")
            if s["ev"] & 0x80:
                crash_at = s["step"]
                if not (s["crash_t"] == 75 and s["speed"] == 0 and s["takes"] == prev["takes"] - 1):
                    errs.append(f"crash at {s['step']}: pause {s['crash_t']}, takes {prev['takes']}->{s['takes']}")
            elif prev["crash_t"]:
                if s["crash_t"] != prev["crash_t"] - 1 or (s["crash_t"] and (s["x"], s["track"]) != (prev["x"], prev["track"])):
                    errs.append(f"crash pause broken at {s['step']}")
                if not s["crash_t"]:
                    if s["step"] - crash_at != 75:
                        errs.append(f"retake {s['step'] - crash_at} steps after the crash")
                    if prev["takes"]:
                        ok = s["ev2"] & 0x04 and s["take"] == prev["take"] + 1 and s["fee"] == prev["fee"] \
                            and s["mode"] == CARD
                    else:
                        ok = s["ev2"] & 0x08 and s["mode"] == FIRED and s["fee"] == prev["fee"]
                    if not ok or (s["x"], s["track"], s["speed"]) != (0, 0, 0):
                        errs.append(f"bad retake at {s['step']}")
            if s["ev2"] & 0x01:
                sc_no = base + (prev["flags"] >> 4) + 1
                want = prev["fee"] + 10 * sc_no + (50 * sc_no if s["ev2"] & 0x02 else 0)
                if s["fee"] != want:
                    errs.append(f"fee {prev['fee']}->{s['fee']} at {s['step']}, want {want}")
                if s["ev2"] & 0x02 and (not s["ev"] & 0x10 or prev["take"] != 1):
                    errs.append(f"bonus without a first-take clear at {s['step']}")
            elif s["fee"] != prev["fee"] and not s["ev2"] & RUNSTART:
                errs.append(f"fee changed without a track at {s['step']}")
            if s["ev2"] & 0x10 and not (s["takes"] == prev["takes"] + 1 and (s["flags"] >> 4) % 3 == 0):
                errs.append(f"take given back wrongly at {s['step']}")
        prev = s
    return errs


rhr = {os.path.basename(p): ref_trace(p, n) for p, n in scripts}
rhr["clear.keys"] = ref_trace(os.path.join(KEYS, "clear.keys"), 452)
rhr["takeback.keys"] = ref_trace(os.path.join(KEYS, "takeback.keys"), 2000)
WIN_W = [8, 8, 6, 6, 5, 4]                   # tools/solve/wins.sh
WIN_SCENES = 12                             # tools/solve/wins.sh SCENES: films built x 2
for b in range(1, 7):
    rhr[f"win-b{b}.keys"] = ref_trace(os.path.join(KEYS, f"win-b{b}.keys"), 7800)
errs = sum((f3_rules(t) for t in rhr.values()), [])
evs = [parse(l) for t in rhr.values() for l in t]
check("F3 rules: crash = dead stop, a take lost, 1.5 s pause, then the clapper card for the retake from the "
      "start (or \"YOU'RE FIRED!\"); fee per track = 10 x scene; bonus only on a first-take clear",
      not errs and any(s["ev"] & 0x80 for s in evs) and any(s["ev2"] & 0x08 for s in evs),
      "; ".join(errs[:3]) or f"{sum(1 for s in evs if s['ev'] & 0x80)} crashes, "
      f"{sum(1 for s in evs if s['ev2'] & 0x08)} firings")
# Bob, BUILD 10: a crash only on a real touch.  An independent replay of the
# hazards (police car 244/256 px a step leftwards, wrapping in 0..228-w; ball
# 1 line every 3 steps, 0..25 and back) and the pictures' pixels from
# tools/mkart.py: every step crashes exactly when a bike pixel meets a hazard
# pixel (billing 1: no gaps).
import mkart  # noqa: E402


def pixels(rows, x0, y0):
    return {(x0 + c, y0 + r) for r, row in enumerate(rows) for c, b in enumerate(row) if b}


RUN_MUL = [4, 4, 5, 5, 6, 6, 7]             # runners' speed by billing, quarters (DESIGN "Billings")
DANGLE_HALF = [6, 6, 5, 5, 4, 4, 3]         # danglers: half steps a line
JET_PERIOD = [150, 150, 125, 125, 100, 100, 75]   # jets: steps a round, a third of it on (F6a)


def touch_errors(lines):
    """every riding step: a crash exactly when a rider pixel meets a hazard
    pixel, or his wheels' middle is over a gap on the road; the hazards
    replayed independently from DESIGN (runners 0.95 px a step at billing 1,
    or their own speed, x 1.25, 1.5, 1.75 up the billings, wrapping in
    0..228-w; flyers the same up in the air; danglers 1 line every 3, 2.5,
    2, 1.5 steps, 0..25 and back; jets on for the last third of their period)"""
    errs, prev, st, ctx = [], None, None, None
    seen = []

    def start():
        name, bill = run_scene(seen)
        sc = scene_ref.load_scene(name, bill)
        mv = moving(name, bill)
        # F6: a runner's own speed (b: 1/128 px a step, bit 7 leftwards; 0 =
        # M1's 122 leftwards); flyers at the default; jets round their period
        bs = {(t, k, x, a): b for t, recs in enumerate(sc["tracks"]) for k, x, a, b in recs}
        jp = JET_PERIOD[bill]
        h = {}
        for i, (t, k, x, a) in enumerate(mv):
            b = bs[(t, k, x, a)] if k == 2 else 0
            sp = ((b & 0x7F or 122) * RUN_MUL[bill]) >> 2
            h[i] = [x, 0, 0, DANGLE_HALF[bill], 0, sp, bool(b) and not b & 0x80]  # px, frac, drop, count, rising, speed, right
            if k == 6:
                h[i][3] = ((bs[(t, k, x, a)] & 15) * jp) >> 4
        return dict(sc=sc, mv=mv, bill=bill, r=DANGLE_HALF[bill], jp=jp, jon=jp // 3, h=h)
    for l in lines:
        seen.append(l)
        s_ = parse(l)
        if st is None:
            st = start()
        moved = bool(prev) and prev["mode"] == PLAY and not prev["flags"] & 2 and not s_["ev"] & 0x60
        if moved:
            for i, (t, k, x, a) in enumerate(st["mv"]):
                h = st["h"][i]
                if k in (2, 8):             # the fraction borrows (left) or carries (right) a px
                    mx = 228 - mkart.scenery(mkart.SCENERY[a][0])[1]
                    sp = h[5]
                    f, n = h[1] + (sp & 0x7F if h[6] else -(sp & 0x7F)), sp >> 7
                    if f < 0 or f > 127:
                        f, n = f % 128, n + 1
                    h[1] = f
                    h[0] = (h[0] + (n if h[6] else -n)) % mx
                elif k == 6:                # a jet: round its period
                    h[3] = (h[3] + 1) % st["jp"]
                else:
                    if h[3] > 2:
                        h[3] -= 2
                    else:
                        if h[4]:
                            h[2] -= 1
                            if h[2] == 0:
                                h[4] = 0
                        else:
                            h[2] += 1
                            if h[2] == 25:
                                h[4] = 1
                        h[3] = h[3] - 2 + st["r"]
        if s_["ev"] & 0x10 or s_["ev2"] & (0x04 | 0x08 | RUNSTART):
            st = start()
        elif moved:
            px_ = scene_ref.px(s_["x"])
            frame = "jump" if s_["jt"] else ("ride_b" if px_ & 4 else "ride_a")
            bike = pixels(mkart.bits(dict(dict(mkart.VEHICLES)[st["sc"]["vehicle"]])[frame]), px_, 28 - s_["h"])
            hit = False
            for k, x, a, b in st["sc"]["tracks"][s_["track"]]:
                if k in (1, 2, 3, 5, 6, 8):
                    rows, w, hh = mkart.scenery(mkart.SCENERY[a][0])
                    if k == 6:              # a jet hits only when on
                        i = st["mv"].index((s_["track"], k, x, a))
                        if st["h"][i][3] < st["jp"] - st["jon"]:
                            continue
                        y = 44 - hh
                    elif k in (2, 8):
                        i = st["mv"].index((s_["track"], k, x, a))
                        x, y = st["h"][i][0], (44 - hh if k == 2 else b + 1 - hh)
                    elif k == 3:
                        i = st["mv"].index((s_["track"], k, x, a))
                        y = 6 + st["h"][i][2]
                    else:
                        y = 44 - hh if k == 1 else b + 1 - hh
                    hit |= bool(bike & pixels(rows, x, y))
                elif k == 4 and not s_["jt"]:
                    hit |= x <= px_ + 6 < x + a
            if hit != bool(s_["ev"] & 0x80):
                errs.append(f"step {s_['step']}: touch {hit}, crash {bool(s_['ev'] & 0x80)}")
        prev = s_
    return errs


errs = sum((touch_errors(t) for t in rhr.values()), [])
# F6: src/model.c makes each scene's crash masks into HIT_MAX columns
hit_cols = {}
for name_ in re.findall(r"const struct scene (scene_\w+) =", open(os.path.join(ROOT, "src", "scene.c")).read()):
    arts_ = {a for b_ in range(1, 7) for recs in scene_ref.load_scene(name_, b_)["tracks"] for k, x, a, b in recs if k != 4}
    hit_cols[name_] = sum(mkart.scenery(mkart.SCENERY[a][0])[1] for a in arts_)
HIT_MAX = int(re.search(r"#define HIT_MAX (\d+)", open(os.path.join(ROOT, "src", "model.c")).read()).group(1))
check(f"every scene's pictures fit the crash masks' {HIT_MAX} columns (src/model.c HIT_MAX)",
      max(hit_cols.values()) <= HIT_MAX, f"widest {max(hit_cols, key=hit_cols.get)}: {max(hit_cols.values())}")
check("crashes only on a real touch: a rider pixel on a hazard pixel or the wheels over a gap, every step of "
      "every trace, both scenes, all billings (Bob, BUILD 10)",
      not errs, "; ".join(errs[:3]))
cl = [parse(l) for l in rhr["clear.keys"]]
end = next((s for s in cl if s["ev"] & 0x10), None)
check("clear.keys: scene 1 ridden clean flat out (3 jumps): 4 tracks x $10 + one-take bonus $50",
      end is not None and end["fee"] == 90 and end["ev2"] & 0x02 and not any(s["ev"] & 0x80 for s in cl if s["step"] <= end["step"]),
      f"cleared at step {end['step']}, fee ${end['fee']}" if end else "not cleared")
tb = [parse(l) for l in rhr["takeback.keys"]]
gb = next((s for s in tb if s["ev2"] & 0x10), None)
check("takeback.keys: a crash, then three scenes cleared: the lost take comes back; fees 40 + 220 + 270",
      gb is not None and gb["takes"] == 3 and gb["fee"] == 40 + 80 + 100 + 120 + 150,
      f"step {gb['step']}, takes {gb['takes']}, fee ${gb['fee']}" if gb else "no take given back")
# F5: film 1 proved beatable at every billing (DESIGN "Billings"): each
# win-bN.keys (searched by tools/solve, every jump with a window of WIN_W
# steps to press it in) starts billing N from the title and clears every
# built scene (F6f: the whole season, films 1 to 6) on its first take;
# the Oric plays it the same
wins = []
for b in range(1, 7):
    w = [parse(l) for l in rhr[f"win-b{b}.keys"]]
    clears = [s for s in w if s["ev"] & 0x10]
    ok = len(clears) >= WIN_SCENES and all(s["ev2"] & 0x02 for s in clears[:WIN_SCENES]) \
        and clears[WIN_SCENES - 1]["billing"] == b \
        and not any(s["ev"] & 0x80 for s in w if s["step"] <= clears[WIN_SCENES - 1]["step"])
    wins.append(f"B{b}: {'cleared' if ok else 'FAILED'}"
                + (f" by step {clears[WIN_SCENES - 1]['step']}" if len(clears) >= WIN_SCENES else ""))
    check(f"win-b{b}.keys: billing {b}, films 1-{WIN_SCENES // 2} ({WIN_SCENES} scenes) cleared on the first take "
          f"(every jump has {WIN_W[b - 1]} steps to press it in)", ok, wins[-1])
# F6b: the Western's two scenes on the Oric, mid-ride at billing 6 (the
# horse on the dusty street, the mine cart on the rails) = the reference
# F6c: and the horror's (the hearse-trike on the cobbles, the ghost train)
voice_runs = {}
for fr, what in ((1700, "Dust at Dawn scene 1 (horse, dirt street)"), (2200, "Dust at Dawn scene 2 (mine cart, rails)"),
                 (2800, "Night of the Hearse scene 1 (hearse-trike, cobbles)"),
                 (3400, "Night of the Hearse scene 2 (ghost-train car)"),
                 (4100, "Moonbase Panic scene 1 (hover bike, craters)"),
                 (4750, "Moonbase Panic scene 2 (hover bike, reactor corridor)"),
                 (5250, "Dragon's Keep scene 1 (dragon hatchling, grass)"),
                 (5900, "Dragon's Keep scene 2 (magic carpet, the moat)"),
                 (6600, "Cold Front scene 1 (skis, the glacier)"),
                 (7250, "Cold Front scene 2 (snowmobile, Avalanche Pass)")):
    r = emu.run(TAP, frames=fr, keys=os.path.join(KEYS, "win-b6.keys"), out=os.path.join(OUT, f"f6-{fr}"))
    emu.write_png(os.path.join(OUT, f"f6-{fr}.png"), r.rgb, 2)
    voice_runs[fr] = [(reg, v) for c, reg, v in r.ay if c >= r.marks[fr - 41]]
    want = reference(r)
    got = r.mem[0xA000:0xA000 + 8000]
    bad = [i for i in range(320, 8000) if want[i] != got[i]]
    check(f"win-b6.keys at step {fr}: {what} = scene + rider + moving hazards", not bad,
          f"{scene_of(r)[0]}: {len(bad)} wrong bytes" + (f", first line {bad[0] // 40} cell {bad[0] % 40}" if bad else ""))
# F6g: each vehicle's engine voice (src/sound.s voices): its volume pattern,
# a vibrato on the period for the ghost train, the hover bike and the carpet,
# the skis on noise B; the period x eng_mul / 8 (src/main.c)
VOICES = {1700: ("horse", {11, 4, 0, 9, 3}, 0, 8), 2200: ("mine cart", {12, 7}, 0, 14),
          2800: ("hearse-trike", {11, 6, 8, 10}, 0, 11), 3400: ("ghost train", {9, 8}, 1, 16),
          4100: ("hover bike", {9}, 1, 6), 4750: ("hover bike", {9}, 1, 6),
          5250: ("dragon", {10, 0, 13}, 0, 7), 5900: ("magic carpet", {7}, 1, 9),
          6600: ("skis", set(), 2, 8), 7250: ("snowmobile", {12, 5}, 0, 6)}
vbad = []
for fr, (name, vols, kind, mul) in VOICES.items():
    w = voice_runs[fr]
    got = {v for reg, v in w if reg == 8}
    lo = [v for reg, v in w if reg == 0]
    hi = [v for reg, v in w if reg == 1]
    pers = {a | b << 8 for a, b in zip(lo, hi)}
    swish = [v for reg, v in w if reg == 9 and v]
    # (a steady volume is written once, before the window; the period moves
    # with the speed too, so a vibrato shows as many periods)
    ok = got <= vols if kind != 2 else (got <= {0} and set(swish) & {5, 6, 7})
    if kind == 1:
        ok = ok and len(pers) >= 3
    if pers:
        ok = ok and (min(pers) + 16) * 8 // mul >= 249 and max(pers) * 8 // mul <= 1270
    if not ok:
        vbad.append(f"{fr} {name}: volumes {sorted(got)}, periods {sorted(pers)[:6]}, noise {sorted(set(swish))}")
check("F6g engine voices: each vehicle's volume pattern, vibrato (ghost train, hover bike, carpet), "
      "the skis' swish on noise, the pitch scaled per vehicle", not vbad, "; ".join(vbad))
# hazards in the game: the police car drives left 1 px a step (122/128) and
# wraps; the ball bobs 1 line every 3 steps, 0..25 below its top
syms = emu.symbols(TAP)
r = emu.run(TAP, frames=400, keys=os.path.join(KEYS, "clear.keys"), out=os.path.join(OUT, "f3-clear"))
emu.write_png(os.path.join(OUT, "f3-screen.png"), r.rgb, 2)
check("clear.keys on the Oric: same trace as the native model",
      r.trace.decode().splitlines() == rhr["clear.keys"][:399])
pos, hz = drawn_pos(r), list(r.mem[syms["_hz_px"]:syms["_hz_px"] + 4 * NSLOT])
dr = list(r.mem[syms["_hz_drop"]:syms["_hz_drop"] + 4 * NSLOT])
model_pos = [hz[t * NSLOT + j] if k == 2 else dr[t * NSLOT + j] for t, k, x, a in MOVING
             for j in [next(j for j, rec in enumerate(scene_ref.load_scene()["tracks"][t]) if rec[:2] == (k, x))]]
check("moving hazards drawn where the game has them (a step behind at most)",
      all(abs(p - q) <= (2 if k == 2 else 1) for p, q, (t, k, x, a) in zip(pos, model_pos, MOVING)),
      f"drawn {pos}, game {model_pos}")
want = reference(r)
got = r.mem[0xA000:0xA000 + 8000]
bad = [i for i in range(320, 8000) if want[i] != got[i]]
check("clear.keys screen after 400 steps = scene + rider + police car + wrecking ball and rope", not bad,
      f"{len(bad)} wrong bytes" + (f", first line {bad[0] // 40} cell {bad[0] % 40}" if bad else ""))
r = emu.run(TAP, frames=150, keys=os.path.join(KEYS, "takeback.keys"), out=os.path.join(OUT, "f3-crash"))
emu.write_png(os.path.join(OUT, "f3-crash.png"), r.rgb, 2)
row0 = r.mem[0xBF68:0xBF68 + 40]
want = reference(r)
got = r.mem[0xA000:0xA000 + 8000]
bad = [i for i in range(320, 8000) if want[i] != got[i]]
check("crash: the crash picture, \"CUT!\" on the status line, a take icon gone, screen = reference",
      b"CUT!" in r.mem[0xBF68 + 80:0xBF68 + 120] + row0 and b"@@ " in row0 and not bad,
      f"{row0.decode(errors='replace')!r}, {len(bad)} wrong bytes")

# Bob, BUILD 13: the police lamps flash red and blue, swapping every 8 steps
lk = [emu.run(TAP, frames=n, keys=START, out=os.path.join(OUT, f"lamps{n}")) for n in (60, 68)]
li = [[r_.mem[0xA000 + y * 40] for y in lamp_lines()[0]] for r_ in lk]
emu.write_png(os.path.join(OUT, "f4-lamps.png"), lk[0].rgb, 2)
check("police lamps: the lamp line and the bar in red and blue, swapped 8 steps later",
      sorted(li[0]) == [1, 4] and li[1] == li[0][::-1], str(li))

# Bob, BUILD 15: an ink cell colours its whole line, so the lamps take the
# band's ink while the rider (neck and shoulders) is level with them
CARS = [t for t, k, x, a in MOVING if k == 2]
rl = emu.run(TAP, frames=1200, keys=os.path.join(KEYS, "season.keys"), out=os.path.join(OUT, "lampsr"))
lv = [s_ for s_ in map(parse, rl.trace.decode().splitlines())
      if s_["mode"] == PLAY and s_["track"] == CARS[0] and s_["h"] <= 11 and s_["step"] > 10]
rl = emu.run(TAP, frames=lv[5]["step"], keys=os.path.join(KEYS, "season.keys"), out=os.path.join(OUT, "lampsr"))
s_ = parse(rl.trace.decode().splitlines()[-1])
y0, y1 = lamp_lines()[0]
li = [rl.mem[0xA000 + y * 40] for y in (y0, y1, y1 + 1)]
emu.write_png(os.path.join(OUT, "f4-lamps-level.png"), rl.rgb, 2)
check("police lamps: the band's ink while the rider is level with them (Bob, BUILD 15)",
      s_["track"] == CARS[0] and s_["h"] <= 11 and li[0] == li[1] == li[2] not in (1, 4),
      f"track {s_['track']} height {s_['h']} inks {li}")

# F4: the film frame (DESIGN "Presentation")
r = emu.run(TAP, frames=30, out=os.path.join(OUT, "f4-title"))
emu.write_png(os.path.join(OUT, "f4-title.png"), r.rgb, 2)
r2 = emu.run(TAP, frames=34, out=os.path.join(OUT, "f4-title2"))
bulb_rows = [0xA000 + 4 * 40 + 41, 0xA000 + 47 * 40 + 41]
tl = [parse(l) for l in r.trace.decode().splitlines()]
check("title screen first: red marquee, chasing lights (they move every 4 steps), the menu, no riding",
      all(s_["mode"] == TITLE for s_ in tl) and r.pixel(6, 10) == RED
      and r.mem[0xA000 + 5 * 40:0xA000 + 6 * 40] != r2.mem[0xA000 + 5 * 40:0xA000 + 6 * 40]
      and {r.mem[0xA000 + 5 * 40 + c] for c in range(3, 38, 2)} == {0x5E, 0x52}
      and max(r.frame_stats()[1][3:]) < STEP,
      f"modes {sorted({s_['mode'] for s_ in tl})}, slowest step {max(r.frame_stats()[1][3:])} cycles")


def text_at(r, col, y, s):
    """small text from our charset at $9800 (src/frame.c text())"""
    return all(r.mem[0xA000 + (y + row) * 40 + col + i] == 0x40 | r.mem[0x9800 + ord(c) * 8 + row]
               for i, c in enumerate(s) for row in range(7) if c != " ")


check("title: the menu and today's top five (the film crew's fees to beat)",
      text_at(r, 12, 108, "1  START") and text_at(r, 24, 119, "EXTRA") and text_at(r, 24, 130, "ON")
      and text_at(r, 16, 161, "DIR") and text_at(r, 21, 193, "$50"))
r_play = emu.run(TAP, frames=60, keys=START, out=os.path.join(OUT, "f4-rows"))
check("title: no status rows (Bob, BUILD 19); they come back for play",
      sorted(r.mem[0xBF68:0xBF68 + 120]).count(0x20) == 119   # (and the HIRES mode cell)
      and bytes(r_play.mem[0xBF68 + 1:0xBF68 + 5]) == b"TAKE")
ck = os.path.join(OUT, "card.keys")
open(ck, "w").write("1 1\n")
r = emu.run(TAP, frames=100, keys=ck, out=os.path.join(OUT, "f4-card"))
cl = [parse(l) for l in r.trace.decode().splitlines()]
play_at = next((s_["step"] for s_ in cl if s_["mode"] == PLAY), None)
snap_t = r.marks[1 + CARD_SNAP - 1]
clack = [v for c, reg, v in r.ay if reg == 6 and snap_t <= c < snap_t + 120000]
check("clapper card: 1 starts it; with no key it lasts 1.8 s (90 steps), the arm shuts at 1.1 s with a "
      "noise clack", play_at == 1 + 90 and clack == [2], f"play at step {play_at}, noise at the snap {clack}")
# F6g: the film's sting on the card (src/sound.s s_action): A and C from the
# semitone table, then silence on both when play starts
c0, c1 = 0, r.marks[play_at - 1]
sa = [v for c, reg, v in r.ay if reg == 0 and c0 <= c < c1 and v]   # (snd_init's 0s first)
sc = [v for c, reg, v in r.ay if reg == 4 and c0 <= c < c1 and v]
end = [(reg, v) for c, reg, v in r.ay if c >= c1 and reg in (8, 10)]
check("F6g sting: the action film's card plays its figure on A and C (E4+B4 twice, G4+D5, A4+E5), "
      "then stops for play", sa == [190, 190, 159, 142] and sc == [127, 127, 106, 95] and (8, 0) in end
      and (10, 0) in end, f"A {sa}, C {sc}")
r = emu.run(TAP, frames=1 + CARD_SNAP + 4, keys=ck, out=os.path.join(OUT, "f4-card-action"))
emu.write_png(os.path.join(OUT, "f4-card.png"), r.rgb, 2)
check("clapper card: film, scene 1, take 1, \"ACTION!\" after the snap",
      text_at(r, 5, 76, "FILM") and text_at(r, 5, 112, "SCENE") and text_at(r, 24, 112, "TAKE")
      and any(r.mem[0xA000 + y * 40 + c] != 0x40 for y in range(6, 20) for c in range(2, 40)))
fl = os.path.join(KEYS, "flow.keys")
ft = [parse(l) for l in ref_trace(fl, 1300)]
seq = [ft[0]["mode"]] + [b["mode"] for a_, b in zip(ft, ft[1:]) if a_["mode"] != b["mode"]]
want_seq = [CARD, PLAY, PRINT, CARD, PLAY, CUT, CARD, PLAY, CUT, CARD, PLAY, CUT, FIRED, NAME, TITLE]
cards = [b["take"] for a_, b in zip(ft, ft[1:]) if b["mode"] == CARD and a_["mode"] != CARD]
check("flow.keys: card, ride, \"THAT'S A PRINT!\", card, then three crashes: \"CUT!\" and the card with the "
      "next take each time, \"YOU'RE FIRED!\", initials, the title", seq == want_seq and cards == [1, 2, 3],
      f"modes {seq}, takes on the cards {cards}")
syms = emu.symbols(TAP)
shots = {}
for name, at in (("print", next(s_["step"] for s_ in ft if s_["mode"] == PRINT) + 5),
                 ("cut", next(s_["step"] for s_ in ft if s_["mode"] == CUT and s_["crash_t"] == CUT_SHOW - 5)),
                 ("fired", next(s_["step"] for s_ in ft if s_["mode"] == FIRED) + 5),
                 ("name", next(s_["step"] for s_ in ft if s_["mode"] == NAME) + 12), ("end", 1300)):
    shots[name] = emu.run(TAP, frames=at, keys=fl, out=os.path.join(OUT, "f4-" + name))
    emu.write_png(os.path.join(OUT, f"f4-{name}.png"), shots[name].rgb, 2)
rf = shots["end"]
check("flow.keys on the Oric: same trace as the native model", rf.trace.decode().splitlines() == ref_trace(fl, 1300)[:len(rf.trace.decode().splitlines())])
bf = [10 * (rf.mem[syms["_best_fee"] + 2 * i] | rf.mem[syms["_best_fee"] + 2 * i + 1] << 8) for i in range(5)]   # (F6: kept in £10s)
bn = bytes(rf.mem[syms["_best_name"]:syms["_best_name"] + 15]).decode()
check("top five: $90 goes in fifth with the initials typed, BOB; the title shows it",
      bf == [250, 200, 150, 100, 90] and bn == "DIRCAMSFXMUABOB" and text_at(rf, 16, 193, "BOB")
      and text_at(rf, 21, 193, "$90"), f"{bf} {bn}")
fm = {s_["step"]: s_["mode"] for s_ in ft}
check("initials (Bob, BUILD 17): a third letter doesn't end them; Del takes one back; Return finishes",
      fm[1103] == NAME and fm[1114] == NAME and fm[1117] == TITLE, f"modes {fm[1103]}, {fm[1114]}, {fm[1117]}")
check("popups over the scene (Bob, BUILD 12): \"CUT!\" with a big megaphone; \"THAT'S A PRINT!\" tallies $40 + $50 bonus; "
      "\"YOU'RE FIRED!\" shows the fee",
      text_at(shots["print"], 32, 90, "$40") and text_at(shots["print"], 32, 102, "$50")
      and all(any(shots["cut"].mem[0xA000 + y * 40 + c] != 0x40 for y in range(74, 98)) for c in range(11, 17))
      and text_at(shots["cut"], 2 + (38 - 16) // 2, 110, "RETAKE COMING UP")
      and text_at(shots["fired"], 16, 84, "FINAL FEE"))
rc_ = shots["fired"]
cut_t = rc_.marks[next(s_["step"] for s_ in ft if s_["mode"] == CUT and s_["crash_t"] == CUT_SHOW) - 1]
tones = []
for c, reg, v in rc_.ay:                   # the sweep moves each tone up a little
    note = 1 if 150 < v <= 170 else 2 if 200 < v <= 230 else None
    if cut_t <= c < cut_t + 400000 and reg == 4 and note and note not in tones:
        tones.append(note)
check("\"CUT!\": a two-note squawk on channel C as the megaphone pops up (falling: ~415 Hz, then ~310)",
      tones == [1, 2], str(tones))
sk = os.path.join(KEYS, "season.keys")
st_ = [parse(l) for l in ref_trace(sk, 7800)]
wrap = next((s_ for s_ in st_ if s_["mode"] == WRAP), None)
after = [s_ for s_ in st_ if wrap and s_["step"] > wrap["step"]]
fee13 = next((s_["fee"] - p_["fee"] for p_, s_ in zip(after, after[1:]) if s_["ev2"] & 1), None)
check("season.keys: twelve scenes cleared, \"THAT'S A WRAP!\", then the next billing up (2) at $130 a track",
      wrap is not None and (wrap["flags"] >> 4) == 12 and after and after[-1]["billing"] == 2 and fee13 == 130
      and not any(s_["ev"] & 0x80 for s_ in st_),
      f"wrap at step {wrap['step'] if wrap else None}, then billing {after[-1]['billing'] if after else None}, "
      f"track fee {fee13}")
if wrap:
    rw = emu.run(TAP, frames=wrap["step"] + 2, keys=sk, out=os.path.join(OUT, "f4-wrap"))   # (Space skips it next)
    emu.write_png(os.path.join(OUT, "f4-wrap.png"), rw.rgb, 2)
    check("season.keys on the Oric: same trace; the wrap card lists the six films",
          rw.trace.decode().splitlines() == ref_trace(sk, wrap["step"] + 1)
          and text_at(rw, 2 + (38 - 16) // 2, 64, "RUSH HOUR RUMBLE") and text_at(rw, 2 + (38 - 10) // 2, 124, "COLD FRONT"))
qk = os.path.join(KEYS, "quit.keys")
qt = [parse(l) for l in ref_trace(qk, 200)]
md = {s_["step"]: s_["mode"] for s_ in qt}
check("quit.keys: L twice = billing 3 ($250 a track); Esc asks, N rides on, Esc then Y = the title",
      md[99] == PLAY and md[100] == ASK and md[110] == PLAY and md[150] == ASK and md[160] == TITLE
      and qt[30]["billing"] == 3, f"modes {[md[k] for k in (99, 100, 110, 150, 160)]}, billing {qt[30]['billing']}")
ra = emu.run(TAP, frames=105, keys=qk, out=os.path.join(OUT, "f4-ask"))
emu.write_png(os.path.join(OUT, "f4-ask.png"), ra.rgb, 2)
rn = emu.run(TAP, frames=114, keys=qk, out=os.path.join(OUT, "f4-ask-n"))
want = reference(rn)
got = rn.mem[0xA000:0xA000 + 8000]
bad = [i for i in range(320, 8000) if want[i] != got[i]]
# (F6c: the popup saves into the vehicle's buffer and shifts the vehicle afresh after:
# the rider after N is drawn from it)
check("quit.keys: \"QUIT?\" pops up over the scene; after N the scene is back as it was",
      text_at(ra, 2 + (38 - 25) // 2, 105, "Y: THE TITLE   N: RIDE ON") and not bad, f"{len(bad)} wrong bytes")
pk = os.path.join(KEYS, "pause.keys")
rp = emu.run(TAP, frames=85, keys=pk, out=os.path.join(OUT, "f4-pause"))
emu.write_png(os.path.join(OUT, "f4-pause.png"), rp.rgb, 2)
check("pause: \"PAUSED\" pops up (its screen comes back on resuming: the rider checks on pause.keys)",
      text_at(rp, 2 + (38 - 15) // 2, 107, "ANY KEY GOES ON"))
# Bob, BUILD 12: no rider on the first start in Oricutron, whose RAM is not
# all 0 at load (the emulator's was).  crt0 clears zero page now.
junk_ok = []
for seed in (1, 2, 3):
    rj = emu.run(TAP, frames=50, keys=START, junk=seed, out=os.path.join(OUT, f"junk{seed}"))
    want = reference(rj)
    junk_ok.append(rj.mem[0xA000 + 320:0xA000 + 8000] == want[320:8000])
check("RAM full of junk at load (as on a booted Oric): the first scene and rider still = reference (Bob, BUILD 12)",
      all(junk_ok), str(junk_ok))
rq = emu.run(TAP, frames=200, keys=qk, out=os.path.join(OUT, "f4-quit"))
eng = [v for c, reg, v in rq.ay if reg == 8 and v and rq.marks[30] <= c < rq.marks[150]]
check("quit.keys: S turns the sound off (no engine while riding), S again on (the menu says ON)",
      not eng and text_at(rq, 24, 130, "ON") and rq.trace.decode().splitlines() == ref_trace(qk, 200)[:199],
      f"{len(eng)} engine volume writes")

# 8. Oricutron build: noise an octave lower (every R6 value doubled)
path = os.path.join(KEYS, "basic.keys")
rel = emu.run(TAP, frames=400, keys=path, out=os.path.join(OUT, "ay-release"))
ori = emu.run(OTAP, frames=400, keys=path, out=os.path.join(OUT, "ay-oricutron"))
r6r = [v for _, reg, v in rel.ay if reg == 6 and v]
r6o = [v for _, reg, v in ori.ay if reg == 6 and v]
check("ORICUTRON=1: every noise period doubled, all within 1-31",
      len(r6r) >= 2 and r6o == [2 * v for v in r6r] and max(r6o) <= 31, f"release {r6r}, Oricutron {r6o}")
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


def tears(r, ok_steps):
    """(steps in which some video frame showed part old, part new rider, drawing
    steps, slowest burst in cycles a line).  Each step's writes are split into
    bursts (gaps > 300 cycles: the two halves of a wrap); a frame f scans line y,
    cell c at f * VF + y * 64 + c (the emulator's video timing).  A frame is torn
    when it scans some of a burst's bytes before their write and some after.
    Colour cells (0-1: the police lamps' ink) are not pictures: left out."""
    steps = {}
    for a, v, c in r.wlog:
        if c >= r.marks[0] and bisect.bisect_right(r.marks, c) in ok_steps and (a - 0xA000) % 40 >= 2:
            steps.setdefault(bisect.bisect_right(r.marks, c), []).append((a - 0xA000, c))
    bad, pace = 0, 0
    for ws in steps.values():
        bursts, cur = [], [ws[0]]
        for w in ws[1:]:
            if w[1] - cur[-1][1] > 300 or abs(w[0] // 40 - cur[-1][0] // 40) > 2:
                bursts.append(cur)
                cur = []
            cur.append(w)
        bursts.append(cur)
        torn = False
        for bu in bursts:
            t0, t1 = bu[0][1], bu[-1][1]
            nl = len({o // 40 for o, _ in bu})
            if len({o % 40 for o, _ in bu}) > 1:    # (a rope's pixel or two: one cell, can't tear)
                pace = max(pace, (t1 - t0 - 30) / max(1, nl - 1))
            for f in range(t0 // VF - 1, t1 // VF + 1):
                d = [c - (f * VF + (o // 40) * 64 + o % 40) for o, c in bu]
                torn |= min(d) < 0 < max(d)
        bad += torn
    return bad, len(steps), pace


tj = os.path.join(KEYS, "topjumps.keys")
blank = scene_ref.scene_bytes(1, "scene_test")
all_torn = all_drawn = 0
# (F5: with the win runs' first scenes too, ridden on the empty track: the
# rate is a property of the drawing, and more riding steps measure it better;
# F6: the whole win runs, a season each: the tears come in clusters where the
# step's phase drifts past the beam, 32 cycles a frame, and a short sample
# can catch a cluster or two more or less as startup timing moves the phase)
rider_runs = [(tj, 1500)] + scripts[:5]
for b in range(1, 7):
    cl = [parse(l)["step"] for l in rhr[f"win-b{b}.keys"] if int(l.split()[7], 16) & 0x10]
    rider_runs.append((os.path.join(KEYS, f"win-b{b}.keys"), cl[WIN_SCENES - 1] - 30))
for path, n in rider_runs:
    r = emu.run(ETAP, frames=n, keys=path, wlog=True, out=os.path.join(OUT, "rider-" + os.path.basename(path)))
    ok_steps = steady(r.trace.decode().splitlines())
    last, twice = {}, 0
    for a, v, c in r.wlog:
        if c < r.marks[0]:
            continue
        st = bisect.bisect_right(r.marks, c)
        twice += a in last and last[a] == st and st in ok_steps
        last[a] = st
    torn, drawn, pace = tears(r, ok_steps)
    s_ = parse(r.trace.decode().splitlines()[-1])
    x = scene_ref.px(s_["x"])
    want = bytearray(blank)
    for a, v in scene_ref.rider_bytes(s_["track"], x, s_["h"], scene_ref.frame_for(x, s_["jt"])).items():
        want[a - 0xA000] ^= v
    got = r.mem[0xA000:0xA000 + 8000]
    bad = [i for i in range(320, 8000) if want[i] != got[i]]
    all_torn, all_drawn = all_torn + torn, all_drawn + drawn
    check(f"rider {os.path.basename(path)}: one write per byte per step; screen = scene + rider",
          twice == 0 and not bad and pace < 70,   # 67, or 68 when the loop branch crosses a page
          f"{twice} double writes, {torn} torn of {drawn} drawing steps, burst {pace:.0f} cycles a line, "
          f"{len(bad)} wrong bytes")
# A 67-cycle burst can still be caught by a beam less than 45 cycles above the
# box (it gains 3 cycles a line): ~0.2% of draws, in clusters, because the
# 20,000-cycle step drifts only 32 cycles a video frame.  So the rate is taken
# over all the scripts together (BUILD 9: one short script alone had 1 in 37).
check("rider: under 1% torn frames over all the scripts", all_torn * 100 < all_drawn,
      f"{all_torn} torn of {all_drawn} drawing steps")

# F3: the moving hazards drawn with the rider in the full scene (the police
#     car's 6-cell box through the 67-cycle template, src/draw.s burst6)
# (torn frames counted over the four together, as for the rider alone: F4's
# cards move the steps' phase against the video, and with it where the
# clusters fall; BUILD 13: the three short scripts alone read 1.3%, so the
# long season run is in the sample too)
# F5: the win runs too, each stopped 30 steps before it clears its last scene
# (F6b-f: the whole season, Avalanche Pass last)
sc_torn = sc_draw = 0
runs = [(os.path.join(KEYS, "clear.keys"), 430), (os.path.join(KEYS, "takeback.keys"), 1950), (rnd, RND_N),
        (os.path.join(KEYS, "season.keys"), 7700)]
for b in range(1, 7):
    cl = [parse(l)["step"] for l in rhr[f"win-b{b}.keys"] if int(l.split()[7], 16) & 0x10]
    runs.append((os.path.join(KEYS, f"win-b{b}.keys"), cl[WIN_SCENES - 1] - 30))
for path, n in runs:
    r = emu.run(TAP, frames=n, keys=path, wlog=True, out=os.path.join(OUT, "scene-" + os.path.basename(path)))
    torn, ndraw, pace = tears(r, steady(r.trace.decode().splitlines()))
    sc_torn, sc_draw = sc_torn + torn, sc_draw + ndraw
    if os.path.basename(path).startswith("win-"):
        emu.write_png(os.path.join(OUT, "f5-" + os.path.basename(path)[:-5] + ".png"), r.rgb, 2)
        check(f"{os.path.basename(path)} on the Oric: the same trace as the native model",
              r.trace.decode().splitlines() == rhr[os.path.basename(path)][:n - 1])
    want = reference(r)
    got = r.mem[0xA000:0xA000 + 8000]
    bad = [i for i in range(320, 8000) if want[i] != got[i]]
    check(f"scene {os.path.basename(path)}: one pass at the beam's pace; screen = reference",
          pace < 70 and not bad, f"{torn} torn of {ndraw} drawing steps, slowest burst "
          f"{pace:.0f} cycles a line, {len(bad)} wrong bytes"
          + (f", first line {bad[0] // 40} cell {bad[0] % 40}" if bad else ""))
# Bob on BUILD 17: at Legend the police car "has a break and stretches almost
# double": its draws waited, then one far move left the old picture up for a
# step.  BUILD 18: a lagging car catches up 6 px a draw, so outside a wrap
# (33 cells) no runner ever has a second picture.  Sampled every 5th step.
ghosts = []
k6 = os.path.join(KEYS, "win-b6.keys")
for n in range(20, len(rhr["win-b6.keys"]), 5):
    r = emu.run(TAP, frames=n, keys=k6, out=os.path.join(OUT, "ghost"))
    if parse(r.trace.decode().splitlines()[-1])["mode"] != PLAY:
        continue
    for i, (t, k, x, a) in enumerate(moving(*scene_of(r))):
        g = 1 + i + 8
        if k == 2 and r.mem[syms["_sp_on"] + g] and abs(r.mem[syms["_sp_c"] + g] - r.mem[syms["_sp_c"] + 1 + i]) < 30:
            ghosts.append(n)
check("win-b6.keys: no runner drawn twice but across a wrap (Bob, BUILD 17)", not ghosts,
      f"steps {ghosts[:8]}" if ghosts else "")
check(f"scene: rider and hazards under 1% torn frames over the {len(runs)} scripts", sc_torn * 100 < sc_draw,
      f"{sc_torn} torn of {sc_draw} drawing steps")

# F6a: the stress scene (src/scene.c scene_stress, DESIGN "Step budget"): two
# hazards that move every step (the police car, a bat) and three jets, won at
# billing 6 by tools/solve (tests/keys/stress-b6.keys).  The Oric plays it as
# the model does; few steps run long (the screen wraps, as in film 1); the
# runner and flyer are drawn within 12 px of the game's place but at a wrap;
# the screen matches the reference every 50th step.
STAP = os.path.join(B, "stress", "stuntbob.tap")
sk = os.path.join(KEYS, "stress-b6.keys")
scene_ref.ONLY = "scene_stress"
sref = subprocess.run([REF, sk, "1200", "stress"], capture_output=True, check=True).stdout.decode().splitlines()
r = emu.run(STAP, frames=1200, keys=sk, out=os.path.join(OUT, "stress"))
emu.write_png(os.path.join(OUT, "f6a-stress.png"), r.rgb, 2)
sl = [parse(l) for l in sref]
sclear = [q for q in sl if q["ev"] & 0x10]
sb = steady_busy(r)
check("stress-b6.keys: billing 6, both stress scenes cleared on the first take; the Oric's trace = the model's",
      len(sclear) >= 2 and not any(q["ev"] & 0x80 for q in sl) and r.trace.decode().splitlines() == sref[:1199])
check("stress scene: under 2% of riding steps over 20,000 cycles", sum(x > STEP for x in sb) * 50 < len(sb),
      f"{sum(x > STEP for x in sb)} of {len(sb)}, worst {max(sb)}")
ssy = emu.symbols(STAP)
srecs = scene_ref.load_scene("scene_stress")["tracks"]
sslots = [t * NSLOT + j for t, recs in enumerate(srecs) for j, rec in enumerate(recs) if rec[0] in (2, 3, 6, 8)]
lagw, sbad = [], []
for n in range(40, 1200, 10):
    r = emu.run(STAP, frames=n, keys=sk, out=os.path.join(OUT, "stress-n"))
    if parse(r.trace.decode().splitlines()[-1])["mode"] != PLAY:
        continue
    pos = drawn_pos(r, STAP, "scene_stress")
    hz = r.mem[ssy["_hz_px"]:ssy["_hz_px"] + 4 * NSLOT]
    for i, (t, k, x, a) in enumerate(moving("scene_stress", 6)):
        if k in (2, 8) and r.mem[ssy["_sp_on"] + 1 + i] and parse(r.trace.decode().splitlines()[-4])["mode"] == PLAY:
            d = abs(pos[i] - hz[sslots[i]])
            if 12 < d < 150:
                lagw.append((n, k, d))
    if n % 50 == 0:
        want = with_hazards(with_rider(scene_ref.scene_bytes(6, "scene_stress"), r), r, STAP)
        got = r.mem[0xA000:0xA000 + 8000]
        if any(want[i] != got[i] for i in range(320, 8000)):
            sbad.append(n)
check("stress scene: the police car and the bat drawn within 12 px of the game's place (but at a wrap)", not lagw,
      str(lagw[:6]))
check("stress scene: screen = reference (scene, rider, runner, flyer, jets and their vents)", not sbad, str(sbad))
scene_ref.ONLY = None

print(f"\n{'ALL PASS' if not fails else f'{fails} FAILED'}")
sys.exit(1 if fails else 0)
