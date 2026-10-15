; t02: the automapper's timing.
; - #0008, #04C6 (delayed): the instruction there, operands too, comes from the
;   machine's ROM, the board from the next fetch (divmmc.vhd: automap <= the old
;   mapcond). On a divIDE the operands would come from the board.
; - #3Dxx (instant): that fetch already reads the board.
; - a fetch at an entry point while mapped reads the board's opcode.
; - #1FF8 (off-area): the instruction and its operands come from the board, the
;   next fetch from the machine. A divIDE would take the operands from the ROM.
	include "common.inc"
	org #8000

	ld hl, #1111
	ld (#5c5d), hl		; what the ROM's ld hl,(#5C5D) at #0008 reads
	ld hl, #2222
	ld (#9000), hl		; what the board's ld hl,(#9000) would read
	ld hl, stubd		; where jp #1FF8 lands on the ROM's operands
	ld de, #d70d
	ld bc, stubd_end - stubd
	ldir

	; code for RAM bank 0 at #3D00, put there with CONMEM
	E3 #80
	ld hl, code3d
	ld de, #3d00
	ld bc, code3d_end - code3d
	ldir
	E3 #00

	rst 8			; delayed entry from outside
	ld hl, (FW_HL8)
	ld (MBOX + 1), hl	; 11 11: the ROM's operands
	PEEK #0100, 3		; 4F: out again after the RET at #1FFB

	ld hl, 0
	call #04c6		; delayed entry
	ld hl, (FW_HLT)
	ld (MBOX + 4), hl	; 80 1F: the ROM's operands
	PEEK #0100, 6		; 4F

	jp #3d00		; instant entry; goes on at #8100 or #D70D

code3d:
	ld a, (#0100)		; the board is in for this very fetch: 'E'
	ld (MBOX + 7), a
	rst 8			; mapped already: the board's ld hl,(#9000)
	ld hl, (FW_HL8)		; back through the RET at #1FFB, and #3Dxx maps again
	ld (MBOX + 8), hl	; 22 22
	ld a, (#0100)
	ld (MBOX + 10), a	; 'E': mapped again by the fetch here
	jp #1ff8		; off-area: jp #8100 from the board
code3d_end:

stubd:
	ld a, 'D'		; #D70D: the operands came from the ROM
	ld (MBOX + 11), a
	FINISH
stubd_end:

	ds #8100 - $, 0
	ld a, 'P'		; #8100: the operands came from the board
	ld (MBOX + 11), a
	PEEK #0100, 12		; 4F: out
	FINISH
