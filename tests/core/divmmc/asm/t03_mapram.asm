; t03: MAPRAM. Set only; with it the automapper shows RAM page 3 at #0000, read
; only, and RAM[bank] at #2000, read only when bank is 3 - all six bits compared,
; so on a 128K board bank 13h reaches page 3 and writes it. CONMEM still shows
; the EEPROM at #0000 and writes bank 3 at #2000.
; RTL: divmmc.vhd mapram <= D(6) or mapram; romoe/ramoe/ramwr; bank3 <= bank = "000011".
	include "common.inc"
	org #8000

	E3 #83			; the EEPROM copied to page 3, as the esxDOS flasher leaves it
	ld hl, #0000
	ld de, #2000
	ld bc, #2000
	ldir
	ld a, 'M'		; the copy's marker is another
	ld (#2100), a
	E3 #80			; code for page 0 at #3D00
	ld hl, code3d
	ld de, #3d00
	ld bc, code3d_end - code3d
	ldir

	E3 #c3			; CONMEM and MAPRAM
	PEEK #0100, 1		; 'E': CONMEM shows the EEPROM
	ld a, #77
	ld (#2001), a		; CONMEM writes bank 3 at #2000
	PEEK #2001, 2		; 77
	E3 #00			; MAPRAM stays

	ld hl, #1111
	ld (#5c5d), hl
	rst 8			; runs from page 3
	ld hl, (FW_HL8)
	ld (MBOX + 3), hl	; 11 11

	call #3d00		; mapped from #3D00 on, and on through the RAM below
	PEEK #0100, 12		; 4F: out after the RET at #1FFB
	FINISH

code3d:
	jp mapped
code3d_end:

mapped:				; the board is in while this runs from the machine's RAM
	PEEK #0100, 5		; 'M': page 3
	ld a, #99
	ld (#0100), a		; read only
	PEEK #0100, 6		; 'M'
	E3 #03
	ld a, #88
	ld (#2001), a		; bank 3 at #2000: read only
	PEEK #2001, 7		; 77
	E3 #13			; bank 13h: page 3 on 128K, not protected
	ld a, #88
	ld (#2001), a
	PEEK #2001, 8		; 88
	E3 #04
	ld a, #44
	ld (#2000), a
	PEEK #2000, 9		; 44
	E3 #00
	PEEK #0100, 10		; 'M': MAPRAM still set
	jp #1ffb		; the copy's RET, an off-area fetch: back to the caller, out
