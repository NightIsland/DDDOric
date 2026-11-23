/* scene.c - scene layouts and film palettes (src/scene.h).

   F1 has one scene to draw: film 1, Rush Hour Rumble, scene 1 (DESIGN "Films,
   scenes and progression"), a city street at night.  The layout is a first
   sketch; F3 makes it rideable and F5 tunes it against M1's numbers and proves
   it beatable at every billing. */
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
    "RUSH HOUR RUMBLE", 1, &pal_city,
    {
        {   { HZ_BLOCK, 120, ART_DRUMS, 0 },
            { HZ_BLOCK, 200, ART_VAN, 0 } },
        {   { HZ_RUNNER, 170, ART_POLICE, 0 } },      /* a lane of its own (Bob) */
        {   { HZ_CEILING, 50, ART_GIRDER, 22 },
            { HZ_BLOCK, 140, ART_DRUMS, 0 },
            { HZ_GAP, 186, 18, 3 } },
        {   { HZ_BLOCK, 40, ART_VAN, 0 },
            { HZ_GAP, 100, 24, 3 },
            { HZ_DANGLER, 176, ART_BALL, 0 } },
    },
};

/* The test track: the same street with nothing on it, for the harness's
   physics and drawing checks (make test builds it as build/empty with
   -DSCENE=scene_test; tools/ref/ref.c takes "test"). */
const struct scene scene_test = {
    "TEST TRACK", 1, &pal_city,
    {   { { HZ_NONE, 0, 0, 0 } } },
};
