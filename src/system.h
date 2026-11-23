/* system.h - interrupts, step pacing, keys, sound (src/system.s, src/keys.s,
   src/sound.s; docs/PLAN.md F0). */
#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>

void sys_init(void);            /* VIA, AY, 10 ms T1 tick, IRQ hook */
void step_wait(void);           /* end of a 20 ms game step */
extern uint8_t ticks;           /* 10 ms ticks since step_wait last reset it */
extern uint8_t irq_mode;        /* 1 = IRQ handler hooked, 0 = unknown ROM: polling */

uint8_t keys_poll(void);        /* -> IN_* bits (src/model.h) */
extern uint8_t key_row[6], key_col[6], key_bit[6];

void __fastcall__ snd_engine(uint16_t period);   /* tone A, 0 = off (placeholder engine) */
void __fastcall__ snd_burst(uint8_t period);     /* noise burst, period 1-15 */

/* harness debug port (docs/ORIC-LESSONS.md section 6): RAM never displayed */
#define DBG_TRACE (*(volatile uint8_t *)0xBFE0)
#define DBG_MARK  (*(volatile uint8_t *)0xBFE1)
#define DBG_HALT  (*(volatile uint8_t *)0xBFE2)
#define DBG_BUSY  (*(volatile uint8_t *)0xBFE3)

#endif
