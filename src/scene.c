/* scene.c - scene layouts and film palettes (src/scene.h).

   Film 1, Rush Hour Rumble (DESIGN "Films, scenes and progression"): scene 1
   a city street at night on the motorbike, scene 2 the harbour chase on the
   jet-ski (F5).  Each layout serves all six billings: its gaps carry the
   lowest billing that shows them, and the moving hazards speed up with the
   billing (src/model.c).  tools/solve proves every scene beatable at every
   billing; the winning runs are in tests/keys/win-*.keys. */
#include "scene.h"
#include "art.h"

#define SEASON 12                   /* (src/model.h) */

#define BLACK 0
#define RED 1
#define GREEN 2
#define YELLOW 3
#define BLUE 4
#define MAGENTA 5
#define CYAN 6
#define WHITE 7
#define R(n, c) ((n) << 3 | (c))     /* a palette run: n lines of colour c */

/* the six films' names, each stored once (the scenes and the title screen) */
static const char film_rhr[] = "RUSH HOUR RUMBLE";
static const char film_dd[] = "DUST AT DAWN";
static const char film_nh[] = "NIGHT OF THE HEARSE";
static const char film_mp[] = "MOONBASE PANIC";
static const char film_dk[] = "DRAGON'S KEEP";
static const char film_cf[] = "COLD FRONT";
const char * const films[6] = {
    film_rhr, film_dd, film_nh,
    film_mp, film_dk, film_cf
};

/* Night city: a blue sky fading through dithered lines to black where the
   rider runs; white ink from band line 4 down to the road, so the rider never
   changes colour in a jump (apex: band line 8, or 4 with F0's placeholder arc); a yellow kerb and white lane marks. */
static const uint8_t pal_city[] = {
    RED, YELLOW,
    /* paper */
        R(6, BLUE), R(1, BLACK), R(2, BLUE), R(2, BLACK), R(1, BLUE), R(3, BLACK), R(1, BLUE),
        R(31, BLACK), R(1, BLACK),
    /* ink */
        R(4, CYAN), R(31, WHITE), R(9, WHITE), R(1, YELLOW), R(2, WHITE), R(1, BLACK),
};

/* Each billing harder (Bob, BUILD 17: not only gaps): billing 2 speeds the
   movers up; 3 adds small gaps and moves the police car to meet you sooner;
   4 adds more gaps and a second wrecking ball under the girder's exit; 5
   puts gaps in front of the hazards and gives the warm-up track a girder; 6
   breaks the road wide. */
const struct scene scene_rhr1 = {
    film_rhr, 1, VH_BIKE, GROUND_ROAD, pal_city,
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
    "TEST TRACK", 1, VH_BIKE, GROUND_ROAD, pal_city,
    {   { { HZ_NONE, 0, 0, 0, 0 } } },
};

/* F6a's stress scene (DESIGN "Step budget"): two hazards that move every step
   (the police car and a bat) and three jets, staggered, to prove the step
   rules before any film is laid out (make test builds it as build/stress). */
const struct scene scene_stress = {
    "STRESS TEST", 1, VH_BIKE, GROUND_ROAD, pal_city,
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
static const uint8_t pal_harbour[] = {
    BLUE, WHITE,
    /* paper */
        R(31, BLACK), R(13, BLACK), R(4, BLUE),
    /* ink */
        R(4, CYAN), R(31, WHITE), R(9, WHITE), R(4, CYAN),
};

/* The harbour: billing 3 moves the patrol boat up the channel, 4 swings a
   crate on the crane's hook between the footbridge and the buoy, 5 hangs a
   second footbridge over the warm-up track. */
const struct scene scene_rhr2 = {
    film_rhr, 2, VH_JETSKI, GROUND_WATER, pal_harbour,
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

/* ---------------------------------------------- Dust at Dawn (F6b, Western) */

/* Main Street at sunrise: a magenta sky dithered through red to yellow,
   everything in black silhouette against it; red dust with yellow specks. */
static const uint8_t pal_dawn[] = {
    YELLOW, RED,
    /* paper */
        R(10, MAGENTA), R(1, RED), R(1, MAGENTA), R(1, RED), R(1, MAGENTA), R(4, RED),
        R(1, YELLOW), R(1, RED), R(1, YELLOW), R(1, RED), R(22, YELLOW), R(4, RED),
    /* ink */
        R(4, YELLOW), R(31, BLACK), R(9, BLACK), R(4, YELLOW),
};

/* Scene 1 on a horse.  Its signature: jump arcs timed around the saloon
   balcony (a high ceiling block: it clips only the top of a jump).  Billing
   3 rolls the tumbleweed in sooner, 4 hangs a second grain sack by the
   balcony, 5 puts a hitching rail in the warm-up; gaps (dry washes) from 3. */
const struct scene scene_dd1 = {
    film_dd, 1, VH_HORSE, GROUND_DIRT, pal_dawn,
    {
        {   { HZ_BLOCK, 90, ART_TROUGH, 0, 0 },
            { HZ_BLOCK, 170, ART_HITCH, 0, 0 },
            { HZ_BLOCK, 40, ART_HITCH, 0, BILLS(5, 0) },
            { HZ_GAP, 130, 12, 3, 0 },
            { HZ_GAP, 200, 16, 4, 0 },
            { HZ_GAP, 60, 14, 6, BILLS(0, 4) } },
        /* the tumbleweed's lane */
        {   { HZ_RUNNER, 150, ART_TUMBLEWEED, 0, BILLS(0, 2) },
            { HZ_RUNNER, 80, ART_TUMBLEWEED, 0, BILLS(3, 0) },
            { HZ_GAP, 60, 14, 3, 0 },
            { HZ_GAP, 120, 16, 5, 0 },
            { HZ_GAP, 190, 20, 6, 0 } },
        {   { HZ_CEILING, 50, ART_BALCONY, 9, 0 },
            { HZ_BLOCK, 160, ART_TROUGH, 0, 0 },
            { HZ_DANGLER, 120, ART_SACK, 0, BILLS(4, 0) },
            { HZ_GAP, 140, 12, 3, 0 },
            { HZ_GAP, 200, 16, 5, 0 },
            { HZ_GAP, 18, 18, 6, 0 } },
        {   { HZ_BLOCK, 40, ART_HITCH, 0, 0 },
            { HZ_DANGLER, 130, ART_SACK, 0, 0 },
            { HZ_GAP, 80, 14, 3, 0 },
            { HZ_GAP, 170, 16, 4, 0 },
            { HZ_GAP, 200, 20, 6, 0 } },
    },
};

/* The mine by lamplight: black rock with red seams above, the cart in yellow,
   white rails on red sleepers. */
static const uint8_t pal_mine[] = {
    BLACK, YELLOW,
    /* paper */
        R(31, BLACK), R(17, BLACK),
    /* ink */
        R(4, RED), R(31, YELLOW), R(9, YELLOW), R(1, WHITE), R(2, RED), R(1, WHITE),
};

/* Scene 2 in a mine cart.  Its signature: the beam squeeze, low pit beams
   with an ore pile between.  The buckled rail's long jump clears the fallen
   rock; land, then time the gas flare.  Billing 3 sends the stray cart in
   sooner, 4 hangs the rock bucket in the squeeze, 5 lights a second flare
   in the warm-up; gaps (broken track) from 3. */
const struct scene scene_dd2 = {
    film_dd, 2, VH_CART, GROUND_RAILS, pal_mine,
    {
        {   { HZ_CEILING, 60, ART_BEAM, 24, 0 },
            { HZ_BLOCK, 150, ART_ORE, 0, 0 },
            { HZ_JET, 205, ART_FLARE, 8, BILLS(5, 0) },
            { HZ_GAP, 100, 14, 3, 0 },
            { HZ_GAP, 20, 16, 4, 0 },
            { HZ_GAP, 180, 14, 6, 0 } },
        {   { HZ_RAMP, 30, ART_BUCKLED, 0, 0 },
            { HZ_BLOCK, 100, ART_ROCK, 0, 0 },
            { HZ_JET, 190, ART_FLARE, 0, 0 },
            { HZ_GAP, 70, 14, 4, 0 },
            { HZ_GAP, 150, 16, 5, 0 },
            { HZ_GAP, 120, 18, 6, 0 } },
        /* the stray ore cart's lane */
        {   { HZ_RUNNER, 170, ART_ORECART, 0, BILLS(0, 2) },
            { HZ_RUNNER, 90, ART_ORECART, 0, BILLS(3, 0) },
            { HZ_GAP, 60, 14, 3, 0 },
            { HZ_GAP, 140, 16, 4, 0 },
            { HZ_GAP, 200, 20, 6, 0 } },
        {   { HZ_CEILING, 40, ART_BEAM, 24, 0 },
            { HZ_CEILING, 120, ART_BEAM, 24, 0 },
            { HZ_BLOCK, 180, ART_ORE, 0, 0 },
            { HZ_DANGLER, 80, ART_BUCKET, 0, BILLS(4, 0) },
            { HZ_GAP, 150, 14, 3, 0 },
            { HZ_GAP, 100, 12, 5, 0 },
            { HZ_GAP, 20, 16, 6, 0 } },
    },
};

/* Night of the Hearse (F6c).  The graveyard by moonlight: a deep blue sky
   going black, everything white, the cobbles white on black. */
static const uint8_t pal_grave[] = {
    BLACK, GREEN,
    /* paper */
        R(6, BLUE), R(1, BLACK), R(2, BLUE), R(1, BLACK), R(1, BLUE), R(31, BLACK), R(6, BLACK),
    /* ink */
        R(31, WHITE), R(17, WHITE),
};

/* Scene 1 on the hearse-trike.  Its signature: two every-step movers, the
   slow shuffling ghoul and the bat (don't jump while it is over you), and
   open graves at every billing.  Billing 3 wakes the ghoul to full speed, 4
   lowers a second spider by the lych-gate, 5 puts a gravestone in the
   warm-up; more graves from 3. */
const struct scene scene_nh1 = {
    film_nh, 1, VH_HEARSE, GROUND_COBBLES, pal_grave,
    {
        {   { HZ_BLOCK, 80, ART_GRAVE, 0, 0 },
            { HZ_BLOCK, 150, ART_ANGEL, 0, 0 },
            { HZ_BLOCK, 30, ART_GRAVE, 0, BILLS(5, 0) },
            { HZ_GAP, 200, 14, 3, 0 },
            { HZ_GAP, 115, 12, 4, 0 },
            { HZ_GAP, 55, 14, 6, BILLS(0, 4) } },
        /* the ghoul's lane: it shuffles at 0.75 times the runner speed */
        {   { HZ_RUNNER, 160, ART_GHOUL, 0x80 | 92, BILLS(0, 2) },
            { HZ_RUNNER, 120, ART_GHOUL, 0, BILLS(3, 0) },
            { HZ_GAP, 60, 16, 1, 0 },
            { HZ_GAP, 190, 14, 3, 0 },
            { HZ_GAP, 100, 14, 5, 0 } },
        {   { HZ_CEILING, 40, ART_LYCHGATE, 22, 0 },
            { HZ_GAP, 130, 20, 1, 0 },
            { HZ_DANGLER, 20, ART_SPIDER, 0, BILLS(4, 0) },
            { HZ_BLOCK, 190, ART_GRAVE, 0, BILLS(3, 0) },
            { HZ_GAP, 160, 14, 6, 0 } },
        /* the bat's lane, with a spider */
        {   { HZ_DANGLER, 110, ART_SPIDER, 0, 0 },
            { HZ_FLYER, 200, ART_BAT, 24, 0 },
            { HZ_GAP, 60, 14, 3, 0 },
            { HZ_GAP, 170, 16, 4, 0 },
            { HZ_GAP, 30, 14, 6, 0 } },
    },
};

/* The ghost train, red-lit: black figures against red, the track white on
   black below. */
static const uint8_t pal_ghost[] = {
    BLACK, RED,
    /* paper */
        R(4, BLACK), R(31, RED), R(9, RED), R(4, BLACK),
    /* ink */
        R(4, RED), R(31, BLACK), R(9, BLACK), R(1, WHITE), R(2, RED), R(1, WHITE),
};

/* Scene 2 in a ghost-train car.  Its signature: nothing moves every step,
   but everything springs: pop-up ghouls half a period apart, dropping
   skeletons, and the track hump's long jump over a coffin.  Billing 3 adds
   a coffin between the trapdoors, 4 a third trapdoor under the arch, 5 a
   third coffin; gaps (broken track) from 3. */
const struct scene scene_nh2 = {
    film_nh, 2, VH_GHOST, GROUND_RAILS, pal_ghost,
    {
        {   { HZ_BLOCK, 90, ART_COFFIN, 0, 0 },
            { HZ_BLOCK, 170, ART_COFFIN, 0, 0 },
            { HZ_BLOCK, 130, ART_COFFIN, 0, BILLS(5, 0) },
            { HZ_GAP, 50, 14, 3, 0 },
            { HZ_GAP, 130, 14, 4, BILLS(0, 4) },
            { HZ_GAP, 204, 16, 6, 0 } },
        {   { HZ_JET, 70, ART_POPGHOUL, 0, 0 },
            { HZ_JET, 160, ART_POPGHOUL, 8, 0 },
            { HZ_BLOCK, 115, ART_COFFIN, 0, BILLS(3, 0) },
            { HZ_GAP, 30, 14, 4, 0 },
            { HZ_GAP, 200, 16, 5, 0 } },
        {   { HZ_CEILING, 40, ART_ARCH, 22, 0 },
            { HZ_DANGLER, 150, ART_SKELETON, 0, 0 },
            { HZ_JET, 100, ART_POPGHOUL, 4, BILLS(4, 0) },
            { HZ_GAP, 190, 14, 3, 0 },
            { HZ_GAP, 125, 12, 6, 0 } },
        {   { HZ_RAMP, 20, ART_HUMP, 0, 0 },
            { HZ_BLOCK, 110, ART_COFFIN, 0, 0 },
            { HZ_DANGLER, 190, ART_SKELETON, 0, 0 },
            { HZ_GAP, 160, 14, 3, 0 },
            { HZ_GAP, 70, 14, 5, 0 },
            { HZ_GAP, 210, 12, 6, 0 } },
    },
};

/* Moonbase Panic (F6d), both scenes on the hover bike: its jump floats (100
   steps, 150 off a ramp; src/physics.h JUMP_FLOAT), so it carries further.
   Crater Plains: a black sky, the surface and everything on it cyan, the
   ground white. */
static const uint8_t pal_moon[] = {
    BLACK, CYAN,
    /* paper */
        R(31, BLACK), R(17, BLACK),
    /* ink */
        R(4, WHITE), R(31, CYAN), R(9, CYAN), R(4, WHITE),
};

/* Scene 1.  Its signature: two every-step movers again (the patrol robot
   and the drone under the gantry), craters at every billing, and the float:
   jumps that last longer, so the drone is over you for longer too.  Billing
   3 sends the robot sooner, 4 sets a canister by the crater, 5 plants a mast in the
   robot's lane; more craters from 3. */
const struct scene scene_mp1 = {
    film_mp, 1, VH_HOVER, GROUND_MOON, pal_moon,
    {
        {   { HZ_BLOCK, 70, ART_CANISTER, 0, 0 },
            { HZ_BLOCK, 170, ART_MAST, 0, 0 },
            { HZ_GAP, 120, 14, 3, 0 },
            { HZ_GAP, 200, 14, 4, 0 },
            { HZ_GAP, 30, 16, 6, 0 } },
        /* the robot's lane */
        {   { HZ_RUNNER, 160, ART_ROBOT, 0, BILLS(0, 2) },
            { HZ_RUNNER, 100, ART_ROBOT, 0, BILLS(3, 0) },
            { HZ_BLOCK, 40, ART_MAST, 0, BILLS(5, 0) },
            { HZ_GAP, 190, 14, 3, 0 },
            { HZ_GAP, 140, 12, 6, 0 } },
        {   { HZ_GAP, 60, 24, 1, 0 },
            { HZ_DANGLER, 160, ART_CLAW, 0, 0 },
            { HZ_BLOCK, 115, ART_CANISTER, 0, BILLS(4, 0) },
            { HZ_GAP, 200, 14, 3, 0 },
            { HZ_GAP, 130, 14, 6, BILLS(0, 3) } },
        /* the drone's lane, under the gantry */
        {   { HZ_CEILING, 40, ART_GANTRY, 10, 0 },
            { HZ_BLOCK, 140, ART_MAST, 0, 0 },
            { HZ_FLYER, 200, ART_DRONE, 23, 0 },
            { HZ_GAP, 100, 14, 3, 0 },
            { HZ_GAP, 190, 14, 4, 0 },
            { HZ_GAP, 20, 14, 6, 0 } },
    },
};

/* The reactor corridor: blue walls, white figures, a floor of yellow
   warning stripes. */
static const uint8_t pal_reactor[] = {
    BLACK, CYAN,
    /* paper */
        R(31, BLUE), R(13, BLUE), R(4, BLACK),
    /* ink */
        R(31, WHITE), R(13, WHITE), R(4, YELLOW),
};

/* Scene 2.  Its signature: the launch pad's long float over two laser
   gates, across the wrap into track 4, under a conduit that catches a
   launch taken too fast.  Billing 3 adds a crate before the first gate, 4
   a crate by the blast door, 5 a crate in the warm-up; gaps (pits) from 3. */
const struct scene scene_mp2 = {
    film_mp, 2, VH_HOVER, GROUND_STRIPES, pal_reactor,
    {
        {   { HZ_BLOCK, 80, ART_CRATE, 0, 0 },
            { HZ_JET, 160, ART_LASER, 0, 0 },
            { HZ_BLOCK, 125, ART_CRATE, 0, BILLS(3, 0) },
            { HZ_BLOCK, 30, ART_CRATE, 0, BILLS(5, 0) },
            { HZ_GAP, 110, 12, 3, BILLS(0, 2) },
            { HZ_GAP, 200, 16, 4, 0 } },
        {   { HZ_CEILING, 30, ART_BLASTDOOR, 24, 0 },
            { HZ_DANGLER, 130, ART_CRUSHER, 0, 0 },
            { HZ_BLOCK, 200, ART_CRATE, 0, 0 },
            { HZ_BLOCK, 80, ART_CRATE, 0, BILLS(4, 0) },
            { HZ_GAP, 170, 14, 3, 0 },
            { HZ_GAP, 100, 12, 6, 0 } },
        {   { HZ_RAMP, 40, ART_LAUNCHPAD, 0, 0 },
            { HZ_JET, 110, ART_LASER, 4, 0 },
            { HZ_JET, 170, ART_LASER, 12, 0 },
            { HZ_GAP, 140, 12, 4, 0 },
            { HZ_GAP, 200, 14, 6, 0 } },
        {   { HZ_CEILING, 90, ART_CONDUIT, 9, 0 },
            { HZ_DANGLER, 190, ART_CRUSHER, 0, 0 },
            { HZ_GAP, 40, 14, 3, 0 },
            { HZ_GAP, 160, 14, 5, 0 },
            { HZ_GAP, 70, 12, 6, 0 } },
    },
};

/* Dragon's Keep (F6e).  Castle Approach: a bright sky, everything black
   against it, the grass green on the sky below, so a gap (the moat too)
   shows the sky through it (Bob, BUILD 26: on black it looked like ground). */
static const uint8_t pal_hills[] = {
    BLACK, GREEN,
    /* paper */
        R(31, CYAN), R(17, CYAN),
    /* ink */
        R(31, BLACK), R(13, BLACK), R(4, GREEN),
};

/* Scene 1 on a dragon hatchling.  Its signature: the moat at every billing
   (28 px: it needs speed), and the charging knight, half as fast again as
   any runner.  Billing 3 sends the knight sooner, 4 stacks barrels before
   the second boulder, 5 puts a boulder before the gate; gaps from 3. */
const struct scene scene_dk1 = {
    film_dk, 1, VH_DRAGON, GROUND_GRASS, pal_hills,
    {
        {   { HZ_BLOCK, 60, ART_BOULDER, 0, 0 },
            { HZ_BLOCK, 150, ART_BARRELS, 0, 0 },
            { HZ_GAP, 100, 14, 3, 0 },
            { HZ_GAP, 200, 14, 4, 0 },
            { HZ_GAP, 20, 16, 6, 0 } },
        /* the knight's lane: he charges at 1.5 times the runner speed */
        {   { HZ_RUNNER, 180, ART_KNIGHT, 0x80 | 183, BILLS(0, 2) },
            { HZ_RUNNER, 120, ART_KNIGHT, 0x80 | 183, BILLS(3, 0) },
            { HZ_GAP, 60, 14, 3, 0 },
            { HZ_GAP, 150, 12, 5, 0 },
            { HZ_GAP, 200, 14, 6, 0 } },
        {   { HZ_GAP, 80, 28, 1, 0 },
            { HZ_BLOCK, 180, ART_BOULDER, 0, 0 },
            { HZ_BLOCK, 140, ART_BARRELS, 0, BILLS(4, 0) },
            { HZ_GAP, 30, 14, 3, 0 },
            { HZ_GAP, 210, 12, 6, 0 } },
        /* gate, then moat */
        {   { HZ_DANGLER, 70, ART_PORTCULLIS, 0, 0 },
            { HZ_GAP, 150, 28, 1, 0 },
            { HZ_BLOCK, 25, ART_BOULDER, 0, BILLS(5, 0) },
            { HZ_GAP, 200, 14, 3, 0 },
            { HZ_GAP, 110, 12, 6, 0 } },
    },
};

/* The moat by moonlight: torchlit yellow on black, the water blue. */
static const uint8_t pal_moat[] = {
    BLACK, YELLOW,
    /* paper */
        R(31, BLACK), R(13, BLACK), R(4, BLUE),
    /* ink */
        R(4, WHITE), R(31, YELLOW), R(9, YELLOW), R(4, CYAN),
};

/* Scene 2 on a magic carpet.  Its signature: fire: dragons' heads rising
   from the moat, two of them under the half-raised drawbridge's long jump
   (it clears them on or off), and the wyvern overhead.  Billing 3 adds a
   rock in the wyvern's lane, 4 one by the drawbridge, 5 one in the warm-up;
   gaps (rocks breaking the surface) from 3. */
const struct scene scene_dk2 = {
    film_dk, 2, VH_CARPET, GROUND_WATER, pal_moat,
    {
        {   { HZ_BLOCK, 80, ART_ROCK, 0, 0 },
            { HZ_JET, 170, ART_DRAGONHEAD, 0, 0 },
            { HZ_BLOCK, 35, ART_ROCK, 0, BILLS(5, 0) },
            { HZ_GAP, 120, 14, 3, 0 },
            { HZ_GAP, 210, 12, 6, 0 } },
        /* under the drawbridge */
        {   { HZ_CEILING, 50, ART_DRAWBRIDGE, 22, 0 },
            { HZ_BLOCK, 150, ART_ROCK, 0, 0 },
            { HZ_JET, 205, ART_DRAGONHEAD, 8, 0 },
            { HZ_BLOCK, 110, ART_ROCK, 0, BILLS(4, 0) },
            { HZ_GAP, 20, 14, 4, 0 },
            { HZ_GAP, 175, 12, 6, 0 } },
        /* the wyvern's lane */
        {   { HZ_FLYER, 200, ART_WYVERN, 24, 0 },
            { HZ_BLOCK, 100, ART_ROCK, 0, 0 },
            { HZ_BLOCK, 170, ART_ROCK, 0, BILLS(3, 0) },
            { HZ_GAP, 50, 14, 3, 0 },
            { HZ_GAP, 140, 14, 5, 0 } },
        /* over the fire */
        {   { HZ_RAMP, 30, ART_RAMPBRIDGE, 0, 0 },
            { HZ_JET, 100, ART_DRAGONHEAD, 4, 0 },
            { HZ_JET, 150, ART_DRAGONHEAD, 12, 0 },
            { HZ_BLOCK, 210, ART_ROCK, 0, 0 },
            { HZ_GAP, 185, 12, 4, 0 } },
    },
};

/* Cold Front (F6f), the season's finale.  The Glacier: a pale sky over a
   white snowfield, everything blue on white, crevasses filled dark. */
static const uint8_t pal_glacier[] = {
    BLACK, WHITE,
    /* paper */
        R(8, CYAN), R(31, WHITE), R(9, WHITE),
    /* ink */
        R(31, BLUE), R(17, BLUE),
};

/* Scene 1 on skis.  Its signature: crevasses at every billing and the
   abseiling agent dropping in just before one.  Billing 3 rolls the
   snowball in sooner, 4 builds a snowman past the crevasse, 5 plants a
   pine in the warm-up; more crevasses from 3. */
const struct scene scene_cf1 = {
    film_cf, 1, VH_SKIS, GROUND_SNOW, pal_glacier,
    {
        {   { HZ_BLOCK, 90, ART_SNOWMAN, 0, 0 },
            { HZ_BLOCK, 170, ART_PINE, 0, 0 },
            { HZ_BLOCK, 40, ART_PINE, 0, BILLS(5, 0) },
            { HZ_GAP, 130, 14, 3, 0 },
            { HZ_GAP, 210, 12, 4, 0 },
            { HZ_GAP, 18, 14, 6, BILLS(0, 4) } },
        /* the snowball's lane */
        {   { HZ_RUNNER, 160, ART_SNOWBALL, 0, BILLS(0, 2) },
            { HZ_RUNNER, 100, ART_SNOWBALL, 0, BILLS(3, 0) },
            { HZ_GAP, 60, 14, 3, 0 },
            { HZ_GAP, 200, 14, 5, 0 },
            { HZ_GAP, 140, 12, 6, 0 } },
        {   { HZ_CEILING, 40, ART_OVERHANG, 24, 0 },
            { HZ_GAP, 130, 22, 1, 0 },
            { HZ_BLOCK, 190, ART_SNOWMAN, 0, BILLS(4, 0) },
            { HZ_GAP, 95, 12, 3, 0 },
            { HZ_GAP, 210, 12, 6, 0 } },
        /* the agent */
        {   { HZ_DANGLER, 80, ART_AGENT, 0, 0 },
            { HZ_GAP, 150, 22, 1, 0 },
            { HZ_BLOCK, 205, ART_SNOWMAN, 0, 0 },
            { HZ_GAP, 30, 14, 3, 0 },
            { HZ_GAP, 120, 12, 6, 0 } },
    },
};

/* Avalanche Pass by night: cyan figures on black, the snow lit white. */
static const uint8_t pal_pass[] = {
    BLACK, WHITE,
    /* paper */
        R(31, BLACK), R(13, BLACK), R(4, WHITE),
    /* ink */
        R(4, WHITE), R(31, CYAN), R(9, CYAN), R(4, BLUE),
};

/* Scene 2 on a snowmobile, the season's last.  Its signature: the snow
   kicker's long jump threaded between two ski-lift chairs over two steam
   vents, with the enemy snowmobile and the autogyro both on the move.
   Billing 3 sends the enemy sooner, 4 opens the track past the vents, 5 a
   tank trap in the warm-up and another for the first vent (the step
   budget: two every-step movers and three jets at billing 6 starved the
   enemy's draw); gaps (crevasses) from 3. */
const struct scene scene_cf2 = {
    film_cf, 2, VH_SLED, GROUND_SNOW, pal_pass,
    {
        {   { HZ_BLOCK, 80, ART_TANKTRAP, 0, 0 },
            { HZ_JET, 160, ART_STEAM, 0, BILLS(0, 4) },
            { HZ_BLOCK, 160, ART_TANKTRAP, 0, BILLS(5, 0) },
            { HZ_BLOCK, 35, ART_TANKTRAP, 0, BILLS(5, 0) },
            { HZ_GAP, 120, 14, 3, 0 },
            { HZ_GAP, 200, 14, 6, 0 } },
        /* the enemy snowmobile's lane */
        {   { HZ_RUNNER, 170, ART_ENEMYSLED, 0, BILLS(0, 2) },
            { HZ_RUNNER, 110, ART_ENEMYSLED, 0, BILLS(3, 0) },
            { HZ_GAP, 60, 14, 3, 0 },
            { HZ_GAP, 200, 14, 4, 0 },
            { HZ_GAP, 140, 12, 6, 0 } },
        /* the kicker under the lift */
        {   { HZ_RAMP, 20, ART_BUCKLED, 0, 0 },
            { HZ_JET, 100, ART_STEAM, 4, 0 },
            { HZ_JET, 150, ART_STEAM, 12, 0 },
            { HZ_CEILING, 70, ART_LIFTCHAIR, 9, 0 },
            { HZ_CEILING, 180, ART_LIFTCHAIR, 9, 0 },
            { HZ_GAP, 205, 12, 4, 0 } },
        /* the autogyro's lane */
        {   { HZ_FLYER, 200, ART_AUTOGYRO, 22, 0 },
            { HZ_BLOCK, 60, ART_TANKTRAP, 0, 0 },
            { HZ_BLOCK, 180, ART_PINE, 0, 0 },
            { HZ_GAP, 120, 14, 3, 0 },
            { HZ_GAP, 25, 12, 5, 0 } },
    },
};

/* The season: 12 scenes, 6 films of 2 (DESIGN "Films, scenes and
   progression"). */
static const struct scene * const season[SEASON] = {
    &scene_rhr1, &scene_rhr2, &scene_dd1, &scene_dd2, &scene_nh1, &scene_nh2,
    &scene_mp1, &scene_mp2, &scene_dk1, &scene_dk2, &scene_cf1, &scene_cf2,
};

/* (GR_KERB only where line 44's paper is black: the kerb's byte takes the
   paper attribute's cell) */
const struct ground grounds[] = {
    {   { { 0x7F, 0x7F, 0x7F, 0x7F }, { 0x40, 0x40, 0x40, 0x40 },     /* road */
          { 0x7F, 0x7F, 0x40, 0x40 }, { 0x40, 0x40, 0x40, 0x40 } }, GR_KERB },
    {   { { 0x4C, 0x4C, 0x4C, 0x4C }, { 0x73, 0x73, 0x73, 0x73 },     /* water */
          { 0x40, 0x40, 0x40, 0x40 }, { 0x40, 0x46, 0x40, 0x46 } }, GR_FILL },
    {   { { 0x7F, 0x7F, 0x7F, 0x7F }, { 0x52, 0x40, 0x49, 0x40 },     /* dirt */
          { 0x40, 0x44, 0x40, 0x60 }, { 0x48, 0x40, 0x40, 0x42 } }, 0 },
    {   { { 0x7F, 0x7F, 0x7F, 0x7F }, { 0x4C, 0x40, 0x4C, 0x40 },     /* rails */
          { 0x4C, 0x40, 0x4C, 0x40 }, { 0x7F, 0x7F, 0x7F, 0x7F } }, 0 },
    {   { { 0x7F, 0x7F, 0x7F, 0x7F }, { 0x44, 0x50, 0x42, 0x48 },     /* cobbles */
          { 0x40, 0x40, 0x40, 0x40 }, { 0x52, 0x49, 0x44, 0x51 } }, 0 },
    {   { { 0x7F, 0x7F, 0x7F, 0x7F }, { 0x7B, 0x6F, 0x7E, 0x5F },     /* moon */
          { 0x55, 0x6A, 0x55, 0x6A }, { 0x44, 0x40, 0x50, 0x41 } }, 0 },
    {   { { 0x7F, 0x7F, 0x7F, 0x7F }, { 0x78, 0x78, 0x78, 0x78 },     /* stripes */
          { 0x5C, 0x5C, 0x5C, 0x5C }, { 0x4E, 0x4E, 0x4E, 0x4E } }, 0 },
    {   { { 0x7F, 0x7F, 0x7F, 0x7F }, { 0x6D, 0x5B, 0x76, 0x6D },     /* grass */
          { 0x52, 0x49, 0x64, 0x52 }, { 0x40, 0x44, 0x40, 0x50 } }, 0 },
    {   { { 0x7F, 0x7F, 0x7F, 0x7F }, { 0x40, 0x48, 0x40, 0x41 },     /* snow */
          { 0x44, 0x40, 0x50, 0x40 }, { 0x40, 0x42, 0x40, 0x60 } }, GR_FILL },
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
    return season[(run_scene - 1) % SEASON];
}
