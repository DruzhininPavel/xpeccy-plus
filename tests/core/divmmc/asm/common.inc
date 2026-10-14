; shared definitions for the divMMC bench programs (org #8000, mailbox at #BF00)
MBOX	equ #BF00
DONE	equ #AA

; what the test firmware (ee_test.asm) leaves for the program
FW_HL8	equ #BF60	; HL at #000B, after the instruction at #0008
FW_INT	equ #BF62	; '1' once the #0038 handler ran
FW_NMI	equ #BF63	; NMIs the #0066 handler took
FW_HLT	equ #BF64	; HL at #04C9, after the instruction at #04C6

	macro FINISH
		di
		ld a, DONE
		ld (MBOX), a
		halt
	endm

; out (#E3),val
	macro E3 val
		ld a, val
		out (#e3), a
	endm

; (adr) into (MBOX+slot)
	macro PEEK adr, slot
		ld a, (adr)
		ld (MBOX + slot), a
	endm
