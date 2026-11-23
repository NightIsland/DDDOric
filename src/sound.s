; sound.s - AY sound: engine, jump, landing and stop (docs/PLAN.md F2; DESIGN
; "Sound and music").
;
; Channel A: the engine.  Tone pitch set by snd_engine from the speed; the
;            10 ms tick alternates its volume (ENG_HI, ENG_LO) for a motorbike's
;            buzz, 50 times a second.
; Channel B: noise bursts (snd_burst and the effects), volume stepped down from
;            the tick.
; Channel C: a tone sweep (the jump's rising whoosh, the landing's thump, the
;            stop's squeal): every tick the period moves by sw_step and the
;            volume drops by one.
;
; DESIGN "Sound": built with ORICUTRON (make ORICUTRON=1) every noise period is
; doubled, because Oricutron plays AY noise an octave high.  Effects use noise
; periods <= 15 so the doubled value still fits R6's 5 bits (never 32: the AY
; reads 0 as 1).  Tones are right in Oricutron and are never changed.
;
; Everything that touches the AY runs with IRQs off (ay_write shares the
; address latch with the key scan).

        .export         snd_init, snd_tick, _snd_engine, _snd_burst
        .export         _snd_jump, _snd_land, _snd_stop, _snd_crash, _snd_fee
        .export         _snd_clack, _snd_squawk
        .import         ay_write

MIXER    = %01101010            ; port A out; tone A, tone C, noise B on
ENG_HI   = 10                   ; engine volumes, alternated each tick
ENG_LO   = 7

        .segment        "BSS"
burst_vol:      .res 1          ; channel B volume, counts down each tick
eng_on:         .res 1          ; engine volume toggle: 0 = engine off
sw_vol:         .res 1          ; channel C sweep: volume (0 = silent)
sw_lo:          .res 1          ;   period
sw_hi:          .res 1
sw_step:        .res 1          ;   period change a tick (signed)
sw_new:         .res 1          ; sweep's starting volume (argument)

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
        sta     eng_on
        sta     sw_vol
        rts

; snd_tick: from the IRQ (IRQs off), every 10 ms.
snd_tick:
        lda     eng_on                  ; engine buzz
        beq     @burst
        eor     #ENG_HI ^ ENG_LO
        sta     eng_on
        tay
        lda     #8
        jsr     ay_write
@burst: lda     burst_vol               ; noise fade
        beq     @sweep
        dec     burst_vol
        ldy     burst_vol
        lda     #9
        jsr     ay_write
@sweep: lda     sw_vol                  ; tone sweep
        beq     @done
        dec     sw_vol
        lda     sw_step
        clc
        adc     sw_lo
        sta     sw_lo
        lda     sw_step
        and     #$80
        beq     @pos
        lda     #$FF
@pos:   adc     sw_hi
        sta     sw_hi
        ldy     sw_lo
        lda     #4
        jsr     ay_write
        ldy     sw_hi
        lda     #5
        jsr     ay_write
        ldy     sw_vol
        lda     #10
        jmp     ay_write
@done:  rts

; void __fastcall__ snd_engine(uint16_t period): engine tone at the given
; period (62,500 / f), or off for 0.
_snd_engine:
        php
        sei
        cmp     #0
        bne     @on
        cpx     #0
        bne     @on
        lda     #0
        sta     eng_on
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
        lda     eng_on
        bne     @run
        lda     #ENG_HI
        sta     eng_on
@run:   plp
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

; sweep: start the channel C sweep: period A (low) X (high), step Y, volume
; 13; sweep15: the same at volume 15, so it lasts 150 ms.
sweep15:
        pha
        lda     #15
        bne     sw_go
sweep:  pha
        lda     #13
sw_go:  sta     sw_new
        pla
sw_set: php
        sei
        sta     sw_lo
        stx     sw_hi
        sty     sw_step
        lda     sw_new
        sta     sw_vol
        plp
        rts

; The three effects must not sound alike (Bob, BUILD 7): take-off is a clean
; rising tone with no noise; landing a low noise thump under a falling tone;
; the stop a high squeal over a bright hiss.

; void snd_jump(void): take-off: a rising whoosh, tone only, 150 ms.
_snd_jump:
        lda     #<420
        ldx     #>420
        ldy     #<-24
        jmp     sweep15

; void snd_land(void): landing: a low thump.
_snd_land:
        lda     #12
        jsr     _snd_burst
        lda     #<500
        ldx     #>500
        ldy     #60
        jmp     sweep

; void snd_stop(void): the stop: brakes squeal over a hiss.
_snd_stop:
        lda     #1
        jsr     _snd_burst
        lda     #<42
        ldx     #>42
        ldy     #1
        jmp     sweep15

; void snd_crash(void): a crash: a long rough burst and a falling tone.
_snd_crash:
        lda     #15
        jsr     _snd_burst
        lda     #<180
        ldx     #>180
        ldy     #45
        jmp     sweep15

; void snd_fee(void): a track finished and paid: a short high blip.
_snd_fee:
        lda     #6
        sta     sw_new
        lda     #<70
        ldx     #>70
        ldy     #<-2
        jmp     sw_set

; F4, the film frame (DESIGN "Sound"):

; void snd_clack(void): the clapper arm snaps shut: a sharp, short clack.
_snd_clack:
        lda     #2
        jsr     _snd_burst
        lda     #8              ; quieter, so it dies in 80 ms
        sta     burst_vol
        rts

; void __fastcall__ snd_squawk(uint8_t note): "CUT!" through the megaphone,
; two notes (main.c plays 0 then 1): a nasal tone that bends a little.
_snd_squawk:
        cmp     #0
        bne     @two
        lda     #<150           ; ~415 Hz
        ldx     #>150
        ldy     #2
        jmp     sweep15
@two:   lda     #<200           ; ~310 Hz
        ldx     #>200
        ldy     #3
        jmp     sweep15
