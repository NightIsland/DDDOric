# DDDOric: Stunt Man Bob

An original four-track stunt game for the 48K Oric-1 and Atmos, in the spirit of Daredevil Dennis
on the BBC Micro.

- `docs/DESIGN.md`: the game design (agreed 23 Nov 2026)
- `docs/PLAN.md`: build plan and status
- `docs/ORIC-LESSONS.md`: what the earlier Oric Chuckie Egg project learned about the platform

## Building

    make               # build/stuntbob.tap, the release build
    make ORICUTRON=1   # build/oricutron/stuntbob.tap, the build to test in Oricutron
    make test          # the harness checks

See `tools/README.md` for the harness.
