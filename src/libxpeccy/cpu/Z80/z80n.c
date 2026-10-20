// The ZX Spectrum Next's Z80N: the Z80 core on opcode tables of its own.
// What it adds and where it parts from a Zilog Z80 is the Next's own CPU, the
// T80N of the FPGA core (3.02.03), as a simulation of that core showed it:
// the extended opcodes, and a handful of undocumented flags the T80N sets its
// own way. The plain Z80 tables are left as they are.

#include <string.h>
#include "z80.h"
#include "z80_macro.h"

extern opCode npTab[256];
extern opCode cbTab[256];
extern opCode edTab[256];
extern opCode ddTab[256];
extern opCode fdTab[256];
extern opCode ddcbTab[256];
extern opCode fdcbTab[256];

void npr37(CPU*);
void npr3F(CPU*);
void ed46(CPU*);
void ed56(CPU*);
void ed5E(CPU*);

static opCode n_np[256];
static opCode n_cb[256];
static opCode n_ed[256];
static opCode n_edx[256];	// ED after a DD/FD prefix: the same, but the IM snoop does not see it
static opCode n_dd[256];
static opCode n_fd[256];
static opCode n_ddcb[256];
static opCode n_fdcb[256];

static const opTabSet z80n_tabs = {n_np, n_cb, n_ed, n_dd, n_fd, n_ddcb, n_fdcb};

// flags of an 8-bit result: S Z and the two undocumented bits
static void n_szxy(CPU* cpu, unsigned char v) {
	cpu->flgS = !!(v & 0x80);
	cpu->flgZ = !v;
	cpu->flgF5 = !!(v & 0x20);
	cpu->flgF3 = !!(v & 0x08);
}

// == the standard set where the T80N differs ==

// scf/ccf: bits 3 and 5 come from A alone, no Q
static void n_scf(CPU* cpu) {
	cpu->flgFW = 1;
	cpu->flgF5 = !!(cpu->regA & 0x20);
	cpu->flgH = 0;
	cpu->flgF3 = !!(cpu->regA & 0x08);
	cpu->flgN = 0;
	cpu->flgC = 1;
}

static void n_ccf(CPU* cpu) {
	cpu->flgFW = 1;
	cpu->flgF5 = !!(cpu->regA & 0x20);
	cpu->flgH = cpu->flgC;
	cpu->flgF3 = !!(cpu->regA & 0x08);
	cpu->flgN = 0;
	cpu->flgC ^= 1;
}

// bit n,(hl): bits 3 and 5 are 0 (no MEMPTR shows anywhere)
static void n_bit_hl(CPU* cpu) {
	int bit = (cpu->com >> 3) & 7;
	cpu->tmp = z80_mrd(cpu, cpu->regHL);
	z80_wait(cpu, cpu->regHL, 1);
	BIT(bit, cpu->tmp);
	cpu->flgF5 = 0;
	cpu->flgF3 = 0;
}

// bit n,(ix+d): 0 in bits 3 and 5 for the documented opcode (low bits 110), the
// byte's own bits 3 and 5 for the aliases
static void n_bitx(CPU* cpu, unsigned short base) {
	int bit = (cpu->com >> 3) & 7;
	cpu->regWZ = base + (signed char)cpu->tmp;
	z80_wait(cpu, cpu->regPC - 1, 2);
	cpu->tmpb = z80_mrd(cpu, cpu->regWZ);
	z80_wait(cpu, cpu->regWZ, 1);
	BIT(bit, cpu->tmpb);
	if ((cpu->com & 7) == 6) {
		cpu->flgF5 = 0;
		cpu->flgF3 = 0;
	}
}

static void n_bitx_ix(CPU* cpu) {n_bitx(cpu, cpu->regIX);}
static void n_bitx_iy(CPU* cpu) {n_bitx(cpu, cpu->regIY);}

// in r,(c): bits 3 and 5 are left as they were
static unsigned char n_in(CPU* cpu) {
	cpu->flgFW = 1;
	cpu->regWZ = cpu->regBC;
	unsigned char v = z80_iord(cpu, cpu->regWZ++);
	cpu->flgS = !!(v & 0x80);
	cpu->flgZ = !v;
	cpu->flgH = 0;
	cpu->flgPV = parity(v);
	cpu->flgN = 0;
	return v;
}

static void n_ed40(CPU* cpu) {cpu->regB = n_in(cpu);}
static void n_ed48(CPU* cpu) {cpu->regC = n_in(cpu);}
static void n_ed50(CPU* cpu) {cpu->regD = n_in(cpu);}
static void n_ed58(CPU* cpu) {cpu->regE = n_in(cpu);}
static void n_ed60(CPU* cpu) {cpu->regH = n_in(cpu);}
static void n_ed68(CPU* cpu) {cpu->regL = n_in(cpu);}
static void n_ed70(CPU* cpu) {n_in(cpu);}
static void n_ed78(CPU* cpu) {cpu->regA = n_in(cpu);}

// ld a,i / ld a,r: bits 3 and 5 left as they were; P/V is IFF2 as read, with
// no interrupt-right-after race to clear it
static void n_ld_air(CPU* cpu, unsigned char v) {
	cpu->flgFW = 1;
	cpu->regA = v;
	cpu->flgS = !!(v & 0x80);
	cpu->flgZ = !v;
	cpu->flgH = 0;
	cpu->flgPV = cpu->flgIFF2;
	cpu->flgN = 0;
}

static void n_ed57(CPU* cpu) {n_ld_air(cpu, cpu->regI);}
static void n_ed5F(CPU* cpu) {n_ld_air(cpu, (cpu->regR & 0x7f) | (cpu->regR7 & 0x80));}

// the block i/o instructions: F is that of an 8-bit SUB B,1, whatever the data
static void n_dec_b(CPU* cpu) {
	unsigned char b = cpu->regB;
	unsigned char r = b - 1;
	cpu->flgFW = 1;
	n_szxy(cpu, r);
	cpu->flgH = !(b & 0x0f);		// borrow from bit 4
	cpu->flgPV = (b == 0x80);		// overflow
	cpu->flgN = 1;
	cpu->flgC = (b == 0x00);		// borrow
	cpu->regB = r;
}

// ini	5 4in 3wr
static void n_edA2(CPU* cpu) {
	cpu->regWZ = cpu->regBC + 1;
	cpu->tmp = z80_iord(cpu, cpu->regBC);
	z80_mwr(cpu, cpu->regHL++, cpu->tmp);
	n_dec_b(cpu);
}

// ind
static void n_edAA(CPU* cpu) {
	cpu->regWZ = cpu->regBC - 1;
	cpu->tmp = z80_iord(cpu, cpu->regBC);
	z80_mwr(cpu, cpu->regHL--, cpu->tmp);
	n_dec_b(cpu);
}

// outi	5 3rd 4wr: B is decremented before it goes out
static void n_edA3(CPU* cpu) {
	cpu->tmp = z80_mrd(cpu, cpu->regHL);
	n_dec_b(cpu);
	cpu->regWZ = cpu->regBC + 1;
	z80_iowr(cpu, cpu->regBC, cpu->tmp);
	cpu->regHL++;
}

// outd
static void n_edAB(CPU* cpu) {
	cpu->tmp = z80_mrd(cpu, cpu->regHL);
	n_dec_b(cpu);
	cpu->regWZ = cpu->regBC - 1;
	z80_iowr(cpu, cpu->regBC, cpu->tmp);
	cpu->regHL--;
}

// a block instruction going round again: five more ticks with adr on the bus,
// and the flags stay those of the step (no PC-derived bits as on a Zilog)
static void n_repeat(CPU* cpu, int adr) {
	cpu->regPC -= 2;
	z80_wait(cpu, adr, 5);
	cpu->regWZ = cpu->regPC + 1;
}

static void n_edB2(CPU* cpu) {n_edA2(cpu); if (cpu->regB) n_repeat(cpu, cpu->regHL - 1);}
static void n_edBA(CPU* cpu) {n_edAA(cpu); if (cpu->regB) n_repeat(cpu, cpu->regHL + 1);}
// otir/otdr go round at the address of the byte just put out, not at BC
static void n_edB3(CPU* cpu) {n_edA3(cpu); if (cpu->regB) n_repeat(cpu, cpu->regHL - 1);}
static void n_edBB(CPU* cpu) {n_edAB(cpu); if (cpu->regB) n_repeat(cpu, cpu->regHL + 1);}

// cpi/cpd: bits 3 and 5 from the byte compared, as cp n does
static void n_cpx(CPU* cpu, int dir) {
	cpu->flgFW = 1;
	cpu->tmpb = z80_mrd(cpu, cpu->regHL);
	cpu->tmpw = cpu->regA - cpu->tmpb;
	cpu->tmp = ((cpu->regA & 0x08) >> 3) | ((cpu->tmpb & 0x08) >> 2) | ((cpu->tmpw & 0x08) >> 1);
	z80_wait(cpu, cpu->regHL, 5);
	cpu->regHL += dir;
	cpu->regBC--;
	cpu->flgS = !!(cpu->tmpw & 0x80);
	cpu->flgZ = !(cpu->tmpw & 0xff);
	cpu->flgH = !!FHsubTab[cpu->tmp];
	cpu->flgPV = !!cpu->regBC;
	cpu->flgN = 1;
	cpu->flgF5 = !!(cpu->tmpb & 0x20);
	cpu->flgF3 = !!(cpu->tmpb & 0x08);
	cpu->regWZ += dir;
}

static void n_edA1(CPU* cpu) {n_cpx(cpu, 1);}
static void n_edA9(CPU* cpu) {n_cpx(cpu, -1);}
static void n_edB1(CPU* cpu) {n_cpx(cpu, 1); if (cpu->flgPV && !cpu->flgZ) n_repeat(cpu, cpu->regHL - 1);}
static void n_edB9(CPU* cpu) {n_cpx(cpu, -1); if (cpu->flgPV && !cpu->flgZ) n_repeat(cpu, cpu->regHL + 1);}

// ldi/ldd as on a Zilog, and their flags; only the repeat differs
void edA0(CPU*);
void edA8(CPU*);
static void n_edB0(CPU* cpu) {edA0(cpu); if (cpu->regBC) n_repeat(cpu, cpu->regDE - 1);}
static void n_edB8(CPU* cpu) {edA8(cpu); if (cpu->regBC) n_repeat(cpu, cpu->regDE + 1);}

// im 0/1/2: the Next's interrupt controller snoops the mode off the ED fetch, but
// not one that comes after a DD/FD prefix (im2_control.vhd 200-227)
static void n_im0(CPU* cpu) {ed46(cpu); cpu->regIMN = 0;}
static void n_im1(CPU* cpu) {ed56(cpu); cpu->regIMN = 1;}
static void n_im2(CPU* cpu) {ed5E(cpu); cpu->regIMN = 2;}

// == the Next's own opcodes ==

// ED 23 swapnib
static void n_swapnib(CPU* cpu) {
	cpu->regA = (cpu->regA << 4) | (cpu->regA >> 4);
}

// ED 24 mirror a
static void n_mirror(CPU* cpu) {
	unsigned char v = cpu->regA;
	unsigned char r = 0;
	for (int i = 0; i < 8; i++) {
		r = (r << 1) | (v & 1);
		v >>= 1;
	}
	cpu->regA = r;
}

// ED 27 test n	4 4 3rd: the flags of and a,n, A kept
static void n_test(CPU* cpu) {
	unsigned char v = cpu->regA & z80_mrd(cpu, cpu->regPC++);
	cpu->flgFW = 1;
	n_szxy(cpu, v);
	cpu->flgH = 1;
	cpu->flgPV = parity(v);
	cpu->flgN = 0;
	cpu->flgC = 0;
}

// ED 28..2C barrel shifts of DE by B(4:0); B(7:5) ignored, F untouched
static void n_bsla(CPU* cpu) {
	int n = cpu->regB & 0x1f;
	cpu->regDE = (n > 15) ? 0 : (cpu->regDE << n);
}

static void n_bsra(CPU* cpu) {
	int n = cpu->regB & 0x1f;
	signed short v = (signed short)cpu->regDE;
	cpu->regDE = (unsigned short)(v >> ((n > 15) ? 15 : n));
}

static void n_bsrl(CPU* cpu) {
	int n = cpu->regB & 0x1f;
	cpu->regDE = (n > 15) ? 0 : (cpu->regDE >> n);
}

static void n_bsrf(CPU* cpu) {
	int n = cpu->regB & 0x1f;
	cpu->regDE = (n > 15) ? 0xffff : (unsigned short)~((unsigned short)~cpu->regDE >> n);
}

static void n_brlc(CPU* cpu) {
	int n = cpu->regB & 0x0f;
	cpu->regDE = (unsigned short)((cpu->regDE << n) | (cpu->regDE >> ((16 - n) & 15)));
}

// ED 30 mul d,e
static void n_mul(CPU* cpu) {
	cpu->regDE = cpu->regD * cpu->regE;
}

// ED 31..33 add rr,a: C is cleared, the other flags kept
static void n_add_hl_a(CPU* cpu) {cpu->regHL += cpu->regA; cpu->flgC = 0; cpu->flgFW = 1;}
static void n_add_de_a(CPU* cpu) {cpu->regDE += cpu->regA; cpu->flgC = 0; cpu->flgFW = 1;}
static void n_add_bc_a(CPU* cpu) {cpu->regBC += cpu->regA; cpu->flgC = 0; cpu->flgFW = 1;}

// ED 34..36 add rr,nn	4 4 4rd 4rd: no flags
static unsigned short n_imm16x(CPU* cpu) {
	xpair p;
	p.l = z80_mrd(cpu, cpu->regPC++);
	z80_wait(cpu, cpu->regPC - 1, 1);
	p.h = z80_mrd(cpu, cpu->regPC++);
	z80_wait(cpu, cpu->regPC - 1, 1);
	return p.w;
}

static void n_add_hl_nn(CPU* cpu) {cpu->regHL += n_imm16x(cpu);}
static void n_add_de_nn(CPU* cpu) {cpu->regDE += n_imm16x(cpu);}
static void n_add_bc_nn(CPU* cpu) {cpu->regBC += n_imm16x(cpu);}

// ED 8A push nn	4 4 3rd 3wr 3rd 3wr 3rd: the operand is high byte first,
// each byte pushed as it is read, then one more read of the low byte
static void n_push_nn(CPU* cpu) {
	unsigned char h = z80_mrd(cpu, cpu->regPC++);
	z80_mwr(cpu, --cpu->regSP, h);
	unsigned char l = z80_mrd(cpu, cpu->regPC++);
	z80_mwr(cpu, --cpu->regSP, l);
	z80_mrd(cpu, cpu->regPC - 1);
}

// ED 90 outinb	4 5 3rd 4out: out (bc),(hl), hl+1; B kept, no flags
static void n_outinb(CPU* cpu) {
	cpu->tmp = z80_mrd(cpu, cpu->regHL);
	z80_iowr(cpu, cpu->regBC, cpu->tmp);
	cpu->regHL++;
}

// ED 91 nextreg r,v	4 4 3rd 3rd 3 3: the write is a side-band request at the
// start of the first of two dummy reads, not an i/o cycle
static void n_nextreg(CPU* cpu, int reg, int val) {
	if (cpu->xnextreg)
		cpu->xnextreg(cpu->xptr, reg & 0xff, val & 0xff);
	z80_mrd(cpu, cpu->regPC - 1);
	z80_mrd(cpu, cpu->regPC - 1);
}

static void n_nextreg_nn(CPU* cpu) {
	int reg = z80_mrd(cpu, cpu->regPC++);
	int val = z80_mrd(cpu, cpu->regPC++);
	n_nextreg(cpu, reg, val);
}

// ED 92 nextreg r,a	4 4 3rd 3 3
static void n_nextreg_a(CPU* cpu) {
	int reg = z80_mrd(cpu, cpu->regPC++);
	n_nextreg(cpu, reg, cpu->regA);
}

// ED 93 pixeldn: down one pixel line in the ULA screen; 5FFF wraps to 401F
static void n_pixeldn(CPU* cpu) {
	unsigned char h = cpu->regH;
	unsigned char l = cpu->regL;
	unsigned char v = (((h >> 3) & 3) << 6) | ((l & 0xe0) >> 2) | (h & 7);
	v++;
	cpu->regH = (h & 0xe0) | ((v >> 6) << 3) | (v & 7);
	cpu->regL = ((v << 2) & 0xe0) | (l & 0x1f);
}

// ED 94 pixelad: the ULA screen address of (x = E, y = D)
static void n_pixelad(CPU* cpu) {
	unsigned char d = cpu->regD;
	unsigned char e = cpu->regE;
	cpu->regH = 0x40 | ((d >> 3) & 0x18) | (d & 7);
	cpu->regL = ((d << 2) & 0xe0) | (e >> 3);
}

// ED 95 setae: A = the pixel mask of x = E
static void n_setae(CPU* cpu) {
	cpu->regA = 0x80 >> (cpu->regE & 7);
}

// ED 98 jp (c)	4 4 5in: PC(13:6) = the byte read from port BC, PC(5:0) = 0,
// PC(15:14) those of the address after the instruction
static void n_jp_c(CPU* cpu) {
	unsigned char v = z80_iord(cpu, cpu->regBC);
	z80_wait(cpu, cpu->regBC, 1);
	cpu->regPC = (cpu->regPC & 0xc000) | (v << 6);
}

// the ldix family: (DE) = (HL) unless the byte equals A, when the write cycle is
// a read of (DE) instead; DE goes up in all of them. The flags are LDI's.
static void n_ldx(CPU* cpu, unsigned char v) {
	if (v != cpu->regA) {
		z80_mwr(cpu, cpu->regDE, v);
	} else {
		z80_mrd(cpu, cpu->regDE);
	}
	z80_wait(cpu, cpu->regDE, 2);
	cpu->regDE++;
	cpu->regBC--;
	v += cpu->regA;
	cpu->flgFW = 1;
	cpu->flgF5 = !!(v & 2);
	cpu->flgH = 0;
	cpu->flgF3 = !!(v & 8);
	cpu->flgPV = !!cpu->regBC;
	cpu->flgN = 0;
}

// ED A4 ldix	4 4 3rd 5wr
static void n_ldix(CPU* cpu) {
	n_ldx(cpu, z80_mrd(cpu, cpu->regHL++));
}

// ED AC lddx
static void n_lddx(CPU* cpu) {
	n_ldx(cpu, z80_mrd(cpu, cpu->regHL--));
}

// ED B4 ldirx: goes round at DE, as ldir does
static void n_ldirx(CPU* cpu) {
	n_ldix(cpu);
	if (cpu->regBC) n_repeat(cpu, cpu->regDE - 1);
}

// ED B6, decoded as ldirx; it, lddrx and ldpirx go round at the address of the next
// instruction (their last M-cycle leaves PC on the bus)
static void n_ldirx_b6(CPU* cpu) {
	n_ldix(cpu);
	if (cpu->regBC) n_repeat(cpu, cpu->regPC);
}

// ED BC lddrx
static void n_lddrx(CPU* cpu) {
	n_lddx(cpu);
	if (cpu->regBC) n_repeat(cpu, cpu->regPC);
}

// ED B7 ldpirx: the byte from (HL & FFF8) + (DE & 7), HL never moves. Bits 3 and
// 1 of (B after the step OR the byte) give the flags' bits 3 and 5.
static void n_ldpirx(CPU* cpu) {
	unsigned char v = z80_mrd(cpu, (cpu->regHL & 0xfff8) | (cpu->regDE & 7));
	if (v != cpu->regA) {
		z80_mwr(cpu, cpu->regDE, v);
	} else {
		z80_mrd(cpu, cpu->regDE);
	}
	z80_wait(cpu, cpu->regDE, 2);
	cpu->regDE++;
	cpu->regBC--;
	unsigned char f = cpu->regB | v;
	cpu->flgFW = 1;
	cpu->flgF5 = !!(f & 2);
	cpu->flgH = 0;
	cpu->flgF3 = !!(f & 8);
	cpu->flgPV = !!cpu->regBC;
	cpu->flgN = 0;
	if (cpu->regBC) n_repeat(cpu, cpu->regPC);
}

// ED A5 ldws	4 4 3rd 3wr: (DE) = (HL), L+1 in its page, D+1, F as add D,1
static void n_ldws(CPU* cpu) {
	unsigned char v = z80_mrd(cpu, cpu->regHL);
	z80_mwr(cpu, cpu->regDE, v);
	cpu->regL++;
	unsigned char d = cpu->regD;
	unsigned char r = d + 1;
	cpu->flgFW = 1;
	n_szxy(cpu, r);
	cpu->flgH = ((d & 0x0f) == 0x0f);
	cpu->flgPV = (d == 0x7f);
	cpu->flgN = 0;
	cpu->flgC = (d == 0xff);
	cpu->regD = r;
}

// == building the tables ==

static opCode* n_map(opCode* tab) {
	if (tab == npTab) return n_np;
	if (tab == cbTab) return n_cb;
	if (tab == edTab) return n_ed;
	if (tab == ddTab) return n_dd;
	if (tab == fdTab) return n_fd;
	if (tab == ddcbTab) return n_ddcb;
	if (tab == fdcbTab) return n_fdcb;
	return tab;
}

static void n_set(opCode* tab, int code, int flag, int t, cbcpu exec, const char* mnem) {
	tab[code].flag = flag;
	tab[code].t = t;
	tab[code].exec = exec;
	tab[code].tab = NULL;
	tab[code].mnem = mnem;
}

static void n_build(void) {
	static int done = 0;
	if (done) return;
	done = 1;
	opCode* const n_all[] = {n_np, n_cb, n_ed, n_dd, n_fd, n_ddcb, n_fdcb};
	memcpy(n_np, npTab, sizeof(n_np));
	memcpy(n_cb, cbTab, sizeof(n_cb));
	memcpy(n_ed, edTab, sizeof(n_ed));
	memcpy(n_dd, ddTab, sizeof(n_dd));
	memcpy(n_fd, fdTab, sizeof(n_fd));
	memcpy(n_ddcb, ddcbTab, sizeof(n_ddcb));
	memcpy(n_fdcb, fdcbTab, sizeof(n_fdcb));
	for (int i = 0; i < 7; i++) {
		for (int c = 0; c < 256; c++) {
			opCode* op = &n_all[i][c];
			op->tab = n_map(op->tab);
			// scf/ccf wherever they are (the dd/fd tables have their own rows)
			if (op->exec == npr37) op->exec = n_scf;
			if (op->exec == npr3F) op->exec = n_ccf;
		}
	}
	for (int b = 0; b < 8; b++)
		n_cb[0x46 | (b << 3)].exec = n_bit_hl;
	for (int c = 0x40; c < 0x80; c++) {
		n_ddcb[c].exec = n_bitx_ix;
		n_fdcb[c].exec = n_bitx_iy;
	}
	n_ed[0x40].exec = n_ed40;
	n_ed[0x48].exec = n_ed48;
	n_ed[0x50].exec = n_ed50;
	n_ed[0x58].exec = n_ed58;
	n_ed[0x60].exec = n_ed60;
	n_ed[0x68].exec = n_ed68;
	n_ed[0x70].exec = n_ed70;
	n_ed[0x78].exec = n_ed78;
	n_ed[0x57].exec = n_ed57;
	n_ed[0x5F].exec = n_ed5F;
	n_ed[0xA1].exec = n_edA1;
	n_ed[0xA2].exec = n_edA2;
	n_ed[0xA3].exec = n_edA3;
	n_ed[0xA9].exec = n_edA9;
	n_ed[0xAA].exec = n_edAA;
	n_ed[0xAB].exec = n_edAB;
	n_ed[0xB0].exec = n_edB0;
	n_ed[0xB1].exec = n_edB1;
	n_ed[0xB2].exec = n_edB2;
	n_ed[0xB3].exec = n_edB3;
	n_ed[0xB8].exec = n_edB8;
	n_ed[0xB9].exec = n_edB9;
	n_ed[0xBA].exec = n_edBA;
	n_ed[0xBB].exec = n_edBB;
	// the Next's opcodes; t is the ED byte's M1 with any internal ticks after it
	n_set(n_ed, 0x23, 0, 4, n_swapnib, "swapnib");
	n_set(n_ed, 0x24, 0, 4, n_mirror, "mirror a");
	n_set(n_ed, 0x27, 0, 4, n_test, "test :1");
	n_set(n_ed, 0x28, 0, 4, n_bsla, "bsla de,b");
	n_set(n_ed, 0x29, 0, 4, n_bsra, "bsra de,b");
	n_set(n_ed, 0x2A, 0, 4, n_bsrl, "bsrl de,b");
	n_set(n_ed, 0x2B, 0, 4, n_bsrf, "bsrf de,b");
	n_set(n_ed, 0x2C, 0, 4, n_brlc, "brlc de,b");
	n_set(n_ed, 0x30, 0, 4, n_mul, "mul d,e");
	n_set(n_ed, 0x31, 0, 4, n_add_hl_a, "add hl,a");
	n_set(n_ed, 0x32, 0, 4, n_add_de_a, "add de,a");
	n_set(n_ed, 0x33, 0, 4, n_add_bc_a, "add bc,a");
	n_set(n_ed, 0x34, 0, 4, n_add_hl_nn, "add hl,:2");
	n_set(n_ed, 0x35, 0, 4, n_add_de_nn, "add de,:2");
	n_set(n_ed, 0x36, 0, 4, n_add_bc_nn, "add bc,:2");
	n_set(n_ed, 0x8A, 0, 4, n_push_nn, "push :6");
	n_set(n_ed, 0x90, 0, 5, n_outinb, "outinb");
	n_set(n_ed, 0x91, 0, 4, n_nextreg_nn, "nextreg :1,:1");
	n_set(n_ed, 0x92, 0, 4, n_nextreg_a, "nextreg :1,a");
	n_set(n_ed, 0x93, 0, 4, n_pixeldn, "pixeldn");
	n_set(n_ed, 0x94, 0, 4, n_pixelad, "pixelad");
	n_set(n_ed, 0x95, 0, 4, n_setae, "setae");
	n_set(n_ed, 0x98, 0, 4, n_jp_c, "jp (c)");
	n_set(n_ed, 0xA4, 0, 4, n_ldix, "ldix");
	n_set(n_ed, 0xA5, 0, 4, n_ldws, "ldws");
	n_set(n_ed, 0xAC, 0, 4, n_lddx, "lddx");
	n_set(n_ed, 0xB4, OF_SKIPABLE, 4, n_ldirx, "ldirx");
	n_set(n_ed, 0xB6, OF_SKIPABLE, 4, n_ldirx_b6, "ldirx *");	// LDIRSCALE in the core: decoded, does what ldirx does
	n_set(n_ed, 0xB7, OF_SKIPABLE, 4, n_ldpirx, "ldpirx");
	n_set(n_ed, 0xBC, OF_SKIPABLE, 4, n_lddrx, "lddrx");
	// the IM snoop: on the plain ED table only
	memcpy(n_edx, n_ed, sizeof(n_edx));
	for (int c = 0; c < 256; c++) {
		if (n_ed[c].exec == ed46) n_ed[c].exec = n_im0;
		if (n_ed[c].exec == ed56) n_ed[c].exec = n_im1;
		if (n_ed[c].exec == ed5E) n_ed[c].exec = n_im2;
	}
	n_dd[0xED].tab = n_edx;
	n_fd[0xED].tab = n_edx;
}

void z80n_init(CPU* cpu) {
	n_build();
	cpu->tabs = &z80n_tabs;
}

void z80n_reset(CPU* cpu) {
	z80n_init(cpu);
	z80_reset(cpu);
	cpu->regIMN = 0;
}

xAsmScan z80n_asm(int adr, const char* cbuf, char* buf) {
	n_build();
	return z80_asm_tabs(&z80n_tabs, adr, cbuf, buf);
}
