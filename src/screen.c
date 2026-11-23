/* screen.c - F0 placeholder display: an empty four-track scene, the BUILD
   number and a status line, and a block for the rider.  F1 replaces all of it
   (HIRES init, scene drawing, bitmap status font, pre-shifted sprites).

   Layout (DESIGN "Screen structure"): lines 0-7 title strip, four 48-line
   bands; in each line cell 0 holds the paper colour and cell 1 the ink, so
   play uses cells 2-39 (228 pixels).  Text rows 25-27 hold the status lines in
   our own 5x7 font, put into the HIRES character set at $9800. */
#include <string.h>
#include "screen.h"
#include "model.h"
#include "physics.h"
#include "version.h"

#define BLACK 0
#define RED 1
#define GREEN 2
#define YELLOW 3
#define BLUE 4
#define MAGENTA 5
#define CYAN 6
#define WHITE 7
#define PAPER(c) (16 + (c))

static const char glyph_chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ:-.=,/!";
static const uint8_t glyphs[][7] = {   /* 5x7, bits 4-0 (from Oric Chuckie Egg's f0test) */
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
};

static uint8_t *lineaddr[200];
static uint8_t xcell[PLAY_W], xshift[PLAY_W];

static const uint8_t *glyph(char c)
{
    const char *p = strchr(glyph_chars, c);
    return p ? glyphs[p - glyph_chars] : 0;
}

/* text into a status row (0-2 = text rows 25-27) from cell col */
static void put(uint8_t row, uint8_t col, const char *s)
{
    memcpy(TEXTROW(row) + col, s, strlen(s));
}

/* text into HIRES lines 0-6 (the title strip), one glyph per cell */
static void title(uint8_t col, const char *s)
{
    const uint8_t *g;
    uint8_t r;
    for (; *s; ++s, ++col) {
        g = glyph(*s);
        if (g)
            for (r = 0; r < 7; ++r)
                lineaddr[r][col] = 0x40 | g[r];
    }
}

static void line_colours(uint8_t y, uint8_t paper, uint8_t ink)
{
    lineaddr[y][0] = PAPER(paper);
    lineaddr[y][1] = ink;
}

void screen_init(void)
{
    uint8_t y, b, i, *p;
    const uint8_t *g;

    for (y = 0, p = HIRES; y < 200; ++y, p += 40)
        lineaddr[y] = p;
    for (i = 0; i < PLAY_W; ++i) {
        xcell[i] = 2 + i / 6;
        xshift[i] = i % 6;
    }

    memset(HIRES, 0x40, 200 * 40);          /* bit 6 set in every bitmap byte */

    for (y = 0; y < TITLE_LINES; ++y)
        line_colours(y, BLUE, YELLOW);
    title(2, "STUNT MAN BOB");
    title(22, "F0 SKELETON");

    for (b = 0; b < NTRACKS; ++b) {
        y = TITLE_LINES + b * BAND_LINES;
        for (i = 0; i < BAND_LINES; ++i, ++y) {
            if (i < 28)
                line_colours(y, BLACK, CYAN);           /* air */
            else if (i < ROAD_LINE)
                line_colours(y, BLACK, YELLOW);         /* rider */
            else {
                line_colours(y, BLACK, GREEN);          /* road */
                memset(lineaddr[y] + 2, i == ROAD_LINE ? 0x7F : ((i & 1) ? 0x6A : 0x55), 38);
            }
        }
    }

    /* character set: our glyphs at their ASCII codes, everything else blank */
    memset(CHARSET, 0, 0x400);
    for (i = 0; glyph_chars[i]; ++i) {
        g = glyphs[i];
        memcpy(CHARSET + (uint8_t)glyph_chars[i] * 8, g, 7);
    }

    for (i = 0; i < 3; ++i)
        memset(TEXTROW(i), ' ', 39);
    TEXTROW(0)[0] = YELLOW;
    TEXTROW(1)[0] = CYAN;
    TEXTROW(2)[0] = WHITE;
    put(0, 1, "BUILD");
    TEXTROW(0)[7] = '0' + BUILD_VERSION / 10;
    TEXTROW(0)[8] = '0' + BUILD_VERSION % 10;
    put(0, 11, "TRACK");
    put(0, 20, "SPEED");
    put(0, 32, "STOP");
    put(1, 1, "SPACE JUMP  SHIFT FASTER  RETURN STOP");
    put(2, 1, "P PAUSE  ESC RESTART");
    MODE_CELL = 0x1E;                       /* HIRES, 50 Hz: switches the ULA */
    status_update();
}

/* the placeholder rider: a 6 x 12 block, XORed on and off */
#define RIDER_H 12

static uint8_t rider_line(uint8_t track, uint8_t height)   /* screen line of its lowest row */
{
    return TITLE_LINES + track * BAND_LINES + RIDER_BASE - height;
}

void __fastcall__ rider_xor(uint8_t track, uint8_t xpx, uint8_t height)
{
    uint8_t s = xshift[xpx], c = xcell[xpx], n;
    uint8_t lm = 0x3F >> s, rm = (uint8_t)(0x3F << (6 - s)) & 0x3F;
    uint8_t y = rider_line(track, height);
    uint8_t *p;
    if (c == 39)
        rm = 0;                             /* clip at the right edge */
    for (n = 0; n < RIDER_H; ++n, --y) {
        p = lineaddr[y] + c;
        p[0] ^= lm;
        if (rm)
            p[1] ^= rm;
    }
}

/* Move the rider in one pass: every screen byte is written once, with old ^ new,
   so the rider is never missing from the screen while the video scans it
   (docs/ORIC-LESSONS.md section 7).  BUILD 1 erased, then redrew: whenever the
   beam passed the rider between the two, it vanished for that frame, and as the
   20,000-cycle step drifts against the 19,968-cycle video frame that happened
   for about 2 s in every 12.5 s (Bob's "fading" bug).  A move to another track
   or more than one cell along (wrap, restart) still uses two passes. */
void rider_move(uint8_t ot, uint8_t ox, uint8_t oh, uint8_t nt, uint8_t nx, uint8_t nh)
{
    uint8_t co = xcell[ox], cn = xcell[nx], c0, so, sn, yo, yn, y, ytop, *p;
    uint8_t ol, or, nl, nr, a0, a1, a2, d;

    if (ot != nt || (uint8_t)(cn - co + 1) > 2) {
        rider_xor(ot, ox, oh);
        rider_xor(nt, nx, nh);
        return;
    }
    so = xshift[ox];
    sn = xshift[nx];
    ol = 0x3F >> so;
    or = co == 39 ? 0 : (uint8_t)(0x3F << (6 - so)) & 0x3F;
    nl = 0x3F >> sn;
    nr = cn == 39 ? 0 : (uint8_t)(0x3F << (6 - sn)) & 0x3F;
    c0 = co < cn ? co : cn;
    yo = rider_line(ot, oh);
    yn = rider_line(nt, nh);
    y = yo > yn ? yo : yn;                  /* lowest line of either */
    ytop = (yo < yn ? yo : yn) - (RIDER_H - 1);
    for (;; --y) {
        a0 = a1 = a2 = 0;
        if (y <= yo && y > yo - RIDER_H) {
            d = co - c0;
            if (d) { a1 = ol; a2 = or; } else { a0 = ol; a1 = or; }
        }
        if (y <= yn && y > yn - RIDER_H) {
            d = cn - c0;
            if (d) { a1 ^= nl; a2 ^= nr; } else { a0 ^= nl; a1 ^= nr; }
        }
        p = lineaddr[y] + c0;
        if (a0) p[0] ^= a0;
        if (a1) p[1] ^= a1;
        if (a2) p[2] ^= a2;                 /* a2 is 0 whenever c0 + 2 > 39 */
        if (y == ytop)
            break;
    }
}

static uint8_t shown_track = 0xFF, shown_stop = 0xFF, shown_pause = 0xFF;
static uint16_t shown_speed = 0xFFFF;

void status_update(void)
{
    uint8_t *t = TEXTROW(0);
    uint16_t v;
    if (m.track != shown_track) {
        shown_track = m.track;
        t[17] = '1' + m.track;
    }
    if (m.speed != shown_speed) {           /* px per step, 2 decimals */
        shown_speed = m.speed;
        v = (m.speed * 25u) >> 6;           /* = speed * 100 / 256 */
        t[26] = '0';
        while (v >= 100) { v -= 100; ++t[26]; }
        t[27] = '.';
        t[28] = '0';
        while (v >= 10) { v -= 10; ++t[28]; }
        t[29] = '0' + v;
    }
    if (m.stop_left != shown_stop) {
        shown_stop = m.stop_left;
        memcpy(t + 37, m.stop_left ? "ON" : "--", 2);
    }
    if (m.paused != shown_pause) {
        shown_pause = m.paused;
        put(2, 30, m.paused ? "PAUSED" : "      ");
    }
}
