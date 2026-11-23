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
;         more than the box's height (16 + the step's rise or fall), so this
;         frame shows the whole new picture.
;     Either way no frame shows half of each.  (Faster writes would let the
;     burst overtake a beam that is inside the box: that was BUILD 3's
;     occasional one-line shear.)
;
; Bytes that don't change are written back with their own value.  Cells 40-42
; of a line are cells 0-2 of the next: never XORed, only rewritten unchanged.
;
; Pictures: 3 byte columns of 16 lines, each column stored bottom line first
; (tools/mkart.py).  Buffers: index j = r_h - 1 - (line - r_top), so the top
; line has the highest index and the burst counts X down to 0.

        .export         _spr_region, _line_lo, _line_hi, _line_addr
        .exportzp       _r_top, _r_h, _r_c0
        .exportzp       _a_img, _a_row, _a_col, _b_img, _b_row, _b_col
        .include        "zeropage.inc"

PIC_H   = 16                    ; picture height (rider)

        .segment        "ZEROPAGE"
_r_top: .res 1                  ; box: first screen line
_r_h:   .res 1                  ;      lines (16..32)
_r_c0:  .res 1                  ;      first cell (2..39)
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

        .segment        "ALIGNED"       ; one page: abs,X never crosses a page
buf0:   .res 32
buf1:   .res 32
buf2:   .res 32
buf3:   .res 32
alo:    .res 32
ahi:    .res 32

        .segment        "BSS"
_line_lo:       .res 200        ; screen line addresses, set by screen_init
_line_hi:       .res 200

        .segment        "RODATA"
buflo:  .byte   <buf0, <buf1, <buf2, <buf3

        .segment        "CODE"

; uint8_t * __fastcall__ line_addr(uint8_t y): screen address of HIRES line y
_line_addr:
        tay
        lda     _line_lo,y
        ldx     _line_hi,y
        rts

; void spr_region(void): prepare the box from the zero-page arguments, then
; write it (cycles measured by the harness: PLAN F1 Results).
_spr_region:
; 1. read the box and note the line addresses
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
        lda     #0
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

; 3. the burst: 67 cycles a line, top line first
@burst: php
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

; xorpic: XOR the picture described at zero page X (img, row, col), and the
; picture at jb if jb's high byte is not 0, into the buffers.  Column c of the picture goes to buffer (col + c), rows from
; index r_h - PIC_H - row up, skipping columns past cell 39.
xorpic: lda     0,x
        sta     ip
        lda     1,x
        sta     ip+1
        lda     _r_h
        sec
        sbc     #PIC_H
        sec
        sbc     2,x             ; row
        sta     bp              ; buffer index of the picture's bottom line
        lda     3,x             ; col
        sta     k
        clc
        adc     _r_c0
        sta     ln              ; screen cell of this column
        ldx     #3              ; columns to do
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
        ldy     #PIC_H-1
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
        adc     #PIC_H
        sta     jb
        bcc     @next
        inc     jb+1
@next:  lda     ip
        clc
        adc     #PIC_H
        sta     ip
        bcc     @n2
        inc     ip+1
@n2:    inc     k
        inc     ln
        dex
        bne     @col
@done:  rts
