; Two ways a program reads a key and its shift, counting what each one saw.
; At #9000, one byte each:
;   0 sep_m    M down with SYMBOL SHIFT up, the shift read by an IN of its own after the key
;   1 sep_0    the same for 0 and CAPS SHIFT
;   2 edge_m   M going down, SYMBOL SHIFT down in the pass before
;   3 bare_m   M going down, SYMBOL SHIFT up in the pass before
;   4 edge_0   0 going down, CAPS SHIFT down in the pass before
;   5 bare_0   0 going down, CAPS SHIFT up in the pass before
; Fed . (SS+M) and Delete (CS+0) alone, a right keyboard leaves 0, 1, 3 and 5 at 0.

	org #8000

CNT	equ #9000

start:
	di
	ld sp,#9f00
	ld hl,CNT
	ld b,8
.clr:	ld (hl),0
	inc hl
	djnz .clr
	xor a
	ld (prev7f),a
	ld (prevef),a
	ld (prevfe),a

loop:
	; the key's row, then its shift by another IN
	ld a,#7f
	in a,(#fe)
	bit 2,a			; M
	jr nz,.no_m
	ld a,#7f
	in a,(#fe)
	bit 1,a			; SYMBOL SHIFT
	jr z,.no_m
	ld hl,CNT+0
	inc (hl)
.no_m:
	ld a,#ef
	in a,(#fe)
	bit 0,a			; 0
	jr nz,.no_0
	ld a,#fe
	in a,(#fe)
	bit 0,a			; CAPS SHIFT
	jr z,.no_0
	ld hl,CNT+1
	inc (hl)
.no_0:
	; a key's press edge, the shift from the pass before
	ld a,#7f
	in a,(#fe)
	cpl
	ld e,a			; 1 = down now
	ld a,(prev7f)
	ld d,a
	ld a,e
	ld (prev7f),a
	ld a,d
	cpl
	and e
	bit 2,a
	jr z,.no_em
	ld hl,CNT+2
	bit 1,d
	jr nz,.em
	inc hl
.em:	inc (hl)
.no_em:
	ld a,(prevfe)
	ld d,a			; CAPS SHIFT's row in the pass before
	ld a,#fe
	in a,(#fe)
	cpl
	ld (prevfe),a
	ld a,#ef
	in a,(#fe)
	cpl
	ld e,a
	ld a,(prevef)
	cpl
	and e
	ld c,a
	ld a,e
	ld (prevef),a
	bit 0,c
	jr z,loop
	ld hl,CNT+4
	bit 0,d
	jr nz,.e0
	inc hl
.e0:	inc (hl)
	jr loop

prev7f:	db 0
prevef:	db 0
prevfe:	db 0
