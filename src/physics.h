/* physics.h - rider physics (DESIGN "Physics and core mechanics"), measured
   in the BBC original by M1 (docs/M1-MEASUREMENTS.md, "In Oric units").

   The model keeps the original's own units, run at 50 Hz, which is exactly
   how the original moves (it adds the speed to x three times a 60 ms step):
   - speed 0..250, +1 a step while Accelerate is held (start to top: 5.0 s);
   - x in 1/256 BBC screen byte, x += speed every step; a track is 75 bytes;
   - on screen, 1 BBC byte = 228 / 80 = 2.85 pixels (model_px). */
#ifndef PHYSICS_H
#define PHYSICS_H

#define PLAY_W       228u                  /* play width in pixels (cells 2-39) */
#define NTRACKS      4

#define TRACK_BYTES  75u                   /* the rider wraps on reaching byte 75 */
#define TRACK_W_FP   (TRACK_BYTES * 256u)  /* one track in 1/256 byte: 19,200 */
#define SPEED_START  0u                    /* every take starts standing */
#define SPEED_TOP    250u                  /* 2.78 px a step */
#define SPEED_ACCEL  1u                    /* a step, while held, also in the air */
#define JUMP_STEPS   85u                   /* 84 steps in the air (1.68 s), then land */
#define JUMP_REPEAT  3u                    /* Space held: steps on the road between jumps */
#define JUMP_APEX    20u

/* Height above the road at each step of a jump: the original's 28-entry arc
   (M1 "Jump"), each entry held for 3 of our steps, as the original holds it
   for one of its 60 ms steps. */
#define A3(v) v, v, v
#define JUMP_ARC_DATA 0, \
    A3(3), A3(6), A3(8), A3(11), A3(13), A3(14), A3(16), A3(17), A3(18), A3(19), \
    A3(19), A3(20), A3(20), A3(20), A3(20), A3(20), A3(20), A3(19), A3(19), A3(18), \
    A3(17), A3(16), A3(14), A3(13), A3(11), A3(8), A3(6), A3(3)

/* The slipway (F5, DESIGN "Ramp"): a jump 1.5 times as long, same height:
   the same arc with its entries held 4 and 5 steps in turn (126 in the air). */
#define JUMP_LONG    127u
#define A4(v) v, v, v, v
#define A5(v) v, v, v, v, v
#define JUMP_LONG_DATA 0, \
    A4(3), A5(6), A4(8), A5(11), A4(13), A5(14), A4(16), A5(17), A4(18), A5(19), \
    A4(19), A5(20), A4(20), A5(20), A4(20), A5(20), A4(20), A5(19), A4(19), A5(18), \
    A4(17), A5(16), A4(14), A5(13), A4(11), A5(8), A4(6), A5(3)

#endif
