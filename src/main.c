/* main.c - Stunt Man Bob (docs/PLAN.md F0-F4).

   The game model (src/model.c) decides everything, menus and cards too (F4);
   this loop shows it.  Each 20 ms step: MARK, read the keys, step the model,
   write its trace line, show the new state, BUSY.  A new mode gets its screen
   drawn whole (src/frame.c, or the scene for play); after that only what
   moves is redrawn. */
#include "art.h"
#include "frame.h"
#include "model.h"
#include "physics.h"
#include "screen.h"
#include "system.h"
#include "trace.h"

#ifndef SCENE
#define SCENE scene_rhr1        /* test builds: make CDEFS=-DSCENE=scene_test */
#endif
#ifndef BILLING_START
#define BILLING_START 1         /* test builds: make CDEFS=-DBILLING_START=3 */
#endif

/* a fresh scene for play: the card's screen is replaced */
static void scene_show(void)
{
    play_reset();
    scene_draw(&SCENE, m.billing);
    rider_draw(m.track, 0, 0, FR_RIDE_A);
    hazards_list(&SCENE);
    hazards_draw();
}

static void mode_show(uint8_t was)
{
    switch (m.mode) {
    case M_TITLE: frame_title(); break;
    case M_CARD:  frame_card(&SCENE); break;
    case M_PLAY:  if (was == M_CARD) scene_show(); break;
    case M_PRINT: frame_print(); break;
    case M_FIRED: frame_fired(); break;
    case M_NAME:  frame_name(); break;
    case M_WRAP:  frame_wrap(); break;
    }
}

void main(void)
{
    uint8_t in, key, x, frame, shown = M_TITLE;
    uint16_t engine = 1, period;

    screen_init();
    frame_init();
    model_init(&SCENE, BILLING_START);
    frame_title();
    status_update();
    sys_init();

    for (;;) {
        step_wait();
        DBG_MARK = 0;
        in = keys_poll();
        key = 0;
        if (m.mode != M_PLAY)               /* the full scan off play only */
            key = keys_ascii();
        model_step(in, key);
        trace_step(in, key);

        if (m.mode != shown) {
            mode_show(shown);
            shown = m.mode;
        }
        switch (m.mode) {
        case M_PLAY:
        case M_CUT:
        case M_ASK:
            /* frame: the jump picture in the air; on the road the wheels
               turn every 4 px travelled */
            x = model_px();
            frame = FR_RIDE_A;
            if (m.crash_t)
                frame = FR_CRASH;
            else if (m.jump_t)
                frame = FR_JUMP;
            else if (x & 4)
                frame = FR_RIDE_B;
            rider_draw(m.track, x, m.height, frame);
            hazards_draw();
            if (m.crash_t == CUT_SHOW) {    /* the megaphone pops up: "CUT!" */
                frame_cut();
                if (m.sound)
                    snd_squawk(0);
            } else if (m.crash_t == CUT_SHOW - 8 && m.sound) {
                snd_squawk(1);
            }
            break;
        case M_TITLE:
            frame_title_step();
            break;
        case M_CARD:
            frame_card_step();
            if (m.t == CARD_SNAP && m.sound)
                snd_clack();
            break;
        case M_NAME:
            frame_name_step();
            break;
        }

        if (m.sound) {
            if (m.events & EV_CRASH)
                snd_crash();
            else if (m.ev2 & EV2_FEE)
                snd_fee();
            if (m.events & EV_STOP)
                snd_stop();
            else if (m.events & EV_JUMP)
                snd_jump();
            else if (m.events & EV_LAND)
                snd_land();
        }
        /* engine: ticking over at 50 Hz standing, rising to 320 Hz at top
           speed (period = 62,500 / f); only while riding, sound on */
        period = 0;
        if (m.mode == M_PLAY && !m.paused && m.sound)
            period = 1250 - 4 * m.speed;
        if (period != engine) {
            engine = period;
            snd_engine(period);
        }
        status_update();
        DBG_BUSY = 0;
    }
}
