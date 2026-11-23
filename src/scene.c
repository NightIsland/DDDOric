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

const struct scene scene_rhr1 = {
    "RUSH HOUR RUMBLE", 1, VH_BIKE, GROUND_ROAD, &pal_city,
    {
        {   { HZ_BLOCK, 110, ART_DRUMS, 0 },
            { HZ_GAP, 180, 12, 3 },
            { HZ_GAP, 50, 14, 4 },
            { HZ_GAP, 94, 14, 5 },
            { HZ_GAP, 150, 26, 6 } },
        /* the police car has a lane of its own (Bob) */
        {   { HZ_RUNNER, 170, ART_POLICE, 0 },
            { HZ_GAP, 70, 14, 3 },
            { HZ_GAP, 130, 18, 5 },
            { HZ_GAP, 30, 20, 6 },
            { HZ_GAP, 186, 22, 6 },
            { HZ_GAP, 110, 16, 4 } },
        {   { HZ_CEILING, 50, ART_GIRDER, 22 },
            { HZ_BLOCK, 140, ART_DRUMS, 0 },
            { HZ_GAP, 186, 18, 3 },
            { HZ_GAP, 100, 16, 4 },
            { HZ_GAP, 18, 20, 6 },
            { HZ_GAP, 166, 14, 5 } },
        {   { HZ_BLOCK, 20, ART_VAN, 0 },
            { HZ_GAP, 100, 24, 3 },
            { HZ_DANGLER, 200, ART_BALL, 0 },
            { HZ_GAP, 165, 14, 5 },
            { HZ_GAP, 60, 20, 6 },
            { HZ_GAP, 140, 12, 6 } },
    },
};

/* The test track: the same street with nothing on it, for the harness's
   physics and drawing checks (make test builds it as build/empty with
   -DSCENE=scene_test; tools/ref/ref.c takes "test"). */
const struct scene scene_test = {
    "TEST TRACK", 1, VH_BIKE, GROUND_ROAD, &pal_city,
    {   { { HZ_NONE, 0, 0, 0 } } },
};

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

const struct scene scene_rhr2 = {
    "RUSH HOUR RUMBLE", 2, VH_JETSKI, GROUND_WATER, &pal_harbour,
    {
        {   { HZ_BLOCK, 100, ART_BUOY, 0 },
            { HZ_BLOCK, 190, ART_BUOY, 0 },
            { HZ_GAP, 150, 14, 3 },
            { HZ_GAP, 60, 16, 4 },
            { HZ_GAP, 84, 14, 5 },
            { HZ_GAP, 160, 28, 6 } },
        {   { HZ_RAMP, 40, ART_SLIPWAY, 0 },
            { HZ_RUNNER, 160, ART_BOAT, 0 },
            { HZ_GAP, 110, 18, 4 },
            { HZ_GAP, 20, 16, 5 },
            { HZ_GAP, 180, 30, 6 },
            { HZ_GAP, 70, 20, 5 } },
        {   { HZ_CEILING, 60, ART_BRIDGE, 24 },
            { HZ_BLOCK, 150, ART_BUOY, 0 },
            { HZ_GAP, 200, 18, 3 },
            { HZ_GAP, 120, 18, 5 },
            { HZ_GAP, 20, 24, 6 },
            { HZ_GAP, 98, 14, 5 } },
        {   { HZ_BLOCK, 30, ART_BUOY, 0 },
            { HZ_RAMP, 110, ART_SLIPWAY, 0 },
            { HZ_GAP, 80, 16, 3 },
            { HZ_GAP, 180, 20, 4 },
            { HZ_GAP, 150, 18, 5 },
            { HZ_GAP, 54, 22, 6 } },
    },
};

const struct scene *scene_only;

const struct scene *scene_at(uint8_t run_scene)
{
    if (scene_only)
        return scene_only;
    if (run_scene & 1)
        return &scene_rhr1;
    return &scene_rhr2;
}
