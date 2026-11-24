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
static char nbuf[9];
static const uint16_t pow10[4] = { 10000, 1000, 100, 10 };

/* pound: money, kept in £10s (F6), shown with "$" (the pound sign) and its 0 */
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
    if (pound && started | v)
        nbuf[n++] = '0';
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

/* The title's menu and top five: the menu a little tighter and the top five
   lower, for a gap between them (Bob, BUILD 20: with the status rows gone
   the screen's foot was empty) */
#define MENU_Y  108
#define MENU_DY 11
#define TOP5_Y  149

#ifdef SCENE_PICK
static uint8_t shown_pick;
#endif

static void title_menu(void)
{
    text(24, MENU_Y + MENU_DY, "           ");
    text(24, MENU_Y + MENU_DY, billing_name[m.billing]);
    text(24, MENU_Y + 2 * MENU_DY, "OFF");
    if (m.sound)
        text(24, MENU_Y + 2 * MENU_DY, "ON ");
    shown_billing = m.billing;
    shown_sound = m.sound;
#ifdef SCENE_PICK
    {   /* debug build: "P  FILM 3 SCENE 2" in place of the strapline */
        static char pk[] = "   P: FILM 1 SCENE 1   ";
        pk[11] = '1' + (pick_scene >> 1);
        pk[19] = '1' + (pick_scene & 1);
        ctext(36, pk);
        shown_pick = pick_scene;
    }
#endif
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
    text(12, MENU_Y, "1  START");
    text(12, MENU_Y + MENU_DY, "L  BILLING");
    text(12, MENU_Y + 2 * MENU_DY, "S  SOUND");
    title_menu();

    band(142, 199, BLACK, WHITE);           /* the top five */
    ink(TOP5_Y, TOP5_Y + 7, BLACK, YELLOW);
    ctext(TOP5_Y, "TODAY'S TOP FIVE");
    for (i = 0; i < NBEST; ++i)
        best_line(i, TOP5_Y + 12 + 8 * i);
}

void frame_title_step(void)
{
    uint8_t ph = (m.t >> 2) % 3;
    if (ph != shown_phase) {
        shown_phase = ph;
        bulbs(ph);
    }
    if (m.billing != shown_billing || m.sound != shown_sound
#ifdef SCENE_PICK
        || pick_scene != shown_pick
#endif
        )
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

/* the film's name, large, with one-cell spaces between its words: NIGHT OF
   THE HEARSE (35 cells) didn't fit the slate with two-cell ones (Bob, BUILD 27) */
static void film_name(const char *f)
{
    static char ch[2];
    uint8_t col = 5;
    if (strlen(f) > 16)
        col = 3;
    for (; *f; ++f) {
        if (*f == ' ') {
            ++col;
        } else {
            ch[0] = *f;
            big(col, 88, ch);
            col += 2;
        }
    }
}

void frame_card(const struct scene *s)
{
    uint8_t y, *p;
    band(0, 21, BLACK, YELLOW);             /* "ACTION!" goes here */
    band(22, 199, BLACK, WHITE);
    arm(24, 24);
    shown_arm = 24;
    shown_action = 0;
    for (y = BOARD_Y; y < BOARD_Y + 8; ++y)     /* the board's striped top, slanting */
        stripes(LINE(y), (120 - y) % 12, 2, 40); /* against the arm (Bob, BUILD 12) */
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
    film_name(s->film);
    text(5, 112, "SCENE");
    text(24, 112, "TAKE");
    big(5, 122, num(s->number, 0));
    big(24, 122, num(m.take_no, 0));
    text(5, 148, "BILLING");
    text(16, 148, billing_name[m.billing]);
    text(5, 160, "FEE SO FAR");
    text(16, 160, num(m.fee, 1));
    text(5, 172, "SCENE FEE");
    text(16, 172, num(m.run_scene, 1));     /* (fees in £10s)  */
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

/* ------------------------------------------------------------ popups */

/* Bob, BUILD 12: the messages pop up over the scene, which stays behind
   them.  A popup fills cells 2-39 of lines y0 .. y1: cell 2 sets its ink
   (showing the line's own paper), cell 3 its paper, cells 4-37 hold the
   text, and cells 38-39 put the line's own paper and ink back.  A popup
   that goes away again (pause, the quit question) saves what it covers,
   F6c: in the vehicle's shift buffer (30 lines x 38 = 1,140 of its 1,152
   bytes), shifted afresh when the popup goes. */
#define POP_SAVE 30                     /* lines a popup can save */
#define pop_buf veh_buf
static uint8_t pop_y0, pop_n;

static void popup(uint8_t y0, uint8_t y1, uint8_t paper, uint8_t c, uint8_t save)
{
    register uint8_t *p = LINE(y0);
    uint8_t *b = pop_buf;
    pop_n = 0;
    if (save) {
        pop_y0 = y0;
        pop_n = y1 - y0 + 1;
    }
    for (;; p += 40) {
        if (save) {
            memcpy(b, p + 2, 38);
            b += 38;
        }
        p[38] = p[1];                       /* (not "?:": ORIC-LESSONS 5) */
        if (p[1] == 0x40)
            p[38] = PAPER(BLACK);
        p[39] = p[0];
        p[2] = c;
        p[3] = PAPER(paper);
        memset(p + 4, 0x40, 34);
        if (y0++ == y1)
            break;
    }
}

/* the ink of lines y0 .. y1 inside the popup */
static void pop_ink(uint8_t y0, uint8_t y1, uint8_t c)
{
    for (; y0 <= y1; ++y0)
        LINE(y0)[2] = c;
}

void popup_restore(void)
{
    register uint8_t *p;
    uint8_t *b = pop_buf, n = pop_n;
    for (p = LINE(pop_y0); n; --n, p += 40, b += 38)
        memcpy(p + 2, b, 38);
    pop_n = 0;
    vehicle_reshift();
}

/* "CUT!": the director's megaphone and the word, large */
void popup_cut(void)
{
    popup(68, 123, RED, YELLOW, 0);
    pic2(PIC_MEGAPHONE, 11, 74);
    big(21, 79, "CUT!");
    if (m.takes)
        ctext(110, "RETAKE COMING UP");
    else
        ctext(110, "THAT WAS THE LAST TAKE");
}

void popup_pause(void)
{
    popup(86, 115, BLUE, WHITE, 1);
    cbig(91, "PAUSED");
    ctext(107, "ANY KEY GOES ON");
}

void popup_ask(void)
{
    popup(85, 114, RED, WHITE, 1);         /* (POP_SAVE lines) */
    pop_ink(89, 102, YELLOW);
    cbig(89, "QUIT?");
    ctext(105, "Y: THE TITLE   N: RIDE ON");
}

/* ------------------------------------------------- "THAT'S A PRINT!" */

void frame_print(void)
{
    popup(40, 151, BLUE, WHITE, 0);
    pop_ink(48, 61, YELLOW);
    cbig(48, "THAT'S A PRINT!");
    text(17, 72, "SCENE");
    text(23, 72, num(m.run_scene - 1, 0));
    row(90, "TRACK FEES", num(m.scene_fee - m.bonus, 1));
    if (m.bonus)
        row(102, "ONE-TAKE BONUS", num(m.bonus, 1));
    if (m.gave_take) {
        pop_ink(116, 122, GREEN);
        ctext(116, "A LOST TAKE COMES BACK");
    }
    pop_ink(134, 140, CYAN);
    row(134, "RUN FEE", num(m.fee, 1));
}

/* --------------------------------------------------- "YOU'RE FIRED!" */

void frame_fired(void)
{
    popup(52, 151, RED, WHITE, 0);
    pop_ink(60, 73, YELLOW);
    cbig(60, "YOU'RE FIRED!");
    ctext(84, "FINAL FEE");
    pop_ink(96, 109, YELLOW);
    cbig(96, num(m.fee, 1));
    text(12, 120, "BEST TODAY");
    text(23, 120, num(m.best, 1));
    if (m.fee > best_fee[NBEST - 1]) {
        pop_ink(136, 142, GREEN);
        ctext(136, "A TOP FIVE FEE!");
    }
}

/* ---------------------------------------------------- the initials */

#define NAME_TABLE 96                   /* first line of the table */
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
    big(15, 60, s);
    best_line(m.name_slot, NAME_TABLE + 10 * m.name_slot);
    shown_len = m.name_len;
}

void frame_name(void)
{
    uint8_t i;
    popup(16, 187, BLUE, WHITE, 0);
    pop_ink(24, 37, YELLOW);
    cbig(24, "TOP FIVE!");
    ctext(46, "TYPE YOUR INITIALS");
    pop_ink(60, 73, CYAN);
    for (i = 0; i < NBEST; ++i)
        best_line(i, NAME_TABLE + 10 * i);
    ctext(160, "DEL GOES BACK");
    ctext(172, "RETURN WHEN DONE");
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
    popup(16, 187, BLUE, WHITE, 0);
    pop_ink(24, 37, YELLOW);
    cbig(24, "THAT'S A WRAP!");
    pop_ink(48, 54, MAGENTA);
    ctext(48, "THE PREMIERE");
    for (i = 0; i < 6; ++i)
        ctext(64 + 12 * i, films[i]);
    ctext(144, "NEXT SEASON YOU ARE BILLED AS");
    pop_ink(160, 173, CYAN);
    i = m.billing;
    if (i < BILLINGS)
        ++i;
    cbig(160, billing_name[i]);
}
