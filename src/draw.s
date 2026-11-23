; draw.s - rider_apply: write the rider's prepared masks to the screen in one
; quick burst (src/screen.c rider_move, BUILD 3).  For each entry i < rm_n:
; the three bytes at rm_hi:rm_lo[i] are XORed with rm_a0/a1/a2[i].  About 70
; cycles an entry.  The third byte may be the next line's cell 0 with a mask
; of 0: rewritten unchanged.

        .export         _rider_apply
        .import         _rm_n, _rm_lo, _rm_hi, _rm_a0, _rm_a1, _rm_a2
        .include        "zeropage.inc"

        .segment        "CODE"

_rider_apply:
        ldx     _rm_n
        beq     @done
        dex
@line:  lda     _rm_lo,x
        sta     ptr1
        lda     _rm_hi,x
        sta     ptr1+1
        ldy     #0
        lda     (ptr1),y
        eor     _rm_a0,x
        sta     (ptr1),y
        iny
        lda     (ptr1),y
        eor     _rm_a1,x
        sta     (ptr1),y
        iny
        lda     (ptr1),y
        eor     _rm_a2,x
        sta     (ptr1),y
        dex
        bpl     @line
@done:  rts
