; The debugger's sprite decode against the TSU (video_ts.v, video_ts_render.v): four
; sprites with no layer under them but the border (NOGFX), each colour of palette rows
; 1 and 2 and the border a CRAM entry of its own. The bench (SPRCHK=1) draws every
; active sprite the debugger's way and compares it dot for dot with the frame.
;   0: 16x16 at 10,20, tile 0, pal 1
;   1: 16x16 at 60,20, tile 2, pal 2, XFlip
;   2: 8x24 at 10,60, tile 5, pal 1, YFlip
;   3: 24x16 at 60,60, tile 62, pal 2, XFlip + YFlip - three tiles from 62: the line of
;      the graphics wraps round to tile 0 of the same line; LEAP
	include "common.inc"
	org #8000
	di
	; CRAM through FMAPS at #C000: entry k (16..47) = R k>>5, G 16, B k&31
	TSOUT FMADDR, #1C
	ld hl, #C000 + 16 * 2
	ld b, 32
	ld e, 16
cr:	ld a, e
	and 31
	ld (hl), a			; B, and G's low bits 0
	inc hl
	ld a, e
	rrca
	rrca
	rrca
	and 4				; R bit 0 at bit 2 of the high byte (R is b14..b10)
	or 2				; G bit 4 -> b9
	ld (hl), a
	inc hl
	inc e
	djnz cr
	ld hl, #7C1F			; border colour #3F: one no sprite colour has
	ld (#C000 + #3F * 2), hl
	; SFILE at #C200
	ld hl, spr
	ld de, #C200
	ld bc, 4 * 6
	ldir
	TSOUT FMADDR, 0
	; sprite graphics, page #20: byte at line l, offset o = (o + 3 * l) & #FF
	TSOUT PAGE3, #20
	ld hl, #C000
	ld d, 0				; line
gl:	ld a, d
	add a, d
	add a, d
	ld e, a
gb:	ld a, l
	add a, e
	ld (hl), a
	inc l
	jr nz, gb
	inc h
	inc d
	ld a, d
	cp 64
	jr nz, gl
	TSOUT SGPAGE, #20
	TSOUT BORDER, #3F
	TSOUT VCONFIG, #22		; 256x192, no bitmap: the border under the sprites
	TSOUT TSCONFIG, #80
	jr $

	; y, YF|LEAP|ACT|ys|y8, x, XF|xs|x8, tnum, pal<<4|tnum8
spr:	db 20, #22, 10, #02, 0, #10
	db 20, #22, 60, #82, 2, #20
	db 60, #A4, 10, #00, 5, #10
	db 60, #E2, 60, #84, 62, #20
