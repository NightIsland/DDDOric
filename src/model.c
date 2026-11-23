/* model.c - the game model (src/model.h).  Portable C: cc65 and the host. */
#include "model.h"
#include "physics.h"
#include "art.h"

struct model m;

uint8_t hz_px[NHAZ], hz_drop[NHAZ];
static const struct scene *sc;
static const uint8_t art_w[] = { SCENERY_W_DATA }, art_h[] = { SCENERY_H_DATA };

/* Runner speed: M1 measured 1 BBC byte per 3 steps = 0.95 px a step; a
   record's b holds it in 1/128 px a step (bit 7: drives left), 0 = M1's. */
#define RUN_STD     122             /* 122 / 128 = 0.953 px a step */
#define DANGLE_RATE 3               /* steps per line (M1: 1 line a BBC step) */
#define RIDER_TOP_LINE 28           /* band line of the rider's top row on the road */
#define ROAD_TOP_LINE  44           /* band line of the road: hazards stand on it */

/* Every hazard slot (track * SC_MAX + i) as flat tables, set up by
   model_init, so the per-step code is plain byte arithmetic: cc65 spent
   ~2,800 cycles a step on the struct version (PLAN F3 Results). */
static uint8_t hk[NHAZ];            /* kind for collisions (HZ_NONE: none, or a gap
                                       this billing doesn't show) */
static uint8_t hw[NHAZ];            /* width in px (a gap's width) */
static uint8_t htop[NHAZ];          /* band line of its picture's top row */
static uint8_t hh[NHAZ];            /* its picture's height */
static const uint16_t *hcol[NHAZ];  /* its picture's column masks */
static uint8_t hz_frac[NHAZ];       /* runners: x fraction, 1/256 px */
static uint8_t hz_sp[NHAZ];         /*   speed, 1/256 px a step */
static uint8_t hz_right[NHAZ];      /*   1: drives right */
static uint8_t hz_max[NHAZ];        /*   wraps in 0 .. hz_max - 1 */
static uint8_t hz_t[NHAZ];          /* danglers: steps to the next line; bit 7: rising */
static uint8_t hz_x0[NHAZ];         /* x at the scene's start */

/* the moving hazards, in slot order (DESIGN: at most 4 a scene) */
#define MAXMOV 6
static uint8_t nmov, mov[MAXMOV];

/* the rider's x in px for each x / 32 (model_px) */
#define PXT_N (TRACK_W_FP / 32)
static uint8_t pxt[PXT_N];

/* the moving hazards back to their places (the others never move) */
static void hazards_start(void)
{
    uint8_t i, j;
    for (j = 0; j < nmov; ++j) {
        i = mov[j];
        hz_px[i] = hz_x0[i];
        hz_frac[i] = 0;
        hz_drop[i] = 0;
        hz_t[i] = DANGLE_RATE;
        if (hk[i] == HZ_DANGLER)
            htop[i] = DANGLE_TOP;
    }
}

static void hazards_step(void)
{
    uint8_t i, j, f, sum = 0;
    for (j = 0; j < nmov; ++j) {
        i = mov[j];
        if (hk[i] == HZ_RUNNER) {           /* drives on, and wraps at the edges */
            f = hz_frac[i];
            if (hz_right[i]) {
                hz_frac[i] = f + hz_sp[i];
                if (hz_frac[i] < f && ++hz_px[i] == hz_max[i])
                    hz_px[i] = 0;
            } else {
                hz_frac[i] = f - hz_sp[i];
                if (hz_frac[i] > f && hz_px[i]-- == 0)
                    hz_px[i] = hz_max[i] - 1;
            }
        } else if (!(--hz_t[i] & 0x7F)) {   /* a dangler: 1 line every 3 steps */
            if (hz_t[i] & 0x80) {
                if (--hz_drop[i] == 0)
                    hz_t[i] &= 0x7F;
            } else if (++hz_drop[i] == DANGLE_MAX) {
                hz_t[i] |= 0x80;
            }
            hz_t[i] = (hz_t[i] & 0x80) | DANGLE_RATE;
            htop[i] = DANGLE_TOP + hz_drop[i];
        }
        sum = (uint8_t)((sum << 1 | sum >> 7) + hz_px[i] + hz_drop[i]);
    }
    m.hz_sum = sum;
}

/* Crashing (Bob, BUILD 10: only on a real touch): where the rider's box
   meets a hazard's, their pictures' column masks (tools/mkart.py, bit r =
   row r) are compared column by column, the hazard's shifted by the
   difference in their top rows.  The rider's picture is the one main.c
   shows.  A gap: the wheels' middle over it, on the road. */
static const uint16_t hit_rider[3 * RIDER_W] = { HIT_RIDER_DATA };  /* ride_a, ride_b, jump */
static const uint16_t hit_scenery[] = { HIT_SCENERY_DATA };
static const uint8_t hit_ofs[] = { HIT_SCENERY_OFS };

static uint8_t crashed(void)
{
    uint8_t slot, n, k, x0, c, c1, rt;
    int8_t dy;
    const uint16_t *rc, *hc;
    rc = hit_rider + FR_JUMP * RIDER_W;     /* (not "?:": ORIC-LESSONS 5) */
    if (!m.jump_t) {
        rc = hit_rider + FR_RIDE_A * RIDER_W;
        if (m.px & 4)
            rc = hit_rider + FR_RIDE_B * RIDER_W;
    }
    rt = RIDER_TOP_LINE - m.height;
    for (slot = m.track * SC_MAX, n = SC_MAX; n; --n, ++slot) {
        k = hk[slot];
        if (!k)
            continue;
        x0 = hz_px[slot];
        if (k == HZ_GAP) {
            if (!m.jump_t && (uint8_t)(m.px + 6 - x0) < hw[slot])
                return 1;
            continue;
        }
        if (x0 >= (uint8_t)(m.px + RIDER_W) || (uint8_t)(x0 + hw[slot]) <= m.px)
            continue;                       /* apart across */
        dy = (int8_t)(htop[slot] - rt);
        if (dy >= RIDER_H || dy <= -(int8_t)hh[slot])
            continue;                       /* apart up and down */
        c = m.px;                           /* the shared columns c .. c1 - 1 */
        if (x0 > c)
            c = x0;
        c1 = (uint8_t)(x0 + hw[slot]);
        if (c1 > (uint8_t)(m.px + RIDER_W))
            c1 = m.px + RIDER_W;
        rc += c - m.px;
        hc = hcol[slot] + (c - x0);
        for (; c < c1; ++c, ++rc, ++hc) {
            if (dy >= 0) {
                if ((uint16_t)(*hc << dy) & *rc)
                    return 1;
            } else if ((uint16_t)(*rc << -dy) & *hc) {
                return 1;
            }
        }
        rc -= c1 - m.px;                    /* back to column 0 */
    }
    return 0;
}

static const uint8_t jump_arc[JUMP_STEPS] = { JUMP_ARC_DATA };

void model_scene_start(void)
{
    hazards_start();
    m.track = 0;
    m.x = 0;
    m.px = 0;
    m.speed = SPEED_START;
    m.jump_t = 0;
    m.height = 0;
    m.road_t = 255;
    m.stop_left = 1;
    m.paused = 0;
}

uint8_t model_px(void)
{
    return m.px;
}

static void run_start(void)
{
    m.scenes = 0;
    m.run_scene = 1;
    m.takes = TAKES;
    m.take_no = 1;
    m.fee = 0;
}

void model_init(const struct scene *s, uint8_t billing)
{
    uint8_t i, sp;
    uint16_t acc = 0;
    const struct hazard *h = &s->h[0][0];  /* the bands' hazards, all in a row */
    sc = s;
    /* x * 228 / 80 / 256: (x / 32) * 91 / 256, 0.2% short of exact (the
       rider ends at 213 px), by addition: no multiply each step */
    for (sp = 0; sp < 3; ++sp)              /* PXT_N = 600: three runs of 200 */
        for (i = 0; i < PXT_N / 3; ++i) {
            pxt[sp * (PXT_N / 3) + i] = acc >> 8;
            acc += 91;
        }
    nmov = 0;
    for (i = 0; i < NHAZ; ++i, ++h) {
        hk[i] = h->kind;
        hz_px[i] = hz_x0[i] = h->x;
        hz_drop[i] = 0;
        if (h->kind != HZ_GAP && h->kind != HZ_NONE) {
            hw[i] = art_w[h->a];
            hh[i] = art_h[h->a];
            hcol[i] = hit_scenery + hit_ofs[h->a];
            htop[i] = ROAD_TOP_LINE - hh[i];   /* blocks and runners stand on the road */
        }
        switch (h->kind) {
        case HZ_RUNNER:
            sp = h->b & 0x7F;
            hz_sp[i] = (sp ? sp : RUN_STD) * 2;
            hz_right[i] = h->b && !(h->b & 0x80);   /* 0: the default, leftwards */
            hz_max[i] = PLAY_W - hw[i];
            /* fall through */
        case HZ_DANGLER:
            if (nmov < MAXMOV)
                mov[nmov++] = i;
            break;
        case HZ_GAP:
            hw[i] = h->a;
            if (billing < h->b)
                hk[i] = HZ_NONE;
            break;
        case HZ_CEILING:
            htop[i] = h->b + 1 - hh[i];     /* b: its bottom row's band line */
            break;
        case HZ_BLOCK:
            break;
        default:                            /* jets and ramps: F6 */
            hk[i] = HZ_NONE;
        }
    }
    m.step = 0;
    m.best = 0;
    m.keys_prev = 0;
    m.events = 0;
    m.ev2 = 0;
    m.crash_t = 0;
    run_start();
    model_scene_start();
}

void __fastcall__ model_step(uint8_t in)
{
    uint8_t edge = in & (uint8_t)~m.keys_prev;

    m.keys_prev = in;
    m.events = 0;
    m.ev2 = 0;
    ++m.step;

    if (m.crash_t) {                        /* the crash pause: everything waits */
        if (--m.crash_t)
            return;
        if (!m.takes) {                     /* fired: F4 shows it; for now the run restarts */
            if (m.fee > m.best)
                m.best = m.fee;
            run_start();
            m.ev2 = EV2_FIRED;
        } else {
            ++m.take_no;
            m.ev2 = EV2_RETAKE;
        }
        model_scene_start();
        return;
    }

    if (m.paused) {                         /* any key-down resumes */
        if (edge) {
            m.paused = 0;
            m.events = EV_PAUSE;
        }
        return;
    }
    if (!(in & IN_GAME)) {                  /* Pause and Quit only with no game key */
        if (edge & IN_PAUSE) {              /* held: a ghost can't fake them */
            m.paused = 1;
            m.events = EV_PAUSE;
            return;
        }
        if (edge & IN_QUIT) {
            model_scene_start();
            m.events = EV_QUIT;
            return;
        }
    }

    if (in & IN_ACCEL) {                    /* speed only rises; also in the air */
        m.speed += SPEED_ACCEL;
        if (m.speed > SPEED_TOP)
            m.speed = SPEED_TOP;
    }

    if ((edge & IN_STOP) && m.stop_left) {  /* the one stop per take: dead stop, */
        m.speed = 0;                        /* also in mid-air, where the bike  */
        m.stop_left = 0;                    /* drops straight down through the  */
        m.events |= EV_STOP;                /* rest of the arc (M1, Bob)        */
    }
    if (m.jump_t == 0) {
        /* jump from the ground only: a key-down, or Space held, which jumps
           again after JUMP_REPEAT steps on the road (M1: one BBC step) */
        if ((edge & IN_JUMP) || ((in & IN_JUMP) && m.road_t >= JUMP_REPEAT)) {
            m.jump_t = 1;
            m.events |= EV_JUMP;
        } else if (m.road_t != 255) {
            ++m.road_t;
        }
    } else if (++m.jump_t >= JUMP_STEPS) {
        m.jump_t = 0;
        m.road_t = 1;                       /* the landing step is on the road */
        m.events |= EV_LAND;
    }
    m.height = jump_arc[m.jump_t];

    hazards_step();

    m.x += m.speed;                         /* wrap keeps height, arc and speed */
    if (m.x >= TRACK_W_FP) {
        m.x -= TRACK_W_FP;
        m.events |= EV_WRAP;
        m.fee += 10 * m.run_scene;          /* rule 5: each finished track pays */
        m.ev2 |= EV2_FEE;
        if (++m.track == NTRACKS) {         /* scene cleared */
            if (m.take_no == 1) {
                m.fee += BONUS_X * 10 * m.run_scene;
                m.ev2 |= EV2_BONUS;
            }
            ++m.scenes;
            ++m.run_scene;
            if (m.scenes % 3 == 0 && m.takes < TAKES) {
                ++m.takes;
                m.ev2 |= EV2_TAKE;
            }
            m.take_no = 1;
            model_scene_start();
            m.events |= EV_SCENE;
            return;
        }
    }
    m.px = pxt[m.x >> 5];
    if (crashed()) {                        /* rule 4 */
        m.speed = 0;
        --m.takes;
        m.crash_t = CRASH_STEPS;
        m.events |= EV_CRASH;
    }
}
