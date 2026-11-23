; crt0.s - ROM-free startup for Oric-1 / Atmos 48K (docs/ARCHITECTURE.md D1, D9)
;
; Loaded by CLOAD"" at $0500 and entered at $0500 by the ROM's autorun.
; From here on the ROM is never called: IRQs off for good, own stack, BSS
; zeroed, then main().  DATA is not copied: the tape load puts it at its run
; address and the program is entered exactly once (never restarted in place).

        .export         __STARTUP__ : absolute = 1
        .export         _exit
        .import         _main, zerobss
        .import         __STACKSIZE__, __MAIN_START__, __MAIN_SIZE__
        .include        "zeropage.inc"

        .segment        "STARTUP"

        sei                     ; the ROM's IRQ handler is never used again
        cld
        ldx     #$FF
        txs                     ; 6502 stack: all of page 1
        lda     #$7F
        sta     $030E           ; VIA IER: disable every VIA interrupt source
        lda     #<(__MAIN_START__ + __MAIN_SIZE__ + __STACKSIZE__)
        sta     c_sp
        lda     #>(__MAIN_START__ + __MAIN_SIZE__ + __STACKSIZE__)
        sta     c_sp+1          ; cc65 C stack grows down from $9800
        jsr     zerobss
        jsr     _main
_exit:  jmp     _exit           ; main never returns; if it does, stop here
