/* frame.c - the film frame (src/frame.h; docs/PLAN.md F4; DESIGN
   "Presentation"): the title screen, the clapper card, "CUT!", "THAT'S A
   PRINT!", "YOU'RE FIRED!", the initials for the top five and "THAT'S A
   WRAP!".

   Each card is drawn whole on the HIRES screen when its mode starts (that
   step runs long: nothing moves then), and only small parts change after it:
   the chasing lights, the clapper arm, "ACTION!", the initials.  Text comes in
   two sizes from the status line's 5 x 7 font: one cell a letter, or two
   cells and 14 lines (every pixel doubled). */
#include <string.h>
#include "frame.h"
#include "art.h"
#include "model.h"
#include "screen.h"

#define BLACK 0
#define RED 1
#define GREEN 2
#define YELLOW 3
#define BLUE 4
#define MAGENTA 5
#define CYAN 6
#define WHITE 7

#define LINE(y) line_addr(y)

static uint8_t stripe[12][2];          /* the clapper's stripes: line y uses y mod 12 */

/* src/text.s: text, big text and the stripes' fill, in assembly */
extern uint8_t tx_col, tx_y, fa_a, fa_b;
extern uint8_t *fa_p;
#pragma zpsym ("tx_col")
#pragma zpsym ("tx_y")
#pragma zpsym ("fa_p")
#pragma zpsym ("fa_a")
#pragma zpsym ("fa_b")
void __fastcall__ txt(const char *s);
void __fastcall__ txt2(const char *s);
void __fastcall__ fill_alt(uint8_t n);
extern uint8_t dbl_a[64], dbl_b[64];   /* 6 pixels -> 12, as two screen bytes */

static const char * const billing_name[BILLINGS + 1] = {
    "", "EXTRA", "STAND-IN", "DOUBLE", "STUNT MAN", "COORDINATOR", "LEGEND"
};
static const char * const films[6] = {
    "RUSH HOUR RUMBLE", "DUST AT DAWN", "NIGHT OF THE HEARSE",
    "MOONBASE PANIC", "DRAGON'S KEEP", "COLD FRONT"
};

void frame_init(void)
{
    uint8_t v, b, k, x, j;
    uint16_t d;
    for (v = 0; v < 64; ++v) {
        d = 0;
        for (b = 0x20; b; b >>= 1)
            d = (d << 2) | ((v & b) ? 3 : 0);
        dbl_a[v] = 0x40 | (uint8_t)(d >> 6);
        dbl_b[v] = 0x40 | ((uint8_t)d & 0x3F);
    }
    for (k = 0; k < 12; ++k) {              /* the clapper's stripe pattern */
        d = 0;
        for (x = 0, j = k; x < 12; ++x) {
            d = (d << 1) | (j < 6);
            if (++j == 12)
                j = 0;
        }
        stripe[k][0] = 0x40 | (uint8_t)(d >> 6);
        stripe[k][1] = 0x40 | ((uint8_t)d & 0x3F);
    }
}

/* lines y0 .. y1 in these colours, their picture blank */
static void band(uint8_t y0, uint8_t y1, uint8_t paper, uint8_t ink)
{
    register uint8_t *p = line_colours(y0, paper, ink);
    for (;;) {
        memset(p + 2, 0x40, 38);
        if (y0 == y1)
            break;
        ++y0;
        p += 40;
        p[0] = p[-40];
        p[1] = p[-39];
    }
}

/* only the colours of lines y0 .. y1 */
static void ink(uint8_t y0, uint8_t y1, uint8_t paper, uint8_t c)
{
    for (; y0 <= y1; ++y0)
        line_colours(y0, paper, c);
}

/* small text: a letter a cell, lines y .. y + 6 */
static void text(uint8_t col, uint8_t y, const char *s)
{
    tx_col = col;
    tx_y = y;
    txt(s);
}

/* big text: two cells a letter, lines y .. y + 13 */
static void big(uint8_t col, uint8_t y, const char *s)
{
    tx_col = col;
    tx_y = y;
    txt2(s);
}

static uint8_t mid(const char *s)       /* first cell to centre small text */
{
    return 2 + (38 - strlen(s)) / 2;
}

static uint8_t bigmid(const char *s)
{
    return 2 + (38 - 2 * strlen(s)) / 2;
}

static void ctext(uint8_t y, const char *s)
{
    text(mid(s), y, s);
}

static void cbig(uint8_t y, const char *s)
{
    big(bigmid(s), y, s);
}

/* a picture from tools/mkart.py at double size, its top left at cell col,
   line y */
static void pic2(uint8_t n, uint8_t col, uint8_t y)
{
    const uint8_t *src = pic_art[n];
    uint8_t nb = pic_nb[n], h = pic_h[n], i, v;
    register uint8_t *p;
    for (p = LINE(y) + col; h; --h, p += 80)
        for (i = 0; i < nb; ++i) {
            v = *src++;
            p[2 * i] = p[2 * i + 40] = dbl_a[v];
            p[2 * i + 1] = p[2 * i + 41] = dbl_b[v];
        }
}

/* "$1234" or "1234" */
static char nbuf[8];
static const uint16_t pow10[4] = { 10000, 1000, 100, 10 };

static char *num(uint16_t v, uint8_t pound)
{
    uint8_t i, n = 0, d, started = 0;
    if (pound)
        nbuf[n++] = '$';
    for (i = 0; i < 4; ++i) {
        for (d = '0'; v >= pow10[i]; v -= pow10[i])
            ++d;
        if (d != '0' || started) {
            nbuf[n++] = d;
            started = 1;
        }
    }
    nbuf[n++] = '0' + (uint8_t)v;
    nbuf[n] = 0;
    return nbuf;
}

/* a label at cell 6 and a value ending at cell 34 */
static void row(uint8_t y, const char *label, const char *value)
{
    text(6, y, label);
    text(35 - strlen(value), y, value);
}

/* ------------------------------------------------------- the title screen */

/* The marquee's 48 bulbs, round the box clockwise from the top left: 18
   along the top (every other cell), 6 down the right, 18 back along the
   bottom, 6 up the left.  One in three is lit, and the lit ones step on every
   4 steps: the lights chase round. */
#define NBULB 48
#define MQ_TOP 4
#define MQ_BOT 47
static uint8_t shown_phase, shown_billing, shown_sound;
/* src/draw.s: each bulb's top line address, and the chase */
extern uint8_t bulb_lo[NBULB], bulb_hi[NBULB];
void __fastcall__ bulbs(uint8_t phase);

static void bulb_at(uint8_t n, uint8_t y, uint8_t c)
{
    uint8_t *p = LINE(y) + c;
    bulb_lo[n] = (uint8_t)(uint16_t)p;
    bulb_hi[n] = (uint16_t)p >> 8;
    p[0] = p[80] = 0x4C;                    /* a bulb's round top and bottom */
}

static void title_menu(void)
{
    text(24, 122, "           ");
    text(24, 122, billing_name[m.billing]);
    text(24, 134, "OFF");
    if (m.sound)
        text(24, 134, "ON ");
    shown_billing = m.billing;
    shown_sound = m.sound;
}

static void best_line(uint8_t i, uint8_t y)
{
    char s[4];
    s[0] = '1' + i;
    s[1] = '.';
    s[2] = 0;
    text(12, y, s);
    s[0] = best_name[i][0];
    s[1] = best_name[i][1];
    s[2] = best_name[i][2];
    s[3] = 0;
    text(16, y, s);
    text(21, y, "      ");
    text(21, y, num(best_fee[i], 1));
}

void frame_title(void)
{
    uint8_t i, n = 0;
    band(0, 55, RED, YELLOW);
    for (i = 0; i < 18; ++i)                /* top, left to right */
        bulb_at(n++, MQ_TOP, 3 + 2 * i);
    for (i = 0; i < 6; ++i)                 /* right side, down */
        bulb_at(n++, MQ_TOP + 6 + 6 * i, 37);
    for (i = 18; i; --i)                    /* bottom, right to left */
        bulb_at(n++, MQ_BOT, 1 + 2 * i);
    for (i = 6; i; --i)                     /* left side, up */
        bulb_at(n++, MQ_TOP + 6 * i, 3);
    big(8, 18, "STUNT MAN BOB");
    ctext(36, "SIX FILMS - THREE TAKES");
    shown_phase = 0;
    bulbs(0);

    band(56, 103, BLACK, WHITE);            /* Bob by his bike */
    pic2(PIC_BOB_POSE, 14, 100 - 2 * 19 + 1);
    pic2(PIC_BIKE, 19, 100 - 2 * 16 + 1);
    memset(line_colours(102, BLACK, YELLOW) + 2, 0x7F, 38);

    band(104, 141, BLACK, CYAN);            /* the menu */
    text(12, 110, "1  START");
    text(12, 122, "L  BILLING");
    text(12, 134, "S  SOUND");
    title_menu();

    band(142, 199, BLACK, WHITE);           /* the top five */
    ink(146, 153, BLACK, YELLOW);
    ctext(146, "TODAY'S TOP FIVE");
    for (i = 0; i < NBEST; ++i)
        best_line(i, 158 + 8 * i);
}

void frame_title_step(void)
{
    uint8_t ph = (m.t >> 2) % 3;
    if (ph != shown_phase) {
        shown_phase = ph;
        bulbs(ph);
    }
    if (m.billing != shown_billing || m.sound != shown_sound)
        title_menu();
}

/* ------------------------------------------------------ the clapper card */

/* The arm and the board's top are striped in diagonal bands 6 pixels wide:
   line y uses phase y mod 12 of a 12-pixel (two-cell) pattern. */
#define ARM_Y0  22                  /* top of the open arm's reach */
#define ARM_Y   46                  /* the shut arm's top line */
#define ARM_H   10
#define BOARD_Y 57

/* the arm raised by `rise` lines at its free (right) end, hinged at the
   left.  The top of column c is arm_top[c], falling to the right, so each
   line holds one run of columns ca .. cb - 1, and both ends move left as
   the lines go down. */
static uint8_t arm_top[40];

static void stripes(uint8_t *p, uint8_t k, uint8_t c0, uint8_t c1)
{
    const uint8_t *st = stripe[k];
    fa_p = p + c0;
    fa_a = st[c0 & 1];
    fa_b = st[(c0 & 1) ^ 1];
    fill_alt(c1 - c0);
}

/* the arm raised by `rise` lines at its free end, over the old one raised
   by `old` (lines from ARM_Y - old down are redrawn) */
static void arm(uint8_t rise, uint8_t old)
{
    uint8_t c, y, k, ca = 40, cb = 40, top = ARM_Y, acc = 0, *p;
    for (c = 2; c < 40; ++c) {              /* top = ARM_Y - rise x (c - 2) / 37 */
        arm_top[c] = top;
        for (acc += rise; acc >= 37; acc -= 37)
            --top;
    }
    y = ARM_Y - old;
    p = LINE(y);
    for (k = y % 12; y < ARM_Y + ARM_H; ++y, p += 40) {
        while (ca > 2 && arm_top[ca - 1] <= y)
            --ca;
        while (cb > ca && (uint8_t)(arm_top[cb - 1] + ARM_H) <= y)
            --cb;
        memset(p + 2, 0x40, 38);
        if (cb > ca)
            stripes(p, k, ca, cb);
        if (++k == 12)
            k = 0;
    }
}

static uint8_t shown_arm, shown_action;

void frame_card(const struct scene *s)
{
    uint8_t y, *p;
    band(0, 21, BLACK, YELLOW);             /* "ACTION!" goes here */
    band(22, 199, BLACK, WHITE);
    arm(24, 24);
    shown_arm = 24;
    shown_action = 0;
    for (y = BOARD_Y; y < BOARD_Y + 8; ++y)     /* the board's striped top */
        stripes(LINE(y), (y + 6) % 12, 2, 40);
    memset(LINE(67) + 2, 0x7F, 38);         /* the slate's frame */
    memset(LINE(68) + 2, 0x7F, 38);
    memset(LINE(196) + 2, 0x7F, 38);
    memset(LINE(197) + 2, 0x7F, 38);
    for (y = 69; y < 196; ++y) {
        p = LINE(y);
        p[2] = 0x70;
        p[39] = 0x43;
    }
    text(5, 76, "FILM");
    big(5, 88, s->film);
    text(5, 112, "SCENE");
    text(24, 112, "TAKE");
    big(5, 122, num(s->number, 0));
    big(24, 122, num(m.take_no, 0));
    text(5, 148, "BILLING");
    text(16, 148, billing_name[m.billing]);
    text(5, 160, "FEE SO FAR");
    text(16, 160, num(m.fee, 1));
    text(5, 172, "SCENE FEE");
    text(16, 172, num(10 * m.run_scene, 1));
    text(26, 172, "A TRACK");
    text(5, 184, "STUNTS BY BOB");
}

void frame_card_step(void)
{
    uint8_t on;
    if (m.t == CARD_SNAP - 3 && shown_arm == 24) {
        arm(10, 24);                        /* on its way down */
        shown_arm = 10;
    } else if (m.t >= CARD_SNAP && shown_arm) {
        arm(0, shown_arm);                  /* shut: clack (src/main.c) */
        shown_arm = 0;
    }
    if (m.t >= CARD_SNAP) {                 /* "ACTION!" flashes */
        on = ((m.t - CARD_SNAP) & 8) == 0;
        if (on != shown_action) {
            shown_action = on;
            if (on)
                cbig(6, "ACTION!");
            else
                band(6, 19, BLACK, YELLOW);
        }
    }
}

/* ------------------------------------------------------------- "CUT!" */

/* the director's megaphone and "CUT!" in double-width letters, over the
   film's name in the title strip (its colours stay) */
void frame_cut(void)
{
    static const char cut[] = "CUT!";
    uint8_t r, i, g, *p;
    const uint8_t *src = pic_art[PIC_MEGAPHONE];
    for (r = 0; r < 8; ++r)
        memset(LINE(r) + 2, 0x40, 38);
    for (r = 1; r < 8; ++r, src += 2) {
        p = LINE(r);
        p[3] = 0x40 | src[0];
        p[4] = 0x40 | src[1];
        for (i = 0; i < 4; ++i) {
            g = glyph(cut[i])[r - 1];
            p[6 + 2 * i] = dbl_a[g];
            p[7 + 2 * i] = dbl_b[g];
        }
    }
}

/* ------------------------------------------------- "THAT'S A PRINT!" */

void frame_print(void)
{
    band(0, 199, BLACK, WHITE);
    ink(20, 37, BLACK, YELLOW);
    cbig(22, "THAT'S A PRINT!");
    text(17, 50, "SCENE");
    text(23, 50, num(m.run_scene - 1, 0));
    row(70, "TRACK FEES", num(m.scene_fee - m.bonus, 1));
    if (m.bonus)
        row(84, "ONE-TAKE BONUS", num(m.bonus, 1));
    if (m.gave_take) {
        ink(98, 105, BLACK, GREEN);
        ctext(98, "A LOST TAKE COMES BACK");
    }
    ctext(124, "RUN FEE");
    ink(136, 149, BLACK, CYAN);
    cbig(136, num(m.fee, 1));
}

/* --------------------------------------------------- "YOU'RE FIRED!" */

void frame_fired(void)
{
    band(0, 199, BLACK, WHITE);
    ink(26, 43, BLACK, RED);
    cbig(28, "YOU'RE FIRED!");
    ctext(66, "FINAL FEE");
    ink(78, 91, BLACK, YELLOW);
    cbig(78, num(m.fee, 1));
    text(12, 108, "BEST TODAY");
    text(23, 108, num(m.best, 1));
    if (m.fee > best_fee[NBEST - 1]) {
        ink(130, 137, BLACK, GREEN);
        ctext(130, "A TOP FIVE FEE!");
    }
}

/* ---------------------------------------------------- the initials */

static uint8_t shown_len;

static void name_letters(void)
{
    char s[6];
    const char *nm = best_name[m.name_slot];
    s[0] = nm[0];
    s[1] = ' ';
    s[2] = nm[1];
    s[3] = ' ';
    s[4] = nm[2];
    s[5] = 0;
    big(15, 64, s);
    best_line(m.name_slot, 104 + 10 * m.name_slot);
    shown_len = m.name_len;
}

void frame_name(void)
{
    uint8_t i;
    band(0, 199, BLACK, WHITE);
    ink(18, 35, BLACK, YELLOW);
    cbig(20, "TOP FIVE!");
    ctext(46, "TYPE YOUR INITIALS");
    ink(64, 77, BLACK, CYAN);
    for (i = 0; i < NBEST; ++i)
        best_line(i, 104 + 10 * i);
    ctext(164, "DEL GOES BACK");
    ctext(176, "RETURN WHEN DONE");
    name_letters();
}

void frame_name_step(void)
{
    if (m.name_len != shown_len)
        name_letters();
}

/* -------------------------------------------------- "THAT'S A WRAP!" */

void frame_wrap(void)
{
    uint8_t i;
    band(0, 199, BLACK, WHITE);
    ink(18, 35, BLACK, YELLOW);
    cbig(20, "THAT'S A WRAP!");
    ink(50, 57, BLACK, MAGENTA);
    ctext(50, "THE PREMIERE");
    for (i = 0; i < 6; ++i)
        ctext(68 + 12 * i, films[i]);
    ctext(150, "NEXT SEASON YOU ARE BILLED AS");
    ink(164, 177, BLACK, CYAN);
    i = m.billing;
    if (i < BILLINGS)
        ++i;
    cbig(164, billing_name[i]);
}
