; t07: the SPI port's timing, against a card (SDCARD=x.img). The port runs on an
; oscillator of its own and takes 720 ns a byte; an access while a byte is going
; is not taken (an OUT is lost, an IN reads the old byte and starts nothing), and
; an IN sends #FF. At 3.5 MHz every pair below is far enough apart; at 28 MHz
; (MHZ=28: 11..18 T = 393..643 ns) none is.
;
; Sector 1 is written with the data bytes in pairs at a given distance and read
; back safely; sector 0 (written safely) is read with IN pairs at a distance. Each
; reports how many of the 512 bytes came out right, and the first four.
;   OUT pairs: 11 12 15 16 17 18   -> MBOX+1.. (count lo, hi, 4 bytes) x 6
;   IN pairs:  11 15 16 17         -> MBOX+#25..
;   after OUT 00 : IN, and after OUT 01 : IN, the first answer -> MBOX+#3D, +#3E
;   (FF: an IN sends FF, which is no command)
	include "common.inc"
	org #8000

SPI	equ #eb
BUF	equ #a000		; 512 bytes read back
PAT	equ #a200		; 512 bytes of pattern: i & #FF

	ld hl, PAT
	ld b, 0
.p1	ld (hl), b		; two rounds of 0..255
	inc hl
	inc b
	jr nz, .p1
.p2	ld (hl), b
	inc hl
	inc b
	jr nz, .p2

	call card_init
	ld (MBOX + #3f), a	; 00: the card is ready

	ld a, 0			; sector 0, safely
	ld hl, wr_safe
	call write_sec

	ld ix, MBOX + 1
	ld hl, wr_11
	call out_case
	ld hl, wr_12
	call out_case
	ld hl, wr_15
	call out_case
	ld hl, wr_16
	call out_case
	ld hl, wr_17
	call out_case
	ld hl, wr_18
	call out_case

	ld ix, MBOX + #25
	ld hl, rd_11
	call in_case
	ld hl, rd_15
	call in_case
	ld hl, rd_16
	call in_case
	ld hl, rd_17
	call in_case

	ld a, #00		; an even byte left in the shifter: the IN sends 7F,
	call spi_out		; which the card takes for a command (CMD63) - illegal
	call spi_in
	call skip_ff
	ld (MBOX + #3d), a	; 04
	ld b, 16
.drain	call spi_in
	djnz .drain
	ld a, #01		; an odd one: the IN sends FF, nothing happens
	call spi_out
	call spi_in
	call skip_ff
	ld (MBOX + #3e), a	; FF
	ld a, #ff
	out (#e7), a
	FINISH

; write sector 1 through the pair writer at hl, read it back safely, count
out_case:
	ld a, 1
	call write_sec
	ld a, 1
	ld hl, rd_safe
	call read_sec
	jr report

; read sector 0 through the pair reader at hl, count; then clock out what the
; card still has of the block (a reader that started too few transfers)
in_case:
	ld a, 0
	call read_sec
	ld de, 600
.dr	call spi_in
	dec de
	ld a, d
	or e
	jr nz, .dr
report:
	ld hl, BUF
	ld de, PAT
	ld bc, 0
.cmp	ld a, (de)
	cp (hl)
	jr nz, .no
	inc bc
.no	inc hl
	inc de
	ld a, h
	cp high (BUF + 512)
	jr nz, .cmp
	ld (ix + 0), c
	ld (ix + 1), b
	ld hl, BUF
	ld a, (hl)
	ld (ix + 2), a
	inc hl
	ld a, (hl)
	ld (ix + 3), a
	inc hl
	ld a, (hl)
	ld (ix + 4), a
	inc hl
	ld a, (hl)
	ld (ix + 5), a
	ld de, 6
	add ix, de
	ret

; --- the card ---

spi_out:			; called: 17 + 11 + 10 T apart at least
	out (SPI), a
	ret
spi_in:
	in a, (SPI)
	ret

; the first byte that is not FF, 1024 tries: a block cut short is finished with
; the FFs read here before the card answers
skip_ff:
	push de
	ld de, 1024
.lp	call spi_in
	cp #ff
	jr nz, .ret
	dec de
	ld a, d
	or e
	ld a, #ff
	jr nz, .lp
.ret	pop de
	ret

; a command: a = index, de = argument high word, hl = low word, then the CRC byte
cmd:
	push af
	call spi_in		; 8 clocks first
	pop af
	or #40
	call spi_out
	ld a, d
	call spi_out
	ld a, e
	call spi_out
	ld a, h
	call spi_out
	ld a, l
	call spi_out
	ld a, c
	call spi_out
	jp skip_ff		; R1

card_init:
	ld a, #ff
	out (#e7), a		; deselected: 80 clocks
	ld b, 10
.c	call spi_out
	djnz .c
	ld a, #fe
	out (#e7), a		; card 0
	ld a, 0
	ld de, 0
	ld hl, 0
	ld c, #95
	call cmd		; CMD0 -> 01
	ld a, 8
	ld de, 0
	ld hl, #01aa
	ld c, #87
	call cmd		; CMD8 -> 01 + R7
	ld b, 4
.r7	call spi_in
	djnz .r7
.acmd	ld a, 55
	ld de, 0
	ld hl, 0
	ld c, #ff
	call cmd
	ld a, 41
	ld de, #4000
	ld hl, 0
	ld c, #ff
	call cmd
	or a
	jr nz, .acmd
	ld a, 16
	ld de, 0
	ld hl, #0200
	ld c, #ff
	jp cmd			; CMD16 512 -> 00

; write sector a (byte address a * 512) from PAT through the writer at hl
write_sec:
	ld (.wr + 1), hl
	add a, a
	ld h, a
	ld l, 0
	ld de, 0
	ld c, #ff
	ld a, 24
	call cmd		; R1 00
	ld a, #fe
	call spi_out		; token
	ld hl, PAT
	ld c, SPI
.wr	call 0			; 512 bytes
	ld a, #ff
	call spi_out		; crc
	call spi_out
	call skip_ff		; data response, or the bytes still owed taken as data first
	ld de, 1024		; busy
.bz	call spi_in
	cp #ff
	ret z
	dec de
	ld a, d
	or e
	jr nz, .bz
	ret

; read sector a into BUF through the reader at hl
read_sec:
	ld (.rd + 1), hl
	add a, a
	ld h, a
	ld l, 0
	ld de, 0
	ld c, #ff
	ld a, 17
	call cmd		; R1 00
	call skip_ff		; token FE
	ld hl, BUF
	ld c, SPI
.rd	call 0
	call spi_in		; crc
	jp spi_in

; --- writers: 512 bytes from hl to port c, in pairs at a distance ---

wr_safe:
	ld b, 0
	otir
	otir
	ret

wr_11:				; out (c),d [T8 of 12] : out (#EB),a [T7]: 4 + 7
	ld b, 0
.l	ld d, (hl)
	inc hl
	ld a, (hl)
	inc hl
	out (c), d
	out (SPI), a
	djnz .l
	ret

wr_12:				; out (c),d : out (c),e
	ld b, 0
.l	ld d, (hl)
	inc hl
	ld e, (hl)
	inc hl
	out (c), d
	out (c), e
	djnz .l
	ret

wr_15:				; out (c),d : nop : out (#EB),a
	ld b, 0
.l	ld d, (hl)
	inc hl
	ld a, (hl)
	inc hl
	out (c), d
	nop
	out (SPI), a
	djnz .l
	ret

wr_16:				; out (c),d : nop : out (c),e
	ld b, 0
.l	ld d, (hl)
	inc hl
	ld e, (hl)
	inc hl
	out (c), d
	nop
	out (c), e
	djnz .l
	ret

wr_17:				; out (c),d : inc hl : out (#EB),a
	ld b, 0
.l	ld d, (hl)
	inc hl
	ld a, (hl)
	out (c), d
	inc hl
	out (SPI), a
	djnz .l
	ret

wr_18:				; out (c),d : cp (hl) : out (#EB),a
	ld b, 0
.l	ld d, (hl)
	inc hl
	ld a, (hl)
	inc hl
	out (c), d
	cp (hl)
	out (SPI), a
	djnz .l
	ret

; --- readers: 512 bytes from port c to hl, in pairs at a distance ---

rd_safe:
	ld b, 0
	inir
	inir
	ret

rd_11:				; in d,(c) [T8 of 12] : in a,(#EB) [T7]: 4 + 7
	ld b, 0
.l	in d, (c)
	in a, (SPI)
	ld (hl), d
	inc hl
	ld (hl), a
	inc hl
	djnz .l
	ret

rd_15:				; in d,(c) : nop : in a,(#EB)
	ld b, 0
.l	in d, (c)
	nop
	in a, (SPI)
	ld (hl), d
	inc hl
	ld (hl), a
	inc hl
	djnz .l
	ret

rd_16:				; in d,(c) : nop : in e,(c)
	ld b, 0
.l	in d, (c)
	nop
	in e, (c)
	ld (hl), d
	inc hl
	ld (hl), e
	inc hl
	djnz .l
	ret

rd_17:				; in d,(c) : inc hl : in a,(#EB)
	ld b, 0
.l	in d, (c)
	inc hl
	in a, (SPI)
	dec hl
	ld (hl), d
	inc hl
	ld (hl), a
	inc hl
	djnz .l
	ret
