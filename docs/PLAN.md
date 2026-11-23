# Stunt Man Bob: Plan and Status

Owner: the architect thread (the project's design thread). Feature threads update **only** their
own Status row and Results notes. Read `docs/DESIGN.md` and `docs/ORIC-LESSONS.md` first.
Last revised: 23 Nov 2026 (v0.1: first plan, after Bob agreed the design).

## Roles

- **Architect thread:** owns `DESIGN.md`, this plan, the memory map and budgets; writes each
  feature's brief; reviews results. Brings only real choices to Bob.
- **Feature threads:** one feature each, to its brief. Anything that changes the design, an
  interface, the memory map or a budget goes back to the architect, not into code.
- **Bob:** tests each build in Oricutron, later on real hardware, and merges pull requests.
  Nothing is "fixed" until Bob has seen it in Oricutron.

## How work runs (cloud only)

Bob works in the browser; there is no folder on his Mac. Every thread runs in a cloud session:

1. Clone `NightIsland/DDDOric`, branch from `main` (`f0`, `m1`, ...).
2. Build and test in the cloud with the harness.
3. Open a pull request with the `.tap` built for Oricutron attached to the thread (and the
   `docs/PLAN.md` row and Results updated). Bob loads it in Oricutron, then merges.
4. One feature per pull request. A thread that needs a design decision asks in its own thread.

**BUILD numbers:** `src/version.h`, bumped on every build handed to Bob; whoever merges second
takes the next number.

## Rules for every feature

- Stay inside the budget; if you can't, stop and report measured options.
- Builds for Bob are made with `make ORICUTRON=1` (noise an octave lower, DESIGN "Sound"); release
  builds without it.
- Every gameplay change keeps the native C model and the Oric build trace-identical.
- Regression tests reproduce Bob's own key sequence when he reports a bug.
- Finish with: Status row updated, Results notes below, a short report in the thread.

## Status

| ID | Feature | Depends on | Budget | State |
| --- | --- | --- | --- | --- |
| M1 | Measure the BBC original: speeds, acceleration, jump arc and time, start speed, wrap, hanging-hazard period, sea and snow handling, fee on a retake | — | docs only | done, PR open (`docs/M1-MEASUREMENTS.md`) |
| F0 | Skeleton and harness: cc65 2.19 built for the cloud shell, `none.lib`, own crt0 and ld65 config at $0500, ROM-free IRQ hook, autorun `.tap`, Makefile with size report and `ORICUTRON=1`; Chuckie Egg's emulator adapted, PNG output, key scripts, debug port at $BFE0; native C model with per-step trace | — | crt0 + cfg ≤ 256 B | merged (BUILD 1); BUILD 3 fixes Bob's fading and tearing rider |
| F1 | Display: HIRES init, per-line paper/ink columns, a track screen drawn from scene data, status line in the text-row bitmap font, one-pass pre-shifted sprite draw | F0 | code ≤ 1.5 KB; sprite ≤ 1,400 cycles | BUILD 5, PR open (over both budgets, see Results) |
| F2 | One track: rider physics (DESIGN "Physics"), keys (Space, either Shift, Return), 50 Hz step, engine sound | F0, F1 | logic ≤ 4,000 cycles a step | not started |
| F3 | Whole scene: four tracks, wrapping, block, runner, dangler, gap and ceiling block, crashes, fees, the one stop | F2 | whole step ≤ 16,000 cycles | not started |
| F4 | Film frame: clapper card, takes, "Cut!", scene cleared, "You're fired", title screen, billing select, top five | F3 | | not started |
| F5 | Film 1 (*Rush Hour Rumble*) complete at all six billings, tuned to M1's numbers; Bob judges the feel against the original | F4, M1 | | not started |
| F6 | Films 2–6, jets and ramps, per-vehicle handling, music, key redefinition | F5 | memory ≤ 37 KB | not started |
| X1 | Optional: level codes from a seed, with a solver run over thousands of seeds in the harness | F5 | ≤ 2 KB | not started |

M1 and F0 can run in parallel. F5 is the prototype milestone (plan step 3); F6 and X1 are polish
(step 4).

## Briefs

### M1: Measure the original

Run the 1984 BBC disc in JSBeeb (the research thread's setup; `www.bbcmicro.co.uk` is allowed by
the environment's network policy) and step it frame by frame. For each row of DESIGN "Feel targets"
and "Physics", record the measured value and how it was measured, at the Novice level and one
harder level. Also record: the start speed of a take, whether releasing Shift ever slows the bike,
the jump's duration and apex in BBC pixels, the hanging hazard's period, the sea and snow scenes'
acceleration and top speed (Bob remembers sea looser and snow dragging), and whether a retake pays
wages again for hazards already passed. Convert to Oric units (228-pixel play width, 50 Hz step)
and propose new values for DESIGN. Rules and timings only; copy no graphics, sound or layouts.

### F0: Skeleton and harness

Follow `docs/ORIC-LESSONS.md` §5, §6 and §10. Reuse Chuckie Egg's crt0, ld65 config, `mktap.py`,
`tools/emu/emu.c` and its Python helpers from `/mnt/project-files/OricChuckie.zip`, adapted and
credited. The first build shows one empty track screen and the BUILD number, and runs on ROM 1.0
and 1.1 in the harness. `make test` runs the harness checks.

### F1 to F6

Briefs written by the architect when the previous feature is merged, from DESIGN.md.

## Results

### F0: Skeleton and harness (BUILD 1, 23 Nov 2026)

- **Toolchain:** cc65 2.19 (git `eebf54a`) vendored for the cloud shell in `tools/vendor/cc65`
  (x86_64 binaries, headers, `none.lib`); Chuckie Egg's `crt0.s` and `oric48.cfg` unchanged (crt0
  27 B + zerobss 35 B = 62 B). `make` = release build, `make ORICUTRON=1` = Bob's Oricutron build
  in `build/oricutron/`. Details in `tools/README.md`.
- **System:** IRQ hooked through the RAM vector on ROM 1.0 and 1.1 with no ROM code, polling
  fallback for an unknown ROM; 10 ms tick, 50 Hz step (20,000 cycles, measured exact on all three).
  IRQ handler 77–123 cycles.
- **Keys:** Space, either Shift, Return, P, Esc read directly (table in DATA for F6's
  redefinition). Jump is a key-down edge seen at the step; DESIGN's "latch a tap between steps" is
  left to F2.
- **Model:** `src/model.c`, portable C, one rider on four tracks with DESIGN's speed, one-stop,
  jump-from-the-ground, mid-air accelerate, wrap and pause/quit rules, on **placeholder constants**
  in `src/physics.h` (DESIGN's guesses) until M1. Quit restarts the scene until F4 has a title.
- **Trace:** native and Oric traces identical over 7,750 scripted steps (5 scripts incl. 6,000
  random steps); a planted Oric-only bug fails the check.
- **Screen:** placeholder (F1 replaces it): title strip, four bands with road lines, attribute
  cells 0–1, status rows with BUILD, track, speed, stop; the rider is a 6 × 12 XOR block.
- **Sound:** placeholder engine tone (pitch with speed) and noise bursts on jump, landing and
  stop; noise periods doubled in the Oricutron build (checked from the AY log).
- **Cost:** worst step 10,891 of 20,000 cycles, mostly the C placeholder display and the trace
  output (about 1,500 cycles; it can move to a trace-only build if F3 needs the room). Program ends
  at `$190A`; 31,990 bytes free below `$9800`.
- `make test`: 34 checks, all pass.
- **BUILD 2 (Bob's report, 23 Nov):** at full speed, after about two screens the rider faded
  from view. Cause: the rider was erased and then redrawn, and the 20,000-cycle step drifts
  against the Oric's 19,968-cycle video frame, so for about 2 s in every 12.5 s the beam scanned
  the rider between the two writes. Measured in the harness (new `emu --wlog`, HIRES writes with
  cycles): 2,622 rider bytes scanned while erased over 1,500 steps of `tests/keys/topjumps.keys`;
  after the fix, 0. The rider now moves in one pass (old ^ new per byte, ORIC-LESSONS §7), and a
  new check per script proves one write per byte per step and the screen = scene + rider. The
  C version costs more (worst step 15,532 cycles); F1's assembly sprite replaces it. 38 checks.
- **BUILD 3 (Bob: "now the rider tears"):** BUILD 2's C writes were spread over ~7,000 cycles, so
  the beam crossed a half-moved rider in 272 of 1,086 drawing steps. Now C prepares every byte's
  mask first and an assembly loop (`src/draw.s`, ~70 cycles a line) writes them in one burst of
  ~850 cycles: 73 of 1,086 steps, and then only a one-line shear. **No vertical sync on a stock
  Oric** means the beam's position can't be read, so tearing can be made rare and small but not
  removed; locking the step to the video frame would make it permanent on one track instead.
  Worst step 16,708 cycles (the C preparation); F1's assembly sprite brings it down.

### F1: Display (BUILD 4, 23 Nov 2026)

No architect brief had been written; this follows the PLAN row and DESIGN "Screen structure",
"Presentation" (art rules) and "Technical plan".

- **Scene from data:** `src/scene.h` defines a scene as four tracks of up to four 4-byte hazard
  records (kind, x, art or width, speed/period/ceiling line/lowest billing), so all seven behaviours
  share one layout; gaps carry the lowest billing that shows them. `src/scene.c` holds film 1, *Rush
  Hour Rumble*, scene 1 (a first sketch, F3/F5 make it rideable and tune it) and its night-city
  palette: paper and ink per band line, white ink from band line 4 to the road so the rider never
  changes colour in a jump, yellow kerb, white lane marks. `scene_draw` draws blocks, runners (at
  their start), danglers (rope and weight, up), ceiling blocks (hung on two hangers) and gaps.
- **Art:** `tools/mkart.py` holds every picture as text and generates `build/gen/art.s` and
  `art.h`: Bob on his motorbike, 12 × 16, two riding frames (the wheels turn every 4 px) and a jump
  frame, each pre-shifted to the 6 pixel phases (864 B); van, oil drums, police car, wrecking ball,
  girder (unshifted, 270 B, shifted while the scene is drawn). `--png` writes a preview sheet.
- **Status line:** text rows 25–27 in our own 5×7 font in the HIRES character set, plus icons
  (pound sign, stop lamp lit/out, clapper for each take left): take, takes, stop lamp, fee, best fee;
  speed, track and billing for testing; BUILD and keys. Plain character cells were enough: the
  Chuckie Egg pixel-row trick is kept for when a status needs finer art.
- **Rider drawing** (`src/draw.s`, Bob's rule: prepare first, then write in one burst): the old
  and new pictures and the screen bytes under them are combined in a buffer first (one box of 4
  cells when they overlap, else two boxes: new one drawn first, so the rider never vanishes), then
  written top to bottom at **67 cycles a line**, close to the beam's 64, with IRQs held off. A
  burst that keeps pace with the beam can't be overtaken by it, nor overtake it, over a box of up
  to 21 lines. The only frames left that can show part of each picture are those where the beam
  meets the burst within the ~45 cycles that the 4 writes of a line and the 3-cycle lag spread over:
  **6 of 1,086 drawing steps** of `topjumps.keys`, a shear of at most one step's move (≤ 3 px). The
  same harness count gives BUILD 3's 6 × 12 block 7 of 1,086, so a sprite twice the size now tears no
  more often. The harness now counts torn frames this way (`tears()` in `tools/test_f0.py`: a frame is
  torn if it scans some of a burst's bytes before their write and some after) and fails above 1%.
- **Cost:** the rider costs about 4,000 cycles a step on average (read the box ~1,500, XOR the
  pictures ~1,300, burst ~1,100, C ~300) and about 6,900 at a wrap (two boxes), against PLAN's 1,400
  guess, which was for a plain XOR draw. Worst whole step 12,320 of 20,000 (BUILD 3: 16,708). For F3's
  moving hazards: the same routine, but runners and danglers are smaller and move less often; if
  four of them overrun, merge the read and XOR passes or drop to 3 pre-shifts (DESIGN fallback).
- **Size:** display code 2.9 KB of C (`screen.c`) + 0.3 KB of assembly, against the 1.5 KB guess
  (cc65's code is ~3× hand assembly, ORIC-LESSONS §5). Program ends at `$2661`; 28,575 bytes free.
- **Tests** (`make test`, 39 checks): the scene is compared byte for byte with a pixel reference
  built from `src/scene.c`'s data and `tools/mkart.py`'s art (`tools/scene_ref.py`), at billing 1 and,
  in a test build (`make B=build/b3 CDEFS=-DBILLING_START=3`), at billing 3 with its gaps; the screen
  after every key script = that scene + the rider where the trace says, with the right frame; one
  write per byte per step; under 1% torn frames.
- **BUILD 5 (Bob's report on BUILD 4):**
  - *Stop lamp never went out:* cc65 2.19 compiled `t = m.stop_left ? '*' : '+'` after
    `shown = m.stop_left` to test the flags of an earlier `cmp`, so it always chose `'*'`. Now a
    table lookup; a check reads the lamp before and after the stop.
  - *Stop didn't work in mid-air:* the F0 model only allowed it on the ground. Now it works anywhere,
    once per take, and in the air the bike drops straight down through the rest of the arc (M1,
    DESIGN rule 2). New script `tests/keys/airstop.keys` and a rule check;
    `wrapjump.keys` no longer presses Return in the air.
  - *Left edge looked like a gap in the track:* attribute cells show as paper in the colour they
    set, so the blue sky started at pixel 0 and the roads at 12. Now cell 0 sets the ink (black)
    and cell 1 the paper only where it isn't black; on black-paper lines cell 1 is picture, so the
    kerbs start at pixel 6 too. Everything starts at pixel 6; play still starts at cell 2.
  - *Art:* the van, drums and police car are redrawn as solid shapes with windows and wheels cut
    out; the van faces right (parked, the way Bob rides), the police car faces left (oncoming),
    and the drums are two ribbed barrels.
- **Not in F1:** physics stay F0's placeholders (start speed 0.5 px a step, jump apex 24 px), so the
  rider still moves off by himself and jumps 4 px higher than he will: F2 brings M1's values.
  Hazards are scenery: nothing moves or collides until F3.

### M1 (23 Nov 2026)

Measured in headless JSBeeb, frame by frame, with RAM traces (`tools/m1/`). Full numbers and method:
`docs/M1-MEASUREMENTS.md`; DESIGN's guessed physics replaced with measured values in Oric units.

- One BBC game step is 60 ms (80 ms on its busiest screens). Speed 0 to 250, +3 a step while Shift is
  held, in the air too; it never drops by itself. Start speed 0; standing to top speed in 5.0 s; a
  track in 1.54 s flat out.
- Jump: 28 steps (1.68 s) at every speed, apex 20 lines; length 0.33 × speed bytes, just over a track
  at top speed. Holding Space repeats the jump. Wrap keeps speed and arc.
- Dangler period 3.0 s; runner 4.5 s per track; the same speeds on every level.
- Sea and snow handle exactly like land.
- The original gives one stop per take (also in mid-air), pays wages per track completed
  (10 × screen number) and again on a retake, gives a take back after each land, sea, snow round, and
  its skill levels are start points in one 36-screen run of 6 screens a level. Bob chose to follow it on
  all of these (follow-up PR updated DESIGN).
