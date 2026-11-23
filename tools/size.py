#!/usr/bin/env python3
"""size.py - size report from an ld65 map file (Makefile runs it after every link)."""
import re, sys

PROGTOP = 0x9800

def main(mapfile, stack=0x200):
    txt = open(mapfile).read()
    segs = {}
    seglist = txt.split("Segment list:")[1]
    for m in re.finditer(r"^(\w+)\s+([0-9A-F]{6})\s+([0-9A-F]{6})\s+([0-9A-F]{6})", seglist, re.M):
        segs[m.group(1)] = (int(m.group(2), 16), int(m.group(3), 16), int(m.group(4), 16))
    # crt0 budget: STARTUP segment plus the zerobss module it pulls in
    startup = segs.get("STARTUP", (0, 0, 0))[2]
    zb = re.search(r"none\.lib\(zerobss\.o\):\s*\n\s*CODE\s+Offs=\w+\s+Size=([0-9A-F]+)", txt)
    zb = int(zb.group(1), 16) if zb else 0
    code = sum(segs.get(s, (0, 0, 0))[2] for s in ("STARTUP", "LOWCODE", "ONCE", "CODE"))
    ro, data, bss = (segs.get(s, (0, 0, 0))[2] for s in ("RODATA", "DATA", "BSS"))
    top = max(end for (start, end, size) in segs.values() if start >= 0x0500) + 1
    free = PROGTOP - stack - top
    name = mapfile.rsplit("/", 1)[-1].replace(".map", "")
    print(f"{name}: code {code} rodata {ro} data {data} bss {bss} | "
          f"end ${top:04X}, C stack ${stack:X}, free below ${PROGTOP:04X}: {free} bytes | "
          f"crt0 {startup}+zerobss {zb} = {startup + zb} B")

if __name__ == "__main__":
    main(sys.argv[1])
