; esp-bench guest for scripts: a bridge between the bench and UART 0 at 115200.
; Every byte the ESP sends is written to port 0x2003 at once; the bench queues work in a
; ring at 0x8100 (64 entries of 4 bytes: type, a, b, 0; it writes 0x8000, we write 0x8001):
;   0 a      a byte to the ESP
;   1 a b    nextreg a := b
;   2 a      nextreg a read, its value written to port 0x2103
;   3        the run ends (port 0x0103)
;   4 a b    UART 0's prescaler := a + 256 * b (14 bits)

	OPT --zxnext

CON	equ #2003
NRV	equ #2103
FIN	equ #0103
RXD	equ #143B
TXD	equ #133B
SEL	equ #153B
WR	equ #8000
RD	equ #8001
RING	equ #8100

	org 0
	di
	jp start
	ds #38 - $
	ei
	ret

start:
	ld sp, #ff00
	nextreg 7, 3			; 28 MHz
	xor a
	ld (RD), a
	ld bc, #243B			; 115200 at the video timing NR 11 gives
	ld a, #11
	out (c), a
	ld bc, #253B
	in a, (c)
	and 7
	add a, a
	ld e, a
	ld d, 0
	ld hl, baud
	add hl, de
	ld e, (hl)
	inc hl
	ld d, (hl)
	call presc
	im 1
	ei

loop:	ld bc, TXD			; what the ESP sent comes first
	in a, (c)
	rrca
	jr nc, .work
	ld bc, RXD
	in a, (c)
	ld bc, CON
	out (c), a
	jr loop
.work	ld a, (RD)
	ld hl, WR
	cp (hl)
	jr z, loop
	ld l, a
	ld h, 0
	add hl, hl
	add hl, hl
	ld de, RING
	add hl, de
	ld a, (hl)
	inc hl
	ld d, (hl)
	inc hl
	ld e, (hl)
	or a
	jr z, .tx
	dec a
	jr z, .nrw
	dec a
	jr z, .nrr
	dec a
	jr z, .fin
	ld a, e				; 4: the prescaler
	ld e, d
	ld d, a
	call presc
	jr .next
.tx	ld bc, TXD
.txw	in a, (c)
	and 2
	jr nz, .txw
	out (c), d
	jr .next
.nrw	ld bc, #243B
	out (c), d
	ld bc, #253B
	out (c), e
	jr .next
.nrr	ld bc, #243B
	out (c), d
	ld bc, #253B
	in a, (c)
	ld bc, NRV
	out (c), a
	jr .next
.fin	ld bc, FIN
	out (c), a
	jr $
.next	ld a, (RD)
	inc a
	and 63
	ld (RD), a
	jr loop

; DE = UART 0's prescaler
presc:	ld bc, SEL
	ld a, %00100000			; UART 0, the ESP
	out (c), a
	ld bc, RXD
	ld a, e
	and #7f
	out (c), a
	ld a, e
	rla
	ld a, d
	rla
	or #80
	out (c), a
	ret

baud	dw 243, 248, 256, 260, 269, 278, 286, 234
