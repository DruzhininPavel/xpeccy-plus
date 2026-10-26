; sjasmplus sjasm-next.asm writes sjasm-next.labels and sjasm-next.map (1.21.1 and
; 1.23.1 write the same). LABELSLIST: the page in decimal, the address as the offset
; inside the 8K page. CSPECTMAP: the CPU address, the linear address over the RAM, the
; type (00 label, 01 EQU), the name in capitals.
	DEVICE ZXSPECTRUMNEXT
	ORG #8000
start:	nop
	MMU 6, 17, #C000
p17:	nop
	ORG #C123
p17b:	nop
	MMU 7, 223, #E000
last:	nop
	MMU 0 1, 10, #0000
lowpage:	nop
	ORG #2000
dot:	nop
CONST	EQU 3
	LABELSLIST "sjasm-next.labels"
	CSPECTMAP "sjasm-next.map"
