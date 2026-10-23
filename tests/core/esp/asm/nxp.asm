; esp-bench guest: NEXTplorer's Wifi.init and its page fetch, replayed on UART 0 the way
; its drivers/uart.asm and wifi.asm do it (the same rate table, the same answer parser),
; with a time limit on each byte so a missing answer ends the run instead of hanging it.
;
;   CASE 1  init   its init sequence, then CIPDINFO's other forms; each answer printed
;   CASE 2  fetch  init, then a 48000-byte page from the bench's server, read as it reads one
;   CASE 3  dinfo  the same with CIPDINFO=1: the first +IPD header's address part printed
;   CASE 4  stall  the fetch, the reader stopped for ~240 ms at 8192 bytes: the Rx FIFO
;                  overflows, as it does on the board

	OPT --zxnext
	IFNDEF CASE
		DEFINE CASE 1
	ENDIF

CON	equ #2003		; a character for the bench to print
FIN	equ #0103		; the run ends
PORTSTR	equ #3F00		; the server's port as decimal text, zero-ended (put by the bench)
RXD	equ #143B
TXD	equ #133B
SEL	equ #153B
PAGE	equ 48000		; what the server sends for "big"

	org 0
	di
	jp start
	ds #38 - $
	ei
	ret

start:
	ld sp, #ff00
	nextreg 7, 3		; 28 MHz, as NEXTplorer runs
	xor a
	ld hl, vars
	ld b, vars_end - vars
.clr	ld (hl), a
	inc hl
	djnz .clr
	im 1
	ei
	ld b, 100		; 2 s: the ESP boots meanwhile; its lines stay in the FIFO
.boot	halt
	djnz .boot
	call uart_init

	ld hl, cATE0 : call docmd
	ld hl, cServer : call docmd
	ld hl, cClose : call docmd
	ld hl, cMux : call docmd
	ld hl, cDinfo0 : call docmd
	IF CASE == 1
	ld hl, cDinfo1 : call docmd
	ld hl, cDinfo2 : call docmd
	ld hl, cDinfoQ : call docmd
	ld hl, cDinfoN : call docmd
	ld hl, cDinfo0 : call docmd
	ENDIF
	IF CASE == 3
	ld hl, cDinfo1 : call docmd
	ENDIF
	IF CASE > 1
	call fetch
	ENDIF
	ld bc, FIN
	out (c), a
	jr $

; == the page ==

fetch:
	ld hl, cClose : call docmd	; openTCP closes first, whatever is open
	ld hl, sStart : call print	; the port varies: printed without it
	ld hl, cStart : call send
	ld hl, PORTSTR : call send
	ld a, 13 : call write
	ld a, 10 : call write
	call check
	call result
	ld hl, cSend : call print	; tcpSendZ: the length as five digits
	ld a, ':' : call putc
	ld a, ' ' : call putc
	ld hl, cSend : call sendln
.gt	call read
	jr c, .none
	cp '>'
	jr nz, .gt
	ld hl, selector : call send
	call check
	call result
.pk	call packet
	jr nc, .pk
	IF CASE == 4
	ld hl, sAll : call print
	ld hl, (count)
	ld de, PAGE
	or a
	sbc hl, de
	jr nz, .part
	ld a, (closed)
	or a
	jr z, .part
	call yes
	jr .ovf
.part	call no
	ELSE
	ld hl, sBytes : call print
	ld hl, (count) : call num
	call nl
	ld hl, sSum : call print
	ld hl, (sum) : call num
	call nl
	ld hl, sClosed : call print
	ld a, (closed) : call yesno
	ENDIF
.ovf	ld hl, sOvf : call print
	ld a, (ovf)
	jp yesno
.none	ld hl, sNone : call print
	jp nl

; getPacket: +IPD,<n>:<n bytes> or CLOSED. NC: go on; C: closed, or nothing came
packet:
	call read
	ret c
	cp '+'
	jr z, .ipd
	cp 'O'			; it is enough to check "OSED\r"
	jr nz, packet
	call read : ret c : cp 'S' : jr nz, packet
	call read : ret c : cp 'E' : jr nz, packet
	call read : ret c : cp 'D' : jr nz, packet
	call read : ret c : cp 13 : jr nz, packet
	ld a, 1
	ld (closed), a
	scf
	ret
.ipd	call read : ret c : cp 'I' : jr nz, packet
	call read : ret c : cp 'P' : jr nz, packet
	call read : ret c : cp 'D' : jr nz, packet
	call read : ret c		; the comma
	ld hl, 0
.len	call read
	ret c
	cp ':'
	jr z, .data
	cp ','
	jr z, .info
	sub '0'
	push de
	ld e, a
	ld d, 0
	push de
	add hl, hl
	ld d, h
	ld e, l
	add hl, hl
	add hl, hl
	add hl, de
	pop de
	add hl, de
	pop de
	jr .len
.info	ld a, (shown)		; with CIPDINFO=1: the address part, once
	or a
	jr nz, .skip
	inc a
	ld (shown), a
	push hl
	ld hl, sIpd : call print
	pop hl
	ld a, ','
.ii	call putc
	call read
	ret c
	cp ':'
	jr nz, .ii
	call nl
	jr .data
.skip	call read
	ret c
	cp ':'
	jr nz, .skip
.data	ld a, h
	or l
	ret z			; NC from the OR
.d	call read
	ret c
	push hl
	ld hl, (sum)
	ld e, a
	ld d, 0
	add hl, de
	ld (sum), hl
	ld hl, (count)
	inc hl
	ld (count), hl
	IF CASE == 4
	ld a, h
	cp #20
	jr nz, .ns
	ld a, l
	or a
	call z, stall
.ns
	ENDIF
	pop hl
	dec hl
	ld a, h
	or l
	jr nz, .d
	ret			; NC from the OR

	IF CASE == 4
stall:	di			; ~6.8 M T at 28 MHz
	ld d, 4
.s1	ld bc, 0
.s2	dec bc
	ld a, b
	or c
	jr nz, .s2
	dec d
	jr nz, .s1
	ei
	ret
	ENDIF

; == the ESP's answers ==

; checkOkErr: scans for O, E, F; wants OK CR, ERROR, FAIL, then skips to LF.
; A = 0 OK, 1 ERROR, 2 FAIL, 3 nothing came
check:
	call read
	jr c, .tmo
	cp 'O' : jr z, .ok
	cp 'E' : jr z, .er
	cp 'F' : jr z, .fa
	jr check
.ok	call read : jr c, .tmo : cp 'K' : jr nz, check
	call read : jr c, .tmo : cp 13 : jr nz, check
	call .lf
	xor a
	ret
.er	call read : jr c, .tmo : cp 'R' : jr nz, check
	call read : jr c, .tmo : cp 'R' : jr nz, check
	call read : jr c, .tmo : cp 'O' : jr nz, check
	call read : jr c, .tmo : cp 'R' : jr nz, check
	call .lf
	ld a, 1
	ret
.fa	call read : jr c, .tmo : cp 'A' : jr nz, check
	call read : jr c, .tmo : cp 'I' : jr nz, check
	call read : jr c, .tmo : cp 'L' : jr nz, check
	call .lf
	ld a, 2
	ret
.lf	call read
	ret c
	cp 10
	jr nz, .lf
	ret
.tmo	ld a, 3
	ret

; HL = a command: printed, sent with CR LF, its answer printed
docmd:
	push hl
	call print
	ld a, ':' : call putc
	ld a, ' ' : call putc
	pop hl
	call sendln
	call check
result:
	add a, a
	ld e, a
	ld d, 0
	ld hl, rtab
	add hl, de
	ld a, (hl)
	inc hl
	ld h, (hl)
	ld l, a
	call print
	jp nl

; == the UART ==

; Uart.init: the prescaler for 115200 at the video timing NR 11 gives
uart_init:
	ld bc, #243B
	ld a, #11
	out (c), a
	ld bc, #253B
	in a, (c)
	and 7
	add a, a
	ld e, a
	ld d, 0
	ld hl, baud
	add hl, de
	ld e, (hl)
	inc hl
	ld d, (hl)
	ld bc, SEL
	ld a, %00100000		; UART 0, the ESP
	out (c), a
	ld bc, RXD
	ld a, e
	and #7f
	out (c), a
	ld a, e
	rla
	ld a, d
	rla
	or #80
	out (c), a
	ret
baud	dw 243, 248, 256, 260, 269, 278, 286, 234

; A = the next byte; CF if none came for ~2.6 s. An overflow seen on the way is kept
read:
	push bc
	push de
	push hl
	ld l, 16
	ld de, 0
	ld bc, TXD
.w	in a, (c)
	bit 2, a
	jr z, .no
	ld (ovf), a
.no	rrca
	jr c, .got
	dec de
	ld a, d
	or e
	jr nz, .w
	dec l
	jr nz, .w
	pop hl
	pop de
	pop bc
	scf
	ret
.got	ld bc, RXD
	in a, (c)
	pop hl
	pop de
	pop bc
	or a
	ret

; A out when the Tx FIFO has room
write:
	push bc
	push af
	ld bc, TXD
.w	in a, (c)
	bit 2, a
	jr z, .no
	ld (ovf), a
.no	and 2
	jr nz, .w
	pop af
	out (c), a
	pop bc
	ret

send:	ld a, (hl)
	or a
	ret z
	call write
	inc hl
	jr send

sendln:	call send
	ld a, 13 : call write
	ld a, 10
	jp write

; == the bench's console ==

putc:	push bc
	ld bc, CON
	out (c), a
	pop bc
	ret

nl:	ld a, 10
	jp putc

print:	ld a, (hl)
	or a
	ret z
	call putc
	inc hl
	jr print

yesno:	or a
	jr nz, yes
no:	ld hl, sNo : call print
	jp nl
yes:	ld hl, sYes : call print
	jp nl

; HL in decimal
num:	ld e, 0
	ld bc, -10000 : call .dg
	ld bc, -1000 : call .dg
	ld bc, -100 : call .dg
	ld bc, -10 : call .dg
	ld a, l
	add a, '0'
	jp putc
.dg	ld a, '0' - 1
.lp	inc a
	add hl, bc
	jr c, .lp
	sbc hl, bc
	cp '0'
	jr nz, .pr
	bit 0, e
	ret z
.pr	ld e, 1
	jp putc

; == text ==

cATE0	db "ATE0", 0
cServer	db "AT+CIPSERVER=0", 0
cClose	db "AT+CIPCLOSE", 0
cMux	db "AT+CIPMUX=0", 0
cDinfo0	db "AT+CIPDINFO=0", 0
cDinfo1	db "AT+CIPDINFO=1", 0
cDinfo2	db "AT+CIPDINFO=2", 0
cDinfoQ	db "AT+CIPDINFO?", 0
cDinfoN	db "AT+CIPDINFO", 0
cStart	db 'AT+CIPSTART="TCP","127.0.0.1",', 0
cSend	db "AT+CIPSEND=00005", 0
selector db "big", 13, 10, 0
sStart	db 'AT+CIPSTART="TCP","127.0.0.1",<port>: ', 0
sBytes	db "bytes: ", 0
sSum	db "sum: ", 0
sClosed	db "closed: ", 0
sOvf	db "overflow: ", 0
sAll	db "all bytes: ", 0
sIpd	db "+IPD address", 0
sNone	db "no prompt", 0
sOK	db "OK", 0
sERR	db "ERROR", 0
sFAIL	db "FAIL", 0
sNOT	db "nothing", 0
sYes	db "yes", 0
sNo	db "no", 0
rtab	dw sOK, sERR, sFAIL, sNOT

; in RAM, cleared at the start
vars	equ #8000
count	equ vars		; data bytes taken
sum	equ vars + 2		; their sum, 16 bits
closed	equ vars + 4		; CLOSED seen
ovf	equ vars + 5		; the Rx FIFO overflowed (status b2) at some point
shown	equ vars + 6		; the +IPD address part was printed
vars_end equ vars + 7
