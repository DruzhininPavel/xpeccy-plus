; e01: esxDOS 0.8.9's F_MKDIR, called once esxDOS has booted (PROGAT). It fills
; the new directory's cluster through its write-sector routine with the zero
; fill (OUT (C),A : OUT (C),A, 12 T apart). On the CPLD's SPI engine every second
; byte of those is lost: the sector gets 256 x 00 and then FF. DIRDUMP shows it.
	include "common.inc"
	org #8000

F_MKDIR	equ 170

	ld a, '*'
	ld ix, dname
	rst 8
	db F_MKDIR
	ld (MBOX + 1), a	; error code when carry
	ld a, 0
	rla
	ld (MBOX + 2), a	; carry
	FINISH

dname:	db "TEST", 0
