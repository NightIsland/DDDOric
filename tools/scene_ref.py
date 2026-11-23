#!/usr/bin/env python3
"""scene_ref.py - pixel references for the F1 display (docs/PLAN.md F1;
docs/ORIC-LESSONS.md section 6 "Pixel references").

Written from DESIGN and src/scene.h, not from screen.c's code: reads the scene
and palette data out of src/scene.c and the art out of tools/mkart.py, and
builds the HIRES bytes the Oric should hold (lines 8-199; the title strip's
text is checked separately)."""
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import mkart  # noqa: E402

COLOURS = dict(BLACK=0, RED=1, GREEN=2, YELLOW=3, BLUE=4, MAGENTA=5, CYAN=6, WHITE=7)
KINDS = dict(HZ_NONE=0, HZ_BLOCK=1, HZ_RUNNER=2, HZ_DANGLER=3, HZ_GAP=4, HZ_CEILING=5, HZ_JET=6, HZ_RAMP=7)
ART = {f"ART_{n.upper()}": i for i, (n, _) in enumerate(mkart.SCENERY)}
TITLE, BAND, RIDER_TOP, ROAD = 8, 48, 28, 44
FRAMES = [n for n, _ in mkart.RIDER_FRAMES]


def load_scene():
    src = open(os.path.join(ROOT, "src", "scene.c")).read()
    pal = src[src.index("pal_city = {"):src.index("};", src.index("pal_city = {"))]
    names = re.findall(r"\b(BLACK|RED|GREEN|YELLOW|BLUE|MAGENTA|CYAN|WHITE)\b", pal.split("{", 1)[1])
    cols = [COLOURS[n] for n in names]
    title_paper, title_ink, paper, ink = cols[0], cols[1], cols[2:50], cols[50:98]
    body = src[src.index("scene_rhr1 = {"):]
    tracks = []
    for tb in re.findall(r"\{((?:\s*\{[^{}]*\},?)+)\s*\}", body)[:4]:
        recs = []
        for rec in re.findall(r"\{([^{}]*)\}", tb):
            f = [v.strip() for v in rec.split(",")]
            val = lambda v: int(v) if v.isdigit() else {**KINDS, **ART}[v]  # noqa: E731
            recs.append(tuple(val(v) for v in f))
        tracks.append(recs)
    return dict(title_paper=title_paper, title_ink=title_ink, paper=paper, ink=ink, tracks=tracks)


def scene_bytes(billing=1):
    """{address: byte} for HIRES lines 0-199 (title text left blank)"""
    sc = load_scene()
    mem = bytearray([0x40] * 8000)

    def setpx(x, y):
        mem[y * 40 + 2 + x // 6] |= 0x20 >> (x % 6)

    def clrpx(x, y):
        mem[y * 40 + 2 + x // 6] &= ~(0x20 >> (x % 6)) & 0xFF

    def blit(art, x, bottom):
        rows, w, h = mkart.scenery(mkart.SCENERY[art][0])
        for r, row in enumerate(rows):
            for i, b in enumerate(row):
                if b and x + i < 228:
                    setpx(x + i, bottom - h + 1 + r)

    def colours(y, paper, ink):            # cell 0 ink; cell 1 paper unless black
        mem[y * 40], mem[y * 40 + 1] = ink, (16 + paper if paper else 0x40)

    for y in range(TITLE):
        colours(y, sc["title_paper"], sc["title_ink"])
    for t, recs in enumerate(sc["tracks"]):
        y0 = TITLE + t * BAND
        for i in range(BAND):
            colours(y0 + i, sc["paper"][i], sc["ink"][i])
            if i == ROAD:
                for c in range(1, 40):
                    mem[(y0 + i) * 40 + c] = 0x7F
            if i == ROAD + 2:
                for c in range(2, 40):
                    if (c - 2) % 4 < 2:
                        mem[(y0 + i) * 40 + c] = 0x7F
        for kind, x, a, b in recs:
            if kind in (1, 2):
                blit(a, x, y0 + ROAD - 1)
            elif kind == 3:
                rows, w, h = mkart.scenery(mkart.SCENERY[a][0])
                for y in range(y0, y0 + 6):
                    setpx(x + w // 2, y)
                blit(a, x, y0 + 5 + h)
            elif kind == 5:
                rows, w, h = mkart.scenery(mkart.SCENERY[a][0])
                blit(a, x, y0 + b)
                for y in range(y0, y0 + b + 1):
                    setpx(x + 1, y)
                    setpx(x + w - 2, y)
            elif kind == 4 and billing >= b:
                for y in range(y0 + ROAD, y0 + ROAD + 4):
                    for px in range(x, x + a):
                        clrpx(px, y)
    return mem


def frame_for(x, jt):
    """the rider picture main.c picks: jump in the air, wheels turn every 4 px"""
    return "jump" if jt else ("ride_b" if x & 4 else "ride_a")


def rider_bytes(track, x, h, frame):
    """{address: xor mask} of the rider (cells past 39 clipped)"""
    img = mkart.rider(frame)[x % 6]
    top = TITLE + track * BAND + RIDER_TOP - h
    out = {}
    for col in range(3):
        c = 2 + x // 6 + col
        if c > 39:
            continue
        for r in range(16):
            if img[col][r]:
                out[0xA000 + (top + r) * 40 + c] = img[col][r]
    return out
