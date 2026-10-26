; sjasmplus sjasm-128.asm writes sjasm-128.labels and sjasm-128.map: 16K banks
	DEVICE ZXSPECTRUM128
	ORG #8000
start:	nop
	SLOT 3
	PAGE 7
	ORG #C123
b7:	nop
	ORG #4010
scr:	nop
CONST	EQU 3
	LABELSLIST "sjasm-128.labels"
	CSPECTMAP "sjasm-128.map"
