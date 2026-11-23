; draw.s - move a sprite in one pass, then write it to the screen at the video
; beam's own pace (docs/PLAN.md F1; DESIGN "Screen structure"; Bob's rule from
; Chuckie Egg: start drawing as soon as the beam has left the sprite's area).
;
; A stock Oric has no vertical sync, so the program can't see where the beam is.
; BUILD 1-3 learned: erase-then-draw lets the beam see an empty rider (fading);
; writes spread over a long time let it see half a move (tearing).  So:
;
;  1. PREPARE (spr_region, any speed).  For a box of r_h screen lines from line
;     r_top and 4 cells from cell r_c0: read the screen bytes into buf0-3, XOR in
;     up to two pictures (the old one to take it off, the new one to put it on),
;     clipping columns past cell 39, and note each line's address.
;  2. WRITE (the burst).  Store the finished bytes line by line, top to bottom,
;     at 67 cycles a line, with IRQs held off.  The beam takes 64 cycles a line,
;     so the burst keeps pace with it:
;       - beam inside or below the box when the burst starts: it stays ahead of
;         the writes all the way down, and this frame shows the whole old
;         picture; the next frame shows the whole new one;
;       - beam above the box: the writes stay ahead of it for 64 / 3 = 21 lines,
;         more than the box's height (r_ph + the step's rise or fall), so this
;         frame shows the whole new picture.
;     Either way no frame shows half of each.  (Faster writes would let the
;     burst overtake a beam that is inside the box: that was BUILD 3's
;     occasional one-line shear.)
;
; Bytes that don't change are written back with their own value.  Cells 40-42
; of a line are cells 0-2 of the next: never XORed, only rewritten unchanged.
;
; Pictures: r_w (3 or 5) byte columns of r_ph lines, each column stored bottom line first
; (tools/mkart.py).  Buffers: index j = r_h - 1 - (line - r_top), so the top
; line has the highest index and the burst counts X down to 0.

        .export         _spr_region, _line_lo, _line_hi, _line_addr, _spr_move, _spr_erase
        .export         _rope_or, _rope_and, _bulbs, _bulb_lo, _bulb_hi
        .exportzp       _rp_b, _rp_y, _rp_c, _r_fast
        .export         _sp_on, _sp_y, _sp_c, _sp_w, _sp_h, _sp_lo, _sp_hi
        .exportzp       _n_y, _n_c, _n_img
        .exportzp       _r_top, _r_h, _r_c0, _r_w, _r_ph
        .exportzp       _a_img, _a_row, _a_col, _b_img, _b_row, _b_col
        .include        "zeropage.inc"

        .segment        "ZEROPAGE"
_r_top: .res 1                  ; box: first screen line
_r_h:   .res 1                  ;      lines (16..32)
_r_c0:  .res 1                  ;      first cell (2..39)
_r_w:   .res 1                  ; picture width in byte columns: 3 (box 4) or 5 (box 6)
_r_ph:  .res 1                  ; picture height in lines (16; the police car 12)
_a_img: .res 2                  ; picture A (0 = none): 48 bytes
_a_row: .res 1                  ;   its top line - r_top
_a_col: .res 1                  ;   its cell - r_c0 (0 or 1)
_b_img: .res 2                  ; picture B, the same
_b_row: .res 1
_b_col: .res 1
p:      .res 2
bp:     .res 2
ip:     .res 2
jb:     .res 2                  ; xorpic: second picture at the same place, or hi 0
ln:     .res 1
k:      .res 1
_n_y:   .res 1                  ; spr_move: the new top line,
_n_c:   .res 1                  ;   cell,
_n_img: .res 2                  ;   picture
sn:     .res 1                  ;   and the sprite's number
_r_fast: .res 1                 ; spr_region: 1 = the box holds only the old picture on a
                                ; blank background (Bob, BUILD 11): no reading, no XOR out
_rp_b:  .res 1                  ; rope_or / rope_and: the pixel's bit (and: its inverse),
_rp_y:  .res 1                  ;   first line,
_rp_c:  .res 1                  ;   cell

        .segment        "ALIGNED"       ; one page: abs,X never crosses a page
buf0:   .res 32
buf1:   .res 32
buf2:   .res 32
buf3:   .res 32
buf4:   .res 32
buf5:   .res 32
alo:    .res 32
ahi:    .res 32

        .segment        "BSS"
_line_lo:       .res 200        ; screen line addresses, set by screen_init
_line_hi:       .res 200
NSPR    = 16                    ; sprites: 0 the rider, 1-7 the moving hazards,
                                ; 9-15 their old pictures still to take off
_sp_on: .res NSPR               ; drawn yet?
_sp_y:  .res NSPR               ; where it is now: top line,
_sp_c:  .res NSPR               ;   cell,
_sp_lo: .res NSPR               ;   picture
_sp_hi: .res NSPR
_sp_w:  .res NSPR               ; picture columns (set once)
_sp_h:  .res NSPR               ; picture lines (set once)

        .segment        "RODATA"
buflo:  .byte   <buf0, <buf1, <buf2, <buf3, <buf4, <buf5

        .segment        "CODE"

; uint8_t * __fastcall__ line_addr(uint8_t y): screen address of HIRES line y
_line_addr:
        tay
        lda     _line_lo,y
        ldx     _line_hi,y
        rts

; void __fastcall__ spr_move(uint8_t n): move sprite n to n_y, n_c, n_img.
; Close to where it was (most steps): one box holds both, old ^ new in one
; burst.  Apart (the first draw, a wrap, a retake): the new picture first,
; then the old one taken off, so it is never missing from a frame.  (In asm:
; cc65's version of this spent ~2,000 cycles a call on stack handling.)
_spr_move:
        tax
        stx     sn
        lda     _sp_w,x
        sta     _r_w
        lda     _sp_h,x
        sta     _r_ph
        lda     _sp_on,x
        bne     @was
        jmp     @apart
@was:   lda     _n_y
        cmp     _sp_y,x
        bne     @chg
        lda     _n_c
        cmp     _sp_c,x
        bne     @chg
        lda     _n_img
        cmp     _sp_lo,x
        bne     @chg
        lda     _n_img+1
        cmp     _sp_hi,x
        bne     @chg
        rts                     ; not moved
@chg:   lda     _n_y            ; within 15 lines and 1 cell?
        sec
        sbc     _sp_y,x
        clc
        adc     #15
        cmp     #31
        bcs     @apart
        lda     _n_c
        sec
        sbc     _sp_c,x
        clc
        adc     #1
        cmp     #3
        bcs     @apart
        lda     _n_y            ; joint box: the higher top, the left cell
        cmp     _sp_y,x
        bcc     @t
        lda     _sp_y,x
@t:     sta     _r_top
        lda     _sp_y,x
        sec
        sbc     _r_top
        sta     _a_row
        lda     _n_y
        sec
        sbc     _r_top
        sta     _b_row
        clc
        adc     _a_row          ; one of the rows is 0: the rise or fall
        adc     _r_ph
        sta     _r_h
        lda     _n_c
        cmp     _sp_c,x
        bcc     @c
        lda     _sp_c,x
@c:     sta     _r_c0
        lda     _sp_c,x
        sec
        sbc     _r_c0
        sta     _a_col
        lda     _n_c
        sec
        sbc     _r_c0
        sta     _b_col
        lda     _sp_lo,x
        sta     _a_img
        lda     _sp_hi,x
        sta     _a_img+1
        lda     _n_img
        sta     _b_img
        lda     _n_img+1
        sta     _b_img+1
        jsr     _spr_region
        jmp     @keep
@apart: lda     _n_y
        sta     _r_top
        lda     _r_ph
        sta     _r_h
        lda     _n_c
        sta     _r_c0
        lda     _n_img
        sta     _a_img
        lda     _n_img+1
        sta     _a_img+1
        lda     #0
        sta     _a_row
        sta     _a_col
        sta     _b_img+1
        jsr     _spr_region
        ldx     sn
        lda     _sp_on,x
        beq     @keep
        lda     _sp_y,x
        sta     _r_top
        lda     _sp_c,x
        sta     _r_c0
        lda     _sp_lo,x
        sta     _a_img
        lda     _sp_hi,x
        sta     _a_img+1
        jsr     _spr_region
@keep:  ldx     sn
        lda     #1
        sta     _sp_on,x
        lda     _n_y
        sta     _sp_y,x
        lda     _n_c
        sta     _sp_c,x
        lda     _n_img
        sta     _sp_lo,x
        lda     _n_img+1
        sta     _sp_hi,x
        rts

; void __fastcall__ rope_or(uint8_t y1), rope_and(uint8_t y1): a dangler's
; rope, one pixel a line in cell rp_c, from line rp_y to y1 - 1: set (rp_b) or
; cleared (rp_b = the bit's inverse).  Single pixels: no burst needed.
_rope_or:
        ldy     #$11            ; ora (p),y
        bne     rope
_rope_and:
        ldy     #$31            ; and (p),y
rope:   sty     @op
        sta     k
        ldx     _rp_y
@l:     cpx     k
        bcs     @d
        lda     _line_lo,x
        clc
        adc     _rp_c
        sta     p
        lda     _line_hi,x
        adc     #0
        sta     p+1
        ldy     #0
        lda     _rp_b
@op:    ora     (p),y           ; (or and: set above)
        sta     (p),y
        inx
        bne     @l
@d:     rts

; void __fastcall__ spr_erase(uint8_t n): take sprite n's picture off.
_spr_erase:
        tax
        lda     _sp_on,x
        beq     @no
        lda     #0
        sta     _sp_on,x
        sta     _a_row
        sta     _a_col
        sta     _b_img+1
        lda     _sp_w,x
        sta     _r_w
        lda     _sp_h,x
        sta     _r_ph
        sta     _r_h
        lda     _sp_y,x
        sta     _r_top
        lda     _sp_c,x
        sta     _r_c0
        lda     _sp_lo,x
        sta     _a_img
        lda     _sp_hi,x
        sta     _a_img+1
        jmp     _spr_region
@no:    rts

; void spr_region(void): prepare the box from the zero-page arguments, then
; write it (cycles measured by the harness: PLAN F1 Results).
_spr_region:
; 1. read the box and note the line addresses (fast: blank bytes, and the old
;    picture needn't be taken out)
        ldy     _r_top
        lda     _line_lo,y
        clc
        adc     _r_c0
        sta     p
        lda     _line_hi,y
        adc     #0
        sta     p+1
        ldx     _r_h
        dex
        lda     _r_fast
        beq     @rd
@fill:  lda     p               ; 4-cell box only
        sta     alo,x
        lda     p+1
        sta     ahi,x
        lda     #$40
        sta     buf0,x
        sta     buf1,x
        sta     buf2,x
        sta     buf3,x
        lda     p
        clc
        adc     #40
        sta     p
        bcc     @fn
        inc     p+1
@fn:    dex
        bpl     @fill
        lda     #0
        sta     _a_img+1
        beq     @xor
@rd:    lda     p
        sta     alo,x
        lda     p+1
        sta     ahi,x
        ldy     #0
        lda     (p),y
        sta     buf0,x
        iny
        lda     (p),y
        sta     buf1,x
        iny
        lda     (p),y
        sta     buf2,x
        iny
        lda     (p),y
        sta     buf3,x
        iny
        lda     (p),y
        sta     buf4,x
        iny
        lda     (p),y
        sta     buf5,x
        lda     p
        clc
        adc     #40
        sta     p
        bcc     @nc
        inc     p+1
@nc:    dex
        bpl     @rd

; 2. XOR in pictures A and B.  Same place (the rider on the road, within a
;    cell: most steps): both in one pass.
@xor:   lda     #0
        sta     jb+1
        lda     _a_img+1
        beq     @onlyb
        ldx     _b_img+1
        beq     @onlya
        lda     _a_row
        cmp     _b_row
        bne     @two
        lda     _a_col
        cmp     _b_col
        bne     @two
        lda     _b_img
        sta     jb
        stx     jb+1
@onlya: ldx     #_a_img
        jsr     xorpic
        jmp     @burst
@two:   ldx     #_a_img
        jsr     xorpic
        lda     #0
        sta     jb+1
@onlyb: lda     _b_img+1
        beq     @burst
        ldx     #_b_img
        jsr     xorpic

; 3. the burst, top line first: box of 4 at 67 cycles a line, box of 6 at 91
@burst: lda     _r_w
        cmp     #3
        beq     @b4
        jmp     burst6
@b4:    php
        sei
        ldx     _r_h
        dex
@w:     lda     alo,x           ; 4
        sta     p               ; 3
        lda     ahi,x           ; 4
        sta     p+1             ; 3
        ldy     #0              ; 2
        lda     buf0,x          ; 4
        sta     (p),y           ; 6
        iny                     ; 2
        lda     buf1,x          ; 4
        sta     (p),y           ; 6
        iny                     ; 2
        lda     buf2,x          ; 4
        sta     (p),y           ; 6
        iny                     ; 2
        lda     buf3,x          ; 4
        sta     (p),y           ; 6
        dex                     ; 2
        bpl     @w              ; 3
        plp
        rts

; burst6: the 6-cell box (5-column pictures: the police car).  A loop through
; buffers can't write 6 bytes a line at the beam's pace (it took 91 cycles a
; line, and the beam caught the last lines: 1.5% torn frames), so the bytes go
; into code: tpl6 is 16 lines of "lda #byte / sta line + k,y" x 6 (Y = the
; box's first cell), padded to 67 cycles a line like the 4-cell burst.  fill6
; writes this step's bytes into it, and the line addresses only when the box's
; lines have changed (never, for a car on the road).  Box line j (buffer
; index) runs as template line 15 - j, so a box of r_h lines enters at
; template line 16 - r_h.  Taller boxes (none yet) use the loop.
TPL_LINE = 6 * 5 + 11 + 2               ; bytes of code a template line

burst6: lda     _r_h
        cmp     #17
        bcc     fill6
        php                     ; > 16 lines: the loop, 91 cycles a line
        sei
        ldx     _r_h
        dex
@w:     lda     alo,x
        sta     p
        lda     ahi,x
        sta     p+1
        ldy     #0
        lda     buf0,x
        sta     (p),y
        iny
        lda     buf1,x
        sta     (p),y
        iny
        lda     buf2,x
        sta     (p),y
        iny
        lda     buf3,x
        sta     (p),y
        iny
        lda     buf4,x
        sta     (p),y
        iny
        lda     buf5,x
        sta     (p),y
        dex
        bpl     @w
        plp
        rts

fill6:  lda     _r_top          ; the same lines as last time: same addresses
        cmp     t6_top
        bne     @new
        lda     _r_h
        cmp     t6_h
        beq     @vals
@new:   lda     _r_top
        sta     t6_top
        lda     _r_h
        sta     t6_h
        clc
        adc     _r_top
        sta     k               ; screen line + 1 of box line 0 (the bottom)
        lda     #<(tpl6 + 15 * TPL_LINE)
        sta     ip
        lda     #>(tpl6 + 15 * TPL_LINE)
        sta     ip+1
        ldx     #0
@al:    dec     k
        ldy     k
        lda     _line_lo,y
        sta     p
        lda     _line_hi,y
        sta     p+1
        ldy     #3              ; each sta's address: the line's, + 0 .. 5
@ac:    lda     p
        clc
        adc     colofs-3,y
        sta     (ip),y
        iny
        lda     p+1
        adc     #0
        sta     (ip),y
        tya
        clc
        adc     #4
        tay
        cpy     #3 + 6 * 5
        bne     @ac
        lda     ip
        sec
        sbc     #TPL_LINE
        sta     ip
        bcs     @an
        dec     ip+1
@an:    inx
        cpx     _r_h
        bne     @al
@vals:  lda     #<(tpl6 + 15 * TPL_LINE)
        sta     ip
        lda     #>(tpl6 + 15 * TPL_LINE)
        sta     ip+1
        ldx     #0
@vl:    lda     buf0,x
        ldy     #1
        sta     (ip),y
        lda     buf1,x
        ldy     #6
        sta     (ip),y
        lda     buf2,x
        ldy     #11
        sta     (ip),y
        lda     buf3,x
        ldy     #16
        sta     (ip),y
        lda     buf4,x
        ldy     #21
        sta     (ip),y
        lda     buf5,x
        ldy     #26
        sta     (ip),y
        lda     ip
        sec
        sbc     #TPL_LINE
        sta     ip
        bcs     @vn
        dec     ip+1
@vn:    inx
        cpx     _r_h
        bne     @vl
        lda     ip              ; ip + TPL_LINE: the top line's code
        clc
        adc     #TPL_LINE
        sta     ip
        bcc     @go
        inc     ip+1
@go:    ldy     _r_c0
        php
        sei
        jsr     @jmp
        plp
        rts
@jmp:   jmp     (ip)

        .segment        "RODATA"
; colofs-3,y for y = 3, 8, 13, ...: the cell offsets 0..5 of each sta
colofs: .byte   0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 3, 0, 0, 0, 0, 4, 0, 0, 0, 0, 5

        .segment        "BSS"
t6_top: .res    1               ; the lines whose addresses tpl6 holds (h 0: none)
t6_h:   .res    1

        .segment        "CODE"
tpl6:
        .repeat 16
          .repeat 6
            lda     #0          ; 2
            sta     $A000,y     ; 5
          .endrep
          .repeat 11
            nop                 ; 22
          .endrep
            bit     ln          ; 3: 67 cycles a line
        .endrep
        rts

; xorpic: XOR the picture described at zero page X (img, row, col), and the
; picture at jb if jb's high byte is not 0, into the buffers.  Column c of the picture goes to buffer (col + c), rows from
; index r_h - r_ph - row up, skipping columns past cell 39.
xorpic: lda     0,x
        sta     ip
        lda     1,x
        sta     ip+1
        lda     _r_h
        sec
        sbc     _r_ph
        sec
        sbc     2,x             ; row
        sta     bp              ; buffer index of the picture's bottom line
        lda     3,x             ; col
        sta     k
        clc
        adc     _r_c0
        sta     ln              ; screen cell of this column
        ldx     _r_w            ; columns to do
@col:   lda     ln
        cmp     #40
        bcs     @done           ; clipped at the right edge
        ldy     k
        lda     buflo,y
        clc
        adc     bp
        sta     p
        lda     #>buf0
        sta     p+1
        ldy     _r_ph
        dey
        lda     jb+1
        bne     @xx
@x:     lda     (ip),y          ; one picture: 21 cycles a byte
        eor     (p),y
        sta     (p),y
        dey
        bpl     @x
        bmi     @next
@xx:    lda     (ip),y          ; two at the same place: 26 cycles a byte
        eor     (jb),y
        eor     (p),y
        sta     (p),y
        dey
        bpl     @xx
        lda     jb
        clc
        adc     _r_ph
        sta     jb
        bcc     @next
        inc     jb+1
@next:  lda     ip
        clc
        adc     _r_ph
        sta     ip
        bcc     @n2
        inc     ip+1
@n2:    inc     k
        inc     ln
        dex
        bne     @col
@done:  rts

; ---------------------------------------------------------------- F4
; The title marquee's chasing lights (src/frame.c): bulb n's middle line is
; at bulb_lo/hi[n] + 40; its top and bottom lines are the same lit or not.
; void __fastcall__ bulbs(uint8_t phase): every third bulb lit, from `phase`
; (0-2).  ~1,700 cycles (cc65's loop took 15,800).
NBULB = 48
        .segment        "BSS"
_bulb_lo:       .res NBULB
_bulb_hi:       .res NBULB
bk:             .res 1

        .segment        "RODATA"
bulb_mid:       .byte   $5E, $52, $52           ; lit, unlit, unlit

        .segment        "CODE"
_bulbs: sta     bk
        ldx     #0
@b:     lda     _bulb_lo,x
        sta     ptr1
        lda     _bulb_hi,x
        sta     ptr1+1
        ldy     bk
        lda     bulb_mid,y
        ldy     #40
        sta     (ptr1),y
        ldy     bk
        iny
        cpy     #3
        bcc     @k
        ldy     #0
@k:     sty     bk
        inx
        cpx     #NBULB
        bcc     @b
        rts
