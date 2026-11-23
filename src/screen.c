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

#define PAPER(c) (16 + (c))

/* our 5x7 font, bits 4-0 (from Oric Chuckie Egg's f0test), plus icons:
   '$' pound sign, '*' stop lamp lit, '+' stop lamp out, '@' clapper */
static const char glyph_chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ:-.=,/!$*+@";
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
};

/* src/draw.s */
extern uint8_t line_lo[200], line_hi[200];
extern uint8_t r_top, r_h, r_c0, a_row, a_col, b_row, b_col;
extern const uint8_t *a_img, *b_img;
#pragma zpsym ("r_top")
#pragma zpsym ("r_h")
#pragma zpsym ("r_c0")
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

static const uint8_t *glyph(char c)
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
    for (y = 0; y < TITLE_LINES; ++y) {
        p = LINE(y);
        p[0] = PAPER(pal->title_paper);
        p[1] = pal->title_ink;
    }
    title(2, s->film);
    title(31, "SCENE");
    num[0] = '0' + s->number;
    num[1] = 0;
    title(37, num);

    for (t = 0; t < 4; ++t) {
        y = TITLE_LINES + t * BAND_LINES;
        for (i = 0; i < BAND_LINES; ++i, ++y) {
            p = LINE(y);
            p[0] = PAPER(pal->paper[i]);
            p[1] = pal->ink[i];
            if (i == ROAD_LINE)
                memset(p + 2, 0x7F, 38);                /* kerb */
            else if (i == ROAD_LINE + 2)
                for (j = 2; j < 40; j += 4)             /* lane marks */
                    p[j] = p[j + 1] = 0x7F;
        }
        y = TITLE_LINES + t * BAND_LINES;               /* band's line 0 */
        for (hz = s->h[t], j = 0; j < SC_MAX; ++j, ++hz) {
            switch (hz->kind) {
            case HZ_BLOCK:
            case HZ_RUNNER:
                blit(hz->a, hz->x, y + ROAD_LINE - 1);
                break;
            case HZ_DANGLER:                            /* drawn up: rope + weight */
                mid = hz->x + scenery_w[hz->a] / 2;
                vline(mid, y, y + 5);
                blit(hz->a, hz->x, y + 5 + scenery_h[hz->a]);
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
    put(0, 1, "TAKE 1 @@@   STOP     FEE $0   BEST $0");
    put(1, 1, "SPEED       TRACK      BILLING 1 EXTRA");
    put(2, 1, "BUILD");
    TEXTROW(2)[7] = '0' + BUILD_VERSION / 10;
    TEXTROW(2)[8] = '0' + BUILD_VERSION % 10;
    put(2, 11, "SPACE SHIFT RETURN P ESC");
    MODE_CELL = 0x1E;                       /* HIRES, 50 Hz: switches the ULA */
}

/* The rider: where it is drawn now (r_on = 0: not drawn). */
static uint8_t r_on, r_y, r_c;
static const uint8_t *r_img;

#define RT(t) (TITLE_LINES + (t) * BAND_LINES + RIDER_TOP)
static const uint8_t rider_top[4] = { RT(0), RT(1), RT(2), RT(3) };
static const uint8_t shift_ofs[6] = { 0, 48, 96, 144, 192, 240 };
static const uint8_t * const frame_art[3] = {
    rider_art, rider_art + RIDER_FRAME_BYTES, rider_art + 2 * RIDER_FRAME_BYTES
};

void rider_draw(uint8_t track, uint8_t x, uint8_t height, uint8_t frame)
{
    uint8_t y = rider_top[track] - height;
    uint8_t c = xcell[x];
    const uint8_t *img = frame_art[frame] + shift_ofs[xshift[x]];

    if (r_on && y == r_y && c == r_c && img == r_img)
        return;
    if (r_on && (uint8_t)(y - r_y + 15) < 31 && (uint8_t)(c - r_c + 1) <= 2) {
        /* overlapping: one box holds both, old ^ new in one burst */
        r_top = y < r_y ? y : r_y;
        r_h = (y < r_y ? r_y - y : y - r_y) + RIDER_H;
        r_c0 = c < r_c ? c : r_c;
        a_img = r_img;
        a_row = r_y - r_top;
        a_col = r_c - r_c0;
        b_img = img;
        b_row = y - r_top;
        b_col = c - r_c0;
        spr_region();
    } else {
        /* apart (first draw, a wrap, a new scene): draw the new one first, then
           take the old one off, so the rider is never missing from a frame */
        r_top = y;
        r_h = RIDER_H;
        r_c0 = c;
        a_img = img;
        a_row = a_col = 0;
        b_img = 0;
        spr_region();
        if (r_on) {
            r_top = r_y;
            r_c0 = r_c;
            a_img = r_img;
            spr_region();
        }
    }
    r_on = 1;
    r_y = y;
    r_c = c;
    r_img = img;
}

static uint8_t shown_track = 0xFF, shown_stop = 0xFF, shown_pause = 0xFF;
static uint16_t shown_speed = 0xFFFF;

void status_update(void)
{
    uint8_t *t = TEXTROW(1);
    uint16_t v;
    if (m.track != shown_track) {
        shown_track = m.track;
        t[19] = '1' + m.track;
    }
    if (m.speed != shown_speed) {           /* px per step, 2 decimals */
        shown_speed = m.speed;
        v = (m.speed * 25u) >> 6;           /* = speed * 100 / 256 */
        t[7] = '0';
        while (v >= 100) { v -= 100; ++t[7]; }
        t[8] = '.';
        t[9] = '0';
        while (v >= 10) { v -= 10; ++t[9]; }
        t[10] = '0' + v;
    }
    if (m.stop_left != shown_stop) {
        shown_stop = m.stop_left;
        TEXTROW(0)[19] = m.stop_left ? '*' : '+';
    }
    if (m.paused != shown_pause) {
        shown_pause = m.paused;
        put(2, 11, m.paused ? "PAUSED - ANY KEY        " : "SPACE SHIFT RETURN P ESC");
    }
}
