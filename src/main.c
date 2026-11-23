/* main.c - Stunt Man Bob (docs/PLAN.md F0, F1).

   F1: film 1's first scene drawn from its data, and the rider on his bike run
   by the game model (src/model.c, placeholder physics until F2).  Hazards are
   scenery until F3.  Each 20 ms step: MARK, read the keys, step the model,
   write its trace line, redraw, BUSY. */
#include "art.h"
#include "model.h"
#include "physics.h"
#include "screen.h"
#include "system.h"
#include "trace.h"

#ifndef BILLING_START
#define BILLING_START 1         /* test builds: make CDEFS=-DBILLING_START=3 */
#endif

void main(void)
{
    uint8_t in, x;
    uint16_t speed = 0xFFFF;

    screen_init();
    scene_draw(&scene_rhr1, BILLING_START);
    model_init();
    status_update();
    rider_draw(m.track, 0, 0, FR_RIDE_A);
    sys_init();

    for (;;) {
        step_wait();
        DBG_MARK = 0;
        in = keys_poll();
        model_step(in);
        trace_step(in);

        /* frame: the jump picture in the air; on the road the wheels turn
           every 4 px travelled */
        x = m.x >> 8;
        rider_draw(m.track, x, m.height,
                   m.jump_t ? FR_JUMP : (x & 4) ? FR_RIDE_B : FR_RIDE_A);
        if (m.events & EV_JUMP)
            snd_burst(3);
        else if (m.events & (EV_STOP | EV_LAND))
            snd_burst(m.events & EV_STOP ? 12 : 8);
        if (m.speed != speed || (m.events & EV_PAUSE)) {
            speed = m.speed;                /* placeholder engine: pitch rises with speed */
            snd_engine(m.paused || !speed ? 0 : 1000 - speed);
        }
        status_update();
        DBG_BUSY = 0;
    }
}
