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
KINDS = dict(HZ_NONE=0, HZ_BLOCK=1, HZ_RUNNER=2, HZ_DANGLER=3, HZ_GAP=4, HZ_CEILING=5, HZ_JET=6, HZ_RAMP=7, HZ_FLYER=8)
VENT, VENT_HOT = 0x5E, 0x1E                  # a jet's grate on the road's second line; XORed below while it warns
ONLY = None                                 # set: every scene is this one (a -DSCENE= build)
ART = {f"ART_{n.upper()}": i for i, (n, _) in enumerate(mkart.SCENERY)}
TITLE, BAND, RIDER_TOP, ROAD = 8, 48, 28, 44
DANGLE_TOP = 6                              # src/model.h
FRAMES = [n for n, _ in mkart.RIDER_FRAMES]


def shown(rec, billing):
    """src/scene.c hz_shown: the record's billings (5th byte: lowest | highest << 4, 0 = no
    limit), and a gap's lowest billing in b"""
    kind, x, a, b, bills = (tuple(rec) + (0,))[:5]
    lo, hi = bills & 15, bills >> 4
    if kind == 4 and billing < b:
        return False
    return billing >= lo and (not hi or billing <= hi)


def load_scene(name="scene_rhr1", billing=None):
    """the scene's data; with a billing, only the records it shows, as (kind, x, a, b)"""
    src = open(os.path.join(ROOT, "src", "scene.c")).read()
    body = src[src.index(name + " = {"):]
    body = body[:body.index("\n};")]
    head = body.split("{", 2)[1]                # "film", number, vehicle, ground, &pal,
    pname = re.search(r"&(pal_\w+)", head).group(1)
    vehicle = re.search(r"VH_(\w+)", head).group(1).lower()
    ground = 1 if "GROUND_WATER" in head else 0
    pal = src[src.index(pname + " = {"):src.index("};", src.index(pname + " = {"))]
    names = re.findall(r"\b(BLACK|RED|GREEN|YELLOW|BLUE|MAGENTA|CYAN|WHITE)\b", pal.split("{", 1)[1])
    cols = [COLOURS[n] for n in names]
    title_paper, title_ink, paper, ink = cols[0], cols[1], cols[2:50], cols[50:98]
    tracks = []
    for tb in re.findall(r"\{((?:\s*\{[^{}]*\},?)+)\s*\}", body)[:4]:
        recs = []
        for rec in re.findall(r"\{([^{}]*)\}", tb):
            rec = re.sub(r"BILLS\((\d+),\s*(\d+)\)", lambda m_: str(int(m_.group(1)) | int(m_.group(2)) << 4), rec)
            f = [v.strip() for v in rec.split(",")]
            val = lambda v: int(v) if v.isdigit() else {**KINDS, **ART}[v]  # noqa: E731
            r_ = tuple(val(v) for v in f)
            if billing is None or shown(r_, billing):
                recs.append(r_[:4])
        tracks.append(recs)
    tracks += [[] for _ in range(4 - len(tracks))]
    return dict(title_paper=title_paper, title_ink=title_ink, paper=paper, ink=ink, tracks=tracks,
                vehicle=vehicle, water=bool(ground))


def scene_at(run_scene):
    """src/scene.c scene_at: film 1's two scenes take turns (until F6)"""
    if ONLY:
        return ONLY
    return "scene_rhr1" if run_scene % 2 else "scene_rhr2"


def scene_bytes(billing=1, name="scene_rhr1"):
    """{address: byte} for HIRES lines 0-199 (title text left blank): the
    scene as set up, without its moving hazards (police car, wrecking ball:
    hazard_bytes) but with the top of a dangler's rope"""
    sc = load_scene(name, billing)
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
            if sc["water"]:                 # waves: crests, troughs, ripples (DESIGN film 1 scene 2)
                for c in range(2, 40):
                    if i == ROAD:
                        mem[(y0 + i) * 40 + c] = 0x4C
                    elif i == ROAD + 1:
                        mem[(y0 + i) * 40 + c] = 0x73
                    elif i == ROAD + 3 and c % 2:
                        mem[(y0 + i) * 40 + c] = 0x46
                continue
            if i == ROAD:
                for c in range(1, 40):
                    mem[(y0 + i) * 40 + c] = 0x7F
            if i == ROAD + 2:
                for c in range(2, 40):
                    if (c - 2) % 4 < 2:
                        mem[(y0 + i) * 40 + c] = 0x7F
        for kind, x, a, b in recs:
            if kind in (1, 7):                  # blocks and slipways stand on the road
                blit(a, x, y0 + ROAD - 1)
            elif kind == 6:                     # a jet: its vent in the road (the jet moves)
                w = mkart.scenery(mkart.SCENERY[a][0])[1]
                mem[(y0 + ROAD + 1) * 40 + 2 + (x + w // 2) // 6] = VENT
            elif kind == 3:
                rows, w, h = mkart.scenery(mkart.SCENERY[a][0])
                for y in range(y0, y0 + DANGLE_TOP):
                    setpx(x + w // 2, y)
            elif kind == 5:
                rows, w, h = mkart.scenery(mkart.SCENERY[a][0])
                blit(a, x, y0 + b)
                for y in range(y0, y0 + b + 1):
                    setpx(x + 1, y)
                    setpx(x + w - 2, y)
            elif kind == 4:                     # the road missing; on water, rocks
                for y in range(y0 + ROAD, y0 + ROAD + 4):
                    for px in range(x, x + a):
                        (setpx if sc["water"] else clrpx)(px, y)
    return mem


def px(x):
    """model_px (src/model.c): x in 1/256 BBC byte -> pixels, ~x * 228 / 80 / 256"""
    return ((x >> 5) * 91) >> 8


def frame_for(x, jt, crash_t=0):
    """the rider picture main.c picks: crashed; jump in the air; wheels turn every 4 px"""
    return "crash" if crash_t else "jump" if jt else ("ride_b" if x & 4 else "ride_a")


def sprite_bytes(name, x, top):
    """{address: xor mask} of a moving hazard's picture with its left edge at
    x px and its top at screen line top (tools/mkart.py SPRITES)"""
    nc, sh = mkart.sprite(name)
    img = sh[x % 6]
    out = {}
    for col in range(nc):
        c = 2 + x // 6 + col
        for r, v in enumerate(img[col]):
            if v and c <= 39:
                out[0xA000 + (top + r) * 40 + c] = v
    return out


def hazard_bytes(name="scene_rhr1", pos=None, billing=1):
    """{address: xor mask} of the scene's moving hazards where pos says:
    pos[i] = x px of a runner or flyer, the drop of a dangler, or a jet's state
    (2: on), for the i-th moving
    hazard in scene order.  Runners sit on the road; a dangler's rope runs
    from its band's top line down to the ball."""
    sc = load_scene(name, billing)
    out, i = {}, 0
    for t, recs in enumerate(sc["tracks"]):
        y0 = TITLE + t * BAND
        for kind, x, a, b in recs:
            if kind not in (2, 3, 6, 8):
                continue
            art = mkart.SCENERY[a][0]
            rows, w, h = mkart.scenery(art)
            p = pos[i]
            i += 1
            if kind == 2:
                pic = sprite_bytes(art, p, y0 + ROAD - h)
            elif kind == 8:                     # a flyer: b is its underside's band line
                pic = sprite_bytes(art, p, y0 + b + 1 - h)
            elif kind == 6:                     # a jet: drawn while on (pos 2)
                pic = sprite_bytes(art, x, y0 + ROAD - h) if p == 2 else {}
            else:
                top = y0 + DANGLE_TOP + p
                pic = sprite_bytes(art, x, top)
                rx = x + w // 2
                for y in range(y0 + DANGLE_TOP, top):
                    ad = 0xA000 + y * 40 + 2 + rx // 6
                    pic[ad] = pic.get(ad, 0) ^ (0x20 >> (rx % 6))
            for ad, v in pic.items():
                out[ad] = out.get(ad, 0) ^ v
    return out


def rider_bytes(track, x, h, frame, vehicle="bike"):
    """{address: xor mask} of the rider (cells past 39 clipped)"""
    img = mkart.rider(frame, vehicle)[x % 6]
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
