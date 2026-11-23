; sound.s - AY set-up, placeholder engine tone and noise bursts (docs/PLAN.md F0).
;
; Channel A: engine tone (snd_engine).  Channel B: noise bursts (snd_burst),
; volume stepped down from the 10 ms tick.  F2 replaces the sounds; what stays
; is the rule from DESIGN "Sound": built with ORICUTRON (make ORICUTRON=1) every
; noise period is doubled, because Oricutron plays AY noise an octave high.
; Effects use noise periods <= 15 so the doubled value still fits R6's 5 bits
; (never 32: the AY reads 0 as 1).

        .export         snd_init, snd_tick, _snd_engine, _snd_burst
        .import         ay_write

MIXER    = %01101110            ; port A out, noise on B only, tone on A only

        .segment        "BSS"
burst_vol:      .res 1          ; channel B volume, counts down each tick

        .segment        "CODE"

; snd_init: silence, mixer.  IRQs off (called from sys_init).
snd_init:
        ldx     #13
@clr:   txa
        ldy     #0
        jsr     ay_write
        dex
        bpl     @clr
        lda     #7
        ldy     #MIXER
        jsr     ay_write
        lda     #0
        sta     burst_vol
        rts

; snd_tick: from the IRQ (IRQs off).  Burst volume - 1 until silent.
snd_tick:
        lda     burst_vol
        beq     @done
        dec     burst_vol
        ldy     burst_vol
        lda     #9
        jmp     ay_write
@done:  rts

; void __fastcall__ snd_engine(uint16_t period): tone A at the given period
; (62,500 / f), or silent for 0.
_snd_engine:
        php
        sei
        cmp     #0
        bne     @on
        cpx     #0
        bne     @on
        lda     #8              ; period 0: volume A = 0
        ldy     #0
        jsr     ay_write
        plp
        rts
@on:    tay
        stx     @hi+1
        lda     #0
        jsr     ay_write        ; R0 = period low
@hi:    ldy     #0
        lda     #1
        jsr     ay_write        ; R1 = period high
        lda     #8
        ldy     #6              ; quiet engine
        jsr     ay_write
        plp
        rts

; void __fastcall__ snd_burst(uint8_t period): a noise burst at full volume,
; fading over 150 ms.  period 1-15.
_snd_burst:
.ifdef ORICUTRON
        asl     a               ; Oricutron test build: one octave lower
.endif
        php
        sei
        tay
        lda     #6
        jsr     ay_write        ; R6 = noise period
        lda     #15
        sta     burst_vol
        tay
        lda     #9
        jsr     ay_write        ; R9 = volume B
        plp
        rts
