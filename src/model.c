/* model.c - the game model (src/model.h).  Portable C: cc65 and the host. */
#include "model.h"
#include "physics.h"
#include "art.h"

struct model m;
uint16_t best_fee[NBEST] = { 250, 200, 150, 100, 50 };     /* the film crew's */
char best_name[NBEST][3] = { {'D','I','R'}, {'C','A','M'}, {'S','F','X'}, {'M','U','A'}, {'G','A','F'} };

uint8_t hz_px[NHAZ], hz_drop[NHAZ];
static const uint8_t art_w[] = { SCENERY_W_DATA }, art_h[] = { SCENERY_H_DATA };

/* Runner speed: M1 measured 1 BBC byte per 3 steps = 0.95 px a step; a
   record's b holds it in 1/128 px a step (bit 7: drives left), 0 = M1's.
   Danglers move a line every DANGLE_RATE steps (M1: 1 line a BBC step).
   Both are M1's at billing 1 and speed up with the billing (DESIGN
   "Billings": slow, normal, normal, fast, fast, fastest): runners x 1, 1.25,
   1.5, 1.75 (quarters in run_mul); danglers a line every 3, 2.5, 2, 1.5
   steps (half steps in dangle_half). */
#define RUN_STD     122             /* 122 / 128 = 0.953 px a step */
#define DANGLE_RATE 3
static const uint8_t run_mul[BILLINGS + 1] = { 4, 4, 5, 5, 6, 6, 7 };
static const uint8_t dangle_half[BILLINGS + 1] = { 6, 6, 5, 5, 4, 4, 3 };
static uint8_t dangle_reload;       /* this billing's dangle_half */
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
static uint8_t hz_frac[NHAZ];       /* runners: x fraction, 1/128 px */
static uint8_t hz_sp[NHAZ];         /*   speed, 1/128 px a step (this billing's) */
static uint8_t hz_right[NHAZ];      /*   1: drives right */
static uint8_t hz_max[NHAZ];        /*   wraps in 0 .. hz_max - 1 */
static uint8_t hz_t[NHAZ];          /* danglers: half steps to the next line; bit 7: rising */
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
        hz_t[i] = dangle_reload;
        if (hk[i] == HZ_DANGLER)
            htop[i] = DANGLE_TOP;
    }
}

static void hazards_step(void)
{
    uint8_t i, j, f, n, sum = 0;
    for (j = 0; j < nmov; ++j) {
        i = mov[j];
        if (hk[i] == HZ_RUNNER) {           /* drives on, and wraps at the edges */
            /* the fraction counts up driving right and down driving left,
               a px for each carry or borrow (as F3 had it in 1/256 px) */
            f = hz_frac[i] - (hz_sp[i] & 0x7F);
            if (hz_right[i])
                f = hz_frac[i] + (hz_sp[i] & 0x7F);
            n = hz_sp[i] >> 7;              /* whole px this step: 0 .. 2 */
            if (f & 0x80) {
                ++n;
                f &= 0x7F;
            }
            hz_frac[i] = f;
            for (; n; --n) {
                if (hz_right[i]) {
                    if (++hz_px[i] == hz_max[i])
                        hz_px[i] = 0;
                } else if (hz_px[i]-- == 0) {
                    hz_px[i] = hz_max[i] - 1;
                }
            }
        } else {                            /* a dangler: a line when its count runs out */
            f = hz_t[i] & 0x7F;
            if (f > 2) {
                hz_t[i] -= 2;
            } else {
                n = hz_t[i] & 0x80;
                if (n) {
                    if (--hz_drop[i] == 0)
                        n = 0;
                } else if (++hz_drop[i] == DANGLE_MAX) {
                    n = 0x80;
                }
                hz_t[i] = n | (uint8_t)(f - 2 + dangle_reload);
                htop[i] = DANGLE_TOP + hz_drop[i];
            }
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
static const uint16_t hit_rider[2 * 3 * RIDER_W] = { HIT_RIDER_DATA };  /* per vehicle: ride_a, ride_b, jump */
static const uint16_t *hit_vh;      /* this scene's vehicle's */
static const uint16_t hit_scenery[] = { HIT_SCENERY_DATA };
static const uint8_t hit_ofs[] = { HIT_SCENERY_OFS };

static uint8_t crashed(void)
{
    uint8_t slot, n, k, x0, c, c1, rt;
    int8_t dy;
    const uint16_t *rc, *hc;
    rc = hit_vh + FR_JUMP * RIDER_W;        /* (not "?:": ORIC-LESSONS 5) */
    if (!m.jump_t) {
        rc = hit_vh + FR_RIDE_A * RIDER_W;
        if (m.px & 4)
            rc = hit_vh + FR_RIDE_B * RIDER_W;
    }
    rt = RIDER_TOP_LINE - m.height;
    for (slot = m.track * SC_MAX, n = SC_MAX; n; --n, ++slot) {
        k = hk[slot];
        if (!k || k == HZ_RAMP)
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
static const uint8_t jump_long[JUMP_LONG] = { JUMP_LONG_DATA };

/* on the ground with the wheels' middle on a slipway: it fires a long jump */
static uint8_t on_ramp(void)
{
    uint8_t slot, n;
    for (slot = m.track * SC_MAX, n = SC_MAX; n; --n, ++slot)
        if (hk[slot] == HZ_RAMP && (uint8_t)(m.px + 6 - hz_px[slot]) < hw[slot])
            return 1;
    return 0;
}

void model_scene_start(void)
{
    hazards_start();
    m.track = 0;
    m.x = 0;
    m.px = 0;
    m.speed = SPEED_START;
    m.jump_t = 0;
    m.jump_long = 0;
    m.height = 0;
    m.road_t = 255;
    m.stop_left = 1;
    m.paused = 0;
}

uint8_t model_px(void)
{
    return m.px;
}

/* billing b starts at the season's scene (b - 1) x 12 + 1 (DESIGN "Scoring") */
static void run_start(void)
{
    uint8_t b;
    m.scenes = 0;
    m.run_scene = 1;
    for (b = m.billing; b > 1; --b)
        m.run_scene += SEASON;
    m.scene_fee = 0;
    m.takes = TAKES;
    m.take_no = 1;
    m.fee = 0;
    model_billing(m.billing);           /* the run's first scene */
}

/* The scene at m.run_scene, set up for m.billing: every hazard slot
   (track * SC_MAX + i) as flat tables, gaps only where the billing shows
   them, moving hazards at the billing's speed. */
static void scene_load(void)
{
    uint8_t i, sp;
    const struct hazard *h;
    m.scene = scene_at(m.run_scene);
    h = &m.scene->h[0][0];                  /* the bands' hazards, all in a row */
    hit_vh = hit_rider + m.scene->vehicle * (3 * RIDER_W);
    dangle_reload = dangle_half[m.billing];
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
            if (!sp)
                sp = RUN_STD;
            hz_sp[i] = (uint8_t)(((uint16_t)sp * run_mul[m.billing]) >> 2);
            hz_right[i] = h->b && !(h->b & 0x80);   /* 0: the default, leftwards */
            hz_max[i] = PLAY_W - hw[i];
            /* fall through */
        case HZ_DANGLER:
            if (nmov < MAXMOV)
                mov[nmov++] = i;
            break;
        case HZ_GAP:                        /* gaps show from the billing in b */
            hw[i] = h->a;
            if (m.billing < h->b)
                hk[i] = HZ_NONE;
            break;
        case HZ_CEILING:
            htop[i] = h->b + 1 - hh[i];     /* b: its bottom row's band line */
            break;
        case HZ_BLOCK:
        case HZ_RAMP:
            break;
        default:                            /* jets: F6 */
            hk[i] = HZ_NONE;
        }
    }
    hazards_start();
}

void model_init(uint8_t billing)
{
    uint8_t i, sp;
    uint16_t acc = 0;
    /* x * 228 / 80 / 256: (x / 32) * 91 / 256, 0.2% short of exact (the
       rider ends at 213 px), by addition: no multiply each step */
    for (sp = 0; sp < 3; ++sp)              /* PXT_N = 600: three runs of 200 */
        for (i = 0; i < PXT_N / 3; ++i) {
            pxt[sp * (PXT_N / 3) + i] = acc >> 8;
            acc += 91;
        }
    m.step = 0;
    m.best = 0;
    m.keys_prev = 0;
    m.key_prev = 0;
    m.events = 0;
    m.ev2 = 0;
    m.crash_t = 0;
    m.sound = 1;
    m.mode = M_TITLE;
    m.t = 0;
    m.billing = billing;
    run_start();
    model_scene_start();
}

/* the billing (title screen, a new season): reloads the scene for it */
void model_billing(uint8_t billing)
{
    m.billing = billing;
    scene_load();
}

static void go(uint8_t mode)
{
    m.mode = mode;
    m.t = 0;
}

static void best_keep(void)
{
    if (m.fee > m.best)
        m.best = m.fee;
}

/* after "YOU'RE FIRED!": a place in the top five? (ties go below) */
static void fired_next(void)
{
    uint8_t i, j;
    for (i = 0; i < NBEST && m.fee <= best_fee[i]; ++i)
        ;
    if (i == NBEST) {
        go(M_TITLE);
        return;
    }
    for (j = NBEST - 1; j > i; --j) {
        best_fee[j] = best_fee[j - 1];
        best_name[j][0] = best_name[j - 1][0];
        best_name[j][1] = best_name[j - 1][1];
        best_name[j][2] = best_name[j - 1][2];
    }
    best_fee[i] = m.fee;
    best_name[i][0] = best_name[i][1] = best_name[i][2] = '-';
    m.name_slot = i;
    m.name_len = 0;
    go(M_NAME);
}

/* everything but riding: the title screen, the cards, the crash pause, the
   initials and the quit question.  edge: game keys gone down; kd: a key code
   gone down (0: none).  Returns 1 when the step is a riding step. */
static uint8_t frame_step(uint8_t edge, uint8_t kd)
{
    char *nm;
    switch (m.mode) {
    case M_TITLE:
        if (kd == '1') {
            run_start();
            model_scene_start();
            m.ev2 = EV2_RUN;
            go(M_CARD);
        } else if (kd == 'L') {             /* (not "?:": ORIC-LESSONS 5) */
            if (m.billing == BILLINGS)
                m.billing = 0;
            model_billing(m.billing + 1);
        } else if (kd == 'S') {
            m.sound ^= 1;
        }
        return 0;
    case M_CARD:                            /* any game key skips a card */
        if ((edge & IN_GAME) || m.t >= CARD_STEPS) {
            m.ev2 = EV2_ACTION;
            go(M_PLAY);
        }
        return 0;
    case M_CUT:                             /* the crash pause: everything waits */
        if (--m.crash_t)
            return 0;
        model_scene_start();
        if (!m.takes) {
            best_keep();
            m.ev2 = EV2_FIRED;
            go(M_FIRED);
        } else {
            ++m.take_no;
            m.ev2 = EV2_RETAKE;
            go(M_CARD);
        }
        return 0;
    case M_PRINT:
    case M_WRAP:
        if ((edge & IN_GAME) || m.t >= SHOW_STEPS) {
            if (m.mode == M_PRINT && m.scenes % SEASON == 0) {
                go(M_WRAP);
                return 0;
            }
            if (m.mode == M_WRAP && m.billing < BILLINGS)
                model_billing(m.billing + 1);
            m.scene_fee = 0;
            go(M_CARD);
        }
        return 0;
    case M_FIRED:
        if ((edge & IN_GAME) || m.t >= SHOW_STEPS)
            fired_next();
        return 0;
    case M_NAME:
        nm = best_name[m.name_slot];
        if (kd >= 'A' && kd <= 'Z') {
            nm[m.name_len] = kd;
            if (++m.name_len == 3)
                go(M_TITLE);
        } else if (kd == K_DEL && m.name_len) {
            nm[--m.name_len] = '-';
        } else if (kd == K_RETURN) {
            go(M_TITLE);
        }
        return 0;
    case M_ASK:                             /* Y: back to the title; N or Esc: ride on */
        if (kd == 'Y') {
            best_keep();
            m.events = EV_QUIT;
            go(M_TITLE);
        } else if (kd == 'N' || kd == K_ESC) {
            m.events = EV_QUIT;
            go(M_PLAY);
        }
        return 0;
    }
    return 1;
}

void model_step(uint8_t in, uint8_t key)
{
    uint8_t edge = in & (uint8_t)~m.keys_prev, kd = 0;

    m.keys_prev = in;
    if (key != m.key_prev)
        kd = key;
    m.key_prev = key;
    m.events = 0;
    m.ev2 = 0;
    ++m.step;
    ++m.t;

    if (m.mode != M_PLAY && !frame_step(edge, kd))
        return;

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
        if (edge & IN_QUIT) {               /* Esc: "QUIT? Y/N" */
            m.events = EV_QUIT;
            m.key_prev = K_ESC;             /* held now: not yet an answer */
            go(M_ASK);
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
    } else if (++m.jump_t >= JUMP_STEPS && (!m.jump_long || m.jump_t >= JUMP_LONG)) {
        m.jump_t = 0;
        m.jump_long = 0;
        m.road_t = 1;                       /* the landing step is on the road */
        m.events |= EV_LAND;
    }
    if (m.jump_long)                        /* (not "?:": ORIC-LESSONS 5) */
        m.height = jump_long[m.jump_t];
    else
        m.height = jump_arc[m.jump_t];

    hazards_step();

    m.x += m.speed;                         /* wrap keeps height, arc and speed */
    if (m.x >= TRACK_W_FP) {
        m.x -= TRACK_W_FP;
        m.events |= EV_WRAP;
        m.fee += 10 * m.run_scene;          /* rule 5: each finished track pays */
        m.scene_fee += 10 * m.run_scene;
        m.ev2 |= EV2_FEE;
        if (++m.track == NTRACKS) {         /* scene cleared: "THAT'S A PRINT!" */
            m.bonus = 0;
            if (m.take_no == 1) {
                m.bonus = BONUS_X * 10 * m.run_scene;
                m.fee += m.bonus;
                m.scene_fee += m.bonus;
                m.ev2 |= EV2_BONUS;
            }
            ++m.scenes;
            ++m.run_scene;
            scene_load();                   /* the next scene's hazards */
            m.gave_take = 0;
            if (m.scenes % 3 == 0 && m.takes < TAKES) {
                ++m.takes;
                m.gave_take = 1;
                m.ev2 |= EV2_TAKE;
            }
            m.take_no = 1;
            model_scene_start();
            m.events |= EV_SCENE;
            go(M_PRINT);
            return;
        }
    }
    m.px = pxt[m.x >> 5];
    if (!m.jump_t && on_ramp()) {           /* a slipway launches a long jump */
        m.jump_t = 1;
        m.jump_long = 1;
        m.events |= EV_JUMP;
    }
    if (crashed()) {                        /* rule 4 */
        m.speed = 0;
        --m.takes;
        m.crash_t = CRASH_STEPS;
        m.events |= EV_CRASH;
        go(M_CUT);
    }
}
