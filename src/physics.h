/* physics.h - rider physics constants (DESIGN "Physics and core mechanics").

   ALL PLACEHOLDERS: these are DESIGN's guesses, used by the F0 skeleton only so
   the model has something to step.  M1 measures the BBC original and F2 puts the
   real values here.  Units: positions and speeds in 1/256 pixel, one game step
   = 20 ms (50 Hz). */
#ifndef PHYSICS_H
#define PHYSICS_H

#define PLAY_W       228u                  /* play width in pixels (cells 2-39) */
#define TRACK_W_FP   (PLAY_W * 256u)       /* one track in 1/256 px */
#define NTRACKS      4

#define SPEED_START  128u                  /* PLACEHOLDER 0.5 px/step  (M1) */
#define SPEED_TOP    768u                  /* PLACEHOLDER 3.0 px/step  (M1) */
#define SPEED_ACCEL  6u                    /* PLACEHOLDER ~0.025 px/step per held step (M1) */
#define JUMP_STEPS   60u                   /* PLACEHOLDER 1.2 s jump  (M1) */
#define JUMP_APEX    24u                   /* PLACEHOLDER apex in px  (M1) */

/* PLACEHOLDER jump arc: height above the road at each step of a jump, a
   parabola with JUMP_APEX at the middle (made by a one-off Python line). */
#define JUMP_ARC_DATA \
     0,  2,  3,  5,  6,  7,  9, 10, 11, 12, 13, 14, 15, 16, 17, \
    18, 19, 19, 20, 21, 21, 22, 22, 23, 23, 23, 24, 24, 24, 24, \
    24, 24, 24, 24, 24, 23, 23, 23, 22, 22, 21, 21, 20, 19, 19, \
    18, 17, 16, 15, 14, 13, 12, 11, 10,  9,  7,  6,  5,  3,  2

#endif
