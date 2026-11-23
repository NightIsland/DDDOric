/* emu.c - headless Oric-1 / Atmos 48K emulator, taken unchanged from Oric
   Chuckie Egg's harness (its docs/ARCHITECTURE.md D6) for Stunt Man Bob
   (docs/PLAN.md F0).  Built and driven by tools/run.py; see tools/README.md.
   "D1" below: no ROM code runs once the program is loaded.

   Emulated: NMOS 6502 (documented opcodes, cycle counts incl. page-cross and
   branch penalties, decimal mode); 48K RAM; no ROM (D1: executing or vectoring
   into $C000-$FFFF is an error); VIA 6522 (ORA/ORB/DDR, T1 one-shot/free-run,
   T2 one-shot, IFR/IER, PCR -> AY BC1/BDIR); AY-3-8912 registers (writes logged
   with cycle stamps); keyboard matrix via AY port A (R14) and VIA PB3;
   ULA screen renderer (TEXT and HIRES, serial attributes) -> raw RGB.
   IRQs (F3, D17): the VIA's IRQ is taken when I is clear, through the vector
   at $FFFE.  --irqvec ADDR puts the ROM's IRQ vector there (ROM 1.0: $0228,
   ROM 1.1: $0244, read from Oricutron's basic10.rom / basic11b.rom; run.py
   --rom oric1|atmos), the rest of $C000-$FFFF stays 0 and is never executed
   (D1).  Without --irqvec, $FFFE holds 0 and an IRQ is an error, as before.
   VIA, AY bus and ULA behaviour follow Oricutron (Resources/oricutron-master:
   via.c, 8912.c, ula.c); where this file differs, the comment says so.

   Harness debug port (RAM that is never displayed, harmless on real hardware):
     $BFE0 write: append byte to trace.txt   $BFE1 write: MARK (frame start)
     $BFE2 write: HALT (exit code = value)   $BFE3 write: BUSY (frame work done)

   Usage: emu (--tap F | --bin F@ADDR) [--start ADDR] [--clock mark|video]
              [--frames N] [--keys F] [--max-cycles N] [--irqvec ADDR]
              [--stop-trace V[:N]] [--tcycles] [--wlog] --out DIR
   F4a: --stop-trace V:N stops (cleanly) at the N-th write of byte V to $BFE0
   (default N = 1); --tcycles also logs every $BFE0 write as "value cycle" in
   tcycles.txt (the front end's phase markers).
   Stunt Man Bob: --wlog logs every write to HIRES ($A000-$BF3F) as
   "address value cycle" in wlog.txt (flicker checks).
   Outputs in DIR: screen.rgb (240x224 RGB), mem.bin (64K), trace.txt,
   ay.txt ("cycle reg value"), marks.txt ("M cycle" / "B cycle"), irq.txt
   ("cycle cost" per IRQ taken, from the IRQ to the end of its RTI), and one
   summary line on stdout.  Exit status 0 = clean stop, 2 = emulation error. */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t mem[65536];
static uint64_t cycles;
static uint64_t max_cycles = 200000000ULL;
static int stop;                 /* 0 running, 1 clean, 2 error */
static char stop_msg[200];
static int halt_code;
static int flat;                 /* --flat: plain 64K RAM, real BRK (CPU self-test only) */

static FILE *f_trace, *f_ay, *f_marks;
static FILE *f_tcyc;             /* F4a: --tcycles */
static FILE *f_wlog;             /* Stunt Man Bob BUILD 2: --wlog, HIRES writes */
static int stop_tval = -1;       /* F4a: --stop-trace value, count */
static long stop_tcount = 1;
static unsigned long nirq;       /* IRQs taken */
static FILE *f_irq;              /* irq.txt: "cycle cost" per IRQ, entry to RTI incl. the 7-cycle entry */
static int in_irq;
static uint64_t irq_start;

static void update_keybits(void);

/* ---------------------------------------------------------------- keys */
static uint8_t keyrow[8];        /* bit = column pressed */
static struct { long frame; uint8_t rows[8]; } *keyev;
static long nkeyev, keyidx;

static void apply_keys(long frame)
{
    while (keyidx < nkeyev && keyev[keyidx].frame <= frame) {
        memcpy(keyrow, keyev[keyidx].rows, 8);
        keyidx++;
    }
    update_keybits();
}

/* ---------------------------------------------------------------- AY */
/* As Oricutron 8912.c: the address latch takes the whole byte (registers >= 16
   are ignored), values are stored unmasked, a write of $FF to R13 is ignored. */
static uint8_t ay_reg[16], ay_creg;
static int ay_bc1, ay_bdir;
static int keydelay;             /* cycles until PB3 follows an R7/R14 write */

/* ---------------------------------------------------------------- VIA */
static uint8_t via_orb, via_ora, via_ddrb, via_ddra, via_acr, via_pcr;
static uint8_t via_ifr, via_ier, via_t1ll, via_t1lh, via_t2ll, via_t2lh, via_sr;
static uint16_t via_t1c, via_t2c;
static int via_t1run, via_t1reload, via_t2run, via_t2reload;
static uint8_t pa_in = 0xFF;     /* AY data bus as seen by VIA port A (pull-ups) */
static uint8_t pb_in = 0xFF;     /* port B input pins; PB3 = keyboard sense */

static void via_irqflag(void)
{
    if (via_ifr & via_ier & 0x7F) via_ifr |= 0x80; else via_ifr &= 0x7F;
}

/* Oricutron ay_update_keybits: row = ORB & DDRB, PB3 high when a key in the
   row sits in a column selected by a 0 in R14, and only while R7 bit 6 makes
   port A an output.  PB3 is latched here, not computed on every read. */
static void update_keybits(void)
{
    uint8_t col = (ay_reg[7] & 0x40) ? (uint8_t)~ay_reg[14] : 0;
    pb_in = (uint8_t)((pb_in & ~0x08) | ((keyrow[via_orb & via_ddrb & 7] & col) ? 0x08 : 0));
}

static void ay_modeset(void)
{
    if (!(ay_bc1 && !ay_bdir)) pa_in = 0xFF;          /* AY not driving the bus */
    if (ay_bc1 && !ay_bdir) pa_in = ay_creg < 16 ? ay_reg[ay_creg] : 0;
    else if (ay_bdir && !ay_bc1) {
        uint8_t v = via_ora & via_ddra;
        if (ay_creg >= 16 || (ay_creg == 13 && v == 0xFF)) return;
        ay_reg[ay_creg] = v;
        if (f_ay) fprintf(f_ay, "%llu %d %d\n", (unsigned long long)cycles, ay_creg, v);
        if (ay_creg == 7 || ay_creg == 14) keydelay = 3;
    } else if (ay_bdir && ay_bc1) ay_creg = via_ora & via_ddra;
}

/* Oricutron via.c: T1 continuous period = latch + 2, flag on the 0 -> $FFFF
   step; one-shot T1/T2 flag when the count passes below 0. */
static void via_tick(unsigned n)
{
    long crem;
    if (!(via_acr & 0x40)) {                              /* T1 one-shot */
        if (via_t1run && n > via_t1c) { via_ifr |= 0x40; via_t1run = 0; }
        via_t1c -= n;
    } else {
        crem = n;
        if (via_t1reload) { crem--; via_t1c = via_t1ll | via_t1lh << 8; via_t1reload = 0; }
        while (crem > via_t1c) {
            crem -= via_t1c + 1;
            via_t1c = 0xFFFF;
            via_ifr |= 0x40;
            if (!crem) { via_t1reload = 1; break; }
            crem--;
            via_t1c = via_t1ll | via_t1lh << 8;
        }
        via_t1c -= crem;
    }
    if (!(via_acr & 0x20)) {                              /* T2 one-shot (pulse counting not emulated) */
        crem = n;
        if (via_t2reload) { crem--; via_t2reload = 0; }
        if (via_t2run && crem > via_t2c) { via_ifr |= 0x20; via_t2run = 0; }
        via_t2c -= crem;
    }
    via_irqflag();
}

static uint8_t via_read(int r)
{
    switch (r) {
    case 0: return (via_orb & via_ddrb) | (pb_in & ~via_ddrb);
    case 1: case 15: return (via_ora & via_ddra) | (pa_in & ~via_ddra);
    case 2: return via_ddrb;
    case 3: return via_ddra;
    case 4: via_ifr &= ~0x40; via_irqflag(); return (uint8_t)via_t1c;
    case 5: return (uint8_t)(via_t1c >> 8);
    case 6: return via_t1ll;
    case 7: return via_t1lh;
    case 8: via_ifr &= ~0x20; via_irqflag(); return (uint8_t)via_t2c;
    case 9: return (uint8_t)(via_t2c >> 8);
    case 10: return via_sr;
    case 11: return via_acr;
    case 12: return via_pcr;
    case 13: return via_ifr;
    case 14: return via_ier | 0x80;
    }
    return 0;
}

static void via_write(int r, uint8_t v)
{
    int upd = 0;
    switch (r) {
    case 0: via_orb = v; update_keybits(); break;
    case 1: case 15: via_ora = v; ay_modeset(); break;
    case 2: via_ddrb = v; update_keybits(); break;
    case 3: via_ddra = v; break;
    case 4: case 6: via_t1ll = v; break;
    case 5: via_t1lh = v; via_t1c = via_t1ll | v << 8; via_t1reload = 1; via_t1run = 1;
            via_ifr &= ~0x40; break;
    case 7: via_t1lh = v; via_ifr &= ~0x40; break;
    case 8: via_t2ll = v; break;
    case 9: via_t2lh = v; via_t2c = via_t2ll | v << 8; via_t2reload = 1; via_t2run = 1;
            via_ifr &= ~0x20; break;
    case 10: via_sr = v; break;
    case 11: via_acr = v; if (!(v & 0x40)) via_t1reload = 0; break;
    case 12:   /* CA2 -> AY BC1, CB2 -> AY BDIR; only the manual-output modes drive them */
        via_pcr = v;
        if ((v & 0x0E) == 0x0C) { ay_bc1 = 0; upd = 1; }
        if ((v & 0x0E) == 0x0E) { ay_bc1 = 1; upd = 1; }
        if ((v & 0xE0) == 0xC0) { ay_bdir = 0; upd = 1; }
        if ((v & 0xE0) == 0xE0) { ay_bdir = 1; upd = 1; }
        if (upd) ay_modeset();
        break;
    case 13: via_ifr &= ~(v & 0x7F); break;
    case 14: if (v & 0x80) via_ier |= v & 0x7F; else via_ier &= ~(v & 0x7F); break;
    }
    via_irqflag();
}

/* ---------------------------------------------------------------- bus */
static long frame;               /* key-clock frame counter */
static long nframes = 100;
static int clock_mark = 1;
static long nmarks;

static void stop_err(const char *msg)
{
    if (!stop) { stop = 2; snprintf(stop_msg, sizeof stop_msg, "%s", msg); }
}

static void next_frame(void)
{
    frame++;
    if (frame >= nframes && !stop) { stop = 1; snprintf(stop_msg, sizeof stop_msg, "frames"); }
    apply_keys(frame);
}

static uint8_t rd(uint16_t ad)
{
    if (flat) return mem[ad];
    if ((ad & 0xFF00) == 0x0300) return via_read(ad & 15);
    return mem[ad];
}

static void wr(uint16_t ad, uint8_t v)
{
    if (flat) { mem[ad] = v; return; }
    if ((ad & 0xFF00) == 0x0300) { via_write(ad & 15, v); return; }
    if (ad >= 0xC000) return;                         /* ROM: writes go nowhere */
    mem[ad] = v;
    if (f_wlog && ad >= 0xA000 && ad < 0xBF40)
        fprintf(f_wlog, "%u %u %llu\n", ad, v, (unsigned long long)cycles);
    if (ad >= 0xBFE0 && ad <= 0xBFE3) {
        switch (ad) {
        case 0xBFE0: fputc(v, f_trace);
                     if (f_tcyc) fprintf(f_tcyc, "%u %llu\n", v, (unsigned long long)cycles);
                     if (v == stop_tval && --stop_tcount == 0 && !stop) {
                         stop = 1; snprintf(stop_msg, sizeof stop_msg, "trace byte %u", v); }
                     break;
        case 0xBFE1: fprintf(f_marks, "M %llu\n", (unsigned long long)cycles); nmarks++;
                     if (clock_mark) next_frame();
                     break;
        case 0xBFE2: if (!stop) { stop = 1; halt_code = v;
                         snprintf(stop_msg, sizeof stop_msg, "halt %d", v); } break;
        case 0xBFE3: fprintf(f_marks, "B %llu\n", (unsigned long long)cycles); break;
        }
    }
}

/* ---------------------------------------------------------------- 6502 */
enum { FC = 1, FZ = 2, FI = 4, FD = 8, FB = 16, FU = 32, FV = 64, FN = 128 };
static uint16_t pc;
static uint8_t a, x, y, s = 0xFF, p = FU | FI;
static int pgx;

static const uint8_t cyc[256] = {
7,6,0,0,0,3,5,0,3,2,2,0,0,4,6,0, 2,5,0,0,0,4,6,0,2,4,0,0,0,4,7,0,
6,6,0,0,3,3,5,0,4,2,2,0,4,4,6,0, 2,5,0,0,0,4,6,0,2,4,0,0,0,4,7,0,
6,6,0,0,0,3,5,0,3,2,2,0,3,4,6,0, 2,5,0,0,0,4,6,0,2,4,0,0,0,4,7,0,
6,6,0,0,0,3,5,0,4,2,2,0,5,4,6,0, 2,5,0,0,0,4,6,0,2,4,0,0,0,4,7,0,
0,6,0,0,3,3,3,0,2,0,2,0,4,4,4,0, 2,6,0,0,4,4,4,0,2,5,2,0,0,5,0,0,
2,6,2,0,3,3,3,0,2,2,2,0,4,4,4,0, 2,5,0,0,4,4,4,0,2,4,2,0,4,4,4,0,
2,6,0,0,3,3,5,0,2,2,2,0,4,4,6,0, 2,5,0,0,0,4,6,0,2,4,0,0,0,4,7,0,
2,6,0,0,3,3,5,0,2,2,2,0,4,4,6,0, 2,5,0,0,0,4,6,0,2,4,0,0,0,4,7,0 };

#define NZ(v) (p = (uint8_t)((p & ~(FN | FZ)) | ((v) & 0x80) | (((v) & 0xFF) ? 0 : FZ)))
static void push(uint8_t v) { wr(0x100 | s--, v); }
static uint8_t pull(void) { return rd(0x100 | ++s); }

static uint16_t a_zp(void)  { return rd(pc++); }
static uint16_t a_zpx(void) { return (uint8_t)(rd(pc++) + x); }
static uint16_t a_zpy(void) { return (uint8_t)(rd(pc++) + y); }
static uint16_t a_abs(void) { uint16_t l = rd(pc++); return l | rd(pc++) << 8; }
static uint16_t a_absi(uint8_t i) { uint16_t b = a_abs(), r = b + i; pgx = ((b ^ r) & 0xFF00) != 0; return r; }
static uint16_t a_indx(void) { uint8_t z = rd(pc++) + x; return rd(z) | rd((uint8_t)(z + 1)) << 8; }
static uint16_t a_indy(void)
{
    uint8_t z = rd(pc++);
    uint16_t b = rd(z) | rd((uint8_t)(z + 1)) << 8, r = b + y;
    pgx = ((b ^ r) & 0xFF00) != 0;
    return r;
}

static void adc(uint8_t v)
{
    unsigned c = p & FC, bin = a + v + c;
    if (p & FD) {
        unsigned lo = (a & 0x0F) + (v & 0x0F) + c, hi = (a & 0xF0) + (v & 0xF0);
        if (lo > 9) lo += 6;
        if (lo > 0x0F) hi += 0x10;
        p &= ~(FN | FV | FZ | FC);
        if (!(bin & 0xFF)) p |= FZ;
        if (hi & 0x80) p |= FN;
        if (~(a ^ v) & (a ^ hi) & 0x80) p |= FV;
        if (hi > 0x90) hi += 0x60;
        if (hi > 0xFF) p |= FC;
        a = (uint8_t)((hi & 0xF0) | (lo & 0x0F));
    } else {
        p &= ~(FC | FV);
        if (bin > 0xFF) p |= FC;
        if (~(a ^ v) & (a ^ bin) & 0x80) p |= FV;
        a = (uint8_t)bin; NZ(a);
    }
}

static void sbc(uint8_t v)
{
    unsigned b = 1 - (p & FC), bin = a - v - b;
    uint8_t r = (uint8_t)bin;
    p &= ~(FC | FV);
    if (bin < 0x100) p |= FC;
    if ((a ^ v) & (a ^ r) & 0x80) p |= FV;
    NZ(r);
    if (p & FD) {
        unsigned lo = (a & 0x0F) - (v & 0x0F) - b, hi = (a & 0xF0) - (v & 0xF0);
        if (lo & 0x10) { lo -= 6; hi -= 0x10; }
        if (hi & 0x100) hi -= 0x60;
        a = (uint8_t)((hi & 0xF0) | (lo & 0x0F));
    } else a = r;
}

static void cmp(uint8_t r, uint8_t v)
{
    unsigned t = r - v;
    p = (uint8_t)((p & ~FC) | (r >= v ? FC : 0)); NZ(t);
}

static uint8_t asl(uint8_t v) { p = (uint8_t)((p & ~FC) | (v >> 7)); v <<= 1; NZ(v); return v; }
static uint8_t lsr(uint8_t v) { p = (uint8_t)((p & ~FC) | (v & 1)); v >>= 1; NZ(v); return v; }
static uint8_t rol(uint8_t v) { uint8_t c = p & FC; p = (uint8_t)((p & ~FC) | (v >> 7)); v = (uint8_t)(v << 1 | c); NZ(v); return v; }
static uint8_t ror(uint8_t v) { uint8_t c = p & FC; p = (uint8_t)((p & ~FC) | (v & 1)); v = (uint8_t)(v >> 1 | c << 7); NZ(v); return v; }

static unsigned branch(int cond)
{
    int8_t o = (int8_t)rd(pc++);
    if (!cond) return 0;
    uint16_t n = pc + o;
    unsigned e = ((n ^ pc) & 0xFF00) ? 2 : 1;
    pc = n;
    return e;
}

#define RMW(addr, op) do { uint16_t ad_ = (addr); wr(ad_, op(rd(ad_))); } while (0)

static unsigned step(void)
{
    char msg[80];
    uint16_t at = pc, ad;
    uint8_t op = rd(pc++), t;
    unsigned c = cyc[op];
    pgx = 0;
    if (at >= 0xC000 && !flat) { snprintf(msg, sizeof msg, "execution in ROM area at $%04X (D1)", at); stop_err(msg); return 1; }
    switch (op) {
    /* loads */
    case 0xA9: a = rd(pc++); NZ(a); break;
    case 0xA5: a = rd(a_zp()); NZ(a); break;
    case 0xB5: a = rd(a_zpx()); NZ(a); break;
    case 0xAD: a = rd(a_abs()); NZ(a); break;
    case 0xBD: a = rd(a_absi(x)); NZ(a); c += pgx; break;
    case 0xB9: a = rd(a_absi(y)); NZ(a); c += pgx; break;
    case 0xA1: a = rd(a_indx()); NZ(a); break;
    case 0xB1: a = rd(a_indy()); NZ(a); c += pgx; break;
    case 0xA2: x = rd(pc++); NZ(x); break;
    case 0xA6: x = rd(a_zp()); NZ(x); break;
    case 0xB6: x = rd(a_zpy()); NZ(x); break;
    case 0xAE: x = rd(a_abs()); NZ(x); break;
    case 0xBE: x = rd(a_absi(y)); NZ(x); c += pgx; break;
    case 0xA0: y = rd(pc++); NZ(y); break;
    case 0xA4: y = rd(a_zp()); NZ(y); break;
    case 0xB4: y = rd(a_zpx()); NZ(y); break;
    case 0xAC: y = rd(a_abs()); NZ(y); break;
    case 0xBC: y = rd(a_absi(x)); NZ(y); c += pgx; break;
    /* stores */
    case 0x85: wr(a_zp(), a); break;
    case 0x95: wr(a_zpx(), a); break;
    case 0x8D: wr(a_abs(), a); break;
    case 0x9D: wr(a_absi(x), a); break;
    case 0x99: wr(a_absi(y), a); break;
    case 0x81: wr(a_indx(), a); break;
    case 0x91: wr(a_indy(), a); break;
    case 0x86: wr(a_zp(), x); break;
    case 0x96: wr(a_zpy(), x); break;
    case 0x8E: wr(a_abs(), x); break;
    case 0x84: wr(a_zp(), y); break;
    case 0x94: wr(a_zpx(), y); break;
    case 0x8C: wr(a_abs(), y); break;
    /* transfers */
    case 0xAA: x = a; NZ(x); break;
    case 0xA8: y = a; NZ(y); break;
    case 0x8A: a = x; NZ(a); break;
    case 0x98: a = y; NZ(a); break;
    case 0xBA: x = s; NZ(x); break;
    case 0x9A: s = x; break;
    /* stack */
    case 0x48: push(a); break;
    case 0x68: a = pull(); NZ(a); break;
    case 0x08: push(p | FB | FU); break;
    case 0x28: p = (uint8_t)((pull() | FU) & ~FB); break;
    /* logic / arithmetic */
#define ALU(base, OP) \
    case base + 0x09: OP(rd(pc++)); break; \
    case base + 0x05: OP(rd(a_zp())); break; \
    case base + 0x15: OP(rd(a_zpx())); break; \
    case base + 0x0D: OP(rd(a_abs())); break; \
    case base + 0x1D: OP(rd(a_absi(x))); c += pgx; break; \
    case base + 0x19: OP(rd(a_absi(y))); c += pgx; break; \
    case base + 0x01: OP(rd(a_indx())); break; \
    case base + 0x11: OP(rd(a_indy())); c += pgx; break;
#define ORA_(v) do { a |= (v); NZ(a); } while (0)
#define AND_(v) do { a &= (v); NZ(a); } while (0)
#define EOR_(v) do { a ^= (v); NZ(a); } while (0)
#define CMP_(v) cmp(a, (v))
    ALU(0x00, ORA_)
    ALU(0x20, AND_)
    ALU(0x40, EOR_)
    ALU(0x60, adc)
    ALU(0xC0, CMP_)
    ALU(0xE0, sbc)
    case 0xE0: cmp(x, rd(pc++)); break;
    case 0xE4: cmp(x, rd(a_zp())); break;
    case 0xEC: cmp(x, rd(a_abs())); break;
    case 0xC0: cmp(y, rd(pc++)); break;
    case 0xC4: cmp(y, rd(a_zp())); break;
    case 0xCC: cmp(y, rd(a_abs())); break;
    case 0x24: case 0x2C:
        t = rd(op == 0x24 ? a_zp() : a_abs());
        p = (uint8_t)((p & ~(FN | FV | FZ)) | (t & (FN | FV)) | ((a & t) ? 0 : FZ));
        break;
    /* inc / dec */
    case 0xE8: x++; NZ(x); break;
    case 0xC8: y++; NZ(y); break;
    case 0xCA: x--; NZ(x); break;
    case 0x88: y--; NZ(y); break;
#define INC_(v) ((uint8_t)((v) + 1))
#define DEC_(v) ((uint8_t)((v) - 1))
    case 0xE6: ad = a_zp();      t = INC_(rd(ad)); wr(ad, t); NZ(t); break;
    case 0xF6: ad = a_zpx();     t = INC_(rd(ad)); wr(ad, t); NZ(t); break;
    case 0xEE: ad = a_abs();     t = INC_(rd(ad)); wr(ad, t); NZ(t); break;
    case 0xFE: ad = a_absi(x);   t = INC_(rd(ad)); wr(ad, t); NZ(t); break;
    case 0xC6: ad = a_zp();      t = DEC_(rd(ad)); wr(ad, t); NZ(t); break;
    case 0xD6: ad = a_zpx();     t = DEC_(rd(ad)); wr(ad, t); NZ(t); break;
    case 0xCE: ad = a_abs();     t = DEC_(rd(ad)); wr(ad, t); NZ(t); break;
    case 0xDE: ad = a_absi(x);   t = DEC_(rd(ad)); wr(ad, t); NZ(t); break;
    /* shifts */
    case 0x0A: a = asl(a); break;
    case 0x4A: a = lsr(a); break;
    case 0x2A: a = rol(a); break;
    case 0x6A: a = ror(a); break;
#define SHIFTS(base, OP) \
    case base + 0x06: RMW(a_zp(), OP); break; \
    case base + 0x16: RMW(a_zpx(), OP); break; \
    case base + 0x0E: RMW(a_abs(), OP); break; \
    case base + 0x1E: RMW(a_absi(x), OP); break;
    SHIFTS(0x00, asl)
    SHIFTS(0x40, lsr)
    SHIFTS(0x20, rol)
    SHIFTS(0x60, ror)
    /* jumps */
    case 0x4C: pc = a_abs(); break;
    case 0x6C: { uint16_t i = a_abs(); pc = rd(i) | rd((uint16_t)((i & 0xFF00) | ((i + 1) & 0xFF))) << 8; } break;
    case 0x20: ad = a_abs(); pc--; push(pc >> 8); push((uint8_t)pc); pc = ad; break;
    case 0x60: pc = pull(); pc |= pull() << 8; pc++; break;
    case 0x40: p = (uint8_t)((pull() | FU) & ~FB); pc = pull(); pc |= pull() << 8; break;
    case 0x00:
        if (flat) {
            pc++; push(pc >> 8); push((uint8_t)pc); push(p | FB | FU); p |= FI;
            pc = rd(0xFFFE) | rd(0xFFFF) << 8; break;
        }
        snprintf(msg, sizeof msg, "BRK at $%04X (no ROM vectors, D1)", at); stop_err(msg); return c;
    /* branches */
    case 0x10: c += branch(!(p & FN)); break;
    case 0x30: c += branch(p & FN); break;
    case 0x50: c += branch(!(p & FV)); break;
    case 0x70: c += branch(p & FV); break;
    case 0x90: c += branch(!(p & FC)); break;
    case 0xB0: c += branch(p & FC); break;
    case 0xD0: c += branch(!(p & FZ)); break;
    case 0xF0: c += branch(p & FZ); break;
    /* flags */
    case 0x18: p &= ~FC; break;
    case 0x38: p |= FC; break;
    case 0x58: p &= ~FI; break;
    case 0x78: p |= FI; break;
    case 0xB8: p &= ~FV; break;
    case 0xD8: p &= ~FD; break;
    case 0xF8: p |= FD; break;
    case 0xEA: break;
    default:
        snprintf(msg, sizeof msg, "illegal opcode $%02X at $%04X", op, at); stop_err(msg); return 2;
    }
    return c;
}

/* ---------------------------------------------------------------- ULA */
static const uint8_t pal[8][3] = {{0,0,0},{255,0,0},{0,255,0},{255,255,0},
                                  {0,0,255},{255,0,255},{0,255,255},{255,255,255}};
static uint8_t screen[224][240];
static int ula_hires;
static unsigned long vframes;    /* 20 ms frames elapsed: blink phase (Oricutron frames & 0x10) */

/* As Oricutron ula.c: each scanline starts white on black, standard charset;
   the mode is ULA state that persists, and a mode attribute re-points the fetch
   for the rest of the line at once.  Lines 200-223 are always character rows
   from $BB80 + 40*(line/8) (= $BF68 for line 200) using the current mode's
   charset ($9800/$9C00 in HIRES, $B400/$B800 in TEXT). */
static void render_frame(void)
{
    for (int line = 0; line < 224; line++) {
        int ink = 7, paper = 0, alt = 0, blinkmask = 0x3F, chline = line & 7;
        uint16_t text = 0xBB80 + (line >> 3) * 40;
        int bitmap = line < 200 && ula_hires;
        uint16_t base = line < 200 ? (ula_hires ? 0xA000 + line * 40 : text) : text;
        for (int col = 0; col < 40; col++) {
            uint8_t b = mem[base + col], pat;
            int inv = (b & 0x80) ? 7 : 0;
            int mask = (vframes & 0x10) ? 0x3F : blinkmask;
            if ((b & 0x60) == 0) {
                int at = b & 0x1F;
                switch (at & 0x18) {
                case 0x00: ink = at & 7; break;
                case 0x08: alt = at & 1; blinkmask = (at & 4) ? 0 : 0x3F;
                           chline = (at & 2) ? (line >> 1) & 7 : line & 7; break;
                case 0x10: paper = at & 7; break;
                case 0x18: ula_hires = (at & 4) != 0;
                           if (line < 200) { bitmap = ula_hires;
                               base = ula_hires ? 0xA000 + line * 40 : text; }
                           else bitmap = 0;
                           break;
                }
                for (int i = 0; i < 6; i++) screen[line][col * 6 + i] = (uint8_t)(paper ^ inv);
                continue;
            }
            if (bitmap) pat = b & mask;
            else {
                uint16_t cs = ula_hires ? (alt ? 0x9C00 : 0x9800) : (alt ? 0xB800 : 0xB400);
                pat = mem[cs + (b & 0x7F) * 8 + chline] & mask;
            }
            for (int i = 0; i < 6; i++)
                screen[line][col * 6 + i] = (uint8_t)(((pat & (0x20 >> i)) ? ink : paper) ^ inv);
        }
    }
}

/* ---------------------------------------------------------------- loading */
static long load_tap(const char *fn, int *autorun)
{
    FILE *f = fopen(fn, "rb");
    if (!f) { perror(fn); exit(2); }
    static uint8_t buf[70000];
    size_t n = fread(buf, 1, sizeof buf, f), i = 0;
    fclose(f);
    while (i < n && buf[i] == 0x16) i++;
    if (i < 3 || i >= n || buf[i] != 0x24 || i + 10 > n) { fprintf(stderr, "%s: not a .tap\n", fn); exit(2); }
    i++;
    uint8_t *h = &buf[i];
    unsigned end = h[4] << 8 | h[5], start = h[6] << 8 | h[7];
    *autorun = h[3];
    i += 9;
    while (i < n && buf[i]) i++;
    i++;
    if (end < start || i + (end - start + 1) > n) { fprintf(stderr, "%s: truncated\n", fn); exit(2); }
    memcpy(&mem[start], &buf[i], end - start + 1);
    return start;
}

static void load_keys(const char *fn)
{
    FILE *f = fopen(fn, "r");
    if (!f) { perror(fn); exit(2); }
    char line[256];
    long cap = 0;
    while (fgets(line, sizeof line, f)) {
        long fr; unsigned r[8];
        if (sscanf(line, "%ld %x %x %x %x %x %x %x %x", &fr, &r[0], &r[1], &r[2], &r[3],
                   &r[4], &r[5], &r[6], &r[7]) != 9) continue;
        if (nkeyev == cap) { cap = cap ? cap * 2 : 64; keyev = realloc(keyev, cap * sizeof *keyev); }
        keyev[nkeyev].frame = fr;
        for (int k = 0; k < 8; k++) keyev[nkeyev].rows[k] = (uint8_t)r[k];
        nkeyev++;
    }
    fclose(f);
}

static FILE *outf(const char *dir, const char *name, const char *mode)
{
    char path[1024];
    snprintf(path, sizeof path, "%s/%s", dir, name);
    FILE *f = fopen(path, mode);
    if (!f) { perror(path); exit(2); }
    return f;
}

int main(int argc, char **argv)
{
    const char *tap = 0, *bin = 0, *keys = 0, *out = 0;
    long irqvec = 0;
    long start = -1, binaddr = 0;
    int tcyc = 0, wlog = 0;
    int autorun = 0;
    for (int i = 1; i < argc; i++) {
        const char *o = argv[i], *v = i + 1 < argc ? argv[i + 1] : "";
        if (!strcmp(o, "--tap")) { tap = v; i++; }
        else if (!strcmp(o, "--bin")) { bin = v; i++; }
        else if (!strcmp(o, "--start")) { start = strtol(v, 0, 0); i++; }
        else if (!strcmp(o, "--clock")) { clock_mark = strcmp(v, "video") != 0; i++; }
        else if (!strcmp(o, "--frames")) { nframes = strtol(v, 0, 0); i++; }
        else if (!strcmp(o, "--keys")) { keys = v; i++; }
        else if (!strcmp(o, "--max-cycles")) { max_cycles = strtoull(v, 0, 0); i++; }
        else if (!strcmp(o, "--out")) { out = v; i++; }
        else if (!strcmp(o, "--flat")) flat = 1;
        else if (!strcmp(o, "--irqvec")) { irqvec = strtol(v, 0, 0); i++; }
        else if (!strcmp(o, "--stop-trace")) {
            char *e;
            stop_tval = (int)strtol(v, &e, 0);
            if (*e == ':') stop_tcount = strtol(e + 1, 0, 0);
            i++;
        }
        else if (!strcmp(o, "--tcycles")) tcyc = 1;
        else if (!strcmp(o, "--wlog")) wlog = 1;
        else if (!strcmp(o, "--junk")) {    /* RAM as a booted machine leaves it: not zero */
            unsigned x = (unsigned)strtoul(v, 0, 0) | 1;
            for (long a = 0; a < 0xC000; a++) { x ^= x << 13; x ^= x >> 17; x ^= x << 5; mem[a] = (uint8_t)x; }
            i++;
        }
        else { fprintf(stderr, "emu: unknown option %s\n", o); return 2; }
    }
    if ((!tap && !bin) || !out) { fprintf(stderr, "usage: see emu.c header\n"); return 2; }
    if (tap) { long s0 = load_tap(tap, &autorun); if (start < 0) start = s0; }
    else {
        char fn[1024]; const char *at = strrchr(bin, '@');
        if (!at) { fprintf(stderr, "--bin FILE@ADDR\n"); return 2; }
        snprintf(fn, sizeof fn, "%.*s", (int)(at - bin), bin);
        binaddr = strtol(at + 1, 0, 0);
        FILE *f = fopen(fn, "rb");
        if (!f) { perror(fn); return 2; }
        if (fread(&mem[binaddr], 1, (flat ? 0x10000 : 0xC000) - binaddr, f) == 0) { fprintf(stderr, "%s: empty\n", fn); return 2; }
        fclose(f);
        if (start < 0) start = binaddr;
    }
    if (irqvec) {
        mem[0xFFFE] = (uint8_t)irqvec;
        mem[0xFFFF] = (uint8_t)(irqvec >> 8);
    }
    if (keys) load_keys(keys);
    f_trace = outf(out, "trace.txt", "wb");
    f_ay = outf(out, "ay.txt", "w");
    f_marks = outf(out, "marks.txt", "w");
    f_irq = outf(out, "irq.txt", "w");
    if (tcyc) f_tcyc = outf(out, "tcycles.txt", "w");
    if (wlog) f_wlog = outf(out, "wlog.txt", "w");

    pc = (uint16_t)start;
    via_ddrb = 0xF7; via_pcr = 0xDD;      /* roughly where the ROM leaves them */
    via_t1c = via_t2c = 0xFFFF;
    apply_keys(0);
    const uint64_t vframe = 19968;        /* 312 lines x 64 us */
    uint64_t next_video = vframe;
    while (!stop) {
        uint16_t pc0 = pc;
        static uint16_t hist[16]; static unsigned hn;
        hist[hn++ & 15] = pc;
        uint8_t op0 = mem[pc], pbefore = p;
        unsigned c = step();
        uint8_t ipoll = (op0 == 0x58 || op0 == 0x78 || op0 == 0x28) ? pbefore : p;
        if (flat && pc == pc0) { for (unsigned k = 0; k < 16; k++) fprintf(stderr, "%04X ", hist[(hn + k) & 15]); fprintf(stderr, "\n"); }
        if (flat && pc == pc0) { stop = 1; snprintf(stop_msg, sizeof stop_msg, "trap $%04X", pc); }
        cycles += c;
        via_tick(c);
        if (in_irq && op0 == 0x40) {
            in_irq = 0;
            fprintf(f_irq, "%llu %llu\n", (unsigned long long)irq_start, (unsigned long long)(cycles - irq_start));
        }
        if (keydelay > 0) {
            if ((int)c >= keydelay) { keydelay = 0; update_keybits(); }
            else keydelay -= c;
        }
        /* The 6502 samples IRQ with the I flag as it was before CLI/SEI/PLP
           (one instruction's delay after CLI); RTI's restored flag counts at once. */
        if (!flat && (via_ifr & 0x80) && !(ipoll & FI)) {
            uint16_t v = mem[0xFFFE] | mem[0xFFFF] << 8;
            if (v == 0 || v >= 0xC000) {
                stop_err(v ? "IRQ vector points into the ROM (D1)" : "IRQ taken with no vector (no --irqvec, D1)");
            } else {
                if (!in_irq) { in_irq = 1; irq_start = cycles; }
                push(pc >> 8); push((uint8_t)pc); push((p | FU) & ~FB);
                p |= FI;
                pc = v;
                nirq++;
                cycles += 7;
                via_tick(7);
            }
        }
        if (cycles >= next_video) {
            next_video += vframe;
            vframes++;
            if (!clock_mark) next_frame();
        }
        if (cycles >= max_cycles) stop_err("max cycles reached");
    }
    render_frame();               /* settle the ULA mode, then the real frame */
    render_frame();
    FILE *f = outf(out, "screen.rgb", "wb");
    for (int l = 0; l < 224; l++)
        for (int c = 0; c < 240; c++) fwrite(pal[screen[l][c]], 1, 3, f);
    fclose(f);
    f = outf(out, "mem.bin", "wb");
    fwrite(mem, 1, sizeof mem, f);
    fclose(f);
    fclose(f_trace); fclose(f_ay); fclose(f_marks); fclose(f_irq);
    if (f_tcyc) fclose(f_tcyc);
    if (f_wlog) fclose(f_wlog);
    printf("stop=%s cycles=%llu frames=%ld marks=%ld pc=$%04X autorun=$%02X hires=%d irqs=%lu\n",
           stop == 2 ? "ERROR" : "ok", (unsigned long long)cycles, frame, nmarks, pc, autorun, ula_hires, nirq);
    if (stop == 2) fprintf(stderr, "emu: %s\n", stop_msg);
    else printf("reason=%s\n", stop_msg);
    return stop == 2 ? 2 : 0;
}
