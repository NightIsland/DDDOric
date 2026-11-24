# Stunt Man Bob: Game Design

23 Nov 2026 · agreed with Bob. The living copy is the Claude Doc
[Stunt Man Bob: Game Design](https://claude.ai/artifact/KwK3EQPRG5FuQpR1iJoUUh); this file is
the repo's snapshot of it. Change both together.

## Pitch and pillars

Stunt Man Bob is a one-screen, four-track stunt game for the 48K Oric-1 and Atmos: Bob signs on as
the stunt double for six films, one per genre, and must ride every scene without coming off. Each
scene is a fixed screen of four stacked tracks ridden left to right, top to bottom, with three keys:
jump, accelerate, stop. The play should feel like Daredevil Dennis (BBC Micro, 1984); everything you
see and hear is our own.

Numbers marked **(guess)** are starting values, not measured facts. Numbers marked **(M1)** were
measured in the BBC original frame by frame; the method and the full results are in
`docs/M1-MEASUREMENTS.md`.

- **Speed plus timing is the whole game.** A jump is a fixed arc; its length comes only from how fast
  you are going, and you can still accelerate in the air.
- **See it all, plan it all.** The whole scene is on screen, so every attempt is a small puzzle you
  can read before you ride it.
- **Short takes, instant retakes.** A crash costs a few seconds, never a long wait.
- **The film set is the frame, not the rules.** Clapperboards, takes and "Cut!" dress the game; they
  never slow it down.
- **Built for the Oric.** Fixed screen, banded colour, small sprites, AY sound that beats the BBC's.

## Feel targets kept from the original

We keep the original's rules and timing as closely as we can measure them, and change only its look,
sound, names and layouts. Before the prototype is tuned, each target is measured in the BBC original
frame by frame (JSBeeb, see PLAN M1) and the measured value written here.

| Target | Original behaviour | Our starting value |
| --- | --- | --- |
| Track structure | 4 stacked tracks per screen, no scrolling; leaving the right edge drops you to the left of the next track | Same |
| Wrapping | Same height above the road, same point in the jump arc, same speed | Same (confirmed by Bob) |
| Accelerate | Held key adds speed every step, also in mid-air; standing to top speed in 5.0 s (M1) | Same: 0 to top in 5.0 s (M1) |
| Releasing Accelerate | The bike holds its current speed; it never slows by itself | Same (confirmed by Bob) |
| Jump | Fixed arc, no steering, 1.68 s and 20 lines high at every speed; length set by speed, and accelerating in the air stretches it (M1) | Same |
| Jump at top speed | Carries just over one whole track, so it crosses the wrap (M1) | About 234 px of travel (M1) |
| Stop | Return stops the bike dead, once per take: given back on each retake and each new screen, not at a wrap; it also works in mid-air (M1, confirmed by Bob) | Same: once per take, also in mid-air |
| Ceiling | Some fixed obstacles hang from the top of the track and catch a jump | Same: ceiling blocks |
| Handling | Identical on land, sea and snow; only the pictures change (M1) | Same: one standard handling for every vehicle except the sci-fi hover bike, see Vehicles |
| Lives | 3 takes, then back to the menu; one lost take is given back after each land, sea, snow round (M1) | 3 takes per run; one lost take back after every third scene cleared |
| Score | Wages per track completed, 10 × the screen number, paid again on a retake (M1) | Same: £10 × the scene's number in the run per track, paid again on a retake (Bob's decision) |
| Crash | Burst, rider flung away, pause, same screen restarts | Same, with "Cut!" and the clapper |
| Difficulty | 36 screens in a fixed order cycling land, sea, snow; the skill level picks the starting screen (1, 7, 13, 19, 25, 31), so a level is 6 screens; gaps from screen 5, flying hazards from screen 4 (M1) | 6 billings; each is one season of 6 films, then the next billing starts by itself; gaps from billing 3 (guess) |

## Screen structure

A scene is one HIRES screen: a thin title strip, four 48-line track bands, and the status line in
the text rows underneath. Nothing scrolls; only the rider, moving hazards and effects are redrawn.

| Area | Lines | Contents |
| --- | --- | --- |
| Title strip | 0–7 | Film name and scene number, in the film's colour |
| Track 1 | 8–55 | Air and dangler space (top 28 lines), rider (16 lines), road surface (4 lines) |
| Track 2 | 56–103 | Same layout |
| Track 3 | 104–151 | Same layout |
| Track 4 | 152–199 | Same layout |
| Status line | text rows 25–27 | Take, stop icon, fee, best fee, drawn in a small bitmap font (the Chuckie Egg trick) |

- **Width.** Column 0 holds each line's paper colour and column 1 its ink, leaving 38 cells, 228
  pixels, for play. The rider's x runs 0–227.
- **Colour.** Colour comes in horizontal bands, set per line: for example a dark sky, coloured hazards
  and rider in the middle lines, and a bright road line at the bottom of each band. Each film has its
  own palette (green for Western scrub, red for horror, cyan for sci-fi).
- **Shared colour.** The rider and a hazard on the same lines share one ink. We accept that rather
  than spend attribute cells; inverse cells (bit 7) can pick out a single hazard where it matters.
- **Jump room.** The rider stands on lines 28–43 of a band; a jump's apex of 20 pixels (M1) stays
  inside the band, so a jump never draws over the track above.

## Controls

Three game keys, fixed to match the original's layout (Bob dropped key redefinition, 24 Nov 2026): one hand on Space, the other with a finger on Shift and one ready on Return. No joystick.

| Action | Key | Behaviour |
| --- | --- | --- |
| Jump | Space | Starts a jump from the ground; held, it jumps again 3 steps after each landing, as in the original |
| Accelerate | Either Shift | Held: speed builds, on the ground or in the air; released: the bike holds its current speed |
| Stop | Return | Stops the bike dead, on the ground or in the air; one use per take |
| Pause | P | Freezes the game; any key resumes |
| Quit | Esc | Back to the title screen (asks Y/N) |
| Sound | S (title screen) | Sound on or off |

- **Ghosting.** The Oric keyboard has no diodes, so holding three keys in an L can fake a fourth.
  Three game keys alone are safe, but a ghost could trigger Pause, so the game reads Pause and Quit
  only while no game key is held.
- **Feel.** Keys are scanned every game step (50 times a second), and Jump latches a key-down seen
  between steps, so a quick tap is never lost.

## Physics and core mechanics

The game runs a fixed step of 50 per second from the 10 ms timer, with positions and speeds in fixed
point (1/256 pixel), so every run with the same keys plays out exactly the same. The values below are
the original's, measured in M1 and converted: one BBC game step (60 ms) is three of our steps, and one
BBC screen byte is 2.85 of our pixels.

| Quantity | Value (M1) | What it means in play |
| --- | --- | --- |
| Start speed | 0 | Each take starts with the bike standing; Accelerate moves it off |
| Top speed | 2.78 px per step (139 px/s) | A track (213 px of travel before the wrap) takes 1.54 s flat out |
| Acceleration | +0.0111 px per step, each step Accelerate is held (the original's speed 0–250, +1 per step) | Standing to top speed in 5.0 s |
| Releasing Accelerate | No change | The bike holds whatever speed it has reached |
| Jump time | 84 steps (1.68 s), the same at every speed | |
| Jump height | 20 px at the apex, from the original's 28-step arc with each entry held 3 steps | Clears obstacles up to 16 px tall with room to spare |
| Jump length | speed × 84 steps | 117 px at half speed, 234 px at top speed (just over a track), more if you accelerate in the air |

Speed only ever goes up, except for the one stop, so every scene is a question of how fast to be by
when.

**Rules.**

1. **Jumping.** Jump starts the arc from the ground only. The arc's shape never changes; the bike
   keeps its forward speed, and Accelerate still adds to it in the air, so a jump can be stretched
   but never shortened.
2. **Stopping.** Stop sets speed to zero at once and is then used up for this take (the status line
   icon goes out); passing to the next track does not give it back. A stopped bike waits;
   Accelerate moves it off from zero. Stopped in mid-air, the bike drops straight down through the
   rest of the arc. Every retake and every new scene gives the stop back.
3. **Wrapping.** Passing the right edge puts Bob at the left of the next track down at the same
   speed. He keeps his height above the road and his point in the jump arc, so a jump that crosses
   the edge carries on over the next track. Leaving track 4 ends the scene.
4. **Crashing.** Bob crashes only when a pixel of his picture touches a pixel of a hazard's (Bob,
   BUILD 10; it replaced a box shrunk by 2 px a side), which includes
   if he lands on top of an obstacle, if his wheels are over a gap when he is on the ground, or if his
   jump hits a ceiling block or anything else that must be passed on the ground.
5. **Paying.** Finishing a track (wrapping to the next one, or leaving track 4) pays the scene's
   track fee, as in the original. Hazards themselves pay nothing; they only stand in the way.

## Obstacles and vehicles

Every hazard is one of eight behaviours; each film dresses them in its own art. Block, runner,
dangler, gap, ceiling block and flyer are the original's kinds; jet and ramp are new. (Bob agreed
the flyer for films 2–6, 24 Nov 2026.)

| Behaviour | How it acts | How to pass it | New? |
| --- | --- | --- | --- |
| Block | Stands still on the track | Jump it | Original kind |
| Runner | Drives along the track at a set speed, either way, and wraps at the edges | Arrive when it is elsewhere, or jump it | Original kind |
| Flyer | A runner at head height: crosses the track in the air, its underside just above the rider's head | Stay on the ground while it passes over; any jump under it hits it | Original kind (M1: flying hazards from screen 4) |
| Dangler | Hangs on a line from the band above and bobs up and down on a set period | Pass under it on the ground while it is up; jumping hits it | Original kind |
| Gap | A missing piece of road, 12–48 px wide | Jump it | Original kind, harder billings |
| Ceiling block | Fixed to the top of the band. A low one must be passed on the ground; a high one only cuts off the top of a jump arc | Stay on the ground, or time the jump so the arc passes beside it | Original kind (Bob) |
| Jet | Flame or steam bursts up from the road on a timer | Pass while it is off, or jump while it is on | New |
| Ramp | A fixed launch ramp | Riding onto it fires a jump 1.5× longer than normal, same height (F5: 126 steps) | New, helpful |

The interesting scenes come from pairs. A low ceiling block just before a block forces you to land in
the gap between them; a dangler after a gap means you must land, slow down, and time your arrival,
which is where the one stop earns its keep.

**Vehicles.** Each vehicle has its own look, engine sound and handling, where handling is three
numbers: acceleration, top speed and jump time. As in the original (M1), where the wetbike and skis
handle exactly like the bike, every vehicle uses the standard handling (+0.0111 px per step, top
2.78 px per step, jump 84 steps). The one exception is our own sci-fi hover bike, which floats.

Films 2–6 have a vehicle per scene (Bob, 24 Nov 2026). Every vehicle fits the rider's
12 × 16 box, so drawing and crash tests cost the same for all of them.

| Film | Scene 1 vehicle | Scene 2 vehicle | Handling |
| --- | --- | --- | --- |
| 1 Action | Motorbike (built) | Jet-ski (built) | Standard |
| 2 Western | Horse | Mine cart | Standard |
| 3 Horror | Rattling hearse-trike | Ghost-train car | Standard |
| 4 Sci-fi | Hover bike | Hover bike | Floaty: jump time 100 steps, same 20 px height; a ramp jump 150 steps (guess) |
| 5 Fantasy | Dragon hatchling (replaces the chariot, which can't fit a 12 px box) | Magic carpet | Standard |
| 6 Spy | Skis | Snowmobile | Standard |

## Films, scenes and progression

A run is a season of six films, each two scenes long (12 screens, 48 tracks). Finish the season and
the next one starts at the next billing up, with the same films made harder. All film names and
layouts are new.

| # | Film | Genre | Scene 1 hazards | Scene 2 |
| --- | --- | --- | --- | --- |
| 1 | *Rush Hour Rumble* | Action | Parked van (block), police car (runner), oil drums (block near the start), wrecking ball (dangler) | Harbour chase on a jet-ski: buoy (block), patrol boat (runner), low footbridge (ceiling block), slipway (ramp) |
| 2 | *Dust at Dawn* | Western | Main Street on a horse: water trough, hitching rail (blocks), tumbleweed (runner), saloon balcony (high ceiling), grain sack on a hoist (dangler) | Runaway Mine in a mine cart: ore pile, fallen rock (blocks), stray ore cart (runner), pit beams (low ceilings), gas flare (jet), buckled rail (ramp), rock bucket (dangler, billing 4 up) |
| 3 | *Night of the Hearse* | Horror | Graveyard Lane on a hearse-trike: gravestone, stone angel (blocks), shuffling ghoul (runner), bat (flyer), lych-gate (low ceiling), open grave (gap), spider on a thread (dangler) | Tunnel of Terror in a ghost-train car: coffin (block), pop-up ghoul (jet), crypt arch (low ceiling), dropping skeleton (dangler), track hump (ramp) |
| 4 | *Moonbase Panic* | Sci-fi | Crater Plains on the hover bike: fuel canister, antenna mast (blocks), patrol robot (runner), drone (flyer), cargo claw (dangler), gantry arm (high ceiling), crater (gap) | Reactor Corridor on the hover bike: crate (block), laser gate (jet), blast door (low ceiling), crusher piston (dangler), launch pad (ramp) |
| 5 | *Dragon's Keep* | Fantasy | Castle Approach on a dragon hatchling: boulder, barrel stack (blocks), charging knight (fast runner), portcullis (dangler), moat (gap) | Moat by Moonlight on a magic carpet: rock (block), dragon's head breathing fire (jet), drawbridge underside (low ceiling), wyvern (flyer), half-raised drawbridge (ramp) |
| 6 | *Cold Front* | Spy, snow | The Glacier on skis: snowman, pine (blocks), rolling snowball (runner), icicle overhang (low ceiling), abseiling agent (dangler), crevasse (gap) | Avalanche Pass on a snowmobile: tank trap (block), enemy snowmobile (runner), autogyro (flyer), steam vent (jet), ski-lift chairs (high ceiling), snow kicker (ramp) |

Scene 1 of each film keeps to the original's kinds (ceiling blocks can appear in either scene); scene
2 brings in jets and ramps. Each scene holds two to four hazards per track, and the last hazard on the
first screen must be passed on the ground. Films 2–6 are designed track by track in "Films 2 to 6"
below.

**Billings** are the difficulty levels, the starting one picked on the title screen.

| Billing | Gaps | Runner and dangler speed | Fee per track |
| --- | --- | --- | --- |
| 1 Extra | None | Slow | £10 to £120 |
| 2 Stand-in | None | Normal | £130 to £240 |
| 3 Double | Small gaps added | Normal | £250 to £360 |
| 4 Stunt Man | More gaps | Fast | £370 to £480 |
| 5 Stunt Coordinator | Gaps in front of most hazards | Fast | £490 to £600 |
| 6 Legend | Most track broken | Fastest | £610 to £720 |

In the original (M1) runners and danglers move at the same speed per step on every level (runner
0.95 px per step, dangler 1 px per 3 steps in our units); only the layouts get harder, and gaps start
on its fifth screen. "Slow" above can be read as the original's speed, with faster ones our own.

As built in F5: billing 1 runs the original's speeds, then runners go 1.25, 1.25, 1.5, 1.5 and 1.75
times faster and danglers move a line every 2.5, 2.5, 2, 2 and 1.5 steps. On water a gap shows as
rocks breaking the surface. The harness proves each scene at each billing with a searched run in
which every jump can be pressed anywhere in a window of 8 steps (160 ms) at billings 1-2, 6 at 3-4,
5 at 5 and 4 (80 ms) at 6.

Each scene is stored once with every gap it can have; each gap is tagged with the lowest billing that
shows it, so one layout serves all six billings. Every scene must be proved beatable at every billing
by a scripted run in the test harness before it ships.

## Films 2 to 6

Agreed with Bob 24 Nov 2026, with all the recommendations at the end of this file. **Bob's goal:
every scene distinct, and harder both as the billing rises and as the season goes on.** The
guidelines below are starting points, not rules: F6 breaks any of them when that makes a better
challenge. Only the machine's limits are hard: the step budget and memory.
Positions are starting points in pixels from the left of the 228-pixel play width; `tools/solve`
proves each scene at every billing and moves things where it must, as it did for film 1. Heights are
band lines, 0 at the top of a band to 47 at the bottom: the rider stands on lines 28–43, the road is
44–47, and at the top of a jump his head reaches line 8.

### Guidelines (break them for a better scene)

- **One idea a track.** Track 1 warms up, track 4 is the scene's hardest, and each track has one
  named idea (a "beat"): a pair to hop, a lane to time, "under, then over", a long ramp jump.
- **Lanes.** A runner or flyer has a track to itself, with nothing it could drive through (Bob,
  BUILD 10). Its lane only gets the "missing floor" gaps of billing 3 up, as the police car's has.
- **Heights.** Blocks and jets at most 16 lines tall. A low ceiling block's underside sits at band
  line 22–26: pass it on the ground. A high one's underside sits at line 8–10: it only clips the top
  of the arc, so a jump must take off or land beside it. A flyer's underside sits at line 22–26
  (just above the rider's head) and its top no higher than line 12.
- **Gaps.** "Missing floor" gaps are tagged billing 3 to 6 as in film 1. From film 3 on, each film
  also has one themed gap (open grave, crater, moat, crevasse) at every billing, as the original's
  gaps start on its fifth screen (Bob). Every gap gets a run-up before it and a landing
  zone after it.
- **Billings add and move hazards, not only gaps** (Bob). Every record gets a fifth byte with the
  lowest and highest billing that show it (3 bits each), so a higher billing can add a hazard, or
  swap one for the same hazard somewhere nastier. One layout still serves all six billings.
- **One new idea per film.** The season gets harder in order:

| Scenes | Film | New in the season |
| --- | --- | --- |
| 1–2 | *Rush Hour Rumble* (built) | Block, runner, dangler, low ceiling, ramp; gaps from billing 3 |
| 3–4 | *Dust at Dawn* | High ceiling; the jet (scene 4); a ramp jump over a jet |
| 5–6 | *Night of the Hearse* | The flyer; the first themed gap; two moving things that move every step |
| 7–8 | *Moonbase Panic* | Floaty handling; a ramp jump that carries across the wrap |
| 9–10 | *Dragon's Keep* | The fast runner (1.5 times the billing's runner speed); jets in pairs |
| 11–12 | *Cold Front* | Everything together; a ramp jump under high ceilings |

### Each billing harder, each scene distinct

Every scene climbs the same ladder from its one layout, so billing 2 of a scene is never just
billing 1 again:

| Billing | What changes in every scene |
| --- | --- |
| 1 Extra | The base layout at the original's speeds |
| 2 Stand-in | Movers faster (as built); jets and danglers set out of step with the easy route |
| 3 Double | Small missing-floor gaps; one hazard moved to tighten a pair |
| 4 Stunt Man | More gaps; one extra dangler, jet or flyer |
| 5 Stunt Coordinator | Gaps in front of most hazards; the warm-up track gets a second idea |
| 6 Legend | Wide breaks, the fastest movers, and at least one track that needs the stop |

And each scene has one thing no other scene has:

| Scene | Its signature |
| --- | --- |
| 2-1 Main Street | Jump arcs timed around the saloon balcony |
| 2-2 Runaway Mine | The beam squeeze: low ceilings with a block between |
| 3-1 Graveyard Lane | Two every-step movers: the ghoul's lane and the bat overhead |
| 3-2 Tunnel of Terror | Trapdoor timing: pop-up ghouls half a period apart |
| 4-1 Crater Plains | Floaty jumps over a crater and under a gantry |
| 4-2 Reactor Corridor | The launch pad's jump across the wrap, caught by the conduit if too fast |
| 5-1 Castle Approach | The fast knight, and a moat right after the portcullis |
| 5-2 Moat by Moonlight | The drawbridge ramp over a pair of fire-breathing heads |
| 6-1 The Glacier | Under the icicles, straight over a crevasse |
| 6-2 Avalanche Pass | The kicker's jump threaded between the ski-lift chairs |

### Step budget

Measured so far: the worst step is 19,580 of 20,000 cycles (F5), on a screen with one runner (the
police car) and one dangler (the wrecking ball). A step has room for the rider and one or two
hazard redraws of about 5,500 cycles each, and a hazard that doesn't fit is drawn a step late (F3).
New scenes keep inside that:

- **At most two hazards that move every step** (runners and flyers) per scene, and **at most three
  that move now and then** (danglers and jets).
- **Narrow movers.** Runner and flyer art at most 18 px wide (a 4-cell box), except one wider one
  per scene; the police car's 6-cell box is the widest proven.
- **Jets cost only when they change**, and the layout staggers their phases so no two change in the
  same step.
- **Proof.** The harness's worst-step and late-draw checks run on every scene's winning runs at
  every billing. Horror scene 1 is the first with two every-step movers, so it tests the rule. If it
  overruns, or a hazard runs more than a step late, the fallback is for runners and flyers to move
  every other step by twice the distance: half the redraws, a little coarser motion, and the model
  moves them the same way so crashes still match the picture.

### Colour

The F1 rule stays: one paper and one ink per line, and the rider's ink is the same from band line 4
to 43, so he never changes colour in a jump; hazards on those lines share it. Each scene's look comes
from the sky's paper (dithered lines blend two colours), the top four lines' ink (clouds, stars,
rock), and the ground's paper and ink. A ceiling block reaching into lines 0–3 gets that ink too,
which gives it a second colour for free.

| Scene | Title strip (paper / ink) | Sky paper, lines 0–27 | Paper, lines 28–43 | Ink, lines 4–43 (rider) | Ground, lines 44–47 (paper / ink) |
| --- | --- | --- | --- | --- | --- |
| 2-1 Main Street | Yellow / red | Magenta, dithered through red to yellow | Yellow | Black: silhouettes against the dawn | Red / yellow dust |
| 2-2 Runaway Mine | Black / yellow | Black; lines 0–3 red rock ink | Black | Yellow: lamplight | Black / white rails, red sleepers |
| 3-1 Graveyard Lane | Red / black | Blue dithered to black (moonlight); lines 0–3 white ink | Black | Green | Black / white cobbles |
| 3-2 Tunnel of Terror | Magenta / black | Red | Red | Black | Black / yellow rails |
| 4-1 Crater Plains | Blue / cyan | Black; lines 0–3 white stars | Black | Cyan | White / black regolith |
| 4-2 Reactor Corridor | Black / yellow | Blue; lines 0–3 white ceiling lights | Blue | Yellow | Yellow / black hazard stripes |
| 5-1 Castle Approach | Green / yellow | Cyan, dithered to green hills from line 20 | Green | Black | Yellow / green grass path |
| 5-2 Moat by Moonlight | Magenta / yellow | Magenta dithered to black | Black | Yellow: torchlight | Blue / green moat water |
| 6-1 The Glacier | Cyan / blue | Cyan dithered to white | White | Black | White / cyan ice |
| 6-2 Avalanche Pass | Blue / white | Blue | Blue | White | White / blue snow |

Inverse cells (bit 7: paper and ink swap to their complements for one cell) can add one sky feature
per scene in band lines 0–7, above anything the rider or a mover draws: the sun at dawn, the moon over
the graveyard, the Earth over the craters. Optional, and only where no ceiling block reaches.

### Ground styles

Film 1 has two grounds, road and water. Films 2–6 need more looks, but not more rules: every ground
handles the same (Bob: sea and snow handle like land), and a gap crashes when the wheels' middle is
over it on the surface. A ground is data: a byte pattern for each of the four ground lines (two
bytes where it alternates, as rail sleepers do) and one of three gap looks.

| Ground | Used in | Gap looks like |
| --- | --- | --- |
| Road (built) | 1-1 | Road missing |
| Water (built) | 1-2, 5-2 | Rocks breaking the surface |
| Dirt | 2-1 | Road missing (a dry wash) |
| Rails | 2-2, 3-2 | Road missing (broken track) |
| Cobbles | 3-1 | Dark fill (the open grave) |
| Regolith | 4-1 | Dark fill (the crater) |
| Deck plates | 4-2 | Road missing (an open hatch) |
| Grass path | 5-1 | Dark fill (the moat) |
| Snow | 6-1, 6-2 | Dark fill (the crevasse) |

### The two new behaviours

**Jet.** Record: x, art, and in its last byte a period code (high 4 bits) and a phase (low 4 bits,
in sixteenths of the period), so jets on one screen can be staggered.

1. Off: only the vent shows, drawn in the ground lines (44–47), which the rider never touches.
2. Warning, 25 steps: the vent flickers (still harmless).
3. On: the flame, steam or beam stands up to 16 lines above the road, drawn in two halves on
   successive steps; a pixel touch crashes, as with any hazard. Jumping clears it as it clears a
   block.

At billing 1 a jet's period is 150 steps (3.0 s, the dangler's) with 50 steps on (guess); it
shortens with the billing as the danglers' steps do (x 1, 0.83, 0.83, 0.67, 0.67, 0.5).

**Flyer.** Record: x, art, speed (signed, as a runner's). Its height is part of its art: the art
table gives the band line of its underside. It moves and wraps like a runner, at the billing's
runner speed. On the ground the rider passes under it; any jump under it hits it.

### Vehicles

Each vehicle is four 12 × 16 frames (two riding, jump, crash), its column masks for crashes, and an
engine voice. Engines use tone only, because the AY has a single noise generator and the effects
need it; the skis are the exception (below).

| Vehicle | Frames | Engine voice |
| --- | --- | --- |
| Horse | Legs at the gallop in two frames; tucked for the jump; on the crash Bob sails off and the horse sits down | Clip-clop: two short alternating tone blips, quicker with speed |
| Mine cart | Wheels turning; Bob crouched in the tub; on the crash the cart tips | A low rumble with a click every 12 px travelled (a sleeper) |
| Hearse-trike | A trike with a coffin box behind; the box lid lifts on the crash | A lower, rattling buzz than the motorbike (volume in an uneven 3-step pattern) |
| Ghost-train car | A car with a skull on the nose; Bob's hair stands up on the jump | A deep rumble with a slow wobble in pitch |
| Hover bike | No wheels; a glow line underneath that flickers between the two frames | A smooth hum, steady volume, gentle vibrato |
| Dragon hatchling | Running legs, folded wings that open for the jump | Quick pattering blips with a puff every so often |
| Magic carpet | Fringes rippling between the two frames; the carpet curls on the crash | A soft rising and falling tone |
| Skis | Bob crouched, poles back; skis apart on the crash | A low-volume swish on noise, its period following speed; an effect that needs the noise takes it over for its length |
| Snowmobile | Track turning, a ski in front | A harsher, higher two-stroke buzz than the motorbike |

Each film's clapper card plays a 2–3 s sting of its own (F6 music), all composed for the game: a
whistled four-note figure for the Western, falling organ chords for the horror, a rising arpeggio
for the sci-fi, a fanfare in fourths for the fantasy and a twangy bass riff for the spy film.

### *Dust at Dawn* (Western)

**Scene 1, Main Street, on a horse.** A dusty street at sunrise, everything black against the dawn.

| Hazard | Behaviour | Size (px) |
| --- | --- | --- |
| Water trough | Block | 18 × 10 |
| Hitching rail | Block | 16 × 12 |
| Tumbleweed | Runner, rolling towards the rider | 10 × 10 |
| Saloon balcony | High ceiling block, underside at line 9 | 40 wide |
| Grain sack on the store's hoist | Dangler | 8 × 10, on a rope |

- Track 1, a low pair: trough at 90, hitching rail at 170.
- Track 2, the tumbleweed's lane: it starts at 150.
- Track 3, under the balcony: balcony at 50–90, trough at 160. A jump over the trough must not
  take off while under the balcony.
- Track 4, hop and wait: hitching rail at 40, sack at 130. Land, then time the sack, or stop.
- Gaps from billing 3 (a dry wash). Movers: one every step, one now and then.

**Scene 2, Runaway Mine, in a mine cart.** Lamplit tunnels: pit beams overhead everywhere.

| Hazard | Behaviour | Size (px) |
| --- | --- | --- |
| Ore pile | Block | 14 × 10 |
| Fallen rock | Block | 12 × 14 |
| Stray ore cart | Runner, coming the other way | 16 × 12 |
| Pit beam | Low ceiling block, underside at line 24 | 12 wide, with a prop down to it |
| Gas flare | Jet, from a crack in the floor | 8 × 16 when on |
| Buckled rail | Ramp | 16 × 6 |
| Rock bucket on a chain | Dangler, billing 4 up | 8 × 8 |

- Track 1, under then over: beam at 60, ore pile at 150.
- Track 2, ramp over the flare: buckled rail at 30, fallen rock at 100 (the long jump clears it),
  flare at 190 (land first and time it).
- Track 3, the ore cart's lane: it starts at 170.
- Track 4, the squeeze: beams at 40 and 120, ore pile at 180 between them and the edge; the rock
  bucket joins at 80 from billing 4.
- Movers: one every step, one jet (two now-and-then from billing 4).

### *Night of the Hearse* (Horror)

**Scene 1, Graveyard Lane, on the hearse-trike.** A cobbled lane through the graveyard by
moonlight.

| Hazard | Behaviour | Size (px) |
| --- | --- | --- |
| Gravestone | Block | 10 × 14 |
| Stone angel | Block | 12 × 16 |
| Shuffling ghoul | Runner, slow (0.75 times the billing's runner speed) | 10 × 16 |
| Bat | Flyer, underside at line 24 | 12 × 6, two-frame wings |
| Lych-gate | Low ceiling block, underside at line 22 | 24 wide, with posts |
| Open grave | Gap, every billing | 20 |
| Spider on a thread | Dangler | 8 × 8 |

- Track 1, two stones: gravestone at 80, stone angel at 150 (one long jump or two short ones).
- Track 2, the ghoul's lane: it shuffles from 160.
- Track 3, under then over: lych-gate at 40–64, open grave at 130. Keep your speed under the gate.
- Track 4, the bat's lane, with a spider: the spider at 110; the bat crosses the track. Pass the
  spider on the ground when it is up, and don't jump while the bat is over you.
- Movers: two every step (ghoul, bat), one dangler. The test of the step rule.

**Scene 2, Tunnel of Terror, in a ghost-train car.** A fairground ghost train, red-lit.

| Hazard | Behaviour | Size (px) |
| --- | --- | --- |
| Coffin | Block | 18 × 8 |
| Pop-up ghoul | Jet: springs up from a trapdoor | 10 × 16 when up |
| Crypt arch | Low ceiling block, underside at line 22 | 30 wide |
| Dropping skeleton | Dangler | 10 × 14 |
| Track hump | Ramp | 16 × 6 |

- Track 1, coffins: at 90 and 170.
- Track 2, two trapdoors: pop-up ghouls at 70 and 160, half a period apart.
- Track 3, arch and skeleton: arch at 40–70, skeleton at 150.
- Track 4, the hump: hump at 20, coffin at 110 (cleared by the long jump), skeleton at 190 (land
  first).
- Movers: none every step; two danglers and two jets.

### *Moonbase Panic* (Sci-fi)

Both scenes on the hover bike: its floaty jump (100 steps, 150 from a ramp) carries further, so the
spacing is wider than on land.

**Scene 1, Crater Plains.** Black sky, stars, a bright surface.

| Hazard | Behaviour | Size (px) |
| --- | --- | --- |
| Fuel canister | Block | 10 × 12 |
| Antenna mast | Block | 6 × 16 |
| Patrol robot | Runner | 12 × 14 |
| Drone | Flyer, underside at line 23 | 12 × 6 |
| Cargo claw on a gantry | Dangler | 12 × 10 |
| Gantry arm | High ceiling block, underside at line 10 | 36 wide |
| Crater | Gap, every billing | 24 |

- Track 1, learn the float: canister at 70, mast at 170.
- Track 2, the robot's lane: from 160.
- Track 3, crater and claw: crater at 60, claw at 160.
- Track 4, the drone's lane, under the gantry: gantry at 40–76, mast at 140; the drone crosses.
- Movers: two every step (robot, drone), one dangler.

**Scene 2, Reactor Corridor.** A blue corridor with yellow warning stripes.

| Hazard | Behaviour | Size (px) |
| --- | --- | --- |
| Crate | Block | 14 × 12 |
| Laser gate | Jet: an emitter in the floor, beam up to 16 lines | 4 × 16 when on |
| Blast door | Low ceiling block, underside at line 24 | 20 wide |
| Conduit | High ceiling block, underside at line 9 | 60 wide |
| Crusher piston | Dangler | 14 × 12 |
| Launch pad | Ramp | 16 × 6 |

- Track 1, crate and gate: crate at 80, laser gate at 160.
- Track 2, the door: blast door at 30–50, crusher at 130, crate at 200.
- Track 3, the big jump: launch pad at 40, laser gates at 110 and 170 under the jump, which
  carries across the wrap into track 4 at height (Bob: the wrap keeps height and arc).
- Track 4, the landing: clear for 80 px, then a high conduit (high ceiling block, underside at
  line 9) at 90–150 that catches a launch taken too fast, and the crusher at 190. Take the pad at
  middling speed and land in the clear stretch.
- Movers: none every step; two danglers and three jets, staggered.

### *Dragon's Keep* (Fantasy)

**Scene 1, Castle Approach, on a dragon hatchling.** Green hills under a bright sky.

| Hazard | Behaviour | Size (px) |
| --- | --- | --- |
| Boulder | Block | 14 × 12 |
| Barrel stack | Block | 12 × 16 |
| Charging knight | Fast runner (1.5 times the billing's runner speed), towards the rider | 18 × 16 (the scene's wide one) |
| Portcullis | Dangler: rises and drops | 18 × 16 |
| Moat | Gap, every billing | 28 |

- Track 1, a hop and a high one: boulder at 60, barrel stack at 150.
- Track 2, the knight's lane: he charges from 180.
- Track 3, the moat: moat at 80 (needs some speed), boulder at 180.
- Track 4, gate then moat: portcullis at 70, moat at 150. Pass under the gate while it is up, with
  enough speed left to clear the moat.
- Movers: one every step, one dangler.

**Scene 2, Moat by Moonlight, on a magic carpet.** Skimming the moat by torchlight.

| Hazard | Behaviour | Size (px) |
| --- | --- | --- |
| Rock | Block | 12 × 10 |
| Dragon's head | Jet: rises from the water breathing fire | 10 × 16 when up |
| Drawbridge underside | Low ceiling block, underside at line 22 | 36 wide |
| Wyvern | Flyer, underside at line 24 | 16 × 8 |
| Half-raised drawbridge | Ramp | 18 × 8 |

- Track 1, rock and fire: rock at 80, dragon's head at 170.
- Track 2, under the bridge: drawbridge at 50–86, rock at 150, dragon's head at 205.
- Track 3, the wyvern's lane, with a rock at 100.
- Track 4, over the fire: half-raised drawbridge at 30, dragon's heads at 100 and 150 half a
  period apart (the long jump clears both, on or off), rock at 210.
- Movers: one every step (the wyvern), three jets, staggered.

### *Cold Front* (Spy, snow)

**Scene 1, The Glacier, on skis.** A white snowfield under a pale sky.

| Hazard | Behaviour | Size (px) |
| --- | --- | --- |
| Snowman | Block | 12 × 16 |
| Pine | Block | 10 × 16 |
| Rolling snowball | Runner, towards the rider | 12 × 12 |
| Icicle overhang | Low ceiling block, underside at line 24 | 30 wide |
| Abseiling agent | Dangler, on a rope from above | 10 × 14 |
| Crevasse | Gap, every billing | 22 |

- Track 1, two trees' worth: snowman at 90, pine at 170.
- Track 2, the snowball's lane: from 160.
- Track 3, under then over: overhang at 40–70, crevasse at 130.
- Track 4, the agent: agent at 80, crevasse at 150, snowman at 205.
- Movers: one every step, one dangler.

**Scene 2, Avalanche Pass, on a snowmobile.** The season's finale, at night.

| Hazard | Behaviour | Size (px) |
| --- | --- | --- |
| Tank trap | Block | 14 × 12 |
| Enemy snowmobile | Runner | 16 × 12 |
| Autogyro | Flyer, underside at line 22 | 18 × 10 |
| Steam vent | Jet | 8 × 16 when on |
| Ski-lift chair | High ceiling block, underside at line 9 | 12 wide, on the cable |
| Snow kicker | Ramp | 16 × 6 |

- Track 1, trap and vent: tank trap at 80, steam vent at 160.
- Track 2, the snowmobile's lane: from 170.
- Track 3, the kicker under the lift: kicker at 20, steam vents at 100 and 150 under the long
  jump, ski-lift chairs at 70 and 180 overhead: the jump's top must pass between them.
- Track 4, the autogyro's lane, with a tank trap at 60 and a steam vent at 180.
- Movers: two every step (snowmobile, autogyro), three jets, staggered.

### Memory

F5 left 5,988 bytes free below `$9800`. Films 2–6 at film 1's way of storing art would need about
20 KB more (each vehicle is 1,152 bytes pre-shifted and each moving hazard about 300), so the art
has to be stored unshifted and shifted when a scene starts, while the clapper card is up (shifting a
vehicle takes about 60,000 cycles, under the card's 300,000).

| Item | Estimate |
| --- | --- |
| 10 scene layouts and palettes (palettes stored as runs of lines; 5-byte records) | 1.5 KB |
| 9 vehicles, unshifted (4 frames × 32 bytes) with crash masks | 1.8 KB |
| One vehicle buffer, 1,152 bytes, in place of today's 2,304 bytes of pre-shifted bike and jet-ski | −0.9 KB |
| About 45 hazard pictures with crash masks | 2.5 KB |
| Moving hazards unshifted, plus a buffer for a scene's four movers, in place of today's 1,008 bytes | 1.3 KB |
| Code: jet, flyer, ground styles, billing tags, shifting at scene start, engine voices | 1.5 KB |
| F6 music and stings | 2.0 KB |
| **Needed** | **9.7 KB** |

| Where it comes from | Estimate |
| --- | --- |
| Free after F5 | 6.0 KB |
| Pause's 1.4 KB save area shares the shift buffers (re-shift after a pause) | 1.4 KB |
| The alternate character set's 1 KB at `$9C00`, if nothing uses it (to check) | 1.0 KB |
| Palettes as runs (counted above) and `src/frame.c` text tables trimmed | 1.0 KB |
| The trace output left out of Bob's builds (trace equality on a test build) | 0.3 KB |
| **Available** | **9.7 KB** |

That is about even, so the fallbacks stand ready, in this order: the music cut to 1 KB; vehicles at
3 pre-shifts in the buffer (2-pixel steps; this saves buffer space only now); one vehicle per film
(saves about 1 KB).

### Build order for F6

1. **F6a, the shared parts:** shifting at scene start, ground styles, billing tags on every record,
   the jet and the flyer, the memory moves above; a stress scene with two every-step movers and three
   jets proves the step rules before any film is laid out.
2. **F6b to F6f, one film per build** in season order (Western, horror, sci-fi, fantasy, spy), each
   proved at all six billings with `tools/solve`, each a build for Bob.
3. **F6g:** music and stings.

## Presentation

The film set frames every moment between attempts, and each piece lasts two seconds or less so
retakes stay quick. Any game key skips a card.

1. **Title screen.** "STUNT MAN BOB" on a marquee with chasing lights, Bob posing by his bike, and the
   menu: 1 Start, L Billing, S Sound. The top five fees of the session scroll below it.
2. **Clapperboard card.** Before each scene, a full-width clapper shows the film, scene and take
   ("COLD FRONT / SCENE 2 / TAKE 1"). The clapper arm snaps shut with a clack, then "ACTION!" flashes
   and play starts.
3. **Playing.** The status line shows the take, the stop icon (lit while the stop is unused) and the
   fee.
4. **A crash.** A burst on the hazard, Bob flung up and away, and a director's megaphone pops up in
   the title strip with a big "CUT!" and a squawk. After about 1.5 s the clapper card returns with the
   take number one higher, and the scene restarts from track 1.
5. **Scene cleared.** "THAT'S A PRINT!" with a fee tally for the scene, then the next scene's card.
   Clearing a scene on its first take pays a one-take bonus of five times its track fee (guess).
6. **Third crash.** "YOU'RE FIRED!" over the final fee, then initials for the top five if the fee
   qualifies, then the title screen.
7. **Season finished.** "THAT'S A WRAP!" over a premiere poster with all six titles, then the next
   season at the next billing up.

As built in F4 (BUILD 12): the card also shows the billing, the fee so far and the scene's fee a
track; "CUT!" pops up 0.4 s after the crash (the crash picture shows first); "THAT'S A PRINT!" lists
the scene's track fees (all its takes), the bonus, a lost take coming back and the run's fee; Esc's
question is on the status line, and Y ends the run without a top-five entry. The top five is shown
whole, not scrolling, and starts with five film-crew fees to beat. There is no key redefinition (Bob, 24 Nov 2026).

Art rules: everything is drawn fresh at Oric resolution in the film's banded palette. The rider is
about 12 × 16 pixels with two riding frames, a jump frame and a crash frame per vehicle.

## Scoring, takes and fees

- **Fee.** As in the original (M1, Bob's decision), each finished track pays the scene's track fee:
  £10 × the scene's number in the run, counting on through the billings (billing 1 is scenes 1 to
  12, £10 to £120 a track; billing 2 starts at scene 13, £130; and so on). The original paid 10 × its
  screen number. A scene cleared on its first take adds the one-take bonus (our addition).
- **Takes.** Three per run. A crash uses one and restarts the scene from track 1 with its stop
  restored. Fees already earned are kept, and a retake pays again for every track it finishes, as in
  the original. Example on scene 1: take 1 finishes two tracks (£20) and crashes on the third; take 2
  finishes all four (£40 more, £60 in all). A crash costs a take and the one-take bonus, never money
  already earned.
- **Take back.** As in the original (M1), a lost take is given back after every third scene cleared
  (the original did it after each land, sea, snow round of three screens). Never more than three.
- **Best fees.** A top-five table with three initials, kept until the machine is switched off.

## Sound and music

| Sound | Channel | Description |
| --- | --- | --- |
| Engine | A (tone) | Pitch rises with speed; each vehicle has its own timbre (a buzzy bike, hoof clops for the horse, a hum for the hover bike) |
| Jump and landing | B (tone + noise) | A rising whoosh on take-off, a short thud on landing |
| Crash | B + noise | Noise burst with a falling tone |
| "Cut!" | B | Two-note megaphone squawk (no speech: the word is shown) |
| Clapper | Noise | A sharp clack |
| Fee paid | C | A short blip each time a track is finished |
| Jingles | A–C | Title theme (about 20 s, loops), a 2–3 s sting per genre on the clapper card, "That's a print", "You're fired", "That's a wrap" fanfare |

All tunes are composed for this game. Envelopes step from the 10 ms timer, as in Chuckie Egg. Noise
uses period 31, never 32 (the AY's 5-bit quirk).

**Oricutron test builds.** Oricutron plays AY noise an octave higher than a real Oric; its tones are
right. Every build made for testing in Oricutron doubles each noise period (`make ORICUTRON=1`),
which drops noise an octave and gives Bob the real machine's sound; the release build for hardware
leaves that off. The noise register stops at 31, so effects use noise periods of 15 or less, leaving
room to double them.

## Technical plan for the 48K Oric

Reuse the Chuckie Egg setup that passed on Bob's real machines (see `docs/ORIC-LESSONS.md`): cc65
2.19 with none.lib, our own crt0 and an ld65 config at $0500, no ROM calls (the IRQ is hooked through
the RAM vector on ROM 1.0 and 1.1), and one autorun .tap. Game logic in C; drawing, the key scan,
sound and the timer interrupt in 6502 assembly.

**Memory.** About 37 KB is free between $0500 and the character sets at $9800 (all guesses until
built):

| Item | Estimate |
| --- | --- |
| Game logic (C) | 10 KB |
| Drawing, keys, sound, IRQ (assembly) | 4 KB |
| Vehicles: 7 × 4 frames × 6 pre-shifts | 8 KB |
| Hazard art, 6 films | 6 KB |
| Title, clapper, cards, fonts | 4 KB |
| Music and effects | 2 KB |
| 12 scene layouts | 1 KB |
| **Total** | **35 KB of 37 KB** |

If it gets tight, the vehicles drop to 3 pre-shifts (2-pixel steps), which saves 4 KB.

These were first guesses. As built after F5, 5,988 bytes are left, and films 2–6 fit only if art is
stored unshifted and shifted when a scene starts: see "Films 2 to 6: Memory".

**Speed.** A 50 Hz step allows 20,000 cycles. The worst case is the rider plus four moving hazards
at about 1,400 cycles each with one-pass, flicker-free redraws, plus logic and keys: about 12,000
cycles. Scenes are limited to four moving hazards. If a scene ever overruns, the fallback is a 33 Hz
step, as Chuckie Egg used, with speeds scaled to match.

**Data.** A scene is four track records: up to four hazards each (behaviour, x, speed or period,
phase) and a 38-bit gap mask with a minimum billing per gap. Collisions compare the pictures' column
masks from the art data in the logic, never screen pixels, so the C model and the Oric build give
identical traces.

**Testing.** The Chuckie Egg harness (a small emulator after Oricutron's source, with PNG output, key
scripts and a debug port at $BFE0) runs every build. Each scene gets a scripted winning run per
billing, and a trace check proves the native C model and the Oric build agree step for step. Bob
checks each build in Oricutron, then on hardware.

**Level codes (optional extra, after film 1).** A scene can also be generated from a 16-bit seed. A
small generator picks each track's hazards from the film's set and spaces them by rules that keep
everything passable at the chosen billing: a run-up before every jump, a landing zone after it, room
to stop before a dangler. The clapper card shows the scene's code (four letters plus the billing),
and "C" on the title screen takes a code to replay that exact scene; hand-made scenes get codes too.
About 2 KB, so it comes with the 3-pre-shift fallback. Before it ships, the harness generates
thousands of seeds and runs a solver over each to tune the rules.

The build order is in `docs/PLAN.md`.

## Decisions

| Topic | Decision |
| --- | --- |
| Machine | 48K Oric-1 and Atmos only |
| Stop | One per take: restored on each new scene and each retake, not at a wrap; works in mid-air (as the original, confirmed by Bob) |
| Speed | Accelerate, hold your current speed, or stop dead once; the bike never slows by itself |
| Wrapping | Same height, same point in the jump arc, same speed |
| Run | Six films of two scenes; finishing them starts the next billing automatically, like the original's levels |
| Billing | Starting billing picked on the title screen |
| Genres | Action, Western, horror, sci-fi, fantasy, snowbound spy (on skis); the action film's scene 2 is a jet-ski chase |
| Handling | Standard for every vehicle, as in the original; only the hover bike floats |
| Hazards | Ceiling blocks are an original kind; jets and ramps are new, in scene 2 |
| Keys | Space jump, either Shift accelerate, Return stop; fixed, no redefinition (Bob, 24 Nov 2026); no joystick |
| Takes and fees | 3 takes, one lost take back after every third scene cleared (as the original); fees in pounds, paid per track finished and again on a retake (as the original) |
| Oricutron builds | Noise played an octave lower to match real hardware |
| Level codes | Optional extra after film 1 |
| Measuring | Time the BBC original frame by frame to set speeds, arc and handling |
| After M1 | Bob agreed to follow the original on the stop, sea and snow handling, the held jump and the take refund. Levels: our billings already work like its skill levels (a start point in one continuous run); a billing stays one 12-scene season. Fees: per track, as in the original (Bob's decision). |

## Decisions on films 2 to 6

Bob accepted every recommendation on 24 Nov 2026 and added two notes: drop key redefinition, and
make each scene distinct and harder with the billing and the season, breaking any guideline that
gets in the way of a fun challenge.

1. **Seasons** stay 12 scenes a billing, six billings, as built.
2. **Gaps:** from film 3, one themed gap per film at every billing (open grave, crater, moat,
   crevasse); missing-floor gaps still from billing 3.
3. **Flyers** join from film 3 (bat, drone, wyvern, autogyro).
4. **Vehicles:** one per scene, nine new; the fantasy film's chariot becomes a dragon hatchling.
5. **Billings** add and move hazards as well as gaps (see "Each billing harder, each scene
   distinct").
6. **Keys:** no redefinition; Space, either Shift and Return are fixed.
7. **Guidelines, not rules:** the layout guidelines in "Films 2 to 6" can be broken for a better
   scene; the step budget and memory are the only hard limits.
