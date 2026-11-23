/* main.c - Stunt Man Bob, F0 skeleton (docs/PLAN.md F0).

   One empty four-track scene, the BUILD number, and a block for the rider run
   by the game model (src/model.c) on placeholder physics.  Each 20 ms step:
   MARK, read the keys, step the model, write its trace line, redraw, BUSY. */
#include "model.h"
#include "physics.h"
#include "screen.h"
#include "system.h"
#include "trace.h"

void main(void)
{
    uint8_t in, ot, ox, oh, nx;
    uint16_t speed = 0xFFFF;

    screen_init();
    model_init();
    sys_init();
    rider_xor(m.track, m.x >> 8, m.height);

    for (;;) {
        step_wait();
        DBG_MARK = 0;
        in = keys_poll();
        ot = m.track;
        ox = m.x >> 8;
        oh = m.height;
        model_step(in);
        trace_step(in);

        nx = m.x >> 8;
        if (nx != ox || m.track != ot || m.height != oh) {
            rider_move(ot, ox, oh, m.track, nx, m.height);
        }
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
