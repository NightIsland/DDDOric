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

/* F6a: pictures are stored unshifted and pre-shifted here when a scene is
   drawn (DESIGN "Memory"): the scene's vehicle into veh_buf, its moving
   hazards into mov_buf.  art_shift writes ns shifts from s0 of a picture of
   nb bytes a row and h rows (top first): each shift ncols columns of hp
   lines, bottom line first (src/draw.s), lines past h blank. */
#define VENT     0x5E       /* a jet's grate, on the road's second line */
#define VENT_HOT 0x1E       /* XORed into the line below it while the jet warns */

uint8_t veh_buf[RIDER_VEHICLE_BYTES];       /* (F6c: and a popup's save, src/frame.c) */
static uint8_t vh_shifted = 0xFF;
static const uint8_t *vh_art = veh_buf;     /* the scene's vehicle's frames */
#define MOVBUF 1024             /* the alternate character set's place, unused in */
#define mov_buf ((uint8_t *)0x9C00)  /* HIRES (DESIGN "Memory") */
static uint16_t mov_top;

static uint8_t *art_shift(uint8_t *dst, const uint8_t *src, uint8_t nb, uint8_t h,
                          uint8_t ncols, uint8_t hp, uint8_t s0, uint8_t ns)
{
    uint8_t s, c, l, v, rs, ls;
    const uint8_t *row;
    for (s = s0; ns; --ns, ++s) {
        rs = s;
        ls = 6 - s;
        for (c = 0; c < ncols; ++c)
            for (l = hp; l--; ) {
                v = 0;
                if (l < h) {
                    row = src + l * nb;
                    if (c < nb)
                        v = row[c] >> rs;
                    if (c && c <= nb)
                        v |= (uint8_t)(row[c - 1] << ls) & 0x3F;
                }
                *dst++ = v;
            }
    }
    return dst;
}

/* the vehicle's frames, all 6 shifts, into veh_buf (at a scene's start,
   and again after a popup used veh_buf for its save) */
void vehicle_shift(uint8_t v)
{
    uint8_t i;
    vh_shifted = v;
    for (i = 0; i < 4; ++i)
        art_shift(veh_buf + i * RIDER_FRAME_BYTES, veh_art + v * VEH_BYTES + i * VEH_FRAME_BYTES,
                  2, 16, 3, 16, 0, 6);
}

void vehicle_reshift(void)
{
    vehicle_shift(vh_shifted);
}

void scene_draw(const struct scene *s, uint8_t billing)
{
    const palette_t *pal = s->pal, *ink0, *pp, *ip;
    uint8_t pn, pc, in, ic;
    const struct ground *gr = &grounds[s->ground];
    const uint8_t *gl;
    const struct hazard *hz;
    uint8_t t, i, j, y, *p, mid;
    char num[2];

    memset(HIRES, 0x40, 200 * 40);          /* bit 6 set in every bitmap byte */
    if (s->vehicle != vh_shifted)
        vehicle_shift(s->vehicle);
    for (y = 0; y < TITLE_LINES; ++y)
        line_colours(y, pal[0], pal[1]);
    for (ink0 = pal + 2, i = 0; i < BAND_LINES; i += *ink0++ >> 3)
        ;                                   /* the ink's runs follow the paper's */
    title(2, s->film);
    title(31, "SCENE");
    num[0] = '0' + s->number;
    num[1] = 0;
    title(37, num);

    for (t = 0; t < 4; ++t) {
        y = TITLE_LINES + t * BAND_LINES;
        pp = pal + 2;
        ip = ink0;
        pn = in = 0;
        for (i = 0; i < BAND_LINES; ++i, ++y) {
            if (!pn) {                                  /* the palette's next runs */
                pc = *pp & 7;
                pn = *pp++ >> 3;
            }
            if (!in) {
                ic = *ip & 7;
                in = *ip++ >> 3;
            }
            --pn;
            --in;
            p = line_colours(y, pc, ic);
            if (i >= ROAD_LINE) {                       /* the ground's line */
                gl = gr->line[i - ROAD_LINE];
                for (j = 2; j < 40; ++j)
                    p[j] = gl[(j - 2) & 3];
                if (i == ROAD_LINE && (gr->flags & GR_KERB))
                    p[1] = gl[0];                       /* the kerb from cell 1 */
            }
        }
        y = TITLE_LINES + t * BAND_LINES;               /* band's line 0 */
        for (hz = s->h[t], j = 0; j < SC_MAX; ++j, ++hz) {
            if (!hz_shown(hz, billing))                 /* not at this billing */
                continue;
            switch (hz->kind) {
            case HZ_BLOCK:
            case HZ_RAMP:
                blit(hz->a, hz->x, y + ROAD_LINE - 1);
                break;
            case HZ_JET:                                /* its vent in the road; the */
                LINE(y + ROAD_LINE + 1)[xcell[hz->x + scenery_w[hz->a] / 2]] = VENT;   /* jet itself moves */
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
            case HZ_GAP:                                /* the road cleared there; */
                for (i = 0; i < 4; ++i) {               /* on water, solid rocks   */
                    p = LINE(y + ROAD_LINE + i);
                    for (mid = hz->x; mid != (uint8_t)(hz->x + hz->a); ++mid)
                        if (gr->flags & GR_FILL)
                            p[xcell[mid]] |= 0x20 >> xshift[mid];
                        else
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

    memset(TEXTROW(0), ' ', 3 * 40);        /* the status rows: blank on the title */
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
static const uint16_t frame_ofs[4] = {
    0, RIDER_FRAME_BYTES, 2 * RIDER_FRAME_BYTES, 3 * RIDER_FRAME_BYTES
};

/* Bob's speed-up (BUILD 11): crashes come only on a real touch, so around the
   rider there is usually nothing but blank screen.  When the box that holds
   the old and new pictures has no hazard, rope or girder in it (box_blank),
   draw.s skips reading the screen and taking the old picture out: it writes
   blank bytes with the new picture in them.  Otherwise (and for the crash
   picture, which lies on what was hit) the usual old ^ new. */
static uint8_t box_blank(uint8_t t, uint8_t top, uint8_t bot, uint8_t c0, uint8_t c1, uint8_t self);
static uint8_t last_frame = FR_CRASH;

void rider_draw(uint8_t track, uint8_t x, uint8_t height, uint8_t frame)
{
    uint8_t top, bot, c0;
    sp_w[SPR_RIDER] = 3;
    sp_h[SPR_RIDER] = RIDER_H;
    n_y = rider_top[track] - height;
    n_c = xcell[x];
    n_img = vh_art + frame_ofs[frame] + shift_ofs[xshift[x]];
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
        if (c0 <= 36 && box_blank(track, top, bot, c0, c0 + 3, SPR_RIDER))
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
const uint8_t *mv_img[MAXMOV];          /* (not static: the harness reads it) */
static uint16_t mv_so[MAXMOV][6];       /* runners: each shift's offset in mv_img */
static uint8_t mv_lamp[MAXMOV];         /* the police car and the patrol boat: lamps */
uint8_t mv_hot[MAXMOV];                 /* jets: the vent glowing (a warning); the harness reads it */
static uint8_t rr, mv_x[MAXMOV];       /* hazards_draw: first in line; drawn x */
static uint8_t mv_t[MAXMOV];            /* track */
static uint8_t mv_block[MAXMOV][40];    /* runners: cells their road box shares with scenery */

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

/* Is the box (lines top .. bot - 1, cells c0 .. c1) on track t blank but
   for sprite self's own picture?  Then self moves fast (src/draw.s r_fast). */
#define sp_meets(sn, top, bot, c0, c1) (sp_on[sn] && (c0) < (uint8_t)(sp_c[sn] + sp_w[sn]) \
    && sp_c[sn] <= (c1) && (top) < (uint8_t)(sp_y[sn] + sp_h[sn]) && sp_y[sn] < (bot))

static uint8_t box_blank(uint8_t t, uint8_t top, uint8_t bot, uint8_t c0, uint8_t c1, uint8_t self)
{
    uint8_t i, sn;
    for (i = 0; i < nrect; ++i)
        if (rc_t[i] == t && c0 <= rc_c1[i] && rc_c0[i] <= c1 && top < rc_y1[i] && rc_y0[i] < bot)
            return 0;
    if (self != SPR_RIDER && sp_meets(SPR_RIDER, top, bot, c0, c1))
        return 0;
    for (i = 0; i < nmov; ++i) {
        if (mv_t[i] != t || mv_kind[i] != HZ_RUNNER)
            continue;
        for (sn = SPR_MOV + i; sn < NSPR; sn += GHOST)
            if (sn != self && sp_meets(sn, top, bot, c0, c1))
                return 0;
    }
    return 1;
}

void hazards_list(const struct scene *sc, uint8_t billing)
{
    const struct hazard *h = &sc->h[0][0];
    uint8_t i, t, band, w, nc, hp, k, a, x, n, sn, nb;
    uint8_t *dst;
    uint16_t o;
    nmov = rr = nrect = mov_top = 0;
    memset(sp_on + SPR_MOV, 0, NSPR - SPR_MOV);  /* (the last scene's too: a new screen) */
    for (i = 0; i < NHAZ && nmov < MAXMOV; ++i, ++h) {
        if (!hz_shown(h, billing))
            continue;
        k = h->kind;
        if (k == HZ_GAP || k == HZ_NONE)            /* (an unused record shows) */
            continue;
        t = i / SC_MAX;
        band = TITLE_LINES + t * BAND_LINES;
        a = h->a;
        x = h->x;
        w = scenery_w[a];
        hp = scenery_h[a];
        if (k == HZ_CEILING)
            rect_add(t, x, x + w - 1, band, band + h->b + 1);
        else if (k == HZ_DANGLER)
            rect_add(t, x, x + w - 1, band, band + DANGLE_TOP + DANGLE_MAX + 16);
        else if (k != HZ_RUNNER && k != HZ_FLYER)   /* blocks, ramps, jets */
            rect_add(t, x, x + w - 1, band + ROAD_LINE - hp, band + ROAD_LINE);
        if (k != HZ_RUNNER && k != HZ_DANGLER && k != HZ_JET && k != HZ_FLYER)
            continue;
        n = nmov;
        sn = SPR_MOV + n;
        mv_t[n] = t;
        mv_slot[n] = i;
        mv_hot[n] = 0;
        nc = 3;                                 /* (tools/mkart.py sprite_cols) */
        if (w > 13)
            nc = 5;
        sp_w[sn] = nc;
        mv_lamp[n] = a == ART_POLICE || a == ART_BOAT;
        dst = mov_buf + mov_top;
        mv_img[n] = dst;
        nb = (w + 5) / 6;
        sp_h[sn] = hp;
        if (k == HZ_DANGLER) {                  /* danglers: x is fixed, one shift, 16 lines */
            sp_h[sn] = 16;                      /* (band: the band's top line) */
            mv_rope[n] = x + w / 2;
            mv_c[n] = xcell[x];
            dst = art_shift(dst, scenery_art[a], nb, hp, nc, 16, xshift[x], 1);
        } else if (k == HZ_JET) {               /* x is fixed: one shift, its own lines */
            band += ROAD_LINE - hp;             /* standing on the road */
            mv_c[n] = xcell[x];
            mv_rope[n] = xcell[x + w / 2];      /* (jets: the vent's cell) */
            dst = art_shift(dst, scenery_art[a], nb, hp, nc, hp, xshift[x], 1);
        } else {                                /* runners and flyers: all 6 shifts */
            band += ROAD_LINE - hp;
            if (k == HZ_FLYER) {                /* (b: its underside's band line) */
                band += h->b + 1 - ROAD_LINE;
                k = HZ_RUNNER;                  /* (a flyer is drawn as a runner) */
            }
            for (x = 0, o = 0; x < 6; ++x, o += nc * hp)
                mv_so[n][x] = o;
            dst = art_shift(dst, scenery_art[a], nb, hp, nc, hp, 0, 6);
        }
        mv_kind[n] = k;
        mv_band[n] = band;
        mov_top = dst - mov_buf;
        sp_on[sn] = 0;
        sp_on[sn + GHOST] = 0;
        ++nmov;
    }
    /* each runner's box lines are fixed: which cells hold scenery there,
       worked out once so the per-step test (car_blank) is cheap */
    for (i = 0; i < nmov; ++i) {
        if (mv_kind[i] != HZ_RUNNER)
            continue;
        dst = mv_block[i];
        memset(dst, 0, 40);
        t = mv_t[i];
        band = mv_band[i];
        hp = band + sp_h[SPR_MOV + i];
        for (k = 0; k < nrect; ++k)
            if (rc_t[k] == t && band < rc_y1[k] && rc_y0[k] < hp)
                for (w = rc_c0[k]; w <= rc_c1[k]; ++w)
                    dst[w] = 1;
    }
}

/* Is runner i's box (cells c0 .. c0 + its columns) blank but for its own picture? */
static uint8_t car_blank(uint8_t i, uint8_t c0)
{
    uint8_t c, j, sn, top = mv_band[i], bot = mv_band[i] + sp_h[SPR_MOV + i], self = SPR_MOV + i;
    uint8_t c1 = c0 + sp_w[self], *bl = mv_block[i];
    for (c = c0; c <= c1; ++c)
        if (bl[c])
            return 0;
    if (sp_meets(SPR_RIDER, top, bot, c0, c1))
        return 0;
    for (j = 0; j < nmov; ++j) {
        if (mv_t[j] != mv_t[i] || mv_kind[j] != HZ_RUNNER)
            continue;
        for (sn = SPR_MOV + j; sn < NSPR; sn += GHOST)
            if (sn != self && sp_meets(sn, top, bot, c0, c1))
                return 0;
    }
    return 1;
}

/* Bob, BUILD 13: the police car's lamps flash red and blue.  Its top two
   lines (the lamps, then the lamp bar) get their own ink, swapped every 8
   steps: one ink cell each, so it costs nothing.  An ink cell colours its
   whole line, so while the rider is level with the lamps (his neck and
   shoulders share those lines) they take the band's ink, like everything
   else on the line, and flash again once he is clear (Bob, BUILD 15). */
void lamps(uint8_t phase)
{
    uint8_t i, a = 1, b = 4, y, *p;         /* red, blue */
    if (phase) {
        a = 4;
        b = 1;
    }
    for (i = 0; i < nmov; ++i)
        if (mv_lamp[i]) {
            y = mv_band[i];
            p = LINE(y);
            if (sp_on[SPR_RIDER] && (uint8_t)(y + 1 - sp_y[SPR_RIDER]) < RIDER_H + 1) {
                p[0] = p[40] = p[80];       /* the car body's line: band ink */
                continue;
            }
            p[0] = a;
            p[40] = b;
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
/* A car more than 9 px behind its place (its draws waited: on Legend's street
   the rider's slow draw can leave ~27 units a step for 30 steps) goes with
   less room, a fast draw risking the odd long step (BUILD 18). */
#define LAG_ROOM 26

void hazards_draw(void)
{
    uint8_t n, i = rr, x, d, room, fast, y, oy, top, sn, jump2, next = 0xFF;
    for (n = nmov; n; --n, ++i) {
        if (i >= nmov)
            i = 0;
        sn = SPR_MOV + i;
        if (sp_on[sn + GHOST]) {            /* the old picture of a far move: */
            if (step_left() < LAG_ROOM) {   /* (the mover waits for it, so it may go with less room) */
                if (next == 0xFF)
                    next = i;
                continue;
            }
            spr_erase(sn + GHOST);
            continue;
        }
        if (mv_kind[i] == HZ_JET) {         /* on: its picture; a warning: the vent glows */
            y = hz_drop[mv_slot[i]];
            x = y == JET_WARNING && (m.step & 4);
            if (x != mv_hot[i]) {           /* (one byte: no room needed) */
                mv_hot[i] = x;
                LINE(mv_band[i] + sp_h[sn] + 2)[mv_rope[i]] ^= VENT_HOT;
            }
            y = y == JET_ON;
            if (y == sp_on[sn])
                continue;
            if (step_left() < HZ_ROOM) {
                if (next == 0xFF)
                    next = i;
                continue;
            }
            if (y) {
                n_y = mv_band[i];
                n_c = mv_c[i];
                n_img = mv_img[i];
                spr_move(sn);
            } else {
                spr_erase(sn);
            }
            continue;
        }
        if (step_left() < LAG_ROOM) {       /* no room for any draw: no sums either */
            if (next == 0xFF)
                next = i;
            continue;
        }
        if (mv_kind[i] == HZ_RUNNER) {
            x = hz_px[mv_slot[i]];
            if (x == mv_x[i] && sp_on[sn])
                continue;
            /* Bob, BUILD 17: a car whose draws waited fell behind, and one far
               move left its old picture up for a step (a stretched car with a
               break).  Behind by 7 to 39 px it catches up 6 px a draw instead:
               6 px is always one cell, so old and new go in one box (7 or 8 px
               can cross two cells: two draws in one step).  Only a wrap or a
               retake moves far. */
            room = HZ_ROOM;
            if (sp_on[sn]) {
                d = x - mv_x[i];
                if (d > 6 && d < 40) {
                    x = mv_x[i] + 6;
                    if (d > 9)
                        room = LAG_ROOM;
                } else if (d < (uint8_t)-6 && d > (uint8_t)-40) {
                    x = mv_x[i] - 6;
                    if (d < (uint8_t)-9)
                        room = LAG_ROOM;
                }
            }
            jump2 = (uint8_t)(x - mv_x[i] + 8) >= 17;
            n_y = mv_band[i];
            n_c = xcell[x];
            n_img = mv_img[i] + mv_so[i][xshift[x]];
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
            room = HZ_ROOM;
            n_y = y;
            n_c = mv_c[i];
            n_img = mv_img[i];
        }
        if (step_left() < room) {
            if (next == 0xFF)
                next = i;
            continue;
        }
        fast = 0;
        if (mv_kind[i] == HZ_RUNNER && sp_on[sn] && !jump2) {
            /* a step's move on an empty road: no reading, no old picture to
               XOR out (BUILD 18).  Only such a move may go with the lagging
               car's lower room; any other needs HZ_ROOM. */
            top = sp_c[sn];                 /* (the box's left cell) */
            if (n_c < top)
                top = n_c;
            fast = top + sp_w[sn] <= 39 && car_blank(i, top);
        }
        if (!fast && mv_kind[i] == HZ_RUNNER && step_left() < HZ_ROOM) {
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
            r_fast = fast;
            mv_x[i] = x;
            spr_move(sn);
            r_fast = 0;
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
static uint16_t shown_fee = 0xFFFF, shown_best = 0xFFFF;
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
/* money, kept in £10s (F6): its digits and a 0, padded to 6 */
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
    if (started | v)
        p[n++] = '0';
    while (n < 6)
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
    shown_fee = shown_best = 0xFFFF;
}

/* The status rows' fixed text.  The title screen has none (Bob, BUILD 19):
   they are blanked while it shows and written afresh when it goes. */
static uint8_t rows_on;

static void status_rows(void)
{
    TEXTROW(0)[0] = 3;                      /* yellow */
    TEXTROW(1)[0] = 6;                      /* cyan */
    TEXTROW(2)[0] = 7;                      /* white */
    put(0, 1, "TAKE 1 @@@   STOP     FEE $0");
    put(1, 13, "TRACK      BEST $0");     /* no speed (Bob, BUILD 12): you judge it */
    put(2, 1, "BUILD");
    TEXTROW(2)[7] = '0' + BUILD_VERSION / 10;
    TEXTROW(2)[8] = '0' + BUILD_VERSION % 10;
    shown_track = shown_stop = shown_msg = shown_take = shown_takes = 0xFF;
    shown_fee = shown_best = 0xFFFF;
}

void status_update(void)
{
    uint8_t *t = TEXTROW(1), msg;
    big = 0;
    if (m.mode == M_TITLE) {
        if (rows_on) {
            memset(TEXTROW(0), ' ', 3 * 40);
            rows_on = 0;
        }
        return;
    }
    if (!rows_on) {
        status_rows();
        rows_on = 1;
    }
    if (m.track != shown_track) {
        shown_track = m.track;
        t[19] = '1' + m.track;
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
