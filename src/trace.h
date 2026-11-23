/* trace.h - one state line per game step, the same on the Oric and natively
   (docs/PLAN.md F0; docs/ORIC-LESSONS.md section 6 "Trace equality").

   Line: "SSSS T XXXX VVVV JJ HH FF EE II E2 CC TN FFFF HZ MB TT KK\n" in hex: step, track,
   x, speed, jump step, height, flags (bit 0 stop unused, bit 1 paused, bits
   4-7 scenes mod 16), events, input; F3: ev2, crash pause left, takes left
   (high nibble) and take number, fee, a checksum of every hazard's position;
   F4: mode (low nibble) and billing, steps in the mode, the key code read.  On the Oric each byte goes to the harness debug
   port $BFE0 (plain RAM on real hardware); natively to stdout. */
#ifndef TRACE_H
#define TRACE_H
#include <stdint.h>

#ifndef __CC65__
#define __fastcall__
#endif
void trace_step(uint8_t in, uint8_t key);
#endif
