#!/bin/sh
# wins.sh - re-search the winning runs in tests/keys (make wins).
# Each billing's proof clears every built scene of the season (F6: 2 per film) on its first
# take, every jump with a window of W steps (20 ms each) to press it in:
# 8 at billings 1-2, 6 at 3-4, 5 at 5, 4 at 6.  takeback.keys crashes on
# take 1 and then clears three scenes; season.keys clears a whole season
# and the first scene of the next billing.
set -e
S=build/solve/solve
SCENES=12                # films built so far x 2
set -- 8 8 6 6 5 4
b=1
for w in "$@"; do
    $S $b $SCENES 20000 $w > tests/keys/win-b$b.keys
    b=$((b + 1))
done
SOLVE_CRASH=1 $S 1 3 6000 > tests/keys/takeback.keys
$S 1 13 6000 > tests/keys/season.keys
