; text.s - text and fills for the film frame (src/frame.c; docs/PLAN.md F4).
; cc65's C took ~200 cycles a screen byte here; these take 15-50.
;
; Letters come from our character set at $9800 (src/screen.c puts each glyph
; at its ASCII code x 8, rows 0-6, bits 4-0), so no table lookup is needed.
;
; void __fastcall__ txt(const char *s):  small text, a letter a cell, at
;     cell tx_col, lines tx_y .. tx_y + 6.
; void __fastcall__ txt2(const char *s): big text, two cells a letter, each
;     pixel doubled across and down: lines tx_y .. tx_y + 13.
; void __fastcall__ fill_alt(uint8_t n): n bytes from fa_p, alternately
;     fa_a and fa_b (the clapper's stripes).
; dbl_a / dbl_b (filled by frame_init): a 5-bit glyph row doubled to 10
;     pixels in 12, as two screen bytes.

        .export         _txt, _txt2, _fill_alt, _dbl_a, _dbl_b
        .exportzp       _tx_col, _tx_y, _fa_p, _fa_a, _fa_b
        .import         _line_lo, _line_hi
        .include        "zeropage.inc"

CHARSET = $9800

        .segment        "ZEROPAGE"
_tx_col:        .res 1
_tx_y:          .res 1
_fa_p:          .res 2
_fa_a:          .res 1
_fa_b:          .res 1
tx_r:           .res 1
tx_i:           .res 1
tx_o:           .res 1

        .segment        "BSS"
_dbl_a:         .res 64
_dbl_b:         .res 64

        .segment        "CODE"

; ptr3 = the glyph row tx_r of the letter in A (A < 128)
.macro  glyph_row
        tax
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        clc
        adc     #>CHARSET
        sta     ptr3+1
        txa
        asl     a
        asl     a
        asl     a
        ora     tx_r
        sta     ptr3
.endmacro

_txt:   sta     ptr2
        stx     ptr2+1
        lda     #0
        sta     tx_r
@row:   lda     _tx_y
        clc
        adc     tx_r
        tax
        lda     _line_lo,x
        clc
        adc     _tx_col
        sta     ptr1
        lda     _line_hi,x
        adc     #0
        sta     ptr1+1
        ldy     #0
@ch:    lda     (ptr2),y
        beq     @next
        sty     tmp1
        glyph_row
        ldy     #0
        lda     (ptr3),y
        ora     #$40
        ldy     tmp1
        sta     (ptr1),y
        iny
        bne     @ch
@next:  inc     tx_r
        lda     tx_r
        cmp     #7
        bcc     @row
        rts

_txt2:  sta     ptr2
        stx     ptr2+1
        lda     #0
        sta     tx_r
@row:   lda     tx_r
        asl     a
        clc
        adc     _tx_y
        tax
        lda     _line_lo,x
        clc
        adc     _tx_col
        sta     ptr1
        lda     _line_hi,x
        adc     #0
        sta     ptr1+1
        lda     ptr1            ; the line below: 40 bytes on
        clc
        adc     #40
        sta     ptr4
        lda     ptr1+1
        adc     #0
        sta     ptr4+1
        lda     #0
        sta     tx_i
        sta     tx_o
@ch:    ldy     tx_i
        lda     (ptr2),y
        beq     @next
        glyph_row
        ldy     #0
        lda     (ptr3),y
        tax
        ldy     tx_o
        lda     _dbl_a,x
        sta     (ptr1),y
        sta     (ptr4),y
        iny
        lda     _dbl_b,x
        sta     (ptr1),y
        sta     (ptr4),y
        iny
        sty     tx_o
        inc     tx_i
        bne     @ch
@next:  inc     tx_r
        lda     tx_r
        cmp     #7
        bcc     @row
        rts

_fill_alt:
        tax
        beq     @done
        ldy     #0
@f:     lda     _fa_a
        sta     (_fa_p),y
        iny
        dex
        beq     @done
        lda     _fa_b
        sta     (_fa_p),y
        iny
        dex
        bne     @f
@done:  rts
