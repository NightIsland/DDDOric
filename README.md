# DDDOric: Stunt Man Bob

An original four-track stunt game for the 48K Oric-1 and Atmos, in the spirit of Daredevil Dennis
on the BBC Micro.

## Download and play

[`release/stuntbob-v1.0.tap`](release/stuntbob-v1.0.tap) is the release version (BUILD 28), ready
to load: no building needed. On a 48K Oric-1 or Atmos (or an emulator) type `CLOAD""` and it
runs by itself; in Oricutron, insert the .tap as the tape and type the same.

- Title screen: `1` start, `L` billing (the starting difficulty), `S` sound on or off.
- Riding: `Space` jump, `Shift` accelerate, `Return` stop (once a take), `P` pause, `Esc` quit.

(Oricutron plays AY noise an octave high, so the crash, hiss and clack sound a little higher
there than on a real Oric.)

## Documents

- `docs/DESIGN.md`: the game design (agreed 23 Nov 2026)
- `docs/PLAN.md`: build plan and status
- `docs/ORIC-LESSONS.md`: what the earlier Oric Chuckie Egg project learned about the platform

## Building

    make               # build/stuntbob.tap, the release build
    make ORICUTRON=1   # build/oricutron/stuntbob.tap, the build to test in Oricutron
    make test          # the harness checks

See `tools/README.md` for the harness.
