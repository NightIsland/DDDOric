/* trace.c - the per-step trace line (src/trace.h). */
#include "model.h"
#include "trace.h"

#ifdef __CC65__
/* F6a: the bytes as they are to the binary debug port, which the emulator
   writes out as the hex line below (tools/emu/emu.c $BFE4): ~150 cycles a
   step where the hex digits cost ~1,300 */
#define TB(c) (*(volatile uint8_t *)0xBFE4 = (c))

void trace_step(uint8_t in, uint8_t key)
{
    TB(m.step >> 8);
    TB((uint8_t)m.step);
    TB(m.track);
    TB(m.x >> 8);
    TB((uint8_t)m.x);
    TB(m.speed >> 8);
    TB((uint8_t)m.speed);
    TB(m.jump_t);
    TB(m.height);
    TB((uint8_t)(m.stop_left | (m.paused << 1) | (m.scenes << 4)));
    TB(m.events);
    TB(in);
    TB(m.ev2);
    TB(m.crash_t);
    TB((uint8_t)((m.takes << 4) | m.take_no));
    TB(m.fee >> 8);
    TB((uint8_t)m.fee);
    TB(m.hz_sum);
    TB((uint8_t)(m.mode | (m.billing << 4)));
    TB(m.t);
    TB(key);
}
#else
#include <stdio.h>

void trace_step(uint8_t in, uint8_t key)
{
    printf("%04x %c %04x %04x %02x %02x %02x %02x %02x %02x %02x %02x %04x %02x %02x %02x %02x\n",
           m.step, "0123"[m.track], m.x, m.speed, m.jump_t, m.height,
           (uint8_t)(m.stop_left | (m.paused << 1) | (m.scenes << 4)), m.events, in,
           m.ev2, m.crash_t, (uint8_t)((m.takes << 4) | m.take_no), m.fee, m.hz_sum,
           (uint8_t)(m.mode | (m.billing << 4)), m.t, key);
}
#endif
