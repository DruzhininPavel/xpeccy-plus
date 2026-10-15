; t04: writing the EEPROM (J1 open, CONMEM). A 28C64-class part (Atmel AT28C64B
; datasheet): a write opens a 150 us page load, then the internal write takes
; tWC; meanwhile a read gives bit 7 inverted (DATA polling) and bit 6 toggling.
; Software data protection: AA>1555 55>0AAA A0>1555 protects (that load writes),
; a load without it then writes nothing; AA 55 80 AA 55 20 unprotects.
	include "common.inc"
	org #8000

	E3 #80
	ld a, #5a
	ld (#0100), a
	PEEK #0100, 1		; status: bit 7 = 1 (5A inverted), bit 6 toggling
	PEEK #0100, 2
	ld hl, #0100
	ld b, #5a
	call poll
	ld (MBOX + 3), de	; polls of 45 T until it read back
	PEEK #0100, 5		; 5A

	ld a, #aa		; protect, and write #0101 in the same load
	ld (#1555), a
	ld a, #55
	ld (#0aaa), a
	ld a, #a0
	ld (#1555), a
	ld a, #11
	ld (#0101), a
	ld hl, #0101
	ld b, #11
	call poll
	PEEK #0101, 6		; 11

	ld a, #22		; protected: the timer runs, nothing is written
	ld (#0102), a
	call settle
	PEEK #0102, 7		; 'P'

	ld a, #aa		; unprotect
	ld (#1555), a
	ld a, #55
	ld (#0aaa), a
	ld a, #80
	ld (#1555), a
	ld a, #aa
	ld (#1555), a
	ld a, #55
	ld (#0aaa), a
	ld a, #20
	ld (#1555), a
	call settle
	ld a, #33
	ld (#0103), a
	ld hl, #0103
	ld b, #33
	call poll
	PEEK #0103, 8		; 33
	PEEK #1555, 9		; FF: the command bytes are not data
	PEEK #0aaa, 10		; FF
	E3 #00
	FINISH

; read (hl) until it is b, counting in de
poll:
	ld de, 0
.lp	ld a, (hl)
	cp b
	ret z
	inc de
	jr .lp

; wait until two reads in a row agree (the toggle bit stops)
settle:
	ld a, (#0100)
	ld c, a
	ld a, (#0100)
	cp c
	jr nz, settle
	ret
