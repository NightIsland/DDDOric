"""keyscript.py - key scripts shared by the emulator driver (run.py) and the
native reference (tools/ref/ref.c parses the same grammar).  From Oric Chuckie
Egg's harness, with Stunt Man Bob's game keys (DESIGN "Controls").

    # comment
    <from>[-<to>] token token ...      frames inclusive, decimal

Lower-case tokens are game inputs: jump accel stop pause quit (src/model.h).
Upper-case tokens are Oric keys (emulator only), e.g. SPACE RSHIFT RETURN.
Frame n = the n-th game step: the Oric build reads its keys after its n-th MARK.
A frame's input is the union of every line covering it; uncovered = nothing.
"""

BUTTONS = {"jump": 0x01, "accel": 0x02, "stop": 0x04, "pause": 0x08, "quit": 0x10}

# Oric keyboard matrix: key -> (row selected on VIA PB0-2, column bit on AY R14)
_M = """3 0 80|X 0 40|1 0 20|V 0 08|5 0 04|N 0 02|7 0 01|
D 1 80|Q 1 40|ESC 1 20|F 1 08|R 1 04|T 1 02|J 1 01|
C 2 80|2 2 40|Z 2 20|CTRL 2 10|4 2 08|B 2 04|6 2 02|M 2 01|
' 3 80|\\ 3 40|- 3 08|; 3 04|9 3 02|K 3 01|
RIGHT 4 80|DOWN 4 40|LEFT 4 20|LSHIFT 4 10|UP 4 08|. 4 04|, 4 02|SPACE 4 01|
[ 5 80|] 5 40|DEL 5 20|FUNCT 5 10|P 5 08|O 5 04|I 5 02|U 5 01|
W 6 80|S 6 40|A 6 20|E 6 08|G 6 04|H 6 02|Y 6 01|
= 7 80|RETURN 7 20|RSHIFT 7 10|/ 7 08|0 7 04|L 7 02|8 7 01"""
MATRIX = {}
for _e in _M.replace("\n", "").split("|"):
    _k, _r, _c = _e.split()
    MATRIX[_k] = (int(_r), int(_c, 16))

# Default game keys (DESIGN "Controls"); a keymap value may join keys with '+'.
DEFAULT_KEYMAP = {"jump": "SPACE", "accel": "LSHIFT", "stop": "RETURN", "pause": "P", "quit": "ESC"}


def parse(path):
    """-> list of (from, to, [tokens])"""
    out = []
    for n, line in enumerate(open(path), 1):
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        rng, *toks = line.split()
        a, _, b = rng.partition("-")
        try:
            a = int(a); b = int(b) if b else a
        except ValueError:
            raise SystemExit(f"{path}:{n}: bad frame range {rng!r}")
        for t in toks:
            if t not in BUTTONS and t not in MATRIX:
                raise SystemExit(f"{path}:{n}: unknown token {t!r}")
        out.append((a, b, toks))
    return out


def oric_rows(path, frames, keymap=DEFAULT_KEYMAP):
    """-> list of (frame, rows[8]) at every frame where the matrix changes."""
    lines = parse(path)
    events, last = [], None
    for f in range(frames + 1):
        rows = [0] * 8
        for a, b, toks in lines:
            if a <= f <= b:
                for t in toks:
                    for k in keymap.get(t, t).split("+"):
                        r, c = MATRIX[k]
                        rows[r] |= c
        if rows != last:
            events.append((f, rows)); last = rows
    return events
