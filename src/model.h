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
#define EV2_FIRED  0x08     /* the last take was lost: "You're fired!" */
#define EV2_TAKE   0x10     /* a lost take given back (every third scene cleared) */
#define EV2_RUN    0x20     /* a new run started from the title screen */
#define EV2_ACTION 0x40     /* the clapper card is over: play starts */

/* F4: the film frame (DESIGN "Presentation").  The model runs the whole game,
   menus and cards too, so the native reference traces them like play. */
#define M_TITLE 0           /* title screen: 1 start, L billing, S sound */
#define M_CARD  1           /* clapper card before each take */
#define M_PLAY  2           /* riding (and paused) */
#define M_CUT   3           /* the crash pause: "CUT!" */
#define M_PRINT 4           /* scene cleared: "THAT'S A PRINT!" and the fee tally */
#define M_FIRED 5           /* the last take lost: "YOU'RE FIRED!" */
#define M_NAME  6           /* initials for the top five */
#define M_WRAP  7           /* a season (12 scenes) finished: "THAT'S A WRAP!" */
#define M_ASK   8           /* Esc in play: "QUIT? Y/N" */

#define CARD_STEPS 90       /* the clapper card: 1.8 s, then play */
#define CARD_SNAP  55       /* the step of the card when the arm snaps shut */
#define SHOW_STEPS 100      /* print, fired and wrap cards: 2 s */
#define CUT_SHOW   (CRASH_STEPS - 20)   /* crash_t when the megaphone pops up */
#define SEASON     12       /* scenes in a season (6 films x 2) */
#define BILLINGS   6
#define NBEST      5        /* the top five fees */

/* key codes from the full keyboard scan (keys_ascii), besides letters and digits */
#define K_RETURN 13
#define K_ESC    27
#define K_DEL    127

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
    uint16_t fee;           /* earned this run, in £10s (F6: a whole career passes 65,535 pounds) */
    uint16_t best;          /* best fee this session (£10s, as are the other fees) */
    uint8_t  hz_sum;        /* the moving hazards' positions folded into a byte (trace) */
    uint8_t  mode;          /* M_*: F4 */
    uint8_t  t;             /* steps in this mode (wraps) */
    uint8_t  billing;       /* 1-6: the starting billing (title) and the current one */
    uint8_t  sound;         /* 1: sound on (S on the title screen) */
    uint8_t  key_prev;      /* last step's key code, for key-down edges */
    uint16_t scene_fee;     /* earned in this scene over all its takes, bonus included */
    uint16_t bonus;         /* the one-take bonus just paid, 0 = none */
    uint8_t  gave_take;     /* 1: the scene just cleared gave a take back */
    uint8_t  name_slot;     /* M_NAME: the top-five line being named */
    uint8_t  name_len;      /*   initials typed */
    uint8_t  jump_long;     /* 1: this jump came off a slipway (JUMP_LONG steps) */
    uint8_t  jump_frac;     /* F6d, the hover bike: the jump's clock, 1/256 step */
    const struct scene *scene;  /* the scene being played (scene_at(run_scene)) */
};

extern struct model m;
#ifdef SCENE_PICK
extern uint8_t pick_scene;      /* debug build (make pick): P on the title picks the start scene */
#endif

/* hazards, per slot track * SC_MAX + i: where to draw them */
extern uint8_t hz_px[NHAZ];     /* x in pixels (runners move) */
extern uint8_t hz_drop[NHAZ];   /* danglers: lines below the top position, 0 .. DANGLE_MAX;
                                   jets: JET_OFF, JET_WARNING or JET_ON */
#define JET_OFF     0
#define JET_WARNING 1
#define JET_ON      2
#define DANGLE_MAX 25           /* M1: the line runs 1 .. 26 lines */
#define DANGLE_TOP 6            /* band line of a dangler's top when up */

/* the top five fees of the session and their initials (DESIGN "Best fees") */
extern uint16_t best_fee[NBEST];
extern char best_name[NBEST][3];

void model_init(uint8_t billing);
void model_scene_start(void);
void model_billing(uint8_t billing);   /* which gaps show, how fast hazards move */
/* in: IN_* bits; key: the key code held (keys_ascii), read only off M_PLAY */
void model_step(uint8_t in, uint8_t key);
uint8_t model_px(void);     /* the rider's left edge in pixels, 0 .. 213 */

#endif
