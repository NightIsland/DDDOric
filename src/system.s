; system.s - interrupt hook, 10 ms tick and 50 Hz step pacing (docs/PLAN.md F0;
; docs/ORIC-LESSONS.md sections 3 and 8).  Adapted from Oric Chuckie Egg's
; src/system.s (F3), by the same author, for Stunt Man Bob.
;
; sys_init: VIA T1 free-running at 10,000 cycles; our IRQ handler hooked through
; the RAM vector that the ROM's $FFFE points at, so one binary runs on both ROMs
; and no ROM code ever runs:
;     ROM 1.0 (Oric-1): $FFFE = $0228     ROM 1.1 (Atmos): $FFFE = $0244
; The ROM stores a JMP there; we overwrite its three bytes.  A vector anywhere
; else (unknown ROM) -> irq_mode = 0: IRQs stay off and step_wait polls the T1
; flag instead, so the game still runs at the same pace.
;
; The handler (every 10 ms): acknowledge T1, ticks + 1, one sound step, RTI.
;
; step_wait ends a game step: wait until ticks >= 2, then set ticks to 0.  Steady
; state: 20,000 cycles a step (50 Hz, DESIGN "Physics").  A step that has used
; >= 2 ticks returns at once and the next one realigns.
;
; ay_write is for code running with IRQs off (the handler, or between
; php/sei .. plp): the AY's address latch is shared with the sound handler.

        .export         _sys_init, _step_wait, _ticks, _irq_mode, ay_write, _step_left
        .import         snd_tick, snd_init
        .include        "zeropage.inc"

VIA_ORB  = $0300
VIA_ORA  = $0301
VIA_DDRB = $0302
VIA_DDRA = $0303
VIA_T1CL = $0304
VIA_T1CH = $0305
VIA_T2CL = $0308
VIA_T2CH = $0309
VIA_ACR  = $030B
VIA_PCR  = $030C
VIA_IFR  = $030D
VIA_IER  = $030E

TICK       = 10000              ; cycles per tick (1 MHz: 10 ms)
STEP_TICKS = 2                  ; ticks per game step (20 ms: 50 Hz)

        .segment        "BSS"
_ticks:         .res 1          ; 10 ms ticks since step_wait last reset it
_irq_mode:      .res 1          ; 1 = our IRQ handler runs, 0 = polling fallback

        .segment        "CODE"

; void sys_init(void) - VIA, AY, T1 and the IRQ hook.  Leaves IRQs enabled when
; the ROM is known.
_sys_init:
        sei
        lda     #$FF
        sta     VIA_DDRA        ; port A: AY data bus, output
        lda     #$F7
        sta     VIA_DDRB        ; PB0-2 keyboard row out, PB3 key sense in
        lda     #$DD
        sta     VIA_PCR         ; AY bus inactive (BC1 = BDIR = 0)
        lda     #$7F
        sta     VIA_IER         ; every VIA IRQ source off
        jsr     snd_init        ; AY: silent, mixer set (port A output for the keyboard)
        lda     #$40
        sta     VIA_ACR         ; T1 free-running, no PB7 output; T2 one-shot
        lda     #<(TICK - 2)    ; continuous period = latch + 2
        sta     VIA_T1CL
        lda     #>(TICK - 2)
        sta     VIA_T1CH        ; load and start T1
        lda     #$7F
        sta     VIA_IFR         ; clear every pending flag
        lda     #0
        sta     _ticks
        sta     _irq_mode
        lda     $FFFE
        sta     ptr1
        lda     $FFFF
        sta     ptr1+1
        cmp     #$02            ; a RAM vector in pages 2-4 (system area below
        bcc     @poll           ; our program at $0500)?
        cmp     #$05
        bcs     @poll
        ldy     #0
        lda     #$4C            ; JMP irq
        sta     (ptr1),y
        iny
        lda     #<irq
        sta     (ptr1),y
        iny
        lda     #>irq
        sta     (ptr1),y
        inc     _irq_mode
        lda     #$C0
        sta     VIA_IER         ; T1 IRQ on
        cli
@poll:  rts

; The IRQ handler, reached by the ROM's vector through our JMP.
irq:    pha
        txa
        pha
        tya
        pha
        lda     VIA_T1CL        ; acknowledge T1 (clears IFR bit 6)
        jsr     tick
        pla
        tay
        pla
        tax
        pla
        rti

tick:   inc     _ticks
        jmp     snd_tick

; void step_wait(void) - end of a game step.
_step_wait:
        lda     _irq_mode
        beq     @poll
@wait:  lda     _ticks
        cmp     #STEP_TICKS
        bcc     @wait
        lda     #0              ; a tick arriving since the compare only makes the
        sta     _ticks          ; count higher: the reset discards it either way
        rts
@poll:  bit     VIA_IFR         ; fallback: count T1 periods while waiting
        bvc     @poll2
        lda     VIA_T1CL
        jsr     tick
@poll2: lda     _ticks
        cmp     #STEP_TICKS
        bcc     @poll
        lda     #0
        sta     _ticks
        rts

; uint8_t step_left(void) - time left in this step before step_wait would
; return at once, in units of 256 cycles (0 = none), so drawing can put off
; what doesn't fit (PLAN F3).  Ticks still to come, plus T1's count to the
; next one: reading T1's high byte leaves the IRQ flag alone.  Polling
; fallback: 255 (no timer to go by).
_step_left:
        lda     _irq_mode
        bne     @ok
        lda     #255
        ldx     #0
        rts
@ok:    php
        sei
        lda     VIA_T1CH
        ldx     _ticks
        bit     VIA_IFR         ; a tick due but not yet counted?
        bvc     @n
        inx
        lda     VIA_T1CH        ; T1 has restarted: read it again
@n:     plp
        cpx     #STEP_TICKS - 1
        beq     @r              ; last tick of the step: T1's count
        bcs     @none
        clc
        adc     #>TICK          ; one more tick to come
@r:     ldx     #0              ; cc65 reads a char result as A with X = 0
        rts
@none:  lda     #0
        tax
        rts

; ay_write: AY register A := Y.  IRQs must be off.  Clobbers A.
ay_write:
        sta     VIA_ORA
        lda     #$FF            ; BDIR = BC1 = 1: latch the address
        sta     VIA_PCR
        lda     #$DD
        sta     VIA_PCR
        sty     VIA_ORA
        lda     #$FD            ; BDIR = 1, BC1 = 0: write
        sta     VIA_PCR
        lda     #$DD
        sta     VIA_PCR
        rts

; Trace output for the harness (src/trace.c): a byte as two hex digits, and
; the same with a space, straight to the debug port $BFE0 from tables.
        .export         _hex2, _hex2sp

_hex2:  tax
        lda     hexhi,x
        sta     $BFE0
        lda     hexlo,x
        sta     $BFE0
        rts

_hex2sp:
        jsr     _hex2
        lda     #' '
        sta     $BFE0
        rts

        .segment        "RODATA"
hexhi:
        .repeat 256, I
        .byte   .strat("0123456789abcdef", I >> 4)
        .endrep
hexlo:
        .repeat 256, I
        .byte   .strat("0123456789abcdef", I & 15)
        .endrep
