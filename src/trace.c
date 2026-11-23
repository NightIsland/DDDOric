/* trace.c - the per-step trace line (src/trace.h). */
#include "model.h"
#include "trace.h"

#ifdef __CC65__
/* src/system.s: hex digits straight to the debug port from tables (F3: the
   C version cost ~4,000 cycles a step) */
void __fastcall__ hex2(uint8_t v);
void __fastcall__ hex2sp(uint8_t v);
#define OUT(c) (*(volatile uint8_t *)0xBFE0 = (c))
#else
#include <stdio.h>
#define OUT(c) putchar(c)
static void hex2(uint8_t v) { printf("%02x", v); }
static void hex2sp(uint8_t v) { printf("%02x ", v); }
#endif

static void __fastcall__ hex4sp(uint16_t v)
{
    hex2((uint8_t)(v >> 8));
    hex2sp((uint8_t)v);
}

void trace_step(uint8_t in, uint8_t key)
{
    hex4sp(m.step);
    OUT("0123"[m.track]);
    OUT(' ');
    hex4sp(m.x);
    hex4sp(m.speed);
    hex2sp(m.jump_t);
    hex2sp(m.height);
    hex2sp((uint8_t)(m.stop_left | (m.paused << 1) | (m.scenes << 4)));
    hex2sp(m.events);
    hex2sp(in);
    hex2sp(m.ev2);                          /* F3 */
    hex2sp(m.crash_t);
    hex2sp((uint8_t)((m.takes << 4) | m.take_no));
    hex4sp(m.fee);
    hex2sp(m.hz_sum);
    hex2sp((uint8_t)(m.mode | (m.billing << 4)));      /* F4 */
    hex2sp(m.t);
    hex2(key);
    OUT('\n');
}
