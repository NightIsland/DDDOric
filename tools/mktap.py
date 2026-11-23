#!/usr/bin/env python3
"""mktap.py - wrap a raw binary in an autorunning Oric .tap (docs/ARCHITECTURE.md D8).

Header as written by the ROM's CSAVE and read by CLOAD"" on ROM 1.0 (Oric-1) and
1.1 (Atmos): sync $16 x4, $24, $00 $00, type ($80 = machine code), autorun ($C7),
end address (inclusive, hi/lo), start address (hi/lo), $00, name, $00, data.
With autorun set, both ROMs JMP to the start address after loading.

usage: mktap.py [--start 0x0500] [--name NAME] [--no-autorun] in.bin out.tap
"""
import argparse, sys

def make_tap(data, start, name, autorun=True):
    end = start + len(data) - 1
    if not data or end > 0xBFFF:
        sys.exit(f"mktap: bad size {len(data)} at ${start:04X}")
    hdr = bytes([0x16] * 4 + [0x24, 0, 0, 0x80, 0xC7 if autorun else 0,
                 end >> 8, end & 0xFF, start >> 8, start & 0xFF, 0])
    return hdr + name.upper().encode("ascii")[:16] + b"\0" + data

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--start", type=lambda s: int(s, 0), default=0x0500)
    ap.add_argument("--name", default="CHUCKIE")
    ap.add_argument("--no-autorun", action="store_true")
    ap.add_argument("inp"); ap.add_argument("out")
    a = ap.parse_args()
    data = open(a.inp, "rb").read()
    open(a.out, "wb").write(make_tap(data, a.start, a.name, not a.no_autorun))

if __name__ == "__main__":
    main()
