; Test firmware for the divMMC EEPROM (8K at #0000). Each entry point holds an
; instruction of the same length as the 48K ROM's but with other operands, so a
; program can tell which memory the operands were read from. The ROM's bytes:
;   #0008 2A 5D 5C  ld hl,(#5C5D)	-> here ld hl,(#9000)
;   #04C6 21 80 1F  ld hl,#1F80		-> here ld hl,#3333
;   #1FF8 3E 0D D7  ld a,13 : rst 16	-> here jp #8100 (#D70D if read from the ROM)
; Every way out is the RET at #1FFB (an off-area fetch).
	include "common.inc"

	org #0000
	di			; #0000, as the ROM
	jp #1ffb		; a reset into the board just returns
	ds #0008 - $, #ff

	ld hl, (#9000)		; #0008
	ld (FW_HL8), hl		; #000B
	jp #1ffb
	ds #0038 - $, #ff

	push af			; #0038, as the ROM
	ld a, '1'		; #0039
	ld (FW_INT), a
	pop af
	ei
	jp #1ffb
	ds #0066 - $, #ff

	push af			; #0066, as the ROM
	ld a, (FW_NMI)		; #0067
	inc a
	ld (FW_NMI), a
	pop af
	jp #1ffb
	ds #0100 - $, #ff

	db "EEP0"		; #0100, a marker
	ds #04c6 - $, #ff

	ld hl, #3333		; #04C6
	ld (FW_HLT), hl		; #04C9
	jp #1ffb
	ds #0562 - $, #ff

	in a, (#fe)		; #0562, as the ROM
	jp #1ffb
	ds #1ff8 - $, #ff

	jp #8100		; #1FF8
	ret			; #1FFB
	ds #2000 - $, #ff
