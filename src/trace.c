/* trace.c - the per-step trace line (src/trace.h). */
#include "model.h"
#include "trace.h"

#ifdef __CC65__
#define OUT(c) (*(volatile uint8_t *)0xBFE0 = (c))
#else
#include <stdio.h>
#define OUT(c) putchar(c)
#endif

static const char hexd[] = "0123456789abcdef";

static void __fastcall__ hex2(uint8_t v)
{
    OUT(hexd[v >> 4]);
    OUT(hexd[v & 15]);
}

static void __fastcall__ hex4sp(uint16_t v)
{
    hex2((uint8_t)(v >> 8));
    hex2((uint8_t)v);
    OUT(' ');
}

static void __fastcall__ hex2sp(uint8_t v)
{
    hex2(v);
    OUT(' ');
}

void __fastcall__ trace_step(uint8_t in)
{
    hex4sp(m.step);
    OUT(hexd[m.track]);
    OUT(' ');
    hex4sp(m.x);
    hex4sp(m.speed);
    hex2sp(m.jump_t);
    hex2sp(m.height);
    hex2sp((uint8_t)(m.stop_left | (m.paused << 1) | (m.scenes << 4)));
    hex2sp(m.events);
    hex2(in);
    OUT('\n');
}
