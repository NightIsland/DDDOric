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
| F1 | Display: HIRES init, per-line paper/ink columns, a track screen drawn from scene data, status line in the text-row bitmap font, one-pass pre-shifted sprite draw | F0 | code ≤ 1.5 KB; sprite ≤ 1,400 cycles | BUILD 4 merged (PR #6); BUILD 5–6 fixes in a follow-up PR |
| F2 | One track: rider physics (DESIGN "Physics"), keys (Space, either Shift, Return), 50 Hz step, engine sound | F0, F1 | logic ≤ 4,000 cycles a step | merged (BUILD 8, PR #8) |
| F3 | Whole scene: four tracks, wrapping, block, runner, dangler, gap and ceiling block, crashes, fees, the one stop | F2 | whole step ≤ 16,000 cycles | merged (BUILD 11, PR #10); budget missed: average 16,100 riding flat out, worst 19,750 (see Results) |
| F4 | Film frame: clapper card, takes, "Cut!", scene cleared, "You're fired", title screen, billing select, top five | F3 | riding steps stay under 20,000 cycles | merged (BUILD 13, PR #12; police lamps BUILD 14-15, PR #13) |
| F5 | Film 1 (*Rush Hour Rumble*) complete at all six billings, tuned to M1's numbers; Bob judges the feel against the original | F4, M1 | | BUILD 17, PR #14 open |
| F6 | Films 2–6, jets and ramps, per-vehicle handling, music (no key redefinition: Bob, 24 Nov 2026) | F5 | memory ≤ 37 KB | not started |
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
  in `src/physics.h` (DESIGN's guesses) until M1. Quit restarts the scene until F4 has a title
  (F4: Esc asks Y/N and goes back to the title).
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

### F5: Rush Hour Rumble at all six billings (BUILD 16, 23 Nov 2026)

- **Scene 2, the harbour chase** (`src/scene.c` `scene_rhr2`), on the jet-ski: buoys (blocks), the
  patrol boat (a runner, the police car's size, with the same flashing lamp), a low footbridge
  (ceiling block) and two slipways. Night palette: black sky, blue water from the surface line with
  cyan waves; a gap on water shows as rocks breaking the surface (same rule: the wheels' middle
  over it on the surface crashes). A scene now names its vehicle and ground (`src/scene.h`).
- **The slipway** (DESIGN "Ramp"): riding onto it with the wheels' middle fires a jump 1.5 times as
  long (126 steps), same 20 px apex: the M1 arc with its entries held 4 and 5 steps in turn
  (`src/physics.h`). It is never a hazard.
- **A run of scenes**: `scene_at(run_scene)` gives each place in the run its scene; until F6 brings
  films 2-6, film 1's two scenes take turns through the season. The model loads each scene's
  hazards when it starts (`scene_load`); test builds with `-DSCENE=` still ride one scene.
- **Billings change speeds** (DESIGN "Billings": slow, normal, normal, fast, fast, fastest):
  runners x 1, 1.25, 1.25, 1.5, 1.5, 1.75 of M1's 0.95 px a step; danglers a line every 3, 2.5,
  2.5, 2, 2, 1.5 steps. Billing 1 is exactly M1 (and F3's timing).
- **Gaps by billing**: each scene stores every gap with the lowest billing that shows it
  (`SC_MAX` is now 6 records a track). Billing 1-2 is F3's scene 1 untouched; billing 3 adds small
  gaps, 4 more, 5 gaps in front of most hazards, 6 wide breaks.
- **Proved beatable** (`tools/solve`, tools/README "Solver"): a beam search over the native model
  that plays like a person (Accelerate changes at most every 5 steps and not in the air; every
  jump must land safely if pressed any of the next W - 1 steps instead). `tests/keys/win-b1..6.keys`
  clear both scenes on the first take at each billing with W = 8, 8, 6, 6, 5, 4 steps (160 ms down
  to 80 ms to press each jump in); `make wins` re-searches them. `takeback.keys` and
  `season.keys` were re-searched the same way for the new season.
- **Tests** (112 checks, all pass): each win run on the native model and on the Oric (same trace),
  its screen = pixel reference (water, rocks, slipways, jet-ski, boat), pixel-touch crashes and gap
  falls replayed independently at every billing's speeds, F3 fee rules from any billing, torn
  frames over more scripts (rider 24 of 4,588 drawing steps; scene well under 1%).
- Worst step 19,580 cycles over 9,930 scripted steps. 5,988 bytes free below `$9800` (the
  jet-ski's four frames and the harbour art took 2.8 KB; F6's other vehicles will need the
  3-pre-shift fallback from DESIGN "Memory").

- **BUILD 17 (Bob on BUILD 16: "it felt like you were only adding gaps"; initials accepted on the
  third letter):**
  - Billings now add and move hazards too, with DESIGN's billing tags brought forward from F6a:
    every record has a fifth byte, the lowest and highest billing that show it (`BILLS(lo, hi)`,
    `hz_shown` in `src/scene.c`; `SC_MAX` 8). Street: from billing 3 the police car starts at 90
    instead of 170 (it meets you on the track's first half), from 4 a second wrecking ball hangs
    just past the girder on track 3, from 5 the warm-up track gets a low girder after its drums,
    and 6 has a 30 px break before the ball. Harbour: from billing 3 the patrol boat starts at 80,
    from 4 a crate swings on the crane's hook between the footbridge and the buoy (new art), and from
    5 a second footbridge hangs over track 1. Gaps that would sit under a new ceiling stop at
    billing 4.
  - Every billing is still proved with the same windows (8, 8, 6, 6, 5, 4 steps); `make wins`
    re-searched all the runs. The solver also takes `SOLVE_NOSTOP=1`: billing 6 can still be
    cleared without the stop, so DESIGN's "a track that needs the stop" is not there yet.
  - Initials: three letters no longer end the entry; Del takes one back and Return finishes it.
  - While the jet-ski rides the slipway (its slow draw), the patrol boat's draw can wait several
    steps and then jump, leaving its old picture for one step as designed; the test reference now
    draws that ghost too.
  - 5,066 bytes free.

### F4: Film frame (BUILD 12, 23 Nov 2026)

- **The model runs the whole game** (`src/model.c`, so the native reference traces the menus
  and cards too): a mode (`M_TITLE`, `M_CARD`, `M_PLAY`, `M_CUT`, `M_PRINT`, `M_FIRED`, `M_NAME`,
  `M_WRAP`, `M_ASK`) and a step count in it. `model_step(in, key)` takes the key code of a key
  held besides the game inputs; `src/main.c` reads it with a full keyboard scan
  (`keys_ascii`, `src/keys.s`, ~1,000 cycles) only off play, and `tools/ref/ref.c` does the same
  from the script's keys (game tokens press their default keys; upper-case tokens are Oric keys
  for both now). The trace line gains mode and billing, steps in the mode, and the key code.
- **The flow** (DESIGN "Presentation"): the title screen (1 Start, L billing 1-6, S sound) ->
  the clapper card (film, scene, take, billing, fees; the arm snaps shut with a clack at 1.1 s,
  then "ACTION!" flashes; play at 1.8 s, or at once on any game key) -> riding. A crash: the
  crash picture, and 0.4 s later the director's megaphone and "CUT!" in the title strip with a
  two-note squawk; at 1.5 s the card with the next take. A cleared scene: "THAT'S A PRINT!" with
  the scene's track fees, the one-take bonus, a lost take coming back, and the run's fee (2 s or a
  game key), then the next card. Twelve scenes: "THAT'S A WRAP!" with the six films and the next
  billing (the next season is billed one up: gaps follow it). The last take lost: "YOU'RE
  FIRED!" over the fee and the day's best (2 s or a key), then, for a top-five fee, the initials
  (letters, Del, Return; three letters end it) and the title. Esc in play asks "QUIT TO THE
  TITLE? Y/N" on the status line (N or Esc rides on; Y ends the run, keeping the best fee but not
  entering the top five).
- **Billings:** billing b starts the run at scene (b - 1) x 12 + 1, so its fees start at
  10 x that (DESIGN "Billings"); `model_billing` sets which gaps show.
- **Top five** (`best_fee`, `best_name`): five film-crew entries to beat ($250 down to $50),
  kept until switch-off.
- **Drawing** (`src/frame.c`, `src/text.s`): each card is drawn whole when its mode starts;
  text comes from the status font in the character set, a letter a cell or doubled to two cells
  and 14 lines; Bob and his bike on the title are mkart pictures at double size. The marquee's 48
  bulbs chase in assembly (cc65's loop took 15,800 cycles a step, now ~1,700). Drawing a card
  takes ~300,000 cycles (15 steps: the screen fills in 0.3 s) and the scene for play the same;
  nothing moves meanwhile, and the model's steps simply wait. The clapper arm moves in 2 steps.
- **Sound:** a short noise clack (`snd_clack`), the squawk (`snd_squawk`, two falling notes on
  channel C), and S on the title silences everything (engine off, no effects). No jingles yet
  (F6, with the music).
- **Tests** (`make test`, 82 checks, all pass): every play script now starts from the title (1,
  then a tap on Space skips the card: riding starts at step 3, `tests/keys/start.keys`); the
  F3 checks follow the cards (a retake goes through the card; firing goes to "YOU'RE FIRED!").
  New: `flow.keys` (clear, print, three crashes, fired, initials BOB, the title's table),
  `quit.keys` (billing 3, sound off and on, Esc/N/Esc/Y), `season.keys` (twelve scenes, the
  wrap card, billing 2 at $130 a track); the title's chasing lights, the card's timing and clack,
  the squawk, the cards' text, and Oric == native on all of them. The step-work and torn-frame
  checks count riding steps only, from the fourth step of a new screen (the step that draws a
  card overruns, so the next ones start late in their 20 ms). Torn frames are counted over the
  three scene scripts together, as for the rider alone: the cards move the steps' phase against
  the video, and with it where the clusters fall (23 of 3,335 drawing steps).
- **Size:** program ends at `$6A9F`, 11,105 bytes free below `$9800` (F3: 20,800). The film
  frame is ~5 KB of cc65 code (`src/frame.c` 4.5 KB); F6's films and music may need it smaller.
- **Not yet:** K (redefine keys) is F6 with the rest of the key work; the top five is shown
  whole rather than scrolling; jingles are F6.

- **BUILD 13 (Bob on BUILD 12: "the clapperboard stripes on the top bar should point the other
  way; on the first start the rider isn't drawn; the megaphone and Cut! should be larger, a popup
  over the screen, and all the messages like that; remove SPEED"):**
  - The missing rider: Oricutron's RAM is not all 0 when the program loads (the ROM has used
    zero page), and a drawing variable there was read before it was set; the harness emulator
    started from 0, so no test saw it. `crt0` now clears zero page, and the emulator's new
    `--junk SEED` fills RAM before loading: a check draws the first scene from three junk seeds.
  - The board's stripes slant against the arm's, so a shut clapper shows chevrons.
  - "CUT!" (a big megaphone and the word), "THAT'S A PRINT!", "YOU'RE FIRED!", the initials,
    "THAT'S A WRAP!", "PAUSED" and "QUIT?" are popups over the scene: a box of lines with its
    own paper and ink set by attribute cells 2-3 and the line's own put back at cells 38-39.
    Pause and quit save what they cover (up to 36 lines, 1.4 KB) and put it back; nothing is
    drawn under a popup. The clapper card and the title stay full screen.
  - SPEED is off the status line.
  - The 4-cell burst's loop moved to `LOWCODE` (just after crt0) with an `.assert`: the code
    had grown so its branch crossed a page (68 cycles a line). Torn frames are now counted over
    four scene scripts including the long season run (69 of 8,612): the three short ones alone
    read 1.3% with this build's step phase, 0.7% with BUILD 12's.
  - 86 checks pass; 9,200 bytes free.

- **BUILD 14 (Bob: "draw the blue and/or red lights on top of the police car, blinking
  ideally"):** the car's top two lines (the lamps, wider now, and the lamp bar) get their own
  ink, red and blue, swapped every 8 steps: two ink-cell writes a step. Anything else on those
  two lines of the band takes the colour too (the rider's neck as he passes). The torn-frame
  check leaves colour cells out; a check sees the swap.

- **BUILD 15 (Bob: the flashing neck "does look a little weird"):** an ink cell colours its
  whole line, and the rider's neck and shoulders share the lamp lines whenever he is level with
  the car, so while he is (his rows overlap those two lines) the lamps take the band's ink, and
  they flash again once he jumps clear or leaves the track. A per-cell colour change beside the
  lamps was ruled out: the rider's XOR drawing over an attribute byte would turn it into a
  different attribute (even a video-mode change), and pixel-touch crashes would read it as ink.
  A check runs to a step with the rider level with the car and sees band ink on the lamp lines.

### F3: Whole scene (BUILD 9, 23 Nov 2026)

- **Hazards move** (`src/model.c`, from `src/scene.h`'s records, so the native model has them too):
  the police car drives left at M1's 0.95 px a step (122/128) and wraps inside the play width; the
  wrecking ball bobs one line every 3 steps between its top and 25 lines lower. All hazard slots are
  flat byte tables set up once (kind, width, heights, speeds): cc65 spent ~2,800 cycles a step on
  the struct version, now ~1,000. The rider's px comes from a 600-byte table filled at start-up
  (no multiply each step).
- **Collisions** (DESIGN rule 4): the rider's box shrunk 2 px a side against every hazard on his
  track; blocks and the police car from the road up (landing on top crashes too); the ball at its
  current drop; the low girder from its bottom edge up to the sky; a gap when the wheels' middle is
  over it on the road (gaps only at the billings tagged for them).
- **Crashes, takes, fees** (Bob's decisions): a crash stops the bike dead and loses a take; 1.5 s
  (75 steps) with the crash picture and "CUT! RETAKE COMING" (or "YOU ARE FIRED"), then the retake
  from track 1 standing. Each finished track pays 10 × the scene's number in the run, paid again on
  a retake; a scene cleared on its first take pays a bonus of 5 × that (Bob liked it; the size is a
  guess for F5); every third scene cleared gives a lost take back; after the last take the best fee
  is kept and a new run starts (F4 shows the "fired" screen and title). Status line: take number,
  takes left, fee, best.
- **Drawing:** the police car and the ball are sprites moved like the rider (one pass, old ^ new,
  written at the beam's pace). The car's 6-cell box can't be written at 67 cycles a line from a loop
  (91: 1.5% torn frames), so `src/draw.s` writes its bytes into a 16-line block of
  `lda #byte / sta line,y` code and runs that (67 cycles a line; addresses rewritten only when the
  box's lines change, i.e. never for a car). `spr_move` is in assembly now (cc65's took ~2,000
  cycles a call in stack handling). The ball's rope is single pixels, added below the ball after it
  moves down and taken away before it moves up.
- **The step budget:** a step has room for the rider and one or two hazards (~5,500 cycles each).
  `step_left()` reads the VIA timer, and a hazard is drawn only if 34 × 256 cycles are left; one
  that doesn't fit is drawn first next step (a step behind at most, measured; collisions always go
  by the model). A far move (the car wrapping, a retake) is split: the new picture one step, the old
  one taken off the next. The status line's long fields (fee, best, message) go one a step.
- **Cost:** riding flat out through the scene, 16,100 cycles a step on average (66% of steps over
  16,000), worst 19,750 of 20,000 over 8,430 scripted steps; no step overruns. **F3's 16,000 budget
  is missed**: the 1,150-cycle trace output and the rider's 5,000–9,000 are the big items. Options
  if F4–F6 need room: drop the trace from Bob's builds (trace equality then runs on a separate
  test build), or move the model's crash test to assembly (~700 cycles).
- **Tests** (`make test`, 65 checks, all pass): the empty test track build (`build/empty/`)
  carries the physics, sound and rider checks; the scene build adds F3 rules over every trace
  (crash, pause, retake, firing, fee per track, bonus, take back), `clear.keys` (scene 1 ridden
  clean flat out with 3 jumps: $90 with the bonus), `takeback.keys` (a crash, three scenes cleared,
  the take comes back, fees $490), screens equal to a pixel reference with the moving hazards where
  the sprite table says, drawn hazards within a step of the model, torn frames under 1% with the
  hazards, and the native model clean under AddressSanitizer + UBSan (it found a `h[0][i]` read past
  a row that `-O2` turned into a crash). The rider's torn-frame rate is now counted over all
  scripts together: a 67-cycle burst can be caught by a beam under 45 cycles above the box
  (~0.2-0.4% of draws, in clusters as the step drifts 32 cycles a frame), and one 37-draw script
  alone read 2.7%.
- **Bugs found on the way:** cc65 reads a `uint8_t` returned from assembly as A with X, so X must be
  0 (the budget check compared garbage); the rope routine shared zero page with the sprite mover and
  shifted the ball one cell right (the pixel check caught it); `far` is a cc65 keyword.
- Program ends at `$4497`; 20,841 bytes free below `$9800`.
- **BUILD 10 (Bob: "the police car is driving the wrong way for the image; it shouldn't pass over
  the truck"):** the car picture was mirrored; it now faces the way it drives. Scene 1's track 2 is
  the police car's own lane, and its van moved to track 1 (drums at 120, van at 200).
  `clear.keys` and `takeback.keys` re-timed for the new layout (searched on the native model).
- **BUILD 11 (Bob: crash only on a real touch; draw the rider without XOR on a blank background):**
  - The crash test compares the bike's picture with each hazard's, column by column, from
    column masks that `tools/mkart.py` makes from the same art (DESIGN rule 4 updated). A new check
    replays the hazards independently in Python and confirms, for every step of every trace, that
    a crash happens exactly when a bike pixel meets a hazard pixel.
  - The wheels now count, so the old route hit the drums. Scene 1 re-laid so it can still be
    cleared flat out (F5 tunes it properly): track 1 drums only, track 4's van at the start
    (cleared by the long jump from track 3's drums) and the ball at 200. Routes re-searched.
  - Fast rider draw: when the box holding the old and new pictures has no block, girder, ball
    (its whole reach, with the rope) or police car in it, `src/draw.s` fills it with blank bytes
    and puts the new picture in, skipping the screen read and the old picture's XOR: a moving
    rider costs ~1,700-4,000 cycles instead of 5,000-9,000. Near a hazard, and for the crash
    picture, it uses the old ^ new draw (cells are 6 px wide, so a hazard a few pixels away can
    share the box without touching). Steps over 16,000 cycles riding scene 1 flat out: 66% to
    32%; the worst step (a jump beside a hazard) is unchanged at ~19,700.

### F2: One track (BUILD 7, 23 Nov 2026)

- **Physics** (`src/physics.h`): M1's measured values in the original's own units, run at 50 Hz,
  which is exactly how the original moves: speed 0–250, +1 a step while Accelerate is held, in the
  air too; x in 1/256 BBC byte, x += speed every step; a track is 75 bytes; the 28-entry jump arc
  with each entry held 3 steps (84 steps in the air, apex 20 px); Space held jumps again 3 steps
  after a landing; every take starts standing. On screen x is `model_px` = x × 228 / 80 / 256
  (16-bit, 0.2% short), so the rider's left edge runs 0–212 px and he wraps before the screen edge.
- **Checks** (`tests/keys/feel.keys`): standing start, top speed 250 after 250 steps (5.0 s), a
  track in 76–77 steps at top speed (1.54 s), jump 84 steps with a 20 px apex held 18 steps, a
  top-speed jump carrying 82 bytes (234 px, past the wrap). `wrapjump.keys` rewritten for the new
  speeds.
- **Keys:** Space, either Shift, Return, as before; Jump is a key-down or Space held, so a tap
  shorter than a step is the only thing that can be missed (DESIGN's latch is not needed for that:
  a tap would have to be under 20 ms).
- **Sound** (`src/sound.s`): the engine on tone A ticks over at 50 Hz standing and rises to 250 Hz
  flat out (period 1,250 − 4 × speed), its volume alternating 10/7 every 10 ms for a motorbike buzz;
  silent while paused. Tone C sweeps for effects: take-off a hiss and rising whoosh, landing a low
  thump, the stop a brake squeal. Noise still doubled in the Oricutron build (checked).
- **BUILD 8 (Bob: "the take-off sounds the same as the landing and the brakes"):** all three had
  a full-volume noise burst that drowned the tone sweep. Now take-off is a clean rising tone with no
  noise (150 ms), landing keeps its low thump (noise 12 under a falling tone), and the stop is a high
  squeal (tone ~1.5 kHz) over a bright hiss (noise 1). A check reads each effect from the AY log.
- **Status:** speed now shows the original's 0–250.
- Worst step 13,000 of 20,000 cycles.

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
- **BUILD 6 (Bob on BUILD 5):** holding Space now jumps again as the bike lands, as in the
  original (M1: the landing step and two more on the road, then the next jump; DESIGN "Controls").
  Model: `road_t` counts road steps since a landing, `JUMP_REPEAT` = 3 in `src/physics.h`. Script
  `tests/keys/heldjump.keys` and a check. (PR #6 was merged with BUILD 4, so BUILD 5's fixes come
  in this pull request too.)
- **Not in F1:** physics stayed F0's placeholders until F2. Hazards are scenery: nothing moves or
  collides until F3.

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
