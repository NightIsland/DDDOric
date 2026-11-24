/* scene.c - scene layouts and film palettes (src/scene.h).

   Film 1, Rush Hour Rumble (DESIGN "Films, scenes and progression"): scene 1
   a city street at night on the motorbike, scene 2 the harbour chase on the
   jet-ski (F5).  Each layout serves all six billings: its gaps carry the
   lowest billing that shows them, and the moving hazards speed up with the
   billing (src/model.c).  tools/solve proves every scene beatable at every
   billing; the winning runs are in tests/keys/win-*.keys. */
#include "scene.h"
#include "art.h"

#define BLACK 0
#define RED 1
#define GREEN 2
#define YELLOW 3
#define BLUE 4
#define MAGENTA 5
#define CYAN 6
#define WHITE 7

/* Night city: a blue sky fading through dithered lines to black where the
   rider runs; white ink from band line 4 down to the road, so the rider never
   changes colour in a jump (apex: band line 8, or 4 with F0's placeholder arc); a yellow kerb and white lane marks. */
static const struct palette pal_city = {
    RED, YELLOW,
    {   BLUE, BLUE, BLUE, BLUE, BLUE, BLUE, BLACK, BLUE,                /* 0-7 */
        BLUE, BLACK, BLACK, BLUE, BLACK, BLACK, BLACK, BLUE,            /* 8-15 */
        BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,         /* 16-23 */
        BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,         /* 24-31 */
        BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,         /* 32-39 */
        BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK },       /* 40-47 */
    {   CYAN, CYAN, CYAN, CYAN, WHITE, WHITE, WHITE, WHITE,
        WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
        WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
        WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
        WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
        WHITE, WHITE, WHITE, WHITE, YELLOW, WHITE, WHITE, BLACK },
};

/* Each billing harder (Bob, BUILD 17: not only gaps): billing 2 speeds the
   movers up; 3 adds small gaps and moves the police car to meet you sooner;
   4 adds more gaps and a second wrecking ball under the girder's exit; 5
   puts gaps in front of the hazards and gives the warm-up track a girder; 6
   breaks the road wide. */
const struct scene scene_rhr1 = {
    "RUSH HOUR RUMBLE", 1, VH_BIKE, GROUND_ROAD, &pal_city,
    {
        {   { HZ_BLOCK, 110, ART_DRUMS, 0, 0 },
            { HZ_CEILING, 172, ART_GIRDER, 22, BILLS(5, 0) },
            { HZ_GAP, 180, 12, 3, BILLS(0, 4) },
            { HZ_GAP, 50, 14, 4, 0 },
            { HZ_GAP, 94, 14, 5, 0 },
            { HZ_GAP, 140, 22, 6, 0 } },
        /* the police car has a lane of its own (Bob) */
        {   { HZ_RUNNER, 170, ART_POLICE, 0, BILLS(0, 2) },
            { HZ_RUNNER, 90, ART_POLICE, 0, BILLS(3, 0) },
            { HZ_GAP, 70, 14, 3, 0 },
            { HZ_GAP, 130, 18, 5, 0 },
            { HZ_GAP, 30, 20, 6, 0 },
            { HZ_GAP, 186, 22, 6, 0 },
            { HZ_GAP, 110, 16, 4, 0 } },
        {   { HZ_CEILING, 50, ART_GIRDER, 22, 0 },
            { HZ_BLOCK, 140, ART_DRUMS, 0, 0 },
            { HZ_DANGLER, 92, ART_BALL, 0, BILLS(4, 0) },
            { HZ_GAP, 186, 18, 3, 0 },
            { HZ_GAP, 116, 14, 4, 0 },
            { HZ_GAP, 18, 20, 6, 0 },
            { HZ_GAP, 166, 14, 5, 0 } },
        {   { HZ_BLOCK, 20, ART_VAN, 0, 0 },
            { HZ_GAP, 100, 24, 3, 0 },
            { HZ_DANGLER, 200, ART_BALL, 0, 0 },
            { HZ_GAP, 165, 14, 5, 0 },
            { HZ_GAP, 60, 20, 6, 0 },
            { HZ_GAP, 136, 30, 6, 0 } },
    },
};

/* The test track: the same street with nothing on it, for the harness's
   physics and drawing checks (make test builds it as build/empty with
   -DSCENE=scene_test; tools/ref/ref.c takes "test"). */
#if !defined(__CC65__) || defined(SCENE)    /* (not in the game itself) */
const struct scene scene_test = {
    "TEST TRACK", 1, VH_BIKE, GROUND_ROAD, &pal_city,
    {   { { HZ_NONE, 0, 0, 0, 0 } } },
};

/* F6a's stress scene (DESIGN "Step budget"): two hazards that move every step
   (the police car and a bat) and three jets, staggered, to prove the step
   rules before any film is laid out (make test builds it as build/stress). */
const struct scene scene_stress = {
    "STRESS TEST", 1, VH_BIKE, GROUND_ROAD, &pal_city,
    {
        {   { HZ_JET, 100, ART_FLARE, 0, 0 },
            { HZ_BLOCK, 180, ART_DRUMS, 0, 0 } },
        {   { HZ_RUNNER, 150, ART_POLICE, 0, 0 } },
        {   { HZ_JET, 60, ART_FLARE, 5, 0 },
            { HZ_JET, 160, ART_FLARE, 10, 0 } },
        {   { HZ_BLOCK, 100, ART_DRUMS, 0, 0 },
            { HZ_FLYER, 170, ART_BAT, 24, 0 } },
    },
};
#endif

/* Night harbour: black sky over the channel, the rider in white; the water
   from the surface line down is blue with cyan waves (and rocks, where a gap
   shows). */
static const struct palette pal_harbour = {
    BLUE, WHITE,
    {   BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,        /* 0-7 */
        BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,        /* 8-15 */
        BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,        /* 16-23 */
        BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,        /* 24-31 */
        BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,        /* 32-39 */
        BLACK, BLACK, BLACK, BLACK, BLUE, BLUE, BLUE, BLUE },          /* 40-47 */
    {   CYAN, CYAN, CYAN, CYAN, WHITE, WHITE, WHITE, WHITE,
        WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
        WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
        WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
        WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
        WHITE, WHITE, WHITE, WHITE, CYAN, CYAN, CYAN, CYAN },
};

/* The harbour: billing 3 moves the patrol boat up the channel, 4 swings a
   crate on the crane's hook between the footbridge and the buoy, 5 hangs a
   second footbridge over the warm-up track. */
const struct scene scene_rhr2 = {
    "RUSH HOUR RUMBLE", 2, VH_JETSKI, GROUND_WATER, &pal_harbour,
    {
        {   { HZ_BLOCK, 100, ART_BUOY, 0, 0 },
            { HZ_BLOCK, 190, ART_BUOY, 0, 0 },
            { HZ_CEILING, 140, ART_BRIDGE, 24, BILLS(5, 0) },
            { HZ_GAP, 150, 14, 3, BILLS(0, 4) },
            { HZ_GAP, 60, 16, 4, 0 },
            { HZ_GAP, 84, 14, 5, 0 },
            { HZ_GAP, 174, 14, 6, 0 } },
        {   { HZ_RAMP, 40, ART_SLIPWAY, 0, 0 },
            { HZ_RUNNER, 160, ART_BOAT, 0, BILLS(0, 2) },
            { HZ_RUNNER, 80, ART_BOAT, 0, BILLS(3, 0) },
            { HZ_GAP, 110, 18, 4, 0 },
            { HZ_GAP, 20, 16, 5, 0 },
            { HZ_GAP, 180, 30, 6, 0 },
            { HZ_GAP, 70, 20, 5, 0 } },
        {   { HZ_CEILING, 60, ART_BRIDGE, 24, 0 },
            { HZ_BLOCK, 150, ART_BUOY, 0, 0 },
            { HZ_DANGLER, 118, ART_CRATE, 0, BILLS(4, 0) },
            { HZ_GAP, 200, 18, 3, 0 },
            { HZ_GAP, 170, 14, 5, 0 },
            { HZ_GAP, 20, 24, 6, 0 },
            { HZ_GAP, 98, 14, 5, 0 } },
        {   { HZ_BLOCK, 30, ART_BUOY, 0, 0 },
            { HZ_RAMP, 110, ART_SLIPWAY, 0, 0 },
            { HZ_GAP, 80, 16, 3, 0 },
            { HZ_GAP, 180, 20, 4, 0 },
            { HZ_GAP, 150, 18, 5, 0 },
            { HZ_GAP, 54, 22, 6, 0 } },
    },
};

const struct scene *scene_only;

uint8_t hz_shown(const struct hazard *h, uint8_t billing)
{
    uint8_t lo = h->bills & 15, hi = h->bills >> 4;
    if (h->kind == HZ_GAP && billing < h->b)
        return 0;
    return billing >= lo && (!hi || billing <= hi);
}

const struct scene *scene_at(uint8_t run_scene)
{
    if (scene_only)
        return scene_only;
    if (run_scene & 1)
        return &scene_rhr1;
    return &scene_rhr2;
}
