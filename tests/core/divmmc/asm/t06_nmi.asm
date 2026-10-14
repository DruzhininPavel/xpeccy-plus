; t06: the NMI button (NMIAT). On the board it pulls /NMI only while mapcond is
; low (schematic: the button goes to the CPLD's mapcondout), so it is not taken
; while the board is in. The #0066 entry is a delayed one: the ROM's push af,
; then the firmware's handler from #0067. Held through a whole frame, the button
; pulls /NMI again each time the handler leaves (mapcond low): a short handler is
; taken over and over (55h times here).
; GATED=1: the program sits mapped through the frame the button is held.
	include "common.inc"
	org #8000

	xor a
	ld (FW_NMI), a
	if GATED
	E3 #80
	ld hl, code3d
	ld de, #3d00
	ld bc, code3d_end - code3d
	ldir
	E3 #00
	call #3d00		; mapped for three frames
	endif
	ld bc, 0		; a frame and more for an NMI to come
.lp	ld a, (FW_NMI)
	or a
	jr nz, .got
	dec bc
	ld a, b
	or c
	jr nz, .lp
.got	ld a, (FW_NMI)
	ld (MBOX + 1), a	; how many, or 0 when gated
	FINISH

	if GATED
code3d:
	ld h, 2			; 2 x 4000 x 26 T: about 3 frames
.o	ld de, 4000
.w	dec de
	ld a, d
	or e
	jr nz, .w
	dec h
	jr nz, .o
	jp #1ffb
code3d_end:
	endif
