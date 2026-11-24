/* solve.c - proves scenes beatable (DESIGN "Billings": "Every scene must be
   proved beatable at every billing by a scripted run in the test harness").

   usage: solve BILLING NSCENES [BEAM [W]] > win.keys

   Starts a run at BILLING from the title screen (L pressed BILLING - 1
   times, then 1), skips each card with a tap on Space, and searches the
   native model (src/model.c, compiled in) for inputs that clear NSCENES
   scenes in a row on their first take.  Prints the winning run as a key
   script (tools/keyscript.py) that tools/ref/ref.c and the Oric build replay.

   The search is a beam over riding steps.  Every state in the beam has
   ridden the same number of steps, so the hazards (which move by the clock
   alone) are the same for all of them: only the rider's state differs.
   States that crash are dropped; states the same in track, x (to 1/4 BBC
   byte), speed, jump step and stop are merged; when the beam is full, the
   states furthest along are kept, half of the room going to a spread of the
   others so that waiting for a hazard stays possible.  Actions each step:
   accelerate or not, Space down or not, and the stop while it is unused.

   A person can't time a press to the step, so the search plays like one:
   - Accelerate changes only every GRID steps (5: 100 ms), and not in the air;
   - a jump is taken only if pressing it any of the next W - 1 steps instead
     (accelerate held as it is) also lands safely, so every jump in the run
     has a window of at least W steps (W x 20 ms) to press it in. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/model.c"

#define MAXSTEPS 4000
#define MAXSCRIPT 20000

/* the hazards' moving state, the same for every state in the beam */
struct hz {
    uint8_t px[NHAZ], drop[NHAZ], frac[NHAZ], t[NHAZ], top[NHAZ];
};
static void hz_save(struct hz *h)
{
    memcpy(h->px, hz_px, NHAZ); memcpy(h->drop, hz_drop, NHAZ); memcpy(h->frac, hz_frac, NHAZ);
    memcpy(h->t, hz_t, NHAZ); memcpy(h->top, htop, NHAZ);
}
static void hz_load(const struct hz *h)
{
    memcpy(hz_px, h->px, NHAZ); memcpy(hz_drop, h->drop, NHAZ); memcpy(hz_frac, h->frac, NHAZ);
    memcpy(hz_t, h->t, NHAZ); memcpy(htop, h->top, NHAZ);
}

struct node { struct model m; int parent; uint8_t in; uint32_t key; long score; };

static int W = 1, GRID = 5, nostop;      /* SOLVE_NOSTOP=1: never use the stop */

/* the scene's fixed tables: a check that clears the scene loads the next
   scene's over them (scene_load), so they are put back after every check */
static struct {
    uint8_t hk[NHAZ], hw[NHAZ], hh[NHAZ], sp[NHAZ], right[NHAZ], max[NHAZ], x0[NHAZ], nmov, mov[MAXMOV], dr;
    uint8_t jph[NHAZ], jp, jo;
    const uint16_t *hcol[NHAZ], *hit;
    const struct scene *scene;
} tab;
static void tab_save(void)
{
    memcpy(tab.hk, hk, NHAZ); memcpy(tab.hw, hw, NHAZ); memcpy(tab.hh, hh, NHAZ);
    memcpy(tab.sp, hz_sp, NHAZ); memcpy(tab.right, hz_right, NHAZ); memcpy(tab.max, hz_max, NHAZ);
    memcpy(tab.x0, hz_x0, NHAZ); tab.nmov = nmov; memcpy(tab.mov, mov, MAXMOV); tab.dr = dangle_reload;
    memcpy(tab.hcol, hcol, sizeof hcol); tab.hit = hit_vh;
    memcpy(tab.jph, jet_phase, NHAZ); tab.jp = jet_p; tab.jo = jet_on;
}
static void tab_load(void)
{
    memcpy(hk, tab.hk, NHAZ); memcpy(hw, tab.hw, NHAZ); memcpy(hh, tab.hh, NHAZ);
    memcpy(hz_sp, tab.sp, NHAZ); memcpy(hz_right, tab.right, NHAZ); memcpy(hz_max, tab.max, NHAZ);
    memcpy(hz_x0, tab.x0, NHAZ); nmov = tab.nmov; memcpy(mov, tab.mov, MAXMOV); dangle_reload = tab.dr;
    memcpy(hcol, tab.hcol, sizeof hcol); hit_vh = tab.hit;
    memcpy(jet_phase, tab.jph, NHAZ); jet_p = tab.jp; jet_on = tab.jo;
}

/* from state s (hazards hz): wait d steps then jump, for d = 0 .. W - 1,
   accelerate held as acc throughout; 1 if every one lands safely */
static int jump_window(const struct model *s, const struct hz *h, uint8_t acc)
{
    int d, k;
    for (d = 1; d < W; ++d) {
        m = *s;
        hz_load(h);
        tab_load();                     /* (a clear may have loaded the next scene's) */
        for (k = 0; k < d; ++k) {
            model_step(acc, 0);
            if (m.mode == M_CUT)
                return 0;
            if (m.mode == M_PRINT)
                return 1;
            if (m.jump_t)               /* a slipway took him up first */
                break;
        }
        if (!m.jump_t)
            model_step(acc | IN_JUMP, 0);
        for (k = 0; m.mode == M_PLAY && m.jump_t && k < 200; ++k)
            model_step(acc, 0);
        if (m.mode == M_CUT)
            return 0;
        if (m.mode == M_PLAY) {         /* landed: a few steps on the road */
            for (k = 0; k < 4 && m.mode == M_PLAY; ++k)
                model_step(acc, 0);
            if (m.mode == M_CUT)
                return 0;
        }
    }
    return 1;
}

/* the jump just taken (state now in m, from state s with hazards h): rides on
   until it lands, then a few steps; 1 if no crash */
static int lands(const struct model *s, const struct hz *h, uint8_t in)
{
    int k;
    uint8_t acc = in & IN_ACCEL;
    struct model save = m;
    (void)s; (void)h;
    for (k = 0; m.mode == M_PLAY && m.jump_t && k < 200; ++k)
        model_step(acc, 0);
    for (k = 0; k < 4 && m.mode == M_PLAY; ++k)
        model_step(acc, 0);
    k = m.mode != M_CUT;
    m = save;
    return k;
}

static uint8_t script[MAXSCRIPT];   /* the input of each step from step 1 */
static uint8_t keyc[MAXSCRIPT];     /* the key code (title only) */
static int nscript;

static void put_step(uint8_t in, uint8_t key)
{
    script[nscript] = in;
    keyc[nscript] = key;
    ++nscript;
    model_step(in, key);
}

static uint32_t hash_key(const struct model *s)
{
    uint32_t h = s->track;
    h = h * 400 + (s->x >> 6);
    h = h * 251 + s->speed;
    h = h * 128 + s->jump_t;
    h = h * 2 + s->jump_long;
    h = h * 2 + s->stop_left;
    h = h * 2 + (s->keys_prev & IN_JUMP);
    h = h * 4 + (s->road_t > 3 ? 3 : s->road_t);
    return h;
}

static int cmp_score(const void *a, const void *b)
{
    long x = ((const struct node *)a)->score, y = ((const struct node *)b)->score;
    return x < y ? 1 : x > y ? -1 : 0;
}

/* the hash set of keys for merging */
#define HBITS 20
static uint32_t *hset;
static int *hval;
static int hfind(uint32_t k, int idx)
{
    uint32_t i = (k * 2654435761u) >> (32 - HBITS);
    for (;;) {
        if (hval[i] < 0) { hset[i] = k; hval[i] = idx; return -1; }
        if (hset[i] == k) return hval[i];
        i = (i + 1) & ((1u << HBITS) - 1);
    }
}

static struct node *cur, *nxt, *all;
static int **parents;               /* per step: each kept node's parent and input */
static uint8_t **inputs;

/* ride one scene from its first riding step; 0 if no way through */
static int ride(int beam)
{
    static const uint8_t acts[] = { 0, IN_ACCEL, IN_JUMP, IN_ACCEL | IN_JUMP, IN_STOP, IN_STOP | IN_ACCEL };
    struct hz hz;
    int ncur = 1, step, i, a, n, win = -1;
    unsigned u;
    cur[0].m = m;
    cur[0].parent = -1;
    tab_save();
    for (step = 0; step < MAXSTEPS && win < 0; ++step) {
        hz_save(&hz);
        memset(hval, 0xFF, sizeof(int) << HBITS);
        n = 0;
        for (i = 0; i < ncur && win < 0; ++i) {
            for (a = 0; a < 6; ++a) {
                uint8_t in = acts[a];
                if ((in & IN_STOP) && (!cur[i].m.stop_left || nostop))
                    continue;
                if ((in & IN_JUMP) && cur[i].m.jump_t)  /* Space only on the road */
                    continue;
                if ((step % GRID || cur[i].m.jump_t)        /* (in the air: as at take-off, */
                    && (in & IN_ACCEL) != (cur[i].m.keys_prev & IN_ACCEL))  /* as jump_window has it) */
                    continue;
                m = cur[i].m;
                hz_load(&hz);
                model_step(in, 0);
                if (m.mode == M_CUT)
                    continue;
                if (W > 1 && (m.events & EV_JUMP) && !m.jump_long) {
                    struct model now = m;
                    int good = lands(&cur[i].m, &hz, in)
                               && jump_window(&cur[i].m, &hz, in & IN_ACCEL);
                    m = now;
                    tab_load();
                    if (!good)
                        continue;
                }
                if (m.mode == M_PRINT) {        /* cleared */
                    all[n].m = m;
                    all[n].parent = i;
                    all[n].in = in;
                    win = n++;
                    break;
                }
                uint32_t k = hash_key(&m);
                if (hfind(k, n) >= 0)
                    continue;
                all[n].m = m;
                all[n].parent = i;
                all[n].in = in;
                all[n].score = (long)m.track * TRACK_W_FP + m.x;
                ++n;
            }
        }
        /* the hazards after this step (any state: they move by the clock);
           not after a win, whose step has set up the next scene's */
        if (win < 0) {
            hz_load(&hz);
            tab_load();
            hazards_step();
        }
        if (!n) {
            int bi = 0;
            for (i = 1; i < ncur; ++i)
                if (cur[i].m.track * 1000L + cur[i].m.px > cur[bi].m.track * 1000L + cur[bi].m.px)
                    bi = i;
            fprintf(stderr, "solve: no way through after %d steps (furthest: track %d px %d)\n",
                    step, cur[bi].m.track + 1, cur[bi].m.px);
            return 0;
        }
        if (win < 0 && n > beam) {          /* keep the best half, and a spread of the rest */
            qsort(all, n, sizeof *all, cmp_score);
            int keep = beam / 2, rest = n - keep, j;
            for (j = 0; j < beam - keep; ++j)
                all[keep + j] = all[keep + (int)((long)j * rest / (beam - keep))];
            n = beam;
        }
        parents[step] = malloc(sizeof(int) * (size_t)n);
        inputs[step] = malloc((size_t)n);
        for (i = 0; i < n; ++i) {
            parents[step][i] = all[i].parent;
            inputs[step][i] = all[i].in;
        }
        if (win >= 0) {                     /* back through the parents */
            int len = step + 1, k = win;
            uint8_t *path = malloc((size_t)len);
            for (int s = step; s >= 0; --s) {
                path[s] = inputs[s][k];
                k = parents[s][k];
            }
            m = all[win].m;
            for (int s = 0; s < len; ++s) {
                script[nscript] = path[s];
                keyc[nscript] = 0;
                ++nscript;
            }
            free(path);
            for (int s = 0; s <= step; ++s) { free(parents[s]); free(inputs[s]); }
            return len;
        }
        memcpy(cur, all, sizeof *all * (size_t)n);
        ncur = n;
        (void)u;
    }
    {
        int bi = 0;
        for (i = 1; i < ncur; ++i)
            if (cur[i].m.track * 1000L + cur[i].m.px > cur[bi].m.track * 1000L + cur[bi].m.px)
                bi = i;
        fprintf(stderr, "solve: out of time (furthest: track %d px %d)\n", cur[bi].m.track + 1, cur[bi].m.px);
    }
    return 0;
}

static void print_runs(uint8_t bit, const char *tok)
{
    for (int s = 0; s < nscript; ++s)
        if (script[s] & bit) {
            int e = s;
            while (e + 1 < nscript && (script[e + 1] & bit))
                ++e;
            if (e == s)
                printf("%d %s\n", s + 1, tok);
            else
                printf("%d-%d %s\n", s + 1, e + 1, tok);
            s = e;
        }
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: solve BILLING NSCENES [BEAM [W]]\n");
        return 2;
    }
    int billing = atoi(argv[1]), nsc = atoi(argv[2]), beam = argc > 3 ? atoi(argv[3]) : 6000;
    if (argc > 4)
        W = atoi(argv[4]);
    nostop = getenv("SOLVE_NOSTOP") && *getenv("SOLVE_NOSTOP");
    if (getenv("SOLVE_SCENE") && !strcmp(getenv("SOLVE_SCENE"), "stress"))
        scene_only = &scene_stress;     /* F6a's stress scene, every scene */
    hset = malloc(sizeof(uint32_t) << HBITS);
    hval = malloc(sizeof(int) << HBITS);
    cur = malloc(sizeof *cur * (size_t)beam * 6);
    nxt = malloc(sizeof *nxt * (size_t)beam * 6);
    all = malloc(sizeof *all * (size_t)beam * 6);
    parents = malloc(sizeof(int *) * MAXSTEPS);
    inputs = malloc(sizeof(uint8_t *) * MAXSTEPS);

    model_init(1);
    for (int b = 1; b < billing; ++b) {     /* L, released, ... */
        put_step(0, 'L');
        put_step(0, 0);
    }
    put_step(0, '1');
    if (getenv("SOLVE_SKIP")) {             /* measuring only: start further on (not replayable) */
        m.run_scene += atoi(getenv("SOLVE_SKIP"));
        scene_load();
    }
    printf("# solve %d %d: billing %d from the title, %d scene(s), each on its first take\n",
           billing, nsc, billing, nsc);
    int lstep = nscript;                    /* the steps so far are title keys */
    if (getenv("SOLVE_CRASH")) {            /* take 1: flat out into the first hazard */
        put_step(IN_JUMP, ' ');
        while (m.mode == M_PLAY)
            put_step(IN_ACCEL, 0);
        while (m.mode == M_CUT)
            put_step(0, 0);
        if (m.mode != M_CARD) {
            fprintf(stderr, "solve: no retake card (mode %d)\n", m.mode);
            return 1;
        }
        put_step(0, 0);
        fprintf(stderr, "solve: crashed on take 1, retake card at step %d\n", nscript);
    }
    for (int sc = 0; sc < nsc; ++sc) {
        put_step(IN_JUMP, ' ');             /* Space skips the card */
        if (m.mode != M_PLAY) {
            fprintf(stderr, "solve: card not skipped (mode %d)\n", m.mode);
            return 1;
        }
        int r0 = nscript;
        int len = ride(beam);
        if (!len) {
            fprintf(stderr, "solve: scene %d (run scene %d) not cleared\n", sc + 1, m.run_scene);
            return 1;
        }
        fprintf(stderr, "solve: billing %d scene %d cleared in %d steps (steps %d-%d)\n",
                billing, sc + 1, len, r0 + 1, nscript);
        if (sc + 1 < nsc) {
            put_step(0, 0);                 /* let go, then Space skips "THAT'S A PRINT!" */
            put_step(IN_JUMP, ' ');
            if (m.mode == M_WRAP) {         /* and "THAT'S A WRAP!" after a season */
                put_step(0, 0);
                put_step(IN_JUMP, ' ');
            }
            if (m.mode != M_CARD) {
                fprintf(stderr, "solve: no card after the print (mode %d)\n", m.mode);
                return 1;
            }
            put_step(0, 0);
        }
    }
    for (int s = 0; s < lstep; ++s)         /* the title keys */
        if (keyc[s])
            printf("%d %c\n", s + 1, keyc[s]);
    print_runs(IN_ACCEL, "accel");
    print_runs(IN_JUMP, "jump");
    print_runs(IN_STOP, "stop");
    printf("# %d steps\n", nscript);
    return 0;
}
