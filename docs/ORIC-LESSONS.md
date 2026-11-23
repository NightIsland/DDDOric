# Oric Development: What We Learned

14 Nov 2026 (hardware result added 15 Nov) · Oric Chuckie Egg, release candidate 1 (BUILD 13)

## 1. How to use this document

Everything learned porting **Chuckie Egg** (BBC Micro, 1983) to the **Oric-1 48K / Atmos** over
13–14 Nov 2026: one architect thread and 14 feature threads (F0–F4b, R1, B1, B2, P1, H1, A1). It
records the facts that cost time, so the next Oric project starts where this one finished.

**To start a new Oric project:** copy this document into the new Project and tell the first chat
to read it first. Section 10 is a day-one checklist. Project detail (Chuckie Egg's rules, fixes,
layouts) stays in this repo's `docs/ARCHITECTURE.md`, `docs/PLAN.md` and `docs/ORIGINAL.md`.

**How sure each fact is:** measured in the harness, read from the ROM images or Oricutron's source,
or seen by the user in Oricutron, unless it says otherwise. **BUILD 13 then passed the user's
real-hardware test (15 Nov): everything worked on his own Oric machines exactly as in Oricutron** —
loading, the ROM-free IRQ hook, display (layout B, text-row bitmap, bitmap status line), keys, sound,
title and instruction page. Where Oricutron and a real machine differ (§4), it says so.

## 2. Ways of working that held

- **Architect + one feature per thread.** The architect owns `ARCHITECTURE.md` (decisions D1–D21
  with reasons) and `PLAN.md` (status table, one brief per feature, results); a feature thread reads
  both, builds one thing in a worktree, updates only its own row and results, and reports. Design
  questions come back to the architect, not into code.
- **Starting a thread:** "Feature Fn — name. Read docs/ARCHITECTURE.md and docs/PLAN.md in
  ~/Desktop/Projects/<Project> and build it." **Permissions are per thread:** each thread asks for
  folder access and file deletion itself (git needs deletion for its lock files); the desktop app
  does not pass the architect's on.
- **Shared prebuilt toolchain** in `tools/vendor/` for both Linux shells (the Mac's aarch64 VM and the
  x86_64 cloud workspace): no thread rebuilds it.
- **Git:** the shared folder stays on `main`; feature work in `.worktrees/<name>` on its own branch;
  stage named files only; the architect merges after the user says the thread is done. Worktrees made
  by other sessions show as "prunable" (their recorded paths belong to that session's mount):
  harmless; prune them all at the end. They grow large (build output): 5.1 GB for 13 here.
- **BUILD numbers clash** when two threads run at once: whoever merges second takes the next number.
- **The user's ear and eye find what tests can't** (sound balance, a name-entry bug, flicker, hat
  shape); the tests find what play can't (rare paths, byte-level regressions). Nothing is "fixed"
  until the user has seen it in the emulator.
- **Small fixes in the architect thread are fine** (a one-line bug plus a regression test that fails
  without it), recorded in PLAN's results like any feature.
- **The device shell** runs ≤ 180 s per call: long jobs in steps or in the background; `make`
  warns of clock skew on the shared folder (harmless).

## 3. The Oric platform (48K)

**Memory map as used:**

| Range | Use |
|---|---|
| `$0000–$00FF` | zero page: all ours once loaded (cc65's runtime takes 26 B) |
| `$0100–$01FF` | 6502 stack |
| `$0200–$02FF` | ROM variables; **the IRQ's RAM vector** ("The two ROMs" below) |
| `$0300–$03FF` | I/O: the VIA at `$0300` (`$030E` IER) |
| `$0500–` | program origin for a `CLOAD""` autorun tape |
| `$9800–$9FFF` | HIRES-mode character sets: standard `$9800`, alternate `$9C00` |
| `$A000–$BF3F` | HIRES lines 0–199, 40 bytes each |
| `$BF68–$BFDF` | the 3 text rows under HIRES (rows 25–27) |
| `$BFE0–$BFFF` | free RAM, never displayed (our harness debug port) |
| `$C000–$FFFF` | ROM |

**The display (ULA):**
- HIRES: 240 × 200, 6 pixels per byte (bits 5–0), **bit 6 set in every bitmap byte**; then 3 text
  rows from the character set. Screen addresses are fixed: **no page flipping, no double buffering**
  (a full copy is ~80k cycles). No vertical-sync signal on a stock Oric-1/Atmos.
- **Colour is serial:** a byte < 32 is an attribute (ink, paper, mode, charset) for the rest of the
  line and shows as a 6-pixel gap of paper. Every line starts white on black. Full-width graphics
  therefore have no room for colour: plan colour fields around attribute cells.
- **Bit 7** of a byte inverts that cell's colours.
- **The text rows can be pixels:** give each cell of rows 25–26 its own character code (80 cells:
  codes 32–111) and writing the character set *is* writing pixels — 16 more bitmap lines (216 in
  all). Address of line 200+j, cell c: `$9800 + 8·(32 + 40·(j>>3) + c) + (j & 7)`: stride 8 per cell
  instead of 1. The status row can be done the same way in the alternate set (codes 33–70), which
  frees it from the 6×8 font: we used a 3×5 font at a 4-pixel advance.
- Mode attributes: cell 39 of the last text row holds the HIRES mode attribute; never overwrite it.
- A mid-line mode attribute does re-point the fetch (Oricutron's `ula.c`), but a split screen costs
  a cell per switched line: not worth it here.

**Timing:** the CPU runs at 1 MHz. The VIA's T1 in free-running mode gives exact ticks (latch + 2
cycles per period); T2 one-shot is free for short waits. **cc65 deletes a bare volatile read**
(`(void)VIA_T1CL;`), so acknowledge T1 by writing IFR, or read it in assembly.

**Keyboard:** 8 rows by VIA port B bits 0–2; the column is selected through the **AY's port A**
(register 14, with R7 bit 6 set for output); the key is sensed on VIA PB3, **≥ 3 cycles after the
R14 write**. The AY address latch is shared with sound: hold IRQs off around each access. A scan of
just the keys a game needs costs 260–700 cycles in assembly (a general C scan: ~19,700). The matrix
has no diodes: three keys in an L can ghost or mask a fourth.

**Sound (AY-3-8912 at 1 MHz):** tone period = 62,500 / f; noise rate = clock / 16 / R6.
**R6 is 5 bits:** period 32 writes 0, which the chip treats as 1 (a nearly inaudible hiss in
Oricutron): use 31. Volume is logarithmic, 0–15. To imitate the BBC's SOUND/ENVELOPE, step the
envelopes from a 10 ms tick and start notes in the caller, so the interrupt stays short (≤ 360
cycles here).

**The two ROMs:** Oric-1 = ROM 1.0 (`basic10.rom`), Atmos = ROM 1.1 (`basic11b.rom`). A 16K Oric-1
always has ROM 1.0. `$FFFE` points **straight into RAM**: `$0228` (ROM 1.0), `$0244` (ROM 1.1), each a
`JMP` the program can repoint, so one binary hooks the IRQ on both machines **without running any
ROM code**: read `$FFFE` at startup, patch the JMP it lands on, fall back to polling if it lands
anywhere else. The ROM font is at `$FC70` (1.0) / `$FC78` (1.1), the same glyphs; the key legends
table at `$FF78`.

**Tape:** `.tap` with an autorun header loads with `CLOAD""` on both ROMs and starts at the load
address; the data segment arrives in place, so there is nothing to copy at startup.

**16K (A1):** the ULA's addresses alias down by `$8000` (HIRES `$2000`, charset `$1800`), so a 48K
layout runs unchanged, but full-screen HIRES leaves only ~6 KB loadable; TEXT mode leaves ~12.5 KB
with 96 redefinable characters.

## 4. Oricutron as the reference

- Builds and runs on the user's Mac; its source (`via.c`, `8912.c`, `ula.c`, `machine.c`) is the
  best quick reference for hardware behaviour, and the harness emulator follows it.
- **Its AY noise runs an octave high** (`8912.c`: `#define NOISETIME 8`; the datasheet's rate needs
  16). Noise bursts sound brighter than on a real Oric. The user chose to leave it as is.
- It stores AY register values unmasked (the real chip masks period and volume bits on read).

## 5. cc65

- Version 2.19 (git `eebf54a`), built from source for both Linux shells. On the Mac: `brew install cc65`.
- **Use `none.lib`**, not `atmos.lib`: the Atmos library's startup and console call the ROM.
- An own `crt0.s` (62 B with `zerobss`) and an ld65 config: origin `$0500`, limit below the
  character sets, a 512 B C stack.
- `-Cl` (static locals) is safe when nothing recurses and saves stack traffic.
- Narrow every `int` to the smallest exact type and prove it with traces: the ported logic's worst
  frame was 6,576 cycles.
- Pass arguments to hot assembly routines in zero page rather than the C stack: 105 cycles a call here.
- cc65 code is roughly 3× larger than hand-written 6502 for the same logic (A1: 357 B vs 120 B for
  one function; the whole logic 1.8× the original's assembly).
- **cc65 2.19 optimiser bug** (Stunt Man Bob, BUILD 4): after `if (a != b) { b = a; x = a ? 1 : 2; }`
  it tested the flags left by the `cmp` instead of reloading `a`, so `x` was always 1. Use a table
  (`x = tbl[b]`) or an explicit `if`, and check status displays in the harness.
- Watch sign: `move_y <<= 1` on a negative number is undefined in C; write `* 2`.

## 6. The verification method (worth reusing)

- **The original binary is the authority.** With a labelled disassembly (mungre's, byte-identical to
  the user's disc) and a 6502 emulator running the original, a C model (upstream + patches) was
  shown to match it **frame for frame** over tens of thousands of frames from levels 1–48; 11 fixes
  (FIX1–11) came out of the differences.
- **Trace equality:** the native reference and the Oric build in the harness emulator print one
  state line per game frame for the same key script; they must be identical. 21 scripts, ~295,000
  frames. Every port change kept them identical, and planted bugs proved the checks fail.
- **A debug port in unseen RAM** (`$BFE0–$BFE3`: trace byte, MARK, HALT, BUSY) lets the harness
  sync key scripts per frame, time phases and stop at events, at no cost on hardware.
- **Pixel references:** a Python renderer with the same scaling rule as the assembly; every test
  scene must match pixel for pixel, and "nothing outside the playfield changes" catches stray writes.
- **Random play misses targeted bugs:** 1.6 M random frames missed the ladder-edge bug that one
  scripted route found. Read the code for the path, then script it.

## 7. Display and speed numbers (1 MHz)

| Item | Cycles |
|---|---|
| XOR sprite 8 × 16, pre-shifted, 2 / 3 cells | 1,109 / 1,379 |
| per byte of an XOR | ≥ 15 (`lda / eor (zp),y / sta (zp),y`) |
| one-pass move (old ^ new per byte) | 19 per byte, ~1,100 set-up |
| tile | ~710 |
| status-line glyph (3×5, pre-shifted) | 202, or 347 straddling two cells |
| IRQ tick with sound envelopes | 99 idle, ≤ 360 |
| whole game frame (logic + drawing), worst of 295k frames | 17,259 of 30,000 |

- Pre-shifting sprites (4 copies when the x mapping gives 4 phases) beat draw-time shifting by
  45–51% in cycles for ~3.5 KB more data: memory was not the constraint at 48K.
- Two XOR passes (erase, draw) leave a sprite absent for up to 4,000 cycles (20% of a video frame):
  visible flicker. **One pass that writes old ^ new into each byte** removes it completely; it is not
  a speed gain.
- XOR in one colour plane punches holes wherever sprites cross scenery; the BBC hid this with
  separate colour planes. In monochrome the faithful rule is XOR within a plane, OR between planes
  (designed, not built: `docs/PROPOSAL-sprites-on-top.md`).

## 8. Porting a BBC Micro game

- MODE 2 pixels are ~2:1 wide; **×1.5 horizontally** turns 160 BBC pixels into the Oric's 240, the
  closest aspect, but uses every cell, so the playfield is one colour.
- BBC games often measure y from the bottom; plan the vertical fit early: Chuckie Egg's play area
  (~217 lines) plus its status panel did not fit 200 HIRES lines + 3 text rows, so the text-row bitmap
  trick bought 16 lines and the status line moved to the bottom row.
- The BBC's interval timer and SOUND/ENVELOPE map well onto a 10 ms VIA tick: a frame ends when
  ≥ 3 ticks have passed, then resets (an overrunning frame ends at once; the next one realigns).
- Byte arithmetic matters: the original wraps (x 255, level 256) where a C port with `int` does not.
  Model it in the reference.
- MODE 7 teletext pages port to the Oric's TEXT mode almost cell for cell (no mosaics).

## 9. Things that went wrong once

- Merging feature branches with conflicting PLAN rows: keep the architect's rows, take the thread's
  results; check no section is duplicated.
- A thread's test passed while the bug was real (name entry: the test typed a full name, which
  overwrote the deleted characters). Regression tests should reproduce the user's own sequence.
- A test compared the byte written (32) rather than what the register holds (0): check against the
  hardware's view.
- Copying a file over another's name while saving a mock-up script: restore from git.

## 10. Day one of a new Oric project: checklist

- [ ] Add this document to the new Project; write ARCHITECTURE.md (decisions with reasons) and
      PLAN.md (status table, briefs, results); git repo on `main`.
- [ ] Vendor cc65 for both Linux shells in `tools/vendor/`; `tools/env.sh`; `none.lib`, own crt0 and
      ld65 config at `$0500`.
- [ ] Decide early: ROM calls or none (§3: the IRQ RAM vector works on both ROMs without them);
      48K only or 16K (§3: 16K leaves ~6 KB with a HIRES screen).
- [ ] Harness first (F0): a small 6502 + VIA + AY + ULA emulator following Oricutron, PNG output, key
      scripts, a debug port; a native reference with a per-frame trace.
- [ ] If porting: get the original binary and a disassembly, run the original in an emulator, and
      make it the authority.
- [ ] Ask the user to grant folder access and deletion to every new thread; prune worktrees at the end.
- [ ] Check on a real machine before release (this project's BUILD 13 passed first time on the
      user's Oric machines, 15 Nov: building against Oricutron's source paid off).
