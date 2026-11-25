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

/* test builds: make CDEFS=-DSCENE=scene_test rides that scene everywhere */
#ifndef BILLING_START
#define BILLING_START 1         /* test builds: make CDEFS=-DBILLING_START=3 */
#endif

/* a fresh scene for play: the card's screen is replaced */
static void scene_show(void)
{
    play_reset();
    scene_draw(m.scene, m.billing);
    rider_draw(m.track, 0, 0, FR_RIDE_A);
    hazards_list(m.scene, m.billing);
    hazards_draw();
}

/* F6g: the engine's pitch for each vehicle (art.h VH_* order), x 1/8 of the
   motorbike's: the mine cart and the ghost train lower, the hover bike and
   the snowmobile higher */
static const uint8_t eng_mul[] = { 8, 8, 8, 14, 11, 16, 6, 7, 9, 8, 6 };

/* F6g: the film's sting on the clapper card of a scene's first take */
static void sting(void)
{
    uint8_t i;

    if (!m.sound || m.take_no != 1)
        return;
    for (i = 0; i < 6; ++i)
        if (films[i] == m.scene->film)
            snd_sting(i);
}

static void mode_show(uint8_t was)
{
    if (was == M_CARD)
        snd_sting(255);                     /* the card's tune stops with it */
    switch (m.mode) {
    case M_TITLE: frame_title(); break;
    case M_CARD:  frame_card(m.scene); sting(); break;
    case M_PLAY:  if (was == M_CARD) scene_show(); break;
    case M_PRINT: frame_print(); break;
    case M_FIRED: frame_fired(); break;
    case M_NAME:  frame_name(); break;
    case M_WRAP:  frame_wrap(); break;
    case M_ASK:   popup_ask(); break;
    }
    if (was == M_ASK && m.mode == M_PLAY)
        popup_restore();
}

void main(void)
{
    uint8_t in, key, x, frame, shown = M_TITLE, paused = 0;
    uint16_t engine = 1, period;
    uint8_t voice = 0;

    screen_init();
    frame_init();
#ifdef SCENE
    scene_only = &SCENE;
#endif
    model_init(BILLING_START);
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
            if (m.paused != paused) {       /* the "PAUSED" popup */
                paused = m.paused;
                if (paused)
                    popup_pause();
                else
                    popup_restore();
            }
            /* nothing is drawn under a popup: nothing moves then */
            if (paused || (m.mode == M_CUT && m.crash_t <= CUT_SHOW)) {
                if (m.crash_t == CUT_SHOW) {    /* "CUT!" pops up */
                    popup_cut();
                    if (m.sound)
                        snd_squawk(0);
                } else if (m.crash_t == CUT_SHOW - 8 && m.sound) {
                    snd_squawk(1);
                }
                break;
            }
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
            lamps((m.step >> 3) & 1);       /* the police lamps flash (Bob, BUILD 13) */
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
           speed (period = 62,500 / f), the motorbike's; only while riding,
           sound on */
        period = 0;
        snd_speed = m.speed;
        if (m.scene->vehicle != voice) {    /* F6g: each vehicle's own voice */
            voice = m.scene->vehicle;
            snd_voice(voice);
        }
        if (m.mode == M_PLAY && !m.paused && m.sound
            && !(m.jump_t && (voice == VH_HORSE || voice == VH_DRAGON || voice == VH_SKIS)))
            /* (hooves, feet and skis are quiet in the air: Bob, BUILD 27) */
            period = (uint16_t)((1250 - 4 * m.speed) * eng_mul[voice]) >> 3;
        if (period != engine) {
            engine = period;
            snd_engine(period);
        }
        status_update();
        DBG_BUSY = 0;
    }
}
