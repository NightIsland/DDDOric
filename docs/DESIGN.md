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
| Score | Wages per track completed, 10 × the screen number, paid again on a retake (M1) | £10 per hazard, more on harder billings |
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

Three game keys, all redefinable from the title screen, with defaults that match the original's
layout: one hand on Space, the other with a finger on Shift and one ready on Return. No joystick.

| Action | Default key | Behaviour |
| --- | --- | --- |
| Jump | Space | Starts a jump from the ground; held, it jumps again 3 steps after each landing, as in the original |
| Accelerate | Either Shift | Held: speed builds, on the ground or in the air; released: the bike holds its current speed |
| Stop | Return | Stops the bike dead, on the ground or in the air; one use per take |
| Pause | P | Freezes the game; any key resumes |
| Quit | Esc | Back to the title screen (asks Y/N) |
| Sound | S (title screen) | Sound on or off |

- **Redefining keys.** "K" on the title screen asks for each game key in turn. It refuses a key
  already taken and the non-game keys above.
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
4. **Crashing.** Bob crashes if his box (shrunk by 2 px on each side, for fairness) touches a hazard,
   if he lands on top of an obstacle, if his wheels are over a gap when he is on the ground, or if his
   jump hits a ceiling block or anything else that must be passed on the ground.
5. **Clearing.** A hazard is cleared, and pays its fee, once Bob's back wheel passes its far edge
   without a crash.

## Obstacles and vehicles

Every hazard is one of seven behaviours; each film dresses them in its own art. The first five are
the original's kinds, the last two are new.

| Behaviour | How it acts | How to pass it | New? |
| --- | --- | --- | --- |
| Block | Stands still on the track | Jump it | Original kind |
| Runner | Drives along the track at a set speed, either way, and wraps at the edges | Arrive when it is elsewhere, or jump it | Original kind |
| Dangler | Hangs on a line from the band above and bobs up and down on a set period | Pass under it on the ground while it is up; jumping hits it | Original kind |
| Gap | A missing piece of road, 12–48 px wide | Jump it | Original kind, harder billings |
| Ceiling block | Fixed to the top of the band. A low one must be passed on the ground; a high one only cuts off the top of a jump arc | Stay on the ground, or time the jump so the arc passes beside it | Original kind (Bob) |
| Jet | Flame or steam bursts up from the road on a timer | Pass while it is off, or jump while it is on | New |
| Ramp | A fixed launch ramp | Riding over it fires a jump 1.5× longer than normal (guess) | New, helpful |

The interesting scenes come from pairs. A low ceiling block just before a block forces you to land in
the gap between them; a dangler after a gap means you must land, slow down, and time your arrival,
which is where the one stop earns its keep.

**Vehicles.** Each vehicle has its own look, engine sound and handling, where handling is three
numbers: acceleration, top speed and jump time. As in the original (M1), where the wetbike and skis
handle exactly like the bike, every vehicle uses the standard handling (+0.0111 px per step, top
2.78 px per step, jump 84 steps). The one exception is our own sci-fi hover bike, which floats.

| Film | Vehicle | Handling |
| --- | --- | --- |
| Action, scene 1 | Motorbike | Standard |
| Action, scene 2 | Jet-ski | Standard |
| Western | Horse | Standard |
| Horror | Rattling hearse-trike | Standard |
| Sci-fi | Hover bike | Floaty: jump time 100 steps, same height (guess) |
| Fantasy | Chariot | Standard |
| Spy (snow) | Skis | Standard |

## Films, scenes and progression

A run is a season of six films, each two scenes long (12 screens, 48 tracks). Finish the season and
the next one starts at the next billing up, with the same films made harder. All film names and
layouts are new.

| # | Film | Genre | Scene 1 hazards | Scene 2 |
| --- | --- | --- | --- | --- |
| 1 | *Rush Hour Rumble* | Action | Parked van (block), police car (runner), oil drums (block near the start), wrecking ball (dangler) | Harbour chase on a jet-ski: buoy (block), patrol boat (runner), low footbridge (ceiling block), slipway (ramp) |
| 2 | *Dust at Dawn* | Western | Cactus (block), tumbleweed (runner), corral fence (block), swinging saloon sign (dangler) | Adds stampeding steer (fast runner), dry gulch (gap), branding-fire (jet) |
| 3 | *Night of the Hearse* | Horror | Gravestone (block), shuffling ghoul (runner), open grave (gap), spider on a thread (dangler) | Adds crypt arch (ceiling block), will-o'-wisp (jet) |
| 4 | *Moonbase Panic* | Sci-fi | Antenna (block), patrol robot (runner), crater (gap), descending crusher (dangler) | Adds laser gate (jet), launch pad (ramp) |
| 5 | *Dragon's Keep* | Fantasy | Boulder (block), charging knight (runner), moat (gap), dragon's head (dangler) | Adds portcullis (ceiling block), fire breath (jet) |
| 6 | *Cold Front* | Spy, snow, on skis | Snowman (block), rolling snowball (runner), crevasse (gap), icicles (dangler) | Adds ski-lift chair (ceiling block), geyser (jet) |

Scene 1 of each film keeps to the original's kinds (ceiling blocks can appear in either scene); scene
2 brings in jets and ramps. Each scene holds two to four hazards per track, and the last hazard on the
first screen must be passed on the ground.

**Billings** are the difficulty levels, the starting one picked on the title screen.

| Billing | Gaps | Runner and dangler speed | Fee per hazard |
| --- | --- | --- | --- |
| 1 Extra | None | Slow | £10 |
| 2 Stand-in | None | Normal | £20 |
| 3 Double | Small gaps added | Normal | £30 |
| 4 Stunt Man | More gaps | Fast | £40 |
| 5 Stunt Coordinator | Gaps in front of most hazards | Fast | £50 |
| 6 Legend | Most track broken | Fastest | £60 |

In the original (M1) runners and danglers move at the same speed per step on every level (runner
0.95 px per step, dangler 1 px per 3 steps in our units); only the layouts get harder, and gaps start
on its fifth screen. "Slow" above can be read as the original's speed, with faster ones our own.

Each scene is stored once with every gap it can have; each gap is tagged with the lowest billing that
shows it, so one layout serves all six billings. Every scene must be proved beatable at every billing
by a scripted run in the test harness before it ships.

## Presentation

The film set frames every moment between attempts, and each piece lasts two seconds or less so
retakes stay quick. Any game key skips a card.

1. **Title screen.** "STUNT MAN BOB" on a marquee with chasing lights, Bob posing by his bike, and the
   menu: 1 Start, L Billing, K Keys, S Sound. The top five fees of the session scroll below it.
2. **Clapperboard card.** Before each scene, a full-width clapper shows the film, scene and take
   ("COLD FRONT / SCENE 2 / TAKE 1"). The clapper arm snaps shut with a clack, then "ACTION!" flashes
   and play starts.
3. **Playing.** The status line shows the take, the stop icon (lit while the stop is unused) and the
   fee.
4. **A crash.** A burst on the hazard, Bob flung up and away, and a director's megaphone pops up in
   the title strip with a big "CUT!" and a squawk. After about 1.5 s the clapper card returns with the
   take number one higher, and the scene restarts from track 1.
5. **Scene cleared.** "THAT'S A PRINT!" with a fee tally for the scene, then the next scene's card.
   Clearing a scene on its first take pays a one-take bonus of five times the hazard fee (guess).
6. **Third crash.** "YOU'RE FIRED!" over the final fee, then initials for the top five if the fee
   qualifies, then the title screen.
7. **Season finished.** "THAT'S A WRAP!" over a premiere poster with all six titles, then the next
   season at the next billing up.

Art rules: everything is drawn fresh at Oric resolution in the film's banded palette. The rider is
about 12 × 16 pixels with two riding frames, a jump frame and a crash frame per vehicle.

## Scoring, takes and fees

- **Fee.** Each hazard cleared pays the billing's fee (£10 to £60). A scene cleared on its first take
  adds the one-take bonus. Gaps count as hazards.
- **Takes.** Three per run. A crash uses one and restarts the scene from track 1 with its stop
  restored. Fees already earned are kept, but each hazard pays only once per scene, so a retake earns
  only for hazards beyond the point where you fell. Example at Extra: take 1 clears 5 hazards (£50)
  and crashes on the 6th; take 2 pays nothing for the first 5, then £10 for each new one. A crash
  costs a take and the one-take bonus, never money already earned. The original instead pays per
  track and pays again on a retake (M1); for Bob to confirm which we keep.
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
| Fee paid | C | A short blip each time a hazard is cleared |
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

**Speed.** A 50 Hz step allows 20,000 cycles. The worst case is the rider plus four moving hazards
at about 1,400 cycles each with one-pass, flicker-free redraws, plus logic and keys: about 12,000
cycles. Scenes are limited to four moving hazards. If a scene ever overruns, the fallback is a 33 Hz
step, as Chuckie Egg used, with speeds scaled to match.

**Data.** A scene is four track records: up to four hazards each (behaviour, x, speed or period,
phase) and a 38-bit gap mask with a minimum billing per gap. Collisions use boxes in the logic, never
screen pixels, so the C model and the Oric build give identical traces.

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
| Keys | Space jump, either Shift accelerate, Return stop; redefinable; no joystick |
| Takes and fees | 3 takes, one lost take back after every third scene cleared (as the original); fees in pounds, rule still open (see Scoring) |
| Oricutron builds | Noise played an octave lower to match real hardware |
| Level codes | Optional extra after film 1 |
| Measuring | Time the BBC original frame by frame to set speeds, arc and handling |
| After M1 | Bob agreed to follow the original on the stop, sea and snow handling, the held jump and the take refund. Levels: our billings already work like its skill levels (a start point in one continuous run); a billing stays one 12-scene season. Still open: how fees are paid (see Scoring) |
