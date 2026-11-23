/* scene.h - scene data (DESIGN "Obstacles and vehicles", "Technical plan:
   Data"; docs/PLAN.md F1).

   A scene is four tracks of up to SC_MAX hazard records, ridden top to bottom.
   Every record is four bytes, so one layout holds all seven behaviours:

     kind      x (px)        a                     b
     BLOCK     left edge     art                   -
     RUNNER    start         art                   speed (F3)
     DANGLER   left edge     art                   period (F3)
     GAP       left edge     width in px           lowest billing that shows it
     CEILING   left edge     art                   band line of its bottom edge
     JET, RAMP (scene 2s, F6)

   F1 draws a scene; F3 makes the hazards move and collide (in src/model.c, from
   this same data, so the native model can use it too). */
#ifndef SCENE_H
#define SCENE_H

#include <stdint.h>

#define HZ_NONE    0
#define HZ_BLOCK   1
#define HZ_RUNNER  2
#define HZ_DANGLER 3
#define HZ_GAP     4
#define HZ_CEILING 5
#define HZ_JET     6
#define HZ_RAMP    7

#define SC_MAX 4                /* records per track */

struct hazard {
    uint8_t kind, x, a, b;
};

struct palette {
    uint8_t title_paper, title_ink;
    uint8_t paper[48], ink[48];     /* per band line (DESIGN "Colour") */
};

struct scene {
    const char *film;           /* title strip: film name */
    uint8_t number;             /* scene of the film, 1 or 2 */
    const struct palette *pal;
    struct hazard h[4][SC_MAX];
};

extern const struct scene scene_rhr1;
extern const struct scene scene_test;     /* no hazards: harness only */

#endif
