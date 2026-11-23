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

        .export         _keys_poll, _key_row, _key_col, _key_bit, _keys_ascii

VIA_ORB  = $0300
VIA_ORA  = $0301
VIA_PCR  = $030C

NKEYS    = 6

        .segment        "DATA"
;                       Space LShift RShift Return  P    Esc
_key_row:       .byte   4,    4,     7,     7,      5,   1
_key_col:       .byte   $01,  $10,   $10,   $20,    $08, $20
_key_bit:       .byte   $01,  $02,   $02,   $04,    $08, $10

        .segment        "RODATA"
; keys_ascii (F4): the code of each key, row by row, column bits $80 .. $01
; (tools/keyscript.py MATRIX; tools/ref/ref.c has the same table); 0 = none
codes:  .byte   "3X1", 0, "V5N7"                ; row 0
        .byte   "DQ", 27, 0, "FRTJ"             ; row 1: Esc
        .byte   "C2Z", 0, "4B6M"                ; row 2: Ctrl
        .byte   $27, $5C, 0, 0, "-;9K"          ; row 3
        .byte   0, 0, 0, 0, 0, ".,", " "        ; row 4: arrows, left Shift
        .byte   "[]", 127, 0, "POIU"            ; row 5: Del, Funct
        .byte   "WSA", 0, "EGHY"                ; row 6
        .byte   "=", 0, 13, 0, "/0L8"           ; row 7: Return, right Shift

        .segment        "BSS"
in_bits:        .res 1
sc_row:         .res 1
sc_col:         .res 1

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

; Select row sc_row and the columns whose bits are 0 in A, then -> Z clear if
; one of those keys is down (PB3 high).
sense:  php
        sei
        pha
        lda     sc_row
        sta     VIA_ORB
        lda     #14
        sta     VIA_ORA
        lda     #$FF            ; latch R14
        sta     VIA_PCR
        lda     #$DD
        sta     VIA_PCR
        pla
        sta     VIA_ORA
        lda     #$FD            ; write R14
        sta     VIA_PCR
        lda     #$DD
        sta     VIA_PCR
        plp
        nop
        lda     VIA_ORB
        and     #$08
        rts

; uint8_t keys_ascii(void) - F4: the menus, initials and Y/N.  The code of the
; first key held (rows 0-7, column bits $80 down) that has one, else 0.  A
; row with no key down is passed over after one read.  ~1,000 cycles.
_keys_ascii:
        lda     #0
        sta     sc_row
@row:   lda     #0              ; every column of the row
        jsr     sense
        beq     @next
        lda     #$80
        sta     sc_col
        lda     sc_row
        asl     a
        asl     a
        asl     a
        tax                     ; X = row x 8: codes index
@col:   lda     sc_col
        eor     #$FF
        jsr     sense
        beq     @nc
        lda     codes,x
        bne     @got
@nc:    inx
        lsr     sc_col
        bcc     @col
@next:  inc     sc_row
        lda     sc_row
        cmp     #8
        bcc     @row
        lda     #0
@got:   ldx     #0
        rts
