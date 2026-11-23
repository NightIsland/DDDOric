#!/usr/bin/env python3
"""mkart.py - Stunt Man Bob's pixel art, drawn here as text, made into 6502 data
(docs/PLAN.md F1).  All art is our own, drawn fresh at Oric resolution.

  mkart.py OUT.s        write the assembler data (the Makefile does this)
  mkart.py --png F.png  a 4x preview sheet of every picture

Oric HIRES: 6 pixels a byte, bit 5 = leftmost, bit 6 set in screen bytes (not in
this data, which is XOR/OR masks only).

Rider frames (12 x 16) are pre-shifted: for each of the 6 pixel phases, 3 byte
columns x 16 lines.  Columns are stored bottom line first, so src/draw.s can run
one index down a column while the buffer index runs up.  Layout per frame:
shift 0 col 0, col 1, col 2, shift 1 col 0, ...  = 6 x 48 = 288 bytes.

Scenery pictures (hazards) are drawn once at scene set-up, so they are stored
unshifted, a row at a time, ((w + 5) // 6) bytes per row; src/screen.c shifts
them as it draws.  Tests import this file for their pixel references."""
import sys

# ------------------------------------------------------------------ the rider
# Bob on the motorbike, riding right.  '#' = ink.
WHEEL_A = [".###.", "#...#", "#.#.#", "#...#", ".###."]
WHEEL_B = [".###.", "#.#.#", "#...#", "#.#.#", ".###."]


def bike(top, wheel, mid=("##", "##", "#.", "..", "..")):
    """a 12 x 16 rider frame: 11 rows of body over two 5 x 5 wheels"""
    rows = top.strip("\n").split("\n")
    for i in range(5):
        rows.append(wheel[i] + mid[i] + wheel[i])
    return "\n".join(rows)


BODY_RIDE = """
....###.....
...#####....
...###.##...
...#####....
....##......
...####.....
..######....
..##.####...
..###..###..
.########...
###########.
"""
BODY_JUMP = """
.....###....
....#####...
....###.##..
....#####...
.....##.....
....####..#.
...#######..
...##.##....
..##...#....
.##....##...
##########..
"""
RIDE_A = bike(BODY_RIDE, WHEEL_A)
RIDE_B = bike(BODY_RIDE, WHEEL_B)
JUMP = bike(BODY_JUMP, WHEEL_A)
# a crash: Bob thrown up and off, the bike on its side under him
CRASH = """
........#...
.#..#..###..
..#.#.####..
...######...
...######...
..##.##.##..
.##..##..##.
.....##.....
....#..#....
...#....#...
............
.###..#.###.
#...######.#
#.#.#..#.#.#
#...#..#...#
.###....###.
"""
RIDER_FRAMES = [("ride_a", RIDE_A), ("ride_b", RIDE_B), ("jump", JUMP), ("crash", CRASH)]
RIDER_W, RIDER_H = 12, 16

# Bob on the jet-ski (Rush Hour Rumble, scene 2, F5): seated low behind the
# handlebars, the hull on the water and its spray, which changes every 4 px
# like the bike's wheels.  Same 12 x 16 box, same frame order.
JET_BODY = """
....###.....
...#####....
...###.##...
...#####....
....##......
...####..#..
..######.#..
..##.#####..
..###...##..
.#####...#..
"""
JET_HULL = """
############
.##########.
..########..
"""
JET_RIDE_A = JET_BODY.strip("\n") + "\n" + JET_HULL.strip("\n") + """
............
#.#......#..
.#.#.#.#..#.
"""
JET_RIDE_B = JET_BODY.strip("\n") + "\n" + JET_HULL.strip("\n") + """
............
.#.#....#.#.
#.#.#.#.#..#
"""
JET_JUMP = """
.....###....
....#####...
....###.##..
....#####...
.....##.....
....####.#..
...#######..
...##.####..
..##....#...
.##.....##..
..........##
.##########.
###########.
.########...
............
............
"""
JET_CRASH = """
........#...
.#..#..###..
..#.#.####..
...######...
...######...
..##.##.##..
.##..##..##.
.....##.....
....#..#....
...#....#...
#.........#.
.#.#....#.#.
.##########.
############
.#.#.#.#.#.#
#.#.#.#.#.#.
"""
JET_FRAMES = [("ride_a", JET_RIDE_A), ("ride_b", JET_RIDE_B), ("jump", JET_JUMP), ("crash", JET_CRASH)]
VEHICLES = [("bike", RIDER_FRAMES), ("jetski", JET_FRAMES)]

# ---------------------------------------------------- Rush Hour Rumble, scene 1
# Solid shapes, wheels and windows cut out (Bob, BUILD 4: "less wireframe").
# The van is parked facing right, the way Bob rides: cab and windscreen at the
# front, headlamp, rear doors.  The police car faces left, oncoming, with its
# lamp bar on the roof (BUILD 9 had it mirrored: Bob saw it reversing).
VAN = """
.............#######....
.............########...
.............##....###..
############.##.....###.
############.##......##.
##.########.###......##.
##.########.############
##.########.############
##.########.#######.####
##.########.############
############.##########.
########################
###..###########..######
##.##.#########.##.#####
..####.........####.....
...##...........##......
"""
DRUMS = """
.######...######.
#......#.#......#
.######...######.
########.########
#.######.#.######
#.######.#.######
.######...######.
########.########
#.######.#.######
#.######.#.######
.######...######.
########.########
#.######.#.######
.######...######.
"""
POLICE = """
..........##.##.........
..........#####.........
.......############.....
......##....#.....##....
.....###....#......##...
.#######################
##.......###############
##.......#####.######.##
.#######################
#####..###########..####
####.##.#########.##.###
.....##...........##....
"""
BALL = """
....####....
..##..####..
.#....######
.#..########
#..#########
#.##########
############
############
.##########.
.##########.
..########..
....####....
"""
GIRDER = """
########################
#.#.#.#.#.#.#.#.#.#.#.#.
.#.#.#.#.#.#.#.#.#.#.#.#
########################
"""

# ------------------------------------------------- Rush Hour Rumble, scene 2
# The harbour chase (F5).  The patrol boat runs bow first to the left, like
# the police car, with the same lamp on its cabin roof in its top two rows
# (src/screen.c lamps() flashes them).  The footbridge hangs low across the
# channel; the slipway is a concrete launch ramp rising to the right.
BUOY = """
.....##.....
....####....
.....##.....
....####....
...######...
...#.##.#...
..########..
..#.####.#..
.##########.
.#.######.#.
############
############
.##########.
..########..
"""
BOAT = """
..........##.##.........
..........#####.........
.........#######........
.........#.#.#.#........
........#########.......
........#.#.#.#.##......
.......############.....
########################
.#######################
..######################
...#####################
.....###################
"""
BRIDGE = """
#.....#.....#.....#.....#.....
##############################
#.....#.....#.....#.....#.....
##############################
.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#
##############################
"""
SLIPWAY = """
..................######
............############
......######.#.#.#######
######.#.#.#.#.#########
########################
#.#.#.#.#.#.#.#.#.#.#.#.
"""
SCENERY = [("van", VAN), ("drums", DRUMS), ("police", POLICE), ("ball", BALL), ("girder", GIRDER),
           ("buoy", BUOY), ("boat", BOAT), ("bridge", BRIDGE), ("slipway", SLIPWAY)]


# ------------------------------------------------- the film frame (F4)
# Drawn unshifted, a row at a time like scenery; src/frame.c draws them at
# double size (each pixel 2 x 2) on the title screen.
BIKE_BODY = """
............
............
............
............
............
........##..
.........#..
...#####.#..
..########..
.#########..
###########.
"""
BIKE = bike(BIKE_BODY, WHEEL_A)          # Bob's bike, parked
# Bob, helmet off, waving to the crowd
BOB_POSE = """
....###.....
...#####....
...#.#.#..#.
...#####..#.
....###..#..
.....#..##..
..#######...
.#.#####....
#..#####....
#..#####....
#..#####....
##.#####....
...#####....
...##.##....
...##.##....
...##.##....
...##.##....
...##.##....
..###.###...
"""
# the director's megaphone, for the "CUT!" popup (drawn at double size)
MEGAPHONE = """
................##
.............#####
..........########
.###....##########
.###.#############
.#################
.#################
.###.#############
.###....##########
..#.......########
..#..........#####
..##............##
"""
PICTURES = [("bike", BIKE), ("bob_pose", BOB_POSE), ("megaphone", MEGAPHONE)]


def bits(txt):
    """rows of 0/1; letters inside art (e.g. POLICE) are ink too"""
    rows = [r for r in txt.strip("\n").split("\n")]
    w = max(len(r) for r in rows)
    return [[0 if c in ". " else 1 for c in r.ljust(w, ".")] for r in rows]


def shifted(rows, shift, ncols):
    """byte columns of the picture shifted right by `shift` px: list[col][line]"""
    out = [[0] * len(rows) for _ in range(ncols)]
    for y, r in enumerate(rows):
        for x, b in enumerate(r):
            if b:
                px = x + shift
                out[px // 6][y] |= 0x20 >> (px % 6)
    return out


# Moving hazards (F3) are drawn like the rider: pre-shifted, columns bottom
# line first.  The police car keeps its own 12 lines (it only moves sideways,
# so a shorter box saves time every step); the ball is padded to 16 ("top"
# keeps its top at line 0).
SPRITES = [("police", None, 5), ("ball", "top", 3), ("boat", None, 5)]


def sprite(name):
    """(columns, {shift: [col][line]}) for a moving hazard"""
    pad, ncols = dict((n, (p, c)) for n, p, c in SPRITES)[name]
    rows, w, h = scenery(name)
    if pad:
        blank = [[0] * w for _ in range(16 - h)]
        rows = blank + rows if pad == "bottom" else rows + blank
    return ncols, {s_: shifted(rows, s_, ncols) for s_ in range(6)}


def rider(name, vehicle="bike"):
    """{shift: [col][line]} for a rider frame"""
    rows = bits(dict(dict(VEHICLES)[vehicle])[name])
    assert len(rows) == RIDER_H and len(rows[0]) == RIDER_W, name
    return {s: shifted(rows, s, 3) for s in range(6)}


def scenery(name):
    rows = bits(dict(SCENERY)[name])
    return rows, len(rows[0]), len(rows)


def write_s(path):
    o = ["; art.s - generated by tools/mkart.py: do not edit.", "",
         "        .export         _rider_art, _scenery_art, _scenery_w, _scenery_h", "",
         "        .export         " + ", ".join(f"_spr_{n}" for n, _, _ in SPRITES), "",
         "        .export         _pic_art, _pic_nb, _pic_h", "",
         '        .segment        "RODATA"', "",
         "; rider: vehicle x frame x shift x column x line (bottom first), 48 bytes per shift",
         "_rider_art:"]
    for vname, frames in VEHICLES:
      for name, _ in frames:
        r = rider(name, vname)
        o.append(f"; {vname} {name}")
        for s_ in range(6):
            for c in range(3):
                o.append("        .byte   " + ",".join(f"${v:02X}" for v in reversed(r[s_][c])))
    for name, _, _ in SPRITES:
        nc, sp = sprite(name)
        o.append(f"; moving hazard {name}: shift x {nc} columns x {len(sp[0][0])} lines (bottom first)")
        o.append(f"_spr_{name}:")
        for s_ in range(6):
            for c in range(nc):
                o.append("        .byte   " + ",".join(f"${v:02X}" for v in reversed(sp[s_][c])))
    o.append("")
    o.append("; scenery: pointers, widths in px, heights; rows of (w + 5) / 6 bytes")
    o.append("_scenery_art:")
    for name, _ in SCENERY:
        o.append(f"        .addr   sc_{name}")
    o.append("_scenery_w:")
    o.append("        .byte   " + ",".join(str(scenery(n)[1]) for n, _ in SCENERY))
    o.append("_scenery_h:")
    o.append("        .byte   " + ",".join(str(scenery(n)[2]) for n, _ in SCENERY))
    for name, _ in SCENERY:
        rows, w, h = scenery(name)
        nb = (w + 5) // 6
        o.append(f"sc_{name}:")
        for r in rows:
            bs = [0] * nb
            for x, b in enumerate(r):
                if b:
                    bs[x // 6] |= 0x20 >> (x % 6)
            o.append("        .byte   " + ",".join(f"${v:02X}" for v in bs))
    o.append("")
    o.append("; film frame pictures: pointers, bytes a row, heights; rows unshifted")
    o.append("_pic_art:")
    for name, _ in PICTURES:
        o.append(f"        .addr   pic_{name}")
    o.append("_pic_nb:")
    o.append("        .byte   " + ",".join(str((len(bits(t)[0]) + 5) // 6) for _, t in PICTURES))
    o.append("_pic_h:")
    o.append("        .byte   " + ",".join(str(len(bits(t))) for _, t in PICTURES))
    for name, t in PICTURES:
        rows = bits(t)
        nb = (len(rows[0]) + 5) // 6
        o.append(f"pic_{name}:")
        for r in rows:
            bs = [0] * nb
            for x, b in enumerate(r):
                if b:
                    bs[x // 6] |= 0x20 >> (x % 6)
            o.append("        .byte   " + ",".join(f"${v:02X}" for v in bs))
    open(path, "w").write("\n".join(o) + "\n")


def write_h(path):
    o = ["/* art.h - generated by tools/mkart.py: do not edit. */", "#ifndef ART_H", "#define ART_H", "",
         "#include <stdint.h>", "", f"#define RIDER_W {RIDER_W}", f"#define RIDER_H {RIDER_H}",
         "#define RIDER_SHIFT_BYTES 48", "#define RIDER_FRAME_BYTES 288", "#define RIDER_VEHICLE_BYTES 1152"]
    for i, (name, _) in enumerate(VEHICLES):
        o.append(f"#define VH_{name.upper()} {i}")
    for i, (name, _) in enumerate(RIDER_FRAMES):
        o.append(f"#define FR_{name.upper()} {i}")
    for i, (name, _) in enumerate(SCENERY):
        o.append(f"#define ART_{name.upper()} {i}")
    o.append("#define SCENERY_W_DATA " + ", ".join(str(scenery(n)[1]) for n, _ in SCENERY))
    o.append("#define SCENERY_H_DATA " + ", ".join(str(scenery(n)[2]) for n, _ in SCENERY))
    for name, _, nc in SPRITES:
        o.append(f"#define SPR_{name.upper()}_COLS {nc}")
        o.append(f"#define SPR_{name.upper()}_H {len(sprite(name)[1][0][0])}")
        o.append(f"extern const uint8_t spr_{name}[];")
    # pixel-touch crashes (Bob, BUILD 10): every picture as column masks,
    # bit r = row r from the top, for src/model.c (Oric and native alike)
    def cols(rows):
        return [sum(1 << r for r in range(len(rows)) if rows[r][c]) for c in range(len(rows[0]))]
    o.append("#define HIT_RIDER_DATA " + ", ".join(f"{v}u" for _, fr in VEHICLES for n, t in fr[:3] for v in cols(bits(t))))
    hs, ofs = [], []
    for n, _ in SCENERY:
        ofs.append(len(hs))
        hs += cols(scenery(n)[0])
    o.append("#define HIT_SCENERY_DATA " + ", ".join(f"{v}u" for v in hs))
    o.append("#define HIT_SCENERY_OFS " + ", ".join(str(v) for v in ofs))
    for i, (name, _) in enumerate(PICTURES):
        o.append(f"#define PIC_{name.upper()} {i}")
    o += ["", "extern const uint8_t rider_art[];", "extern const uint8_t * const scenery_art[];",
          "extern const uint8_t * const pic_art[];", "extern const uint8_t pic_nb[], pic_h[];",
          "extern const uint8_t scenery_w[], scenery_h[];", "", "#endif"]
    open(path, "w").write("\n".join(o) + "\n")


def preview(path):
    sys.path.insert(0, __import__("os").path.dirname(__file__))
    import struct, zlib
    pics = [bits(t) for _, fr in VEHICLES for _, t in fr] + [bits(t) for _, t in SCENERY] + [bits(t) for _, t in PICTURES]
    S, gap = 8, 4
    W = sum(len(p[0]) + gap for p in pics) + gap
    H = max(len(p) for p in pics) + 2 * gap
    img = [[(0, 0, 80)] * W for _ in range(H)]
    x0 = gap
    for p in pics:
        for y, r in enumerate(p):
            for x, b in enumerate(r):
                if b:
                    img[gap + y][x0 + x] = (255, 255, 255)
        x0 += len(p[0]) + gap
    raw = b"".join(b"\0" + b"".join(bytes(img[y // S][x // S]) for x in range(W * S)) for y in range(H * S))

    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    open(path, "wb").write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", W * S, H * S, 8, 2, 0, 0, 0))
                           + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


if __name__ == "__main__":
    if sys.argv[1] == "--png":
        preview(sys.argv[2])
    else:
        write_s(sys.argv[1])
        write_h(sys.argv[1][:-2] + ".h")
