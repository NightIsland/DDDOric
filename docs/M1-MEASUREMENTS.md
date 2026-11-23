# M1: Measurements of the BBC original

23 Nov 2026 · PLAN step M1. What Daredevil Dennis (Visions, 1984, BBC Micro) actually does, measured
frame by frame, and what that means in Oric units. Rules and timings only: no graphics, sound, code
or layouts were copied. The tools and how to rerun them are in `tools/m1/README.md`.

## How it was measured

- **Emulator.** JSBeeb (current GitHub head) run headless in Node as a BBC Model B with DFS 1.2,
  with its real video chip so screenshots work. The disc is `Disc005-DareDevilDenis.ssd` from
  www.bbcmicro.co.uk (game 79, the released version).
- **Method.** The emulator was stepped one video frame (20 ms) at a time from saved snapshots, with
  keys pressed and released on exact frames, and the whole of RAM recorded on every frame. Diffing
  those traces found the rider's x, speed, jump counter, take, screen number and wages, and a few
  hazards (addresses in `tools/m1/README.md`). Where a rule hangs on a constant (the speed cap, the
  wrap point, the stop flag, the take refund), the constant was read from the game's own code and
  then confirmed by a run.
- **Coverage.** Novice from screen 1 to screen 7 (land, sea, snow, land, sea, snow, land), plus the
  first screen of every other skill level. Sea and snow were reached by moving the rider to the end of
  track 4 so the game went on to its next screen by itself.
- **Units.** The game runs in MODE 2 (160 × 256). Rider x is kept in screen bytes (1 byte = 2 MODE 2
  pixels, 80 bytes across); heights are screen lines. A "BBC step" is one pass of the game loop.

## The game loop

| Item | Measured | How |
| --- | --- | --- |
| Step time | **3 frames (60 ms, 16.7 steps a second)** on every Novice screen and most others | frames between speed and position changes |
| Busy screens | 4 frames (80 ms) on the Ace starting screen (screen 31), 3 or 4 on screen 19 | same |

The loop waits for the vertical sync while it draws, so a screen with more moving things runs slower.
Distances per step never change, only the time a step takes. We should not copy the slowdown.

## Rider physics

| Quantity | Measured (BBC) | How |
| --- | --- | --- |
| Start speed | **0**: each take, and each new screen, starts with the bike standing until Shift is pressed | trace, every take |
| Speed range | 0 to **250** in whole units | code: `CMP #250`, clamp; trace |
| Acceleration | **+3 per step** while Shift is held, on the ground and in the air | trace |
| Releasing Shift | **No change**: the speed holds. Nothing in the game slows the bike except the stop and a crash | trace, code |
| Movement | x grows by speed/256 bytes **three times a step**, so 3 × speed / 256 bytes a step | code, trace |
| Top speed | 250 → 2.93 bytes a step = 48.8 bytes a second (98 MODE 2 pixels a second) | from the above |
| Start to top | 84 steps = **5.0 s** of holding Shift | from the above |
| Track | The rider's x runs 0 to 74 bytes; reaching 75 to 77 wraps (the sprite is 4 bytes wide, so its right edge meets the screen edge) | code: `CMP #$4D/$4C/$4B`; trace |
| Track time | 1.54 s at top speed; 9.4 s at speed 40 | from the above |
| Sprite | Rider and ground hazards about 4 bytes × 16 lines | screenshots |

## Jump

| Quantity | Measured (BBC) | How |
| --- | --- | --- |
| Start | Space is read once a step; on the ground it starts the jump. A press shorter than a step (2 frames) can be missed | trace with 2-frame and 5-frame presses |
| Airborne time | **28 steps (1.68 s)**, the same at every speed | trace at speed 60, and accelerating from 120 |
| Arc | Rise per step, in lines: 3, 6, 8, 11, 13, 14, 16, 17, 18, 19, 19, 20, 20, 20, 20, 20, 20, 19, 19, 18, 17, 16, 14, 13, 11, 8, 6, 3, then down | trace; matches the game's 32-byte height table |
| Apex | **20 lines** above the road, held for 6 steps; obstacles are about 16 lines tall | trace, screenshots |
| Length | 28 × 3 × speed / 256 = **0.33 × speed bytes**: 20 bytes at speed 60, 39 at 120, **82 at top speed** (more than one whole track) | trace (19 bytes at speed 60) |
| In the air | Shift still adds speed, so a jump can be stretched; it can never be shortened except by the stop | trace, code |
| Holding Space | **Repeats**: the rider lands, spends one step on the road and jumps again (30-step cycle) | trace with Space held |
| Wrap in the air | The jump carries on over the next track at the same point in the arc and the same speed | trace (jump from x 27 at speed 120 crossed the wrap) |

## Stop, wrap, gaps

| Rule | Measured (BBC) | How |
| --- | --- | --- |
| Stop | Return sets the speed to 0 at once | trace |
| Stops allowed | **One per take**: the stop flag is set again on every retake and at the start of each new screen, not at a wrap | trace: stop used on track 1, flag still clear after the wrap, a second press on track 2 ignored; confirmed by Bob playing the original. (A first reading of the code had this as once per track; the flag is reset in the end-of-screen branch of the wrap routine, not on every wrap.) |
| Stop in the air | **Allowed**: forward motion stops dead and the rider drops straight down through the rest of the arc | trace (stop 9 steps into a jump) |
| Wrap | Same speed, same jump counter (so the same height), next track down; after track 4 the next screen starts | trace, code |
| Gaps | Each step on the road, the game reads the screen under the rider; no road there means a fall | code; trace (fell into the first gap on skill level B) |

## Hazards (Novice, screen 1)

| Hazard | Measured (BBC) | How |
| --- | --- | --- |
| Runner (ambulance) | **1 byte a step** (16.7 bytes a second) left to right, wrapping at 74 → 0; crosses a track in 4.5 s | trace of its x |
| Flying runner (helicopter, from screen 4) | 1 byte a step, right to left, at mid height | trace |
| Dangler (on a line from the track above) | Line length moves **1 line a step** between 1 and 26 lines, so a full up-and-down period is **50 steps (3.0 s)** | trace of its length |
| Runner and dangler speeds by level | The same per step on every level seen; only the layouts change (and the steps slow on busy screens) | first screen of each level |

## Scenes, levels, takes and wages

| Rule | Measured (BBC) | How |
| --- | --- | --- |
| Screens | **36 screens** in a fixed order, cycling land, sea, snow. Screens 1 to 3 share one layout in three dressings | warp through screens 1 to 7; code |
| Skill level | Picks the **starting screen**: Novice 1, Experienced 7, Expert 13, Oscar Nominee 19, Oscar Winner 25, Ace 31. Play then runs on through the screens; after screen 35 it loops over 33 to 35 | snapshots of each level; code |
| A "level" | So each skill level is **6 screens** (two land, sea, snow rounds) before the next level's first screen | from the above |
| Hazards by screen | Screens 1 to 3: block, runner, block, dangler. Gold bars and the helicopter from screen 4; gaps from screen 5; flying men from screen 7 | screenshots |
| Sea and snow handling | **Identical to land**: same +3 a step, same cap, same jump, same step time. Only the pictures change (wetbike, skis) | traces on screens 2 and 3 |
| Takes | 3; the fourth crash ends the game | code (`CMP #4`), trace |
| Take refund | **One take back** on reaching the land screen after each completed land, sea, snow round (screens 4, 7, 10, ...), if a take was lost ("contract extended") | code; confirmed: take 2 → take 1 on entering screen 4, unchanged on entering 2 and 3 |
| Wages | Paid **per track completed**, not per hazard: **10 × the screen number** each time the rider wraps (10 on screen 1, 20 on screen 2, ...) | trace of the wages digits: on screen 1 the wages stayed 0 after the rider cleared the house and went to 10 on the frame he wrapped; a rider placed at the end of track 4, passing no hazard at all, was still paid for the track |
| Wages on a retake | **Paid again**: a retake that rides track 1 again pays for it again | trace: 10 → crash on track 2 → retake → 20 after track 1, 30 after track 2 |
| Gold bars | Small bars on the road from screen 4; the pickup bonus was not measured | screenshots |

## In Oric units

Scale: the BBC's 80 bytes become our 228-pixel play width, so **1 BBC byte = 2.85 Oric pixels**
(rider 4 bytes → 11.4 px, our 12 px sprite). BBC lines map one to one onto Oric lines. Time: one
BBC step (60 ms) is **three of our 50 Hz steps**.

The original adds the speed to x three times per BBC step, which is once per 20 ms. So the simplest
faithful model keeps the original's own units and runs them at 50 Hz:

- speed `s` from 0 to 250, **+1 per 50 Hz step** while Accelerate is held (5.0 s to top);
- x in 1/256 BBC byte, **x += s every step**; draw at `x × 228 / 80` pixels; wrap at 75 bytes
  (213 px of the rider's left edge);
- jump: the 28-entry arc above with each entry held 3 steps (or interpolated): **84 steps, 20 px
  apex**;
- hazards: runner 1 BBC byte per 3 steps (0.95 px a step); dangler 1 line per 3 steps over 26 lines.

The same numbers in our pixels:

| Quantity | Oric value (50 Hz step) |
| --- | --- |
| Start speed | 0 |
| Top speed | 2.78 px a step (139 px a second); a track in 1.54 s |
| Acceleration | +0.0111 px a step, each step held (start to top in 250 steps, 5.0 s) |
| Jump | 84 steps (1.68 s), apex 20 px |
| Jump length | speed × 84: 234 px at top speed (just over one track), 117 px at half speed |
| Held Space | Next jump after 3 steps on the road |
| Runner | 0.95 px a step (47.5 px a second) |
| Dangler | 26 px of travel at 1 px per 3 steps; period 150 steps (3.0 s) |

## Where the original differs from DESIGN.md

These are not changed in DESIGN.md, because Bob decided them or they change the game; they are listed
there for Bob.

1. **Stop.** The original gives one stop per take (restored on a retake or a new screen), and it
   works in mid-air. DESIGN had one per scene, ignored in the air. Bob: follow the original.
2. **Sea and snow.** The original handles exactly like land. DESIGN gave the jet-ski looser and the
   skis a drag. Bob: follow the original.
3. **Wages.** The original pays per track completed (10 × screen number) and pays again on a retake.
   DESIGN pays per hazard, once per scene. Still open with Bob.
4. **Held jump.** The original repeats the jump while Space is held. DESIGN did not repeat. Bob:
   follow the original.
5. **Takes.** The original gives a lost take back after every land, sea, snow round. DESIGN said no
   extra takes "as in the original". Bob: follow the original.
6. **Levels.** The original's level is 6 screens and the skill level only picks where you start in a
   36-screen run. DESIGN's billing is a season of 12 screens. Bob agreed; billings already work this
   way.
7. **Missed taps.** The original can miss a Space press shorter than one step (60 ms). DESIGN latches taps; that
   is an improvement worth keeping.
