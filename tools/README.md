# Harness (F0)

The build and test set-up, reused from Oric Chuckie Egg (Bob's earlier Oric game, which passed on
his real machines) and adapted for Stunt Man Bob. See `docs/ORIC-LESSONS.md` §5, §6 and §10.

On Linux x86_64 (the cloud shell) `make` uses the vendored cc65 2.19 in `tools/vendor/cc65`
(`bin-linux-x86_64`, `share/include`, `share/asminc`, `share/lib/none.lib`; git `eebf54a`, zlib
licence). Elsewhere put cc65 2.19 on `PATH` (macOS: `brew install cc65`). Everything builds into
`build/`.

| Command | Result |
| --- | --- |
| `make` | `build/stuntbob.tap`: the release build for real hardware, plus `.bin`, `.map`, `.lbl` and a size line |
| `make ORICUTRON=1` | `build/oricutron/stuntbob.tap`: **the build for Bob to test in Oricutron** (every noise period doubled: DESIGN "Sound") |
| `make test` | both builds, a billing-3 test build (`build/b3/`), the empty test track (`build/empty/`, `-DSCENE=scene_test`: physics, sound and rider checks without crashes), emulator and native reference, then `tools/test_f0.py` |
| `make ref` | `build/ref/ref`: the native model with its per-step trace (`ref SCRIPT STEPS [test]`: `test` rides the empty test track) |
| `make emu` | `build/emu/emu`: the headless emulator |

## Program layout

| File | What |
| --- | --- |
| `src/crt0.s`, `src/oric48.cfg` | ROM-free start-up and ld65 config: load and run at `$0500`, 512 B C stack below `$9800` (from Chuckie Egg, unchanged) |
| `src/system.s` | VIA T1 10 ms tick; the IRQ hooked through the RAM vector `$FFFE` points at (`$0228` ROM 1.0, `$0244` ROM 1.1), polling fallback for any other ROM; `step_wait` (2 ticks = 50 Hz) |
| `src/keys.s` | scan of the six keys the game uses (Space, either Shift, Return, P, Esc), table in DATA for redefinition |
| `src/sound.s` | AY set-up, engine, jump, landing and stop sounds; `ORICUTRON` doubles noise periods |
| `src/model.c`, `src/model.h`, `src/physics.h` | the game model in portable C; the physics are M1's measured values (F2) |
| `src/trace.c` | one trace line per step: Oric → debug port `$BFE0`, native → stdout |
| `src/screen.c`, `src/screen.h` | the display: scene drawing, status rows, rider placement (F1) |
| `src/scene.c`, `src/scene.h` | scene layouts and film palettes (F1: Rush Hour Rumble scene 1) |
| `src/draw.s` | rider move: prepare the box, then write it at the beam's pace (F1) |
| `tools/mkart.py` | all pixel art as text → `build/gen/art.s`, `art.h`; `--png F` previews it |
| `tools/scene_ref.py` | pixel reference of a scene, the rider and the moving hazards (police car, wrecking ball and rope), for `make test` |
| `src/main.c` | the step loop: MARK, keys, model, trace, draw, sound, BUSY |
| `tools/ref/ref.c` | native driver: key script in, trace out |

## Trace

One line per game step, in hex (`src/trace.h`):

    SSSS T XXXX VVVV JJ HH FF EE II E2 CC TN FFFF HZ MB TT KK
    step track x(1/256 px) speed(1/256 px/step) jump-step height flags events input
    ev2 crash-pause takes|take fee hazards mode|billing steps-in-mode key-code

flags: bit 0 stop unused, bit 1 paused, bits 4–7 scenes finished. events: 01 took off, 02
landed, 04 stop used, 08 wrapped, 10 scene finished, 20 pause/resume, 40 quit. input: 01 jump,
02 accelerate, 04 stop, 08 pause, 10 quit. ev2 (F3, F4): 01 fee paid, 02 one-take bonus, 04
retake, 08 fired, 10 a take given back, 20 a new run, 40 the card is over. mode (F4, low nibble):
0 title, 1 clapper card, 2 riding, 3 "CUT!", 4 "THAT'S A PRINT!", 5 "YOU'RE FIRED!", 6 initials,
7 "THAT'S A WRAP!", 8 quit Y/N; billing in the high nibble. key code: the key held, read off
riding only (`keys_ascii`: letters, digits, 13 Return, 27 Esc, 127 Del).

**Rule:** the native reference and the Oric build print identical traces for the same key
script. `make test` checks it on every script in `tests/keys/` and 6,000 random steps. A bug
planted in the Oric build only (one speed unit at step 100) made three checks fail.

## Key scripts

    # comment
    <from>[-<to>] token token ...

Frame n is game step n. Lower-case tokens are game inputs (`jump accel stop pause quit`), which
press their default keys; upper-case tokens are Oric keys (`SPACE LSHIFT RSHIFT RETURN P ESC`,
`1`, `L`, `Y`...; full matrix in `tools/keyscript.py`). Both the emulator and the native
reference (F4) read both kinds. The game starts on the title screen: a play script begins
`1 1` (start) and `2 jump` (skip the clapper card), so riding starts at step 3.

## tools/run.py

    python3 tools/run.py build/stuntbob.tap --frames 300 --keys tests/keys/basic.keys \
        --png shot.png --cycles --trace trace.txt --ay ay.txt --dump _m:16 --rom oric1

- `--frames N` counts MARK writes (one per game step).
- `--rom atmos|oric1|none` puts ROM 1.1's or ROM 1.0's IRQ vector at `$FFFE`, or none (the
  game then polls). No ROM code is present or ever run.
- `--cycles` prints the step period, the work per step (MARK to BUSY; budget 20,000) and the IRQ
  handler's cost.
- Exits 2 if the program runs code in `$C000–$FFFF`, hits BRK or an illegal opcode, or takes an
  IRQ with no RAM vector.

- `run.run(..., wlog=True)` (emu `--wlog`) logs every HIRES write as (address, value, cycle) in
  `Result.wlog`: the flicker check in `make test` uses it with the 19,968-cycle video frame.

`import run` gives `run.run(prog, frames, keys=..., rom=...)` → a `Result` with `.mem`, `.rgb`,
`.pixel(x, y)`, `.ay`, `.marks`, `.frame_stats()`, `.trace`, `.irq`.

## Debug port `$BFE0–$BFE3`

RAM after the last text row, never displayed; plain RAM on real hardware.

| Address | Write means |
| --- | --- |
| `$BFE0` | append the byte to the trace |
| `$BFE1` | MARK: start of a game step (key-script clock) |
| `$BFE2` | HALT: stop the emulator, exit code = value |
| `$BFE3` | BUSY: step's work done |

## Emulator (tools/emu/emu.c)

From Chuckie Egg, unchanged apart from its header and `--wlog`. A 6502 (documented opcodes, cycle-exact,
passes Klaus Dormann's functional test, `tools/emu/test/`, GPLv3), VIA, AY registers, keyboard
matrix and ULA renderer following Oricutron's source. Details are in the file's header.
