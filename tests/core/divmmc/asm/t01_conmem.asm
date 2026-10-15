; t01: port #E3 with CONMEM - the EEPROM at #0000, RAM[bank] at #2000, the bank
; bits as the RAM fitted takes them, and the machine back with #E3 = 0.
; RTL: divmmc.vhd romoe/romwr/ramoe/ramwr/bankout; schematic: a 128K chip takes
; bank bits 0-3 only.
	include "common.inc"
	org #8000

	E3 #80
	PEEK #0100, 1		; 'E' of the EEPROM's marker
	ld a, #55		; J1 closed: the EEPROM takes no write
	ld (#0100), a
	PEEK #0100, 2		; still 'E'

	E3 #80
	ld a, #5a
	ld (#2000), a
	E3 #81
	ld a, #a5
	ld (#2000), a
	E3 #8f
	ld a, #0f
	ld (#2000), a
	E3 #80
	PEEK #2000, 3		; 5A
	E3 #81
	PEEK #2000, 4		; A5
	E3 #91			; bank 11h: page 1 on a 128K board
	PEEK #2000, 5		; A5 (128K), 00 (512K)
	E3 #9f			; bank 1Fh: page 15 on a 128K board
	PEEK #2000, 6		; 0F (128K), 00 (512K)

	E3 #00			; the machine's ROM again
	PEEK #0100, 7		; 4F
	PEEK #2000, 8		; 0D
	in a, (#e3)		; written only: nothing drives the bus
	ld (MBOX + 9), a
	in a, (#e7)
	ld (MBOX + 10), a
	FINISH
