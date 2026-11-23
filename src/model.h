/* model.h - the game model: everything that decides what happens, in portable
   C (docs/PLAN.md F0, "Every gameplay change keeps the native C model and the
   Oric build trace-identical").  Compiled by cc65 for the Oric and by the host
   compiler for the native reference (tools/ref/ref.c); both print the same
   trace line per step (src/trace.c).  No screen, sound or key code in here.

   F2: one rider on four tracks with the speed, stop, jump and wrap rules of
   DESIGN "Physics", on the original's measured values (src/physics.h).
   F3: the scene's hazards (src/scene.h) move and collide; crashes, takes,
   retakes and fees (DESIGN rules 4 and 5, "Scoring, takes and fees"). */
#ifndef MODEL_H
#define MODEL_H

#include <stdint.h>
#include "scene.h"

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
#define EV_CRASH  0x80      /* hit a hazard or fell into a gap */

/* more events (m.ev2) */
#define EV2_FEE    0x01     /* a track finished: its fee paid */
#define EV2_BONUS  0x02     /* scene cleared on its first take: one-take bonus */
#define EV2_RETAKE 0x04     /* the crash pause is over: same scene, next take */
#define EV2_FIRED  0x08     /* the last take was lost: the run starts again (F4: title) */
#define EV2_TAKE   0x10     /* a lost take given back (every third scene cleared) */

#define NHAZ       (4 * SC_MAX)
#define CRASH_STEPS 75      /* 1.5 s from the crash to the retake (DESIGN "Presentation") */
#define TAKES      3
#define BONUS_X    5        /* one-take bonus: 5 x the scene's track fee (DESIGN, guess) */

struct model {
    uint16_t step;          /* steps since model_init (wraps) */
    uint16_t x;             /* rider's x on the track, 1/256 BBC byte, 0 .. TRACK_W_FP-1 */
    uint8_t  px;            /* the same in px, 0 .. 213 (model_px) */
    uint16_t speed;         /* 0 .. SPEED_TOP: 1/256 BBC byte per step */
    uint8_t  track;         /* 0-3, top to bottom */
    uint8_t  jump_t;        /* 0 on the ground, else step of the jump 1 .. JUMP_STEPS-1 */
    uint8_t  height;        /* px above the road */
    uint8_t  road_t;        /* steps on the road since the last landing, incl. it (stops at 255) */
    uint8_t  stop_left;     /* 1 while the scene's one stop is unused */
    uint8_t  paused;
    uint8_t  scenes;        /* scenes finished */
    uint8_t  keys_prev;     /* last step's input, for key-down edges */
    uint8_t  events;        /* EV_* of the last step */
    uint8_t  ev2;           /* EV2_* of the last step */
    uint8_t  crash_t;       /* steps of the crash pause left, 0 = riding */
    uint8_t  takes;         /* takes left in the run, TAKES .. 1 (0 during the last crash) */
    uint8_t  take_no;       /* this scene's take, 1 .. */
    uint8_t  run_scene;     /* the scene's number in the run, 1 .. (fees: 10 x this) */
    uint16_t fee;           /* pounds earned this run */
    uint16_t best;          /* best fee this session */
    uint8_t  hz_sum;        /* the moving hazards' positions folded into a byte (trace) */
};

extern struct model m;

/* hazards, per slot track * SC_MAX + i: where to draw them */
extern uint8_t hz_px[NHAZ];     /* x in pixels (runners move) */
extern uint8_t hz_drop[NHAZ];   /* danglers: lines below the top position, 0 .. DANGLE_MAX */
#define DANGLE_MAX 25           /* M1: the line runs 1 .. 26 lines */
#define DANGLE_TOP 6            /* band line of a dangler's top when up */

void model_init(const struct scene *s, uint8_t billing);
void model_scene_start(void);
void __fastcall__ model_step(uint8_t in);
uint8_t model_px(void);     /* the rider's left edge in pixels, 0 .. 213 */

#endif
