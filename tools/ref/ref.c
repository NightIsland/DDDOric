/* ref.c - native reference: the game model (src/model.c) driven by a key
   script, printing one trace line per step (src/trace.c).  The Oric build in
   the harness emulator must print exactly the same lines for the same script
   (docs/PLAN.md F0; tools/README.md).

   usage: ref SCRIPT STEPS > trace.txt

   Key scripts: tools/keyscript.py has the grammar.  Step n (1-based) reads the
   keys of script frame n, as the Oric build does after its n-th MARK.  Only the
   lower-case game tokens count here; upper-case Oric key names are for the
   emulator only. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "model.h"
#include "trace.h"

#define MAXLINES 4096
static struct { long a, b; uint8_t bits; } ln[MAXLINES];
static int nln;

static const struct { const char *name; uint8_t bit; } tok[] = {
    {"jump", IN_JUMP}, {"accel", IN_ACCEL}, {"stop", IN_STOP},
    {"pause", IN_PAUSE}, {"quit", IN_QUIT},
};

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
        ln[nln].bits = 0;
        while ((t = strtok(0, " \t\r\n")))
            for (unsigned i = 0; i < sizeof tok / sizeof tok[0]; i++)
                if (!strcmp(t, tok[i].name)) ln[nln].bits |= tok[i].bit;
        nln++;
    }
    fclose(f);
}

int main(int argc, char **argv)
{
    if (argc != 3) { fprintf(stderr, "usage: ref SCRIPT STEPS\n"); return 2; }
    load(argv[1]);
    long steps = strtol(argv[2], 0, 10);
    model_init();
    for (long n = 1; n <= steps; n++) {
        uint8_t in = 0;
        for (int i = 0; i < nln; i++)
            if (ln[i].a <= n && n <= ln[i].b) in |= ln[i].bits;
        model_step(in);
        trace_step(in);
    }
    return 0;
}
