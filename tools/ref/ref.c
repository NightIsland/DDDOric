/* ref.c - native reference: the game model (src/model.c) driven by a key
   script, printing one trace line per step (src/trace.c).  The Oric build in
   the harness emulator must print exactly the same lines for the same script
   (docs/PLAN.md F0; tools/README.md).

   usage: ref SCRIPT STEPS [test] > trace.txt   (test: the empty test track)

   Key scripts: tools/keyscript.py has the grammar.  Step n (1-based) reads the
   keys of script frame n, as the Oric build does after its n-th MARK.  Game
   tokens press their default keys; upper-case tokens are Oric keys, which give
   game inputs (keys_poll) and, off M_PLAY, a key code (keys_ascii) as on the
   Oric. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "model.h"
#include "trace.h"
#include "scene.h"

#define MAXLINES 4096
static struct { long a, b; uint64_t keys; } ln[MAXLINES];
static int nln;

/* The Oric keyboard (tools/keyscript.py MATRIX): name, row, column bit, the
   code keys_ascii (src/keys.s) returns for it (0: none) and its game input. */
static const struct { const char *name; uint8_t row, col, code, bit; } kb[] = {
    {"3",0,0x80,'3',0}, {"X",0,0x40,'X',0}, {"1",0,0x20,'1',0}, {"V",0,0x08,'V',0},
    {"5",0,0x04,'5',0}, {"N",0,0x02,'N',0}, {"7",0,0x01,'7',0},
    {"D",1,0x80,'D',0}, {"Q",1,0x40,'Q',0}, {"ESC",1,0x20,27,IN_QUIT}, {"F",1,0x08,'F',0},
    {"R",1,0x04,'R',0}, {"T",1,0x02,'T',0}, {"J",1,0x01,'J',0},
    {"C",2,0x80,'C',0}, {"2",2,0x40,'2',0}, {"Z",2,0x20,'Z',0}, {"CTRL",2,0x10,0,0},
    {"4",2,0x08,'4',0}, {"B",2,0x04,'B',0}, {"6",2,0x02,'6',0}, {"M",2,0x01,'M',0},
    {"'",3,0x80,'\'',0}, {"\\",3,0x40,'\\',0}, {"-",3,0x08,'-',0}, {";",3,0x04,';',0},
    {"9",3,0x02,'9',0}, {"K",3,0x01,'K',0},
    {"RIGHT",4,0x80,0,0}, {"DOWN",4,0x40,0,0}, {"LEFT",4,0x20,0,0}, {"LSHIFT",4,0x10,0,IN_ACCEL},
    {"UP",4,0x08,0,0}, {".",4,0x04,'.',0}, {",",4,0x02,',',0}, {"SPACE",4,0x01,' ',IN_JUMP},
    {"[",5,0x80,'[',0}, {"]",5,0x40,']',0}, {"DEL",5,0x20,127,0}, {"FUNCT",5,0x10,0,0},
    {"P",5,0x08,'P',IN_PAUSE}, {"O",5,0x04,'O',0}, {"I",5,0x02,'I',0}, {"U",5,0x01,'U',0},
    {"W",6,0x80,'W',0}, {"S",6,0x40,'S',0}, {"A",6,0x20,'A',0}, {"E",6,0x08,'E',0},
    {"G",6,0x04,'G',0}, {"H",6,0x02,'H',0}, {"Y",6,0x01,'Y',0},
    {"=",7,0x80,'=',0}, {"RETURN",7,0x20,13,IN_STOP}, {"RSHIFT",7,0x10,0,IN_ACCEL},
    {"/",7,0x08,'/',0}, {"0",7,0x04,'0',0}, {"L",7,0x02,'L',0}, {"8",7,0x01,'8',0},
};
#define NKB (sizeof kb / sizeof kb[0])
/* the game tokens press their default keys (keyscript.DEFAULT_KEYMAP) */
static const struct { const char *tok, *key; } game[] = {
    {"jump", "SPACE"}, {"accel", "LSHIFT"}, {"stop", "RETURN"}, {"pause", "P"}, {"quit", "ESC"},
};

static int key_index(const char *name)
{
    for (unsigned i = 0; i < NKB; i++)
        if (!strcmp(name, kb[i].name)) return (int)i;
    return -1;
}

static void load(const char *fn)
{
    char buf[1024], *p, *t;
    FILE *f = fopen(fn, "r");
    if (!f) { perror(fn); exit(2); }
    while (fgets(buf, sizeof buf, f)) {
        if ((p = strchr(buf, '#'))) *p = 0;
        t = strtok(buf, " \t\r\n");
        if (!t) continue;
        if (nln == MAXLINES) { fprintf(stderr, "ref: script too long\n"); exit(2); }
        ln[nln].a = strtol(t, &p, 10);
        ln[nln].b = *p == '-' ? strtol(p + 1, 0, 10) : ln[nln].a;
        ln[nln].keys = 0;
        while ((t = strtok(0, " \t\r\n"))) {
            for (unsigned i = 0; i < sizeof game / sizeof game[0]; i++)
                if (!strcmp(t, game[i].tok)) t = (char *)game[i].key;
            int k = key_index(t);
            if (k < 0) { fprintf(stderr, "ref: unknown token %s\n", t); exit(2); }
            ln[nln].keys |= 1ull << k;
        }
        nln++;
    }
    fclose(f);
}

/* keys_ascii's order: rows 0-7, in each the column bits from $80 down; the
   first held key with a code */
static uint8_t scan_code(uint64_t held)
{
    for (uint8_t r = 0; r < 8; r++)
        for (int c = 0x80; c; c >>= 1)
            for (unsigned i = 0; i < NKB; i++)
                if (kb[i].row == r && kb[i].col == c && (held >> i & 1) && kb[i].code)
                    return kb[i].code;
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 3 && !(argc == 4 && !strcmp(argv[3], "test"))) {
        fprintf(stderr, "usage: ref SCRIPT STEPS [test]\n");
        return 2;
    }
    load(argv[1]);
    long steps = strtol(argv[2], 0, 10);
    if (argc == 4)
        scene_only = &scene_test;
    model_init(1);
    for (long n = 1; n <= steps; n++) {
        uint64_t held = 0;
        uint8_t in = 0, key = 0;
        for (int i = 0; i < nln; i++)
            if (ln[i].a <= n && n <= ln[i].b) held |= ln[i].keys;
        for (unsigned i = 0; i < NKB; i++)
            if (held >> i & 1) in |= kb[i].bit;
        if (m.mode != M_PLAY)               /* as src/main.c reads it */
            key = scan_code(held);
        model_step(in, key);
        trace_step(in, key);
    }
    return 0;
}
