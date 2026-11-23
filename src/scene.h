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
     RAMP      left edge     art                   -   (F5: fires a long jump)
     JET       (F6)

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

#define SC_MAX 6                /* records per track (F5: room for every billing's gaps) */

struct hazard {
    uint8_t kind, x, a, b;
};

struct palette {
    uint8_t title_paper, title_ink;
    uint8_t paper[48], ink[48];     /* per band line (DESIGN "Colour") */
};

#define GROUND_ROAD  0           /* kerb and lane marks; a gap: road missing */
#define GROUND_WATER 1           /* waves; a gap: rocks breaking the surface */

struct scene {
    const char *film;           /* title strip: film name */
    uint8_t number;             /* scene of the film, 1 or 2 */
    uint8_t vehicle;            /* VH_* (art.h) */
    uint8_t ground;             /* GROUND_* */
    const struct palette *pal;
    struct hazard h[4][SC_MAX];
};

extern const struct scene scene_rhr1, scene_rhr2;
extern const struct scene scene_test;     /* no hazards: harness only */

/* the scene at a place in the run (1 ..): a season is 12 scenes, 6 films of
   2; until F6 brings films 2-6, film 1's two scenes take turns.  Test builds
   with -DSCENE=x ride scene x everywhere (scene_pick). */
const struct scene *scene_at(uint8_t run_scene);
extern const struct scene *scene_only;    /* set: every scene is this one */

#endif
