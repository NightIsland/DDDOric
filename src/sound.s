; sound.s - AY sound: engine, jump, landing and stop (docs/PLAN.md F2; DESIGN
; "Sound and music").
;
; Channel A: the engine.  Tone pitch set by snd_engine from the speed; the
;            10 ms tick steps its volume through the vehicle's pattern (F6g,
;            DESIGN "Vehicles": the motorbike's buzz alternates 10 and 7, 50
;            times a second; a horse clip-clops quicker with speed; the hover
;            bike hums with a vibrato...), set by snd_voice.  The skis swish on
;            noise B instead, when no effect has it.
; F6g stings: each film's clapper card plays a short tune on A and C
;            (snd_sting), notes from a semitone table, a falling envelope.
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
        .export         _snd_clack, _snd_squawk, _snd_voice, _snd_sting, _snd_speed
        .import         ay_write

MIXER    = %01101010            ; port A out; tone A, tone C, noise B on

        .segment        "BSS"
burst_vol:      .res 1          ; channel B volume, counts down each tick
eng_on:         .res 1          ; engine volume toggle: 0 = engine off
sw_vol:         .res 1          ; channel C sweep: volume (0 = silent)
sw_lo:          .res 1          ;   period
sw_hi:          .res 1
sw_step:        .res 1          ;   period change a tick (signed)
sw_new:         .res 1          ; sweep's starting volume (argument)
_snd_speed:     .res 1          ; F6g: the rider's speed 0-250, set by main each step
eng_ph:         .res 1          ; engine pattern phase: index = phase / 32
eng_lo:         .res 1          ; engine period (vibrato voices rewrite it each tick)
eng_hi:         .res 1
vib_ph:         .res 1          ; vibrato phase: offset index = phase / 8 mod 8
vib_lo:         .res 1          ;   the period's low byte with it
eng_vol:        .res 1          ; volume A last written by the voice ($FF: none)
v_rate:         .res 1          ; voice: pattern phase a tick, 0 = 12 + speed / 8
v_flags:        .res 1          ;   bit 0: vibrato, bit 1: noise (the skis)
st_on:          .res 1          ; sting: playing
st_pos:         .res 1          ;   next event's offset in sting_data
st_left:        .res 1          ;   ticks left of this event
st_vol:         .res 1          ;   envelope volume
st_mask:        .res 1          ;   bit 0: A sounds, bit 1: C sounds

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
        sta     st_on
        jmp     _snd_voice              ; the motorbike's voice to start with

; snd_tick: from the IRQ (IRQs off), every 10 ms.
snd_tick:
        lda     st_on                   ; a sting
        beq     @eng
        jsr     sting_tick
@eng:   lda     eng_on                  ; the engine's voice
        beq     @burst
        jsr     engine_tick
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

; engine_tick: the voice's volume pattern (and vibrato, or the skis' noise)
engine_tick:
        lda     v_rate                  ; the pattern's pace
        bne     et_r
        lda     _snd_speed              ; 0: quicker with speed
        lsr     a
        lsr     a
        lsr     a
        clc
        adc     #12
et_r:   clc
        adc     eng_ph
        sta     eng_ph
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        tax
v_pat = * + 1
        ldy     $FFFF,x                 ; (snd_voice sets the address)
        lda     v_flags
        and     #2
        bne     et_noise
        cpy     eng_vol                 ; (IRQ time: only a new volume)
        beq     et_vib
        sty     eng_vol
        lda     #8
        jsr     ay_write                ; R8 = volume A
et_vib: lda     v_flags
        lsr     a
        bcc     et_done                 ; no vibrato
        inc     vib_ph
        lda     vib_ph
        and     #7                      ; a new offset every 8 ticks only
        bne     et_done
        lda     vib_ph
        lsr     a
        lsr     a
        lsr     a
        and     #7
        tax
v_wob = * + 1
        lda     $FFFF,x                 ; a signed period offset
        tay
        clc
        adc     eng_lo
        sta     vib_lo
        tya
        and     #$80                    ; (carry kept for the high byte)
        beq     et_pos
        lda     #$FF
et_pos: adc     eng_hi
        tay
        lda     #1
        jsr     ay_write                ; R1
        ldy     vib_lo
        lda     #0
        jsr     ay_write                ; R0
et_done: rts
et_noise: lda     burst_vol               ; the skis: noise B, unless an effect
        bne     et_done                   ; has it
        lda     #9
        jsr     ay_write                ; R9 = the swish's volume
        lda     _snd_speed              ; faster, higher: period 15 .. 1
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        eor     #15
        bne     et_p
        lda     #1
et_p:
.ifdef ORICUTRON
        asl     a
.endif
        tay
        lda     #6
        jmp     ay_write

; void __fastcall__ snd_engine(uint16_t period): engine tone at the given
; period (62,500 / f), or off for 0.
_snd_engine:
        php
        sei
        sta     eng_lo
        stx     eng_hi
        cmp     #0
        bne     @on
        cpx     #0
        bne     @on
        lda     #$FF
        sta     eng_vol
        lda     #0
        sta     eng_on
        lda     #8              ; period 0: volume A = 0
        ldy     #0
        jsr     ay_write
        lda     v_flags         ; the skis' swish off too, if it has B
        and     #2
        beq     @off
        lda     burst_vol
        bne     @off
        lda     #9
        jsr     ay_write
@off:   plp
        rts
@on:    tay
        lda     #0
        jsr     ay_write        ; R0 = period low
        ldy     eng_hi
        lda     #1
        jsr     ay_write        ; R1 = period high
        lda     #1
        sta     eng_on
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

; ------------------------------------------------------------ F6g voices

; void __fastcall__ snd_voice(uint8_t vehicle): the vehicle's engine voice
; (tools/mkart.py VEHICLES order).  Its pitch is main.c's (eng_mul).
_snd_voice:
        php
        sei
        asl     a                       ; 6 bytes a voice
        sta     vib_lo
        asl     a
        adc     vib_lo
        tax
        lda     voices,x
        sta     v_pat
        lda     voices+1,x
        sta     v_pat+1
        lda     voices+2,x
        sta     v_rate
        lda     voices+3,x
        sta     v_wob
        lda     voices+4,x
        sta     v_wob+1
        lda     voices+5,x
        sta     v_flags
        lda     #$FF
        sta     eng_vol
        lda     #8                      ; whichever channel it left on: off
        ldy     #0
        jsr     ay_write
        lda     burst_vol
        bne     @b
        lda     #9
        jsr     ay_write
@b:     plp
        rts

; each voice: volume pattern, pattern pace (0: with speed), vibrato table,
; flags (1 vibrato, 2 noise)
voices:
        .addr   p_buzz                  ; bike: 10 and 7, 50 times a second
        .byte   32
        .addr   0
        .byte   0
        .addr   p_buzz                  ; jet-ski: the same
        .byte   32
        .addr   0
        .byte   0
        .addr   p_hoof                  ; horse: clip-clop, quicker with speed
        .byte   0
        .addr   0
        .byte   0
        .addr   p_cart                  ; mine cart: a low rumble, a click a sleeper
        .byte   0
        .addr   0
        .byte   0
        .addr   p_rattle                ; hearse-trike: an uneven 3-step rattle
        .byte   32
        .addr   0
        .byte   0
        .addr   p_rumble                ; ghost train: deep, with a slow wobble
        .byte   32
        .addr   w_wobble
        .byte   1
        .addr   p_hum                   ; hover bike: a smooth hum, vibrato
        .byte   32
        .addr   w_gentle
        .byte   1
        .addr   p_patter                ; dragon: pattering, a puff now and then
        .byte   0
        .addr   0
        .byte   0
        .addr   p_soft                  ; magic carpet: soft, rising and falling
        .byte   32
        .addr   w_sway
        .byte   1
        .addr   p_swish                 ; skis: a swish on noise
        .byte   32
        .addr   0
        .byte   2
        .addr   p_harsh                 ; snowmobile: a harsher two-stroke buzz
        .byte   32
        .addr   0
        .byte   0

p_buzz:   .byte 10,7,10,7,10,7,10,7
p_hoof:   .byte 11,4,0,9,3,0,0,0
p_cart:   .byte 12,7,7,7,7,7,7,7
p_rattle: .byte 11,6,8,11,6,8,11,10
p_rumble: .byte 9,8,9,8,9,8,9,8
p_hum:    .byte 9,9,9,9,9,9,9,9
p_patter: .byte 10,0,10,0,10,0,13,13
p_soft:   .byte 7,7,7,7,7,7,7,7
p_swish:  .byte 5,6,7,6,5,6,7,6
p_harsh:  .byte 12,5,12,5,12,5,12,5
w_gentle: .byte 0,1,2,1,0,<-1,<-2,<-1
w_wobble: .byte 0,6,12,6,0,<-6,<-12,<-6
w_sway:   .byte 0,8,16,8,0,<-8,<-16,<-8

; ------------------------------------------------------------ F6g stings

; void __fastcall__ snd_sting(uint8_t film): the film's sting (0-5), or 255
; to stop one.
_snd_sting:
        php
        sei
        cmp     #6
        bcs     @stop
        tax
        lda     sting_start,x
        sta     st_pos
        lda     #1
        sta     st_left
        sta     st_on
        plp
        rts
@stop:  jsr     sting_off
        plp
        rts

sting_off:
        lda     #0
        sta     st_on
        lda     #8
        ldy     #0
        jsr     ay_write
        lda     #10
        jmp     ay_write

; sting_tick: the next event when this one runs out; the envelope falls
sting_tick:
        dec     st_left
        bne     @env
        ldx     st_pos
        lda     sting_data,x            ; ticks; 0: the end
        beq     sting_off
        sta     st_left
        lda     #13
        sta     st_vol
        lda     #0
        sta     st_mask
        lda     sting_data+1,x          ; A's note
        jsr     @note
        bcs     @c
        lda     #0
        jsr     ay_write                ; R0, R1
        txa
        tay
        lda     #1
        jsr     ay_write
        inc     st_mask
@c:     ldx     st_pos
        lda     sting_data+2,x          ; C's note
        jsr     @note
        bcs     @next
        lda     #4
        jsr     ay_write                ; R4, R5
        txa
        tay
        lda     #5
        jsr     ay_write
        lda     st_mask
        ora     #2
        sta     st_mask
@next:  lda     st_pos
        clc
        adc     #3
        sta     st_pos
        jmp     @vol
@env:   lda     st_left                 ; a volume lower every 4 ticks, down to 6
        and     #3
        bne     @rts
        lda     st_vol
        cmp     #7
        bcc     @rts
        dec     st_vol
@vol:   ldy     #0
        lda     st_mask
        lsr     a
        bcc     @va
        ldy     st_vol
@va:    lda     #8
        jsr     ay_write
        ldy     #0
        lda     st_mask
        and     #2
        beq     @vc
        ldy     st_vol
@vc:    lda     #10
        jmp     ay_write
@rts:   rts
; @note: A = note (255: a rest, carry set) -> Y = period low, X = high
@note:  cmp     #255
        bcs     @rest
        asl     a
        tax
        ldy     notes,x
        lda     notes+1,x
        tax
        clc
@rest:  rts

; semitones from E3 (0) to A5 (29): periods 62,500 / f
notes:  .word   379,358,338,319,301,284,268,253,239,225,213,201,190,179,169
        .word   159,150,142,134,127,119,113,106,100,95,89,84,80,75,71

R = 255                                 ; a rest
sting_start:
        .byte   s_action-sting_data, s_western-sting_data, s_horror-sting_data
        .byte   s_scifi-sting_data, s_fantasy-sting_data, s_spy-sting_data
; events: ticks (10 ms), A's note, C's note; 0 ends
sting_data:
s_action:                               ; a punchy figure, rising to a held fifth
        .byte   10,12,19, 4,R,R, 10,12,19, 4,R,R, 14,15,22, 50,17,24, 0
s_western:                              ; a whistled four-note figure over a drone
        .byte   22,5,17, 22,5,22, 22,5,24, 60,5,29, 0
s_horror:                               ; falling organ chords
        .byte   36,17,20, 36,16,19, 36,15,18, 60,14,17, 0
s_scifi:                                ; a rising arpeggio
        .byte   10,5,5, 10,5,8, 10,5,12, 10,5,17, 10,5,20, 10,5,24, 50,5,29, 0
s_fantasy:                              ; a fanfare in fourths
        .byte   16,10,15, 8,10,15, 16,15,20, 60,17,22, 0
s_spy:                                  ; a twangy bass riff
        .byte   12,0,R, 12,3,R, 12,5,R, 12,3,R, 12,0,R, 12,R,R, 40,0,12, 0
