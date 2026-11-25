#!/usr/bin/env python3
"""run.py - drive the headless emulator (docs/PLAN.md F0, tools/README.md; from Oric Chuckie Egg).

  run.py PROG.tap [--frames N] [--clock mark|video] [--keys SCRIPT] [--keymap k=K,...]
         [--png OUT.png] [--scale N] [--dump ADDR:LEN ...] [--cycles]
         [--trace OUT.txt] [--ay OUT.txt] [--out DIR] [--rom atmos|oric1|none|FILE]

--rom (F3): which ROM's IRQ vector sits at $FFFE: atmos (default, ROM 1.1: $0244),
oric1 (ROM 1.0: $0228), none (0: an IRQ stops the run), or a number.  Read from
Oricutron's basic11b.rom / basic10.rom; the ROM's code is never executed (D1).

--frames counts MARK writes to the debug port ($BFE1) with --clock mark (default:
one per game frame), or 20 ms video frames with --clock video.  ADDR may be a
number or a symbol from the program's .lbl file (e.g. _keys).
"""
import argparse, os, struct, subprocess, sys, tempfile, zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import keyscript  # noqa: E402

EMU = os.path.join(ROOT, "build", "emu", "emu")
IRQVEC = {"oric1": 0x0228, "atmos": 0x0244}    # $FFFE/$FFFF of basic10.rom / basic11b.rom
W, H = 240, 224


def build_emu():
    r = subprocess.run(["make", "-s", "-C", ROOT, "build/emu/emu"], capture_output=True, text=True)
    if r.returncode:
        sys.exit("run.py: building the emulator failed:\n" + r.stdout + r.stderr)


def write_png(path, rgb, scale=1):
    rows = []
    for y in range(H):
        line = rgb[y * W * 3:(y + 1) * W * 3]
        if scale > 1:
            line = b"".join(line[i:i + 3] * scale for i in range(0, len(line), 3))
        rows.extend([b"\0" + line] * scale)
    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", W * scale, H * scale, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(b"".join(rows), 9)) + chunk(b"IEND", b"")
    open(path, "wb").write(png)


def trace_text(raw):
    """Oric-side D5 trace -> reference order.  The Oric writes each frame's calls
    before its state, as "<calls> US <state> LF" (src/raster_trace.c); this gives
    "<state> |<calls> LF" as tools/ref writes it.  A trace without US is returned
    unchanged; calls after the last state line (the ref never prints them) are dropped."""
    if b"\x1f" not in raw:
        return raw
    out = []
    for rec in raw.split(b"\n")[:-1]:
        calls, _, state = rec.rpartition(b"\x1f")
        out.append(state + b" |" + calls + b"\n")
    return b"".join(out)


def symbols(prog):
    lbl = os.path.splitext(prog.split("@")[0])[0] + ".lbl"
    syms = {}
    if os.path.exists(lbl):
        for line in open(lbl):
            p = line.split()
            if len(p) == 3 and p[0] == "al":
                syms[p[2].lstrip(".")] = int(p[1], 16)
    return syms


class Result:
    """Everything one emulator run produced."""
    def __init__(self, out, summary, rc, err):
        self.out, self.summary, self.rc, self.err = out, summary, rc, err
        self.mem = open(os.path.join(out, "mem.bin"), "rb").read() if rc != 2 or os.path.exists(os.path.join(out, "mem.bin")) else b""
        self.rgb = open(os.path.join(out, "screen.rgb"), "rb").read()
        self.trace = open(os.path.join(out, "trace.txt"), "rb").read()
        self.ay = [tuple(map(int, l.split())) for l in open(os.path.join(out, "ay.txt"))]
        irqf = os.path.join(out, "irq.txt")      # F3: (cycle, cost) per IRQ taken
        self.irq = [tuple(map(int, l.split())) for l in open(irqf)] if os.path.exists(irqf) else []
        tf = os.path.join(out, "tcycles.txt")    # F4a: --tcycles
        self.tcycles = [tuple(map(int, l.split())) for l in open(tf)] if os.path.exists(tf) else []
        wf = os.path.join(out, "wlog.txt")      # --wlog
        self.wlog = [tuple(map(int, l.split())) for l in open(wf)] if os.path.exists(wf) else []
        self.marks, self.busy = [], []
        for l in open(os.path.join(out, "marks.txt")):
            k, c = l.split()
            (self.marks if k == "M" else self.busy).append(int(c))

    def pixel(self, x, y):
        i = (y * W + x) * 3
        return tuple(self.rgb[i:i + 3])

    def frame_stats(self):
        """(mark-to-mark deltas, busy cycles per frame) from MARK/BUSY writes."""
        d = [b - a for a, b in zip(self.marks, self.marks[1:])]
        busy = []
        for m in self.marks:
            nxt = [b for b in self.busy if b > m]
            if nxt:
                busy.append(nxt[0] - m)
        return d, busy


def run(prog, frames=100, clock="mark", keys=None, keymap=None, out=None, max_cycles=None, start=None,
        rom="atmos", stop_trace=None, tcycles=False, wlog=False, junk=None, prof=False):
    if not os.path.exists(EMU) or os.path.getmtime(EMU) < os.path.getmtime(os.path.join(ROOT, "tools/emu/emu.c")):
        build_emu()
    out = out or tempfile.mkdtemp(prefix="emu-")
    os.makedirs(out, exist_ok=True)
    cmd = [EMU, "--tap" if prog.endswith(".tap") else "--bin", prog, "--clock", clock,
           "--frames", str(frames), "--out", out]
    if start is not None:
        cmd += ["--start", str(start)]
    if rom and rom != "none":
        cmd += ["--irqvec", str(IRQVEC[rom] if rom in IRQVEC else int(str(rom), 0))]
    if max_cycles:
        cmd += ["--max-cycles", str(max_cycles)]
    if stop_trace:                  # F4a: (byte, n): stop at the n-th $BFE0 write of byte
        cmd += ["--stop-trace", f"{stop_trace[0]}:{stop_trace[1]}"]
    if tcycles:                     # F4a: Result.tcycles = [(byte, cycle)] of $BFE0 writes
        cmd += ["--tcycles"]
    if junk:                        # BUILD 13: RAM full of junk at load, as on a booted Oric
        cmd += ["--junk", str(junk)]
    if prof:                        # F6a: out/prof.bin = cycles spent at each PC (uint32 x 65536)
        cmd += ["--prof"]
    if wlog:                        # BUILD 2: Result.wlog = [(addr, value, cycle)] of HIRES writes
        cmd += ["--wlog"]
    if keys:
        kf = os.path.join(out, "keys.txt")
        with open(kf, "w") as f:
            for fr, rows in keyscript.oric_rows(keys, frames, keymap or keyscript.DEFAULT_KEYMAP):
                f.write(f"{fr} " + " ".join(f"{r:02x}" for r in rows) + "\n")
        cmd += ["--keys", kf]
    r = subprocess.run(cmd, capture_output=True, text=True)
    return Result(out, r.stdout.strip(), r.returncode, r.stderr.strip())


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("prog")
    ap.add_argument("--frames", type=int, default=100)
    ap.add_argument("--clock", choices=["mark", "video"], default="mark")
    ap.add_argument("--keys")
    ap.add_argument("--keymap", help="button=KEY,... e.g. jump=A,left=Z")
    ap.add_argument("--png"); ap.add_argument("--scale", type=int, default=2)
    ap.add_argument("--dump", action="append", default=[])
    ap.add_argument("--cycles", action="store_true")
    ap.add_argument("--trace"); ap.add_argument("--ay"); ap.add_argument("--out")
    ap.add_argument("--max-cycles", type=int)
    ap.add_argument("--rom", default="atmos")
    a = ap.parse_args()
    km = dict(keyscript.DEFAULT_KEYMAP)
    if a.keymap:
        km.update(kv.split("=", 1) for kv in a.keymap.split(","))
    r = run(a.prog, a.frames, a.clock, a.keys, km, a.out, a.max_cycles, rom=a.rom)
    print(r.summary)
    if r.err:
        print(r.err, file=sys.stderr)
    if a.png:
        write_png(a.png, r.rgb, a.scale)
    if a.trace:
        open(a.trace, "wb").write(trace_text(r.trace))
    if a.ay:
        with open(a.ay, "w") as f:
            f.writelines(f"{c} R{reg} {v}\n" for c, reg, v in r.ay)
    syms = symbols(a.prog)
    for d in a.dump:
        addr, _, ln = d.partition(":")
        addr = syms[addr] if addr in syms else int(addr, 0)
        ln = int(ln or "16", 0)
        for o in range(0, ln, 16):
            chunk = r.mem[addr + o:addr + min(o + 16, ln)]
            print(f"{addr + o:04X}: " + " ".join(f"{b:02X}" for b in chunk))
    if a.cycles:
        d, busy = r.frame_stats()
        if d:
            print(f"frame period: min {min(d)} max {max(d)} cycles over {len(d)} frames")
        if busy:
            print(f"frame work:   min {min(busy)} avg {sum(busy) // len(busy)} max {max(busy)} cycles "
                  f"(budget 20000: one 50 Hz step)")
        if r.irq:
            c = [k for _, k in r.irq]
            print(f"IRQ handler:  {len(c)} taken, min {min(c)} avg {sum(c) // len(c)} max {max(c)} cycles each")
    sys.exit(r.rc)


if __name__ == "__main__":
    main()
