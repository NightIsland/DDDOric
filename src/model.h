/* model.h - the game model: everything that decides what happens, in portable
   C (docs/PLAN.md F0, "Every gameplay change keeps the native C model and the
   Oric build trace-identical").  Compiled by cc65 for the Oric and by the host
   compiler for the native reference (tools/ref/ref.c); both print the same
   trace line per step (src/trace.c).  No screen, sound or key code in here.

   F0 skeleton: one rider on four tracks with the speed, stop, jump and wrap
   rules of DESIGN "Physics", on placeholder constants (src/physics.h).  No
   hazards yet (F3). */
#ifndef MODEL_H
#define MODEL_H

#include <stdint.h>

#ifndef __CC65__
#define __fastcall__
#endif

/* input bits, from keys_poll() on the Oric or a key script natively */
#define IN_JUMP   0x01
#define IN_ACCEL  0x02
#define IN_STOP   0x04
#define IN_PAUSE  0x08
#define IN_QUIT   0x10
#define IN_GAME   (IN_JUMP | IN_ACCEL | IN_STOP)

/* events of the last step (for sound and display) */
#define EV_JUMP   0x01      /* took off */
#define EV_LAND   0x02      /* landed */
#define EV_STOP   0x04      /* used the stop */
#define EV_WRAP   0x08      /* left the right edge onto the next track */
#define EV_SCENE  0x10      /* left track 4: scene finished, next one started */
#define EV_PAUSE  0x20      /* paused or resumed */
#define EV_QUIT   0x40      /* quit: F0 restarts the scene (later: title screen) */

struct model {
    uint16_t step;          /* steps since model_init (wraps) */
    uint16_t x;             /* rider's x on the track, 1/256 px, 0 .. TRACK_W_FP-1 */
    uint16_t speed;         /* 1/256 px per step */
    uint8_t  track;         /* 0-3, top to bottom */
    uint8_t  jump_t;        /* 0 on the ground, else step of the jump 1 .. JUMP_STEPS-1 */
    uint8_t  height;        /* px above the road */
    uint8_t  stop_left;     /* 1 while the scene's one stop is unused */
    uint8_t  paused;
    uint8_t  scenes;        /* scenes finished */
    uint8_t  keys_prev;     /* last step's input, for key-down edges */
    uint8_t  events;        /* EV_* of the last step */
};

extern struct model m;

void model_init(void);
void model_scene_start(void);
void __fastcall__ model_step(uint8_t in);

#endif
