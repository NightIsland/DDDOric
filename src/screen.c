/* screen.c - the display (src/screen.h; docs/PLAN.md F1).

   - screen_init: line address tables, the character set (our own 5x7 font and
     status icons), the status rows, HIRES mode.
   - scene_draw: a whole scene from its data (src/scene.h): per-line paper and
     ink, the roads, the hazards in their start positions, the title strip.
   - rider_draw: moves the rider with src/draw.s, one pass, at the beam's pace.
   - status_update: rewrites only the status cells that changed. */
#include <string.h>
#include "screen.h"
#include "art.h"
#include "model.h"
#include "physics.h"
#include "version.h"
#include "system.h"

#define PAPER(c) (16 + (c))

/* our 5x7 font, bits 4-0 (from Oric Chuckie Egg's f0test), plus icons:
   '$' pound sign, '*' stop lamp lit, '+' stop lamp out, '@' clapper */
static const char glyph_chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ:-.=,/!$*+@'?";
static const uint8_t glyphs[][7] = {
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
    {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}, {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E},
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}, {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},
    {0x0E,0x11,0x11,0x11,0x1F,0x11,0x11}, {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E},
    {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}, {0x1C,0x12,0x11,0x11,0x11,0x12,0x1C},
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}, {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10},
    {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F}, {0x11,0x11,0x11,0x1F,0x11,0x11,0x11},
    {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}, {0x07,0x02,0x02,0x02,0x02,0x12,0x0C},
    {0x11,0x12,0x14,0x18,0x14,0x12,0x11}, {0x10,0x10,0x10,0x10,0x10,0x10,0x1F},
    {0x11,0x1B,0x15,0x15,0x11,0x11,0x11}, {0x11,0x11,0x19,0x15,0x13,0x11,0x11},
    {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}, {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10},
    {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}, {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11},
    {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}, {0x1F,0x04,0x04,0x04,0x04,0x04,0x04},
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}, {0x11,0x11,0x11,0x11,0x11,0x0A,0x04},
    {0x11,0x11,0x11,0x15,0x15,0x15,0x0A}, {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11},
    {0x11,0x11,0x11,0x0A,0x04,0x04,0x04}, {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F},
    {0x00,0x0C,0x0C,0x00,0x0C,0x0C,0x00}, {0x00,0x00,0x00,0x1F,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C}, {0x00,0x00,0x1F,0x00,0x1F,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x0C,0x04,0x08}, {0x01,0x01,0x02,0x04,0x08,0x10,0x10},
    {0x04,0x04,0x04,0x04,0x04,0x00,0x04},
    {0x06,0x09,0x08,0x1C,0x08,0x08,0x1F}, {0x0E,0x1F,0x1F,0x1F,0x1F,0x1F,0x0E},
    {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}, {0x1F,0x15,0x1F,0x00,0x1F,0x11,0x1F},
    {0x04,0x04,0x08,0x00,0x00,0x00,0x00}, {0x0E,0x11,0x01,0x02,0x04,0x00,0x04},
};

/* src/draw.s */
extern uint8_t line_lo[200], line_hi[200];
extern uint8_t r_top, r_h, r_c0, r_w, r_ph, a_row, a_col, b_row, b_col;
extern const uint8_t *a_img, *b_img;
#pragma zpsym ("r_top")
#pragma zpsym ("r_h")
#pragma zpsym ("r_c0")
#pragma zpsym ("r_w")
#pragma zpsym ("r_ph")
#pragma zpsym ("a_img")
#pragma zpsym ("a_row")
#pragma zpsym ("a_col")
#pragma zpsym ("b_img")
#pragma zpsym ("b_row")
#pragma zpsym ("b_col")
void spr_region(void);
uint8_t * __fastcall__ line_addr(uint8_t y);

static uint8_t xcell[PLAY_W], xshift[PLAY_W];

#define LINE(y) line_addr(y)

const uint8_t *glyph(char c)
{
    const char *p = strchr(glyph_chars, c);
    return p ? glyphs[p - glyph_chars] : 0;
}

static void put(uint8_t row, uint8_t col, const char *s)
{
    memcpy(TEXTROW(row) + col, s, strlen(s));
}

/* text into the title strip, one glyph per cell on lines 1-7 */
static void title(uint8_t col, const char *s)
{
    const uint8_t *g;
    uint8_t r;
    for (; *s; ++s, ++col) {
        g = glyph(*s);
        if (g)
            for (r = 0; r < 7; ++r)
                LINE(r + 1)[col] = 0x40 | g[r];
    }
}

/* Colours of line y (Bob, BUILD 4: everything must start at the same column).
   An attribute cell shows as paper in the colour it sets, and every line starts
   white on black, so: cell 0 sets the ink (shows black); cell 1 sets the paper
   if it isn't black (shows in it), else it is an ordinary blank cell.  The
   picture's left edge is then at pixel 6 on every line; play starts at cell 2. */
uint8_t *line_colours(uint8_t y, uint8_t paper, uint8_t ink)
{
    uint8_t *p = LINE(y);
    p[0] = ink;
    p[1] = paper ? PAPER(paper) : 0x40;
    return p;
}

/* OR a scenery picture into the screen: left edge at x, last row on line `bottom` */
static void blit(uint8_t art, uint8_t x, uint8_t bottom)
{
    const uint8_t *src = scenery_art[art];
    uint8_t w = scenery_w[art], h = scenery_h[art], nb = (w + 5) / 6;
    uint8_t s = xshift[x], c = xcell[x], y = bottom + 1 - h;
    uint8_t i, v, carry, *p;
    for (; h; --h, ++y) {
        p = LINE(y);
        carry = 0;
        for (i = 0; i <= nb; ++i) {
            v = i < nb ? *src++ : 0;
            if (c + i < 40)
                p[c + i] |= (v >> s) | carry;
            carry = (uint8_t)(v << (6 - s)) & 0x3F;
        }
    }
}

/* a one-pixel vertical line at x from line y0 to y1 */
static void vline(uint8_t x, uint8_t y0, uint8_t y1)
{
    uint8_t c = xcell[x], b = 0x20 >> xshift[x];
    for (; y0 <= y1; ++y0)
        LINE(y0)[c] |= b;
}

void scene_draw(const struct scene *s, uint8_t billing)
{
    const struct palette *pal = s->pal;
    const struct hazard *hz;
    uint8_t t, i, j, y, *p, mid;
    char num[2];

    memset(HIRES, 0x40, 200 * 40);          /* bit 6 set in every bitmap byte */
    for (y = 0; y < TITLE_LINES; ++y)
        line_colours(y, pal->title_paper, pal->title_ink);
    title(2, s->film);
    title(31, "SCENE");
    num[0] = '0' + s->number;
    num[1] = 0;
    title(37, num);

    for (t = 0; t < 4; ++t) {
        y = TITLE_LINES + t * BAND_LINES;
        for (i = 0; i < BAND_LINES; ++i, ++y) {
            p = line_colours(y, pal->paper[i], pal->ink[i]);
            if (i == ROAD_LINE)                         /* kerb, from cell 1 */
                memset(p + 1, 0x7F, 39);                /* (road paper is black) */
            else if (i == ROAD_LINE + 2)
                for (j = 2; j < 40; j += 4)             /* lane marks */
                    p[j] = p[j + 1] = 0x7F;
        }
        y = TITLE_LINES + t * BAND_LINES;               /* band's line 0 */
        for (hz = s->h[t], j = 0; j < SC_MAX; ++j, ++hz) {
            switch (hz->kind) {
            case HZ_BLOCK:
                blit(hz->a, hz->x, y + ROAD_LINE - 1);
                break;
            case HZ_DANGLER:                            /* the rope's top part; the */
                mid = hz->x + scenery_w[hz->a] / 2;     /* rest and the weight move */
                vline(mid, y, y + DANGLE_TOP - 1);      /* (hazards_draw)           */
                break;
            case HZ_CEILING:                            /* hung from the band top */
                blit(hz->a, hz->x, y + hz->b);
                vline(hz->x + 1, y, y + hz->b);
                vline(hz->x + scenery_w[hz->a] - 2, y, y + hz->b);
                break;
            case HZ_GAP:
                if (billing >= hz->b)
                    for (i = 0; i < 4; ++i) {           /* clear the road there */
                        p = LINE(y + ROAD_LINE + i);
                        for (mid = hz->x; mid != (uint8_t)(hz->x + hz->a); ++mid)
                            p[xcell[mid]] &= ~(0x20 >> xshift[mid]);
                    }
                break;
            }
        }
    }
}

void screen_init(void)
{
    uint8_t y, i;
    uint16_t a = 0xA000;

    for (y = 0; y < 200; ++y, a += 40) {
        line_lo[y] = (uint8_t)a;
        line_hi[y] = a >> 8;
    }
    for (i = 0; i < PLAY_W; ++i) {
        xcell[i] = 2 + i / 6;
        xshift[i] = i % 6;
    }

    /* character set: our glyphs at their ASCII codes, everything else blank */
    memset(CHARSET, 0, 0x400);
    for (i = 0; glyph_chars[i]; ++i)
        memcpy(CHARSET + (uint8_t)glyph_chars[i] * 8, glyphs[i], 7);

    for (i = 0; i < 3; ++i)
        memset(TEXTROW(i), ' ', 39);
    TEXTROW(0)[0] = 3;                      /* yellow */
    TEXTROW(1)[0] = 6;                      /* cyan */
    TEXTROW(2)[0] = 7;                      /* white */
    put(0, 1, "TAKE 1 @@@   STOP     FEE $0");
    put(1, 1, "SPEED       TRACK      BEST $0");
    put(2, 1, "BUILD");
    TEXTROW(2)[7] = '0' + BUILD_VERSION / 10;
    TEXTROW(2)[8] = '0' + BUILD_VERSION % 10;
    put(2, 11, "SPACE SHIFT RETURN P ESC");
    MODE_CELL = 0x1E;                       /* HIRES, 50 Hz: switches the ULA */
}

/* Sprites: the rider (0) and the moving hazards (1 on), each moved with
   src/draw.s in one pass at the beam's pace.  Their state lives in draw.s. */
#define SPR_RIDER 0
#define SPR_MOV   1
#define NSPR      16            /* src/draw.s */
extern uint8_t sp_on[], sp_y[], sp_c[], sp_w[], sp_h[], sp_lo[], sp_hi[];
extern uint8_t n_y, n_c;
extern const uint8_t *n_img;
#pragma zpsym ("n_y")
#pragma zpsym ("n_c")
#pragma zpsym ("n_img")
void __fastcall__ spr_move(uint8_t n);
void __fastcall__ spr_erase(uint8_t n);
#define GHOST 8                 /* sprite n + GHOST: n's old picture, still to take off */
extern uint8_t r_fast;
#pragma zpsym ("r_fast")

#define RT(t) (TITLE_LINES + (t) * BAND_LINES + RIDER_TOP)
static const uint8_t rider_top[4] = { RT(0), RT(1), RT(2), RT(3) };
static const uint8_t shift_ofs[6] = { 0, 48, 96, 144, 192, 240 };
#define PO (SPR_POLICE_COLS * SPR_POLICE_H)
static const uint16_t police_ofs[6] = { 0, PO, 2 * PO, 3 * PO, 4 * PO, 5 * PO };
static const uint8_t * const frame_art[4] = {
    rider_art, rider_art + RIDER_FRAME_BYTES, rider_art + 2 * RIDER_FRAME_BYTES,
    rider_art + 3 * RIDER_FRAME_BYTES
};

/* Bob's speed-up (BUILD 11): crashes come only on a real touch, so around the
   rider there is usually nothing but blank screen.  When the box that holds
   the old and new pictures has no hazard, rope or girder in it (box_blank),
   draw.s skips reading the screen and taking the old picture out: it writes
   blank bytes with the new picture in them.  Otherwise (and for the crash
   picture, which lies on what was hit) the usual old ^ new. */
static uint8_t box_blank(uint8_t t, uint8_t top, uint8_t bot, uint8_t c0);
static uint8_t last_frame = FR_CRASH;

void rider_draw(uint8_t track, uint8_t x, uint8_t height, uint8_t frame)
{
    uint8_t top, bot, c0;
    sp_w[SPR_RIDER] = 3;
    sp_h[SPR_RIDER] = RIDER_H;
    n_y = rider_top[track] - height;
    n_c = xcell[x];
    n_img = frame_art[frame] + shift_ofs[xshift[x]];
    if (frame != FR_CRASH && last_frame != FR_CRASH && sp_on[SPR_RIDER]
        && (uint8_t)(n_y - sp_y[SPR_RIDER] + 15) < 31 && (uint8_t)(n_c - sp_c[SPR_RIDER] + 1) <= 2) {
        top = n_y;
        bot = sp_y[SPR_RIDER];
        if (bot < top) {
            top = bot;
            bot = n_y;
        }
        bot += RIDER_H;
        c0 = n_c;
        if (sp_c[SPR_RIDER] < c0)
            c0 = sp_c[SPR_RIDER];
        if (c0 <= 36 && box_blank(track, top, bot, c0))
            r_fast = 1;
    }
    last_frame = frame;
    spr_move(SPR_RIDER);
    r_fast = 0;
}

/* A dangler's rope runs from its band's top line to just above the weight.
   The weight is a sprite; the rope is single pixels: added below the old top
   after the weight has moved down, taken away above the old top before it
   moves up, so the sprite's XOR never meets a rope pixel. */
extern uint8_t rp_b, rp_y, rp_c;
#pragma zpsym ("rp_b")
#pragma zpsym ("rp_y")
#pragma zpsym ("rp_c")
void __fastcall__ rope_or(uint8_t y1);
void __fastcall__ rope_and(uint8_t y1);

static void rope(uint8_t x, uint8_t y0, uint8_t y1, uint8_t on)
{
    rp_y = y0;
    rp_c = xcell[x];
    rp_b = 0x20 >> xshift[x];
    if (on) {
        rope_or(y1);
    } else {
        rp_b = ~rp_b;
        rope_and(y1);
    }
}

/* The moving hazards of the scene, listed once by hazards_list (at most 4
   per scene, DESIGN "Speed"), so the per-step loop does no searching. */
#define MAXMOV 6
static uint8_t nmov, mv_slot[MAXMOV], mv_kind[MAXMOV], mv_band[MAXMOV], mv_rope[MAXMOV];
static uint8_t mv_c[MAXMOV];
static const uint8_t *mv_img[MAXMOV];
static uint8_t rr, mv_x[MAXMOV];       /* hazards_draw: first in line; drawn x */
static uint8_t mv_t[MAXMOV];            /* track */

/* What the rider's fast draw must keep clear of: blocks, girders and their
   posts, and each wrecking ball's whole reach with its rope (screen lines
   y0 .. y1 - 1, cells c0 .. c1); police cars by where they are drawn. */
#define MAXRECT (4 * SC_MAX)
static uint8_t nrect, rc_t[MAXRECT], rc_c0[MAXRECT], rc_c1[MAXRECT], rc_y0[MAXRECT], rc_y1[MAXRECT];

static void rect_add(uint8_t t, uint8_t x0, uint8_t x1, uint8_t y0, uint8_t y1)
{
    rc_t[nrect] = t;
    rc_c0[nrect] = xcell[x0];
    rc_c1[nrect] = xcell[x1];
    rc_y0[nrect] = y0;
    rc_y1[nrect] = y1;
    ++nrect;
}

static uint8_t box_blank(uint8_t t, uint8_t top, uint8_t bot, uint8_t c0)
{
    uint8_t i, sn, c1 = c0 + 3;
    for (i = 0; i < nrect; ++i)
        if (rc_t[i] == t && c0 <= rc_c1[i] && rc_c0[i] <= c1 && top < rc_y1[i] && rc_y0[i] < bot)
            return 0;
    for (i = 0; i < nmov; ++i) {
        if (mv_t[i] != t || mv_kind[i] != HZ_RUNNER)
            continue;
        for (sn = SPR_MOV + i; sn < NSPR; sn += GHOST)
            if (sp_on[sn] && c0 < (uint8_t)(sp_c[sn] + sp_w[sn]) && sp_c[sn] <= c1
                && top < (uint8_t)(sp_y[sn] + sp_h[sn]) && sp_y[sn] < bot)
                return 0;
    }
    return 1;
}

void hazards_list(const struct scene *sc)
{
    const struct hazard *h = &sc->h[0][0];
    uint8_t i;
    uint8_t band, w;
    nmov = rr = nrect = 0;
    for (i = 0; i < NHAZ && nmov < MAXMOV; ++i, ++h) {
        band = TITLE_LINES + (i / SC_MAX) * BAND_LINES;
        w = 0;
        if (h->kind == HZ_BLOCK || h->kind == HZ_CEILING || h->kind == HZ_DANGLER)
            w = scenery_w[h->a];
        if (h->kind == HZ_BLOCK)
            rect_add(i / SC_MAX, h->x, h->x + w - 1, band + ROAD_LINE - scenery_h[h->a], band + ROAD_LINE);
        else if (h->kind == HZ_CEILING)
            rect_add(i / SC_MAX, h->x, h->x + w - 1, band, band + h->b + 1);
        else if (h->kind == HZ_DANGLER)
            rect_add(i / SC_MAX, h->x, h->x + w - 1, band, band + DANGLE_TOP + DANGLE_MAX + 16);
        if (h->kind != HZ_RUNNER && h->kind != HZ_DANGLER)
            continue;
        mv_t[nmov] = i / SC_MAX;
        mv_slot[nmov] = i;
        mv_kind[nmov] = h->kind;
        mv_band[nmov] = TITLE_LINES + (i / SC_MAX) * BAND_LINES;
        if (h->kind == HZ_RUNNER) {
            sp_w[SPR_MOV + nmov] = SPR_POLICE_COLS;
            sp_h[SPR_MOV + nmov] = SPR_POLICE_H;              /* runners: their top line */
            mv_band[nmov] += ROAD_LINE - SPR_POLICE_H;
        } else {
            sp_w[SPR_MOV + nmov] = SPR_BALL_COLS;       /* danglers: x is fixed */
            sp_h[SPR_MOV + nmov] = 16;
            mv_rope[nmov] = h->x + scenery_w[h->a] / 2;
            mv_c[nmov] = xcell[h->x];
            mv_img[nmov] = spr_ball + shift_ofs[xshift[h->x]];
        }
        sp_on[SPR_MOV + nmov] = 0;
        sp_on[SPR_MOV + nmov + GHOST] = 0;
        ++nmov;
    }
}

/* A step has room for the rider and one or two hazards (each ~5,500 cycles).
   Each hazard is drawn only if the step still has HZ_ROOM x 256 cycles left
   for it and the status line; one that doesn't fit waits, first in line next
   step, while the others are still tried (its picture is then a step behind
   the game: collisions go by the game, never the picture).  A hazard that
   goes far (a police car wrapping round, a retake) costs two draws: the new
   picture this step, the old one taken off the next, so for one step both
   show, and no step needs room for more than one draw. */
#define HZ_ROOM 34

void hazards_draw(void)
{
    uint8_t n, i = rr, x, y, oy, top, sn, jump2, next = 0xFF;
    for (n = nmov; n; --n, ++i) {
        if (i >= nmov)
            i = 0;
        sn = SPR_MOV + i;
        if (sp_on[sn + GHOST]) {            /* the old picture of a far move */
            if (step_left() < HZ_ROOM) {
                if (next == 0xFF)
                    next = i;
                continue;
            }
            spr_erase(sn + GHOST);
            continue;
        }
        if (mv_kind[i] == HZ_RUNNER) {
            x = hz_px[mv_slot[i]];
            if (x == mv_x[i] && sp_on[sn])
                continue;
            jump2 = (uint8_t)(x - mv_x[i] + 8) >= 17;
            n_y = mv_band[i];
            n_c = xcell[x];
            n_img = spr_police + police_ofs[xshift[x]];
            y = oy = 0;
        } else {
            top = mv_band[i] + DANGLE_TOP;
            y = top + hz_drop[mv_slot[i]];
            oy = top;
            if (sp_on[sn])
                oy = sp_y[sn];
            if (y == oy && sp_on[sn])
                continue;
            jump2 = (uint8_t)(y - oy + 15) >= 31;
            n_y = y;
            n_c = mv_c[i];
            n_img = mv_img[i];
        }
        if (step_left() < HZ_ROOM) {
            if (next == 0xFF)
                next = i;
            continue;
        }
        if (jump2 && sp_on[sn]) {             /* leave the old picture as a ghost */
            sp_on[sn + GHOST] = 1;
            sp_y[sn + GHOST] = sp_y[sn];
            sp_c[sn + GHOST] = sp_c[sn];
            sp_lo[sn + GHOST] = sp_lo[sn];
            sp_hi[sn + GHOST] = sp_hi[sn];
            sp_w[sn + GHOST] = sp_w[sn];
            sp_h[sn + GHOST] = sp_h[sn];
            sp_on[sn] = 0;
        }
        if (mv_kind[i] == HZ_RUNNER) {
            mv_x[i] = x;
            spr_move(sn);
        } else {                            /* (far only up: a retake) */
            if (y < oy)
                rope(mv_rope[i], y, oy, 0);
            spr_move(sn);
            if (y > oy)
                rope(mv_rope[i], oy, y, 1);
        }
    }
    rr = i;
    if (next != 0xFF)
        rr = next;
}

static uint8_t shown_track = 0xFF, shown_stop = 0xFF, shown_msg = 0xFF;
static uint8_t shown_take = 0xFF, shown_takes = 0xFF;
static const char lamp[2] = { '+', '*' };  /* stop used, stop left */
static uint16_t shown_speed = 0xFFFF, shown_fee = 0xFFFF, shown_best = 0xFFFF;
static const char * const msgs[8] = {
    "SPACE SHIFT RETURN P ESC", "PAUSED - ANY KEY        ",
    "CUT! RETAKE COMING      ", "CUT! YOU ARE FIRED      ",
    "QUIT TO THE TITLE? Y/N  ", "PRESS 1 TO START        ",
    "ANY GAME KEY: GO ON     ", "TYPE YOUR INITIALS      "
};
/* the message for each mode (M_*); M_PLAY and M_CUT pick their own */
static const uint8_t mode_msg[9] = { 5, 6, 0, 2, 6, 6, 7, 6, 4 };
static const char icons[4][4] = { "   ", "@  ", "@@ ", "@@@" };   /* takes left */

/* v in 5 cells from p, left-aligned (no division: cc65's is slow) */
static const uint16_t pow10[4] = { 10000, 1000, 100, 10 };
static void put_num(uint8_t *p, uint16_t v)
{
    uint8_t i, d, started = 0, n = 0;
    for (i = 0; i < 4; ++i) {
        for (d = '0'; v >= pow10[i]; v -= pow10[i])
            ++d;
        if (d != '0' || started) {
            p[n++] = d;
            started = 1;
        }
    }
    p[n++] = '0' + (uint8_t)v;
    while (n < 5)
        p[n++] = ' ';
}

/* The status line: only what has changed.  The longer fields (best, fee,
   message: ~1,500 cycles each) go one a step, when the step has ST_ROOM x 256
   cycles left: a restart changes all three at once. */
#define ST_ROOM 10
static uint8_t big;

static uint8_t big_ok(void)
{
    if (big || step_left() < ST_ROOM)
        return 0;
    return big = 1;
}

/* a new screen for play: the rider is not on it yet, and every status field
   is written afresh */
void play_reset(void)
{
    sp_on[SPR_RIDER] = 0;
    last_frame = FR_CRASH;
    shown_track = shown_stop = shown_msg = shown_take = shown_takes = 0xFF;
    shown_speed = shown_fee = shown_best = 0xFFFF;
}

void status_update(void)
{
    uint8_t *t = TEXTROW(1), msg;
    uint16_t v;
    big = 0;
    if (m.track != shown_track) {
        shown_track = m.track;
        t[19] = '1' + m.track;
    }
    if (m.speed != shown_speed) {           /* 0 .. 250, the original's units */
        shown_speed = m.speed;
        v = m.speed;
        t[7] = '0';
        while (v >= 100) { v -= 100; ++t[7]; }
        t[8] = '0';
        while (v >= 10) { v -= 10; ++t[8]; }
        t[9] = '0' + v;
    }
    if (m.best != shown_best && big_ok()) {
        shown_best = m.best;
        put_num(t + 30, m.best);
    }
    t = TEXTROW(0);
    if (m.stop_left != shown_stop) {
        shown_stop = m.stop_left;
        t[19] = lamp[shown_stop];           /* not "?:": cc65 2.19 tested the
                                               wrong flags there (Bob, BUILD 4) */
    }
    if (m.take_no != shown_take) {
        shown_take = m.take_no;
        t[6] = '0' + (m.take_no > 9 ? 9 : m.take_no);
    }
    if (m.takes != shown_takes) {
        shown_takes = m.takes;
        memcpy(t + 8, icons[m.takes], 3);
    }
    if (m.fee != shown_fee && big_ok()) {
        shown_fee = m.fee;
        put_num(t + 28, m.fee);
    }
    msg = mode_msg[m.mode];
    if (m.paused)
        msg = 1;
    else if (m.mode == M_CUT && !m.takes)
        msg = 3;
    if (msg != shown_msg && big_ok()) {
        shown_msg = msg;
        put(2, 11, msgs[msg]);
    }
}
