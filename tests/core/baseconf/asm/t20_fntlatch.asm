; ATM text: #0EBE reads back the font byte the video fetched last (zports.v). A frame
; that is not drawn works it out from the memory when it is asked, so a char code
; changed after its fetch must still give the byte that was fetched. Both char code
; planes of the window rewritten while #0EBE is read 8192 times; the sum and the xor
; of all reads go to the mailbox. Run drawn and with NODRAW=1: the same bytes.
	org #8000
	include "common.inc"
	OUT77 6
	BCOUT #EFF7, #14
	ld de, 0
	ld hl, #41C0
	ld ix, 8192
.r:	ld a, r
	ld (hl), a
	set 5, h
	ld (hl), a
	res 5, h
	ld bc, #0EBE
	in a, (c)
	ld b, a
	add a, d
	ld d, a
	ld a, b
	xor e
	ld e, a
	inc hl
	ld a, h
	cp #48
	jr nz, .n
	ld h, #41
.n:	dec ix
	ld a, ixh
	or ixl
	jr nz, .r
	ld a, d
	ld (MBOX + 1), a
	ld a, e
	ld (MBOX + 2), a
	FINISH
