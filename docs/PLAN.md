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
| M1 | Measure the BBC original: speeds, acceleration, jump arc and time, start speed, wrap, hanging-hazard period, sea and snow handling, fee on a retake | — | docs only | not started |
| F0 | Skeleton and harness: cc65 2.19 built for the cloud shell, `none.lib`, own crt0 and ld65 config at $0500, ROM-free IRQ hook, autorun `.tap`, Makefile with size report and `ORICUTRON=1`; Chuckie Egg's emulator adapted, PNG output, key scripts, debug port at $BFE0; native C model with per-step trace | — | crt0 + cfg ≤ 256 B | merged (BUILD 1); BUILD 2 fixes Bob's fading rider |
| F1 | Display: HIRES init, per-line paper/ink columns, a track screen drawn from scene data, status line in the text-row bitmap font, one-pass pre-shifted sprite draw | F0 | code ≤ 1.5 KB; sprite ≤ 1,400 cycles | not started |
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
  C version costs more (worst step 15,532 cycles); F1's assembly sprite replaces it. 39 checks.
