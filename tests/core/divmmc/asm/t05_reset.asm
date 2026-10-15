; t05: what /RESET leaves and what a power cycle leaves (RESETAFTER, POWERAFTER).
; The register and the automapper are cleared at power-on only (the board's RC
; on the CPLD's poweron pin); /RESET clears the card selects and the SPI engine.
; Left mapped, the board shows MAPRAM page 3 at #0000 through the reset.
	include "common.inc"
	org #8000

	E3 #83
	ld a, 'R'
	ld (#2000), a		; page 3, byte 0
	E3 #85
	ld hl, code3d
	ld de, #3d00
	ld bc, code3d_end - code3d
	ldir
	ld a, '5'
	ld (#2000), a		; page 5, byte 0
	E3 #fe			; CONMEM + MAPRAM in one write: both bits taken, bank 3E
	E3 #45			; MAPRAM stays, bank 5
	ld a, #fc
	out (#e7), a		; both cards selected
	jp #3d00

code3d:
	FINISH			; mapped
code3d_end:
