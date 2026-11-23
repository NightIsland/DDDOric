/* model.c - the game model (src/model.h).  Portable C: cc65 and the host. */
#include "model.h"
#include "physics.h"

struct model m;

static const uint8_t jump_arc[JUMP_STEPS] = { JUMP_ARC_DATA };

void model_scene_start(void)
{
    m.track = 0;
    m.x = 0;
    m.speed = SPEED_START;
    m.jump_t = 0;
    m.height = 0;
    m.road_t = 255;
    m.stop_left = 1;
    m.paused = 0;
}

/* x * 228 / 80 / 256 in 16 bits: (x / 32) * 91 / 256, 0.2% short of exact
   (x < 19,200, so the product stays under 54,600; the rider ends at 213 px) */
uint8_t model_px(void)
{
    return (uint8_t)(((m.x >> 5) * 91u) >> 8);
}

void model_init(void)
{
    m.step = 0;
    m.scenes = 0;
    m.keys_prev = 0;
    m.events = 0;
    model_scene_start();
}

void __fastcall__ model_step(uint8_t in)
{
    uint8_t edge = in & (uint8_t)~m.keys_prev;

    m.keys_prev = in;
    m.events = 0;
    ++m.step;

    if (m.paused) {                         /* any key-down resumes */
        if (edge) {
            m.paused = 0;
            m.events = EV_PAUSE;
        }
        return;
    }
    if (!(in & IN_GAME)) {                  /* Pause and Quit only with no game key */
        if (edge & IN_PAUSE) {              /* held: a ghost can't fake them */
            m.paused = 1;
            m.events = EV_PAUSE;
            return;
        }
        if (edge & IN_QUIT) {
            model_scene_start();
            m.events = EV_QUIT;
            return;
        }
    }

    if (in & IN_ACCEL) {                    /* speed only rises; also in the air */
        m.speed += SPEED_ACCEL;
        if (m.speed > SPEED_TOP)
            m.speed = SPEED_TOP;
    }

    if ((edge & IN_STOP) && m.stop_left) {  /* the one stop per take: dead stop, */
        m.speed = 0;                        /* also in mid-air, where the bike  */
        m.stop_left = 0;                    /* drops straight down through the  */
        m.events |= EV_STOP;                /* rest of the arc (M1, Bob)        */
    }
    if (m.jump_t == 0) {
        /* jump from the ground only: a key-down, or Space held, which jumps
           again after JUMP_REPEAT steps on the road (M1: one BBC step) */
        if ((edge & IN_JUMP) || ((in & IN_JUMP) && m.road_t >= JUMP_REPEAT)) {
            m.jump_t = 1;
            m.events |= EV_JUMP;
        } else if (m.road_t != 255) {
            ++m.road_t;
        }
    } else if (++m.jump_t >= JUMP_STEPS) {
        m.jump_t = 0;
        m.road_t = 1;                       /* the landing step is on the road */
        m.events |= EV_LAND;
    }
    m.height = jump_arc[m.jump_t];

    m.x += m.speed;                         /* wrap keeps height, arc and speed */
    if (m.x >= TRACK_W_FP) {
        m.x -= TRACK_W_FP;
        m.events |= EV_WRAP;
        if (++m.track == NTRACKS) {
            ++m.scenes;
            model_scene_start();
            m.events |= EV_SCENE;
        }
    }
}
