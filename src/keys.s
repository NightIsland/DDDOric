; keys.s - the game's key scan (docs/PLAN.md F0; DESIGN "Controls";
; docs/ORIC-LESSONS.md section 3 "Keyboard").  Adapted from Oric Chuckie Egg's
; src/keys.s.
;
; uint8_t keys_poll(void) reads only the keys the game uses and returns input
; bits (src/model.h):
;     IN_JUMP  Space      IN_ACCEL  left Shift or right Shift
;     IN_STOP  Return     IN_PAUSE  P          IN_QUIT  Esc
; The table (row on VIA PB0-2, column bit on AY R14, input bit) is in DATA so
; key redefinition (F6) can rewrite it.  Each key is read with IRQs held off
; between latching R14 and writing it (the sound handler shares the AY address
; latch); PB3 is read >= 3 cycles after the write.

        .export         _keys_poll, _key_row, _key_col, _key_bit

VIA_ORB  = $0300
VIA_ORA  = $0301
VIA_PCR  = $030C

NKEYS    = 6

        .segment        "DATA"
;                       Space LShift RShift Return  P    Esc
_key_row:       .byte   4,    4,     7,     7,      5,   1
_key_col:       .byte   $01,  $10,   $10,   $20,    $08, $20
_key_bit:       .byte   $01,  $02,   $02,   $04,    $08, $10

        .segment        "BSS"
in_bits:        .res 1

        .segment        "CODE"

; uint8_t keys_poll(void)
_keys_poll:
        lda     #0
        sta     in_bits
        ldx     #NKEYS-1
@k:     php
        sei
        lda     _key_row,x
        sta     VIA_ORB         ; row
        lda     #14
        sta     VIA_ORA
        lda     #$FF            ; latch R14
        sta     VIA_PCR
        lda     #$DD
        sta     VIA_PCR
        lda     _key_col,x
        eor     #$FF            ; column selected by a 0 bit
        sta     VIA_ORA
        lda     #$FD            ; write R14
        sta     VIA_PCR
        lda     #$DD
        sta     VIA_PCR
        plp
        lda     VIA_ORB         ; PB3 = key sense
        and     #$08
        beq     @n
        lda     _key_bit,x
        ora     in_bits
        sta     in_bits
@n:     dex
        bpl     @k
        lda     in_bits
        ldx     #0
        rts
