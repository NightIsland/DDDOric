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
     JET       left edge     art                   phase, in 16ths of its period (F6a)
     FLYER     start         art                   band line of its underside (F6a)

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
#define HZ_FLYER   8             /* a runner in the air: the rider passes under on the ground */

#define SC_MAX 8                /* records per track (F5: room for every billing's hazards) */

/* bills: the billings that show the record (Bob, BUILD 17: billings add and
   move hazards, not only gaps): low nibble the lowest, high nibble the
   highest, 0 = no limit, so BILLS(3, 0) is billing 3 up, BILLS(0, 2) billings
   1-2.  A gap's b is its lowest billing as well. */
#define BILLS(lo, hi) ((lo) | (hi) << 4)

struct hazard {
    uint8_t kind, x, a, b, bills;
};

uint8_t hz_shown(const struct hazard *h, uint8_t billing);

struct palette {
    uint8_t title_paper, title_ink;
    uint8_t paper[48], ink[48];     /* per band line (DESIGN "Colour") */
};

/* Grounds (DESIGN "Ground styles", F6a): every ground handles the same; a
   ground is its four lines' bytes, cells 2-39 repeating every 4 cells, and
   how a gap looks: road missing (pixels cleared) or filled (pixels set:
   rocks in water, the dark of a grave or a crevasse). */
#define GROUND_ROAD  0           /* kerb and lane marks; a gap: road missing */
#define GROUND_WATER 1           /* waves; a gap: rocks breaking the surface */
#define GROUND_DIRT  2           /* a dusty street; a gap: a dry wash */
#define GROUND_RAILS 3           /* rails on sleepers; a gap: broken track */

#define GR_KERB 1                /* line 0 starts at cell 1 (the road's kerb) */
#define GR_FILL 2                /* a gap sets the pixels */
struct ground {
    uint8_t line[4][4];
    uint8_t flags;
};
extern const struct ground grounds[];

struct scene {
    const char *film;           /* title strip: film name */
    uint8_t number;             /* scene of the film, 1 or 2 */
    uint8_t vehicle;            /* VH_* (art.h) */
    uint8_t ground;             /* GROUND_* */
    const struct palette *pal;
    struct hazard h[4][SC_MAX];
};

extern const struct scene scene_rhr1, scene_rhr2, scene_dd1, scene_dd2;
extern const struct scene scene_test;     /* no hazards: harness only */
extern const struct scene scene_stress;   /* F6a: two every-step movers, three jets: harness only */

/* the scene at a place in the run (1 ..): a season is 12 scenes, 6 films of
   2; until F6 brings films 2-6, film 1's two scenes take turns.  Test builds
   with -DSCENE=x ride scene x everywhere (scene_pick). */
const struct scene *scene_at(uint8_t run_scene);
extern const struct scene *scene_only;    /* set: every scene is this one */

#endif
