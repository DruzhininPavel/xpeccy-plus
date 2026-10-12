; The debugger's tile map decode against the TSU (video_ts.v): both tile layers on, no
; sprites, no bitmap (NOGFX), so each dot of the 256x192 window is T1 where its dot is
; not 0, else T0, else the border. The bench (TILECHK=1) puts the window together from
; tsl_map_entry/tsl_map_dots and the offsets and compares it with the frame.
;   T0: page #20, offsets X 5, Y 3, T0Z_EN (tile 0 is drawn), palette bits from PalSel 01
;   T1: page #28, offsets X #1F3, Y #1FA (the map wraps), half its cells tile 0 (holes),
;       palette bits from PalSel 10
;   entries: tiles 0..63, all four palettes, XFlip and YFlip in a pattern
; Each color 40..BF is a CRAM entry of its own (the word is the index), the border 7C00.
	include "common.inc"
T0XL	equ #40AF
T0YL	equ #42AF
T1XL	equ #44AF
T1XH	equ #45AF
T1YL	equ #46AF
T1YH	equ #47AF
TMPAGE	equ #16AF
T0GPAGE	equ #17AF
T1GPAGE	equ #18AF
	org #8000
	di
	TSOUT FMADDR, #1C
	ld hl, #C000 + #40 * 2
	ld e, #40
cr:	ld (hl), e
	inc hl
	ld (hl), 0
	inc hl
	inc e
	ld a, e
	cp #C0
	jr nz, cr
	ld hl, #7C00
	ld (#C000 + #3F * 2), hl
	TSOUT FMADDR, 0
	; graphics, first line of tiles only (tiles 0..63): page #20 byte = o + 3l, #28 = (o + 7l) ^ #5A
	TSOUT PAGE3, #20
	ld hl, #C000
	ld d, 0
g0:	ld a, d
	add a, d
	add a, d
	ld e, a
g0b:	ld a, l
	add a, e
	ld (hl), a
	inc l
	jr nz, g0b
	inc h
	inc d
	ld a, d
	cp 8
	jr nz, g0
	TSOUT PAGE3, #28
	ld hl, #C000
	ld d, 0
g1:	ld a, d
	add a, a
	add a, a
	add a, a
	sub d
	ld e, a				; 7l
g1b:	ld a, l
	add a, e
	xor #5A
	ld (hl), a
	inc l
	jr nz, g1b
	inc h
	inc d
	ld a, d
	cp 8
	jr nz, g1
	; the map, page #30: row d at #C000 + d * 256, layer 0 entries at +0, layer 1 at +128
	TSOUT PAGE3, #30
	ld d, 0				; row
mr:	ld a, d
	add a, a
	add a, a
	add a, a
	sub d
	ld (t0b), a			; 7d
	ld a, d
	add a, a
	add a, a
	add a, d
	ld (t1b), a			; 5d
	ld a, d
	add a, #C0
	ld h, a
	ld l, 0
	ld c, 0				; column
mc:	; layer 0: tile (7d + c) & 63, palette (c >> 1) & 3, XF if c & 4, YF if d & 2
	ld a, (t0b)
	add a, c
	and 63
	ld (hl), a
	inc l
	ld a, c
	rlca
	rlca
	rlca
	and #30
	ld b, a
	ld a, c
	and 4
	jr z, m0
	set 6, b
m0:	ld a, d
	and 2
	jr z, m1
	set 7, b
m1:	ld (hl), b
	; layer 1, 128 bytes on: tile 0 where (d + c) is odd, else (5d + 3c) & 63, palette c & 3,
	; XF if d & 4, YF if c & 8
	push hl
	set 7, l
	dec l
	ld a, d
	add a, c
	and 1
	jr z, m2
	ld (hl), 0
	inc l
	ld (hl), 0
	jr m5
m2:	ld a, c
	add a, a
	add a, c
	ld b, a
	ld a, (t1b)
	add a, b
	and 63
	ld (hl), a
	inc l
	ld a, c
	and 3
	rlca
	rlca
	rlca
	rlca
	ld b, a
	ld a, d
	and 4
	jr z, m3
	set 6, b
m3:	ld a, c
	and 8
	jr z, m4
	set 7, b
m4:	ld (hl), b
m5:	pop hl
	inc l
	inc c
	ld a, c
	cp 64
	jr nz, mc
	inc d
	ld a, d
	cp 64
	jr nz, mr
	TSOUT TMPAGE, #30
	TSOUT T0GPAGE, #20
	TSOUT T1GPAGE, #28
	TSOUT T0XL, 5
	TSOUT T0YL, 3
	TSOUT T1XL, #F3
	TSOUT T1XH, 1
	TSOUT T1YL, #FA
	TSOUT T1YH, 1
	TSOUT PALSEL, #90		; T0 palette bits 01, T1 10
	TSOUT BORDER, #3F
	TSOUT VCONFIG, #22
	TSOUT TSCONFIG, #64		; T1_EN, T0_EN, T0Z_EN
	jr $

t0b:	db 0
t1b:	db 0
