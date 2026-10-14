// divMMC, after Mario Prato's CPLD source (divmmc.vhd v1.0, 2012-11-22) and his
// 2013 schematic. The names below are the source's: mapcond and automap are its
// two automapper flip-flops, conmem/mapram/bank its port #E3 register.

#include "divmmc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DIV_NO_XFER	(-(1LL << 60))	// the start of a transfer long gone

DivMMC* div_create(Memory* mem, void* host, long long (*edge)(void*), long long (*ns)(void*)) {
	DivMMC* div = (DivMMC*)calloc(1, sizeof(DivMMC));
	if (!div) return NULL;
	div->mem = mem;
	div->host = host;
	div->edge = edge;
	div->ns = ns;
	div->pages = 16;
	div->tWC = 10000;		// AT28C64B: 10 ms at most, the only figure given
	memset(div->data, 0xff, DIV_EE_SIZE);	// an erased EEPROM
	div->card[0] = sdc_create_spec();
	div->card[1] = sdc_create_spec();
	div_power_on(div);
	return div;
}

void div_destroy(DivMMC* div) {
	if (!div) return;
	sdcDestroy(div->card[0]);
	sdcDestroy(div->card[1]);
	free(div);
}

void div_set_model(DivMMC* div, int model, int pages) {
	div->model = model;
	div->pages = (pages > 16) ? DIV_PAGES_MAX : 16;
	for (int i = 0; i < 0x40; i++)
		if (div_trap_page(i)) mem_set_m1trap(div->mem, i, model != DIV_NONE);
	div_map(div);
}

// the SPI engine and the card selects: what /RESET clears
static void div_spi_reset(DivMMC* div) {
	div->cs = 3;
	div->card[0]->cs = 1;
	div->card[1]->cs = 1;
	div->spiS = DIV_NO_XFER;
	div->spiRes = 0xff;
	div->spiLat = 0xff;
	div->spiNxt = 0xff;
}

void div_reset(DivMMC* div) {
	div_spi_reset(div);
}

// Power-on: an RC on the board clears the register; the CPLD's flip-flops come
// up 0. The SRAM's content is not defined by anything - it is taken as zeros. The
// EEPROM keeps what it holds, and the cards come up as cards do.
void div_power_on(DivMMC* div) {
	div->conmem = 0;
	div->mapram = 0;
	div->bank = 0;
	div->mapcond = 0;
	div->automap = 0;
	div->nmiBtn = 0;
	div->nmiLine = 0;
	memset(&div->ee, 0, sizeof(div->ee));
	memset(div->data + DIV_EE_SIZE, 0x00, DIV_RAM_MAX);
	sdc_power_on(div->card[0]);
	sdc_power_on(div->card[1]);
	div_spi_reset(div);
	div_map(div);
}

// EEPROM (28C64 class: Atmel AT28C64B). A write latches the byte and opens a
// page load window of 150 us; when no write comes in it, the internal write
// starts and takes tWC. Until then every read is a status: bit 7 the inverse of
// the last byte written (DATA polling), bit 6 toggling (toggle bit). Software
// data protection: AA>1555 55>0AAA A0>1555 turns it on and lets that load write;
// AA 55 80 AA 55 20 turns it off. With it on, a load without the prefix starts
// the timer and writes nothing.

#define EE_TBLC_NS	150000LL

static void div_map_ee(DivMMC* div);

// the write cycle as far as time has got: a page written at its end
static void div_ee_run(DivMMC* div) {
	if (!div->ee.busy) return;
	long long now = div->ns(div->host);
	if (!div->ee.busyEnd) {
		if (now < div->ee.loadEnd) return;
		div->ee.busyEnd = div->ee.loadEnd + (long long)div->tWC * 1000;
	}
	if (now < div->ee.busyEnd) return;
	if (!div->ee.sdp || div->ee.arm) {
		for (int i = 0; i < div->ee.n; i++)
			div->data[div->ee.adr[i] & 0x1fff] = div->ee.val[i];
		if (div->ee.n) div->eeDirty = 1;
	}
	div->ee.busy = 0;
	div->ee.busyEnd = 0;
	div->ee.arm = 0;
	div->ee.n = 0;
	div_map_ee(div);		// plain reads again
}

static int div_ee_rd(int adr, void* data) {
	DivMMC* div = (DivMMC*)data;
	div_ee_run(div);
	if (!div->ee.busy)
		return div->data[adr & 0x1fff];
	div->ee.toggle ^= 1;
	return ((~div->ee.lastVal) & 0x80) | (div->ee.toggle << 6) | (div->ee.lastVal & 0x3f);
}

// the command sequences, one byte at a time
static void div_ee_seq(DivMMC* div, int adr, int val) {
	static const unsigned short sa[6] = {0x1555, 0x0aaa, 0x1555, 0x1555, 0x0aaa, 0x1555};
	static const unsigned char sv[6] = {0xaa, 0x55, 0x80, 0xaa, 0x55, 0x20};
	int s = div->ee.seq;
	adr &= 0x1fff;
	if ((s == 2) && (adr == 0x1555) && (val == 0xa0)) {	// protect, and this load writes
		div->ee.sdp = 1;
		div->ee.arm = 1;
		div->ee.n -= 3;
		div->ee.seq = 0;
		return;
	}
	if ((adr == sa[s]) && (val == sv[s])) {
		if (++div->ee.seq == 6) {			// unprotect
			div->ee.sdp = 0;
			div->ee.n -= 6;
			div->ee.seq = 0;
		}
		return;
	}
	div->ee.seq = ((adr == 0x1555) && (val == 0xaa)) ? 1 : 0;
}

static void div_ee_wr(int adr, int val, void* data) {
	DivMMC* div = (DivMMC*)data;
	div_ee_run(div);
	long long now = div->ns(div->host);
	if (div->ee.busy && div->ee.busyEnd) return;	// writing: the chip takes nothing
	if (!div->ee.busy) {
		div->ee.busy = 1;
		div->ee.n = 0;
		div->ee.arm = 0;
		div_map_ee(div);	// status reads from now on
	}
	if (div->ee.n < 64) {
		div->ee.adr[div->ee.n] = adr & 0x1fff;
		div->ee.val[div->ee.n] = val & 0xff;
		div->ee.n++;
	}
	if (div->ee.n < 0) div->ee.n = 0;
	div->ee.lastAdr = adr & 0x1fff;
	div->ee.lastVal = val & 0xff;
	div->ee.loadEnd = now + EE_TBLC_NS;
	div_ee_seq(div, adr, val);
	if (div->ee.n < 0) div->ee.n = 0;
}

// Memory decode, all of it the CPLD's combinational logic on the register and
// automap (romcs, romoe, romwr, ramoe, ramwr, bankout):
//   conmem:                  EEPROM at #0000 (written with J1 open), RAM[bank] at #2000
//   automap, mapram:         RAM page 3 at #0000 read only, RAM[bank] at #2000,
//                            read only when bank is 3 (all six bits compared)
//   automap, J1 closed:      EEPROM at #0000 read only, RAM[bank] at #2000
//   otherwise (J1 open and no mapram too): the machine's own memory

static int div_shows(DivMMC* div) {
	if (!div->model) return 0;
	return div->conmem || (div->automap && (!div->eeWrite || div->mapram));
}

static void div_map_ee(DivMMC* div) {
	if (!div->shown || (!div->conmem && div->mapram)) return;
	int wr = div->conmem && div->eeWrite;
	for (int i = 0; i < 0x20; i++) {
		if (div->ee.busy || wr)
			mem_ovl_set(div->mem, i, MEM_DIV, i, (unsigned char*)div, div_ee_rd, wr ? div_ee_wr : NULL);
		else
			mem_ovl_set(div->mem, i, MEM_DIV, i, div->data + (i << 8), memStdRd, NULL);
	}
}

static void div_map_ram(DivMMC* div, int page, int pg, int wr) {
	int off = DIV_EE_SIZE + ((pg % div->pages) << 13);
	for (int i = 0; i < 0x20; i++)
		mem_ovl_set(div->mem, page + i, MEM_DIV, (off >> 8) + i, div->data + off + (i << 8), memStdRd, wr ? memStdWr : NULL);
}

void div_map(DivMMC* div) {
	int show = div_shows(div);
	if (!show) {
		if (div->shown)
			for (int i = 0; i < 0x40; i++) mem_ovl_clear(div->mem, i);
		div->shown = 0;
		return;
	}
	div->shown = 1;
	if (div->conmem || !div->mapram)
		div_map_ee(div);
	else
		div_map_ram(div, 0x00, 3, 0);
	div_map_ram(div, 0x20, div->bank, div->conmem || !div->mapram || (div->bank != 3));
}

// The automapper. At the falling /MREQ of every opcode fetch:
//   mapcond <= mapterm or map3DXX or (mapcond and not 1FF8-1FFF)
//   automap <= mapcond or map3DXX		(the old mapcond)
// The fetch's byte is read after that edge, so #3Dxx maps for its own fetch,
// an entry point (0000 0008 0038 0066 04C6 0562) for the next one, and a fetch
// in 1FF8-1FFF unmaps from the next one.

static int div_mapterm(int adr) {
	return (adr == 0x0000) || (adr == 0x0008) || (adr == 0x0038) || (adr == 0x0066) || (adr == 0x04c6) || (adr == 0x0562);
}

int div_trap_page(int page) {
	return (page == 0x00) || (page == 0x04) || (page == 0x05) || (page == 0x1f) || (page == 0x3d);
}

int div_m1_acts(DivMMC* div, int adr) {
	if (!div->model) return 0;
	adr &= 0xffff;
	int inst = ((adr & 0xff00) == 0x3d00);
	int cond = div_mapterm(adr) || inst || (div->mapcond && ((adr & 0xfff8) != 0x1ff8));
	int am = div->mapcond || inst;
	return (cond != div->mapcond) || (am != div->automap);
}

int div_m1(DivMMC* div, int adr) {
	if (!div->model) return 0;
	adr &= 0xffff;
	int inst = ((adr & 0xff00) == 0x3d00);
	int am = div->mapcond || inst;
	div->mapcond = div_mapterm(adr) || inst || (div->mapcond && ((adr & 0xfff8) != 0x1ff8));
	if (am == div->automap) return 0;
	div->automap = am;
	int was = div->shown;
	div_map(div);
	return was != div->shown;
}

// SPI. The engine runs on the CPU clock (the edge connector's, so a wait state
// that stops the clock stops it too). Timed from T1 of the I/O cycle, t, against
// S, the start of the transfer in progress:
//   OUT: lost when t-S <= 14, chained when 15..17 (S += 16), started at >= 18
//   IN:  starts nothing when t-S <= 15, starts at >= 16 (S = max(S+18, t))
// An IN reads the byte the transfer in progress got when t-S >= 16, otherwise the
// one before; it sends what the shifter was left with: the last byte written
// shifted out seven times with ones behind, so #7F after an even byte, else #FF.
// (Derived edge by edge from the CPLD's IDLE/SAMPLE/TRANSMIT states; the two
// edge races, an OUT at 15 and an IN at 16, are taken.)

static int div_card_xfer(DivMMC* div, int mosi) {
	int miso = 0xff;	// no card driving it: pulled up
	for (int i = 0; i < 2; i++)
		if (!(div->cs & (1 << i)))
			miso &= sdc_xfer(div->card[i], mosi);
	return miso;
}

static int div_spi(DivMMC* div, int out, int val) {
	long long t = div->edge(div->host);
	long long d = t - div->spiS;
	if (d >= 16) div->spiLat = div->spiRes;
	int ret = div->spiLat;
	if (out ? (d < 15) : (d < 16)) return ret;
	int send = out ? (val & 0xff) : div->spiNxt;
	div->spiRes = div_card_xfer(div, send);
	div->spiNxt = ((send & 1) << 7) | 0x7f;
	div->spiS = (d >= 18) ? t : (div->spiS + (out ? 16 : 18));
	return ret;
}

int div_out(DivMMC* div, int port, int val) {
	if (!div->model) return 0;
	switch (port & 0xff) {
		case 0xe3:
			div->bank = val & 0x3f;
			div->mapram |= (val >> 6) & 1;
			div->conmem = (val >> 7) & 1;
			div_map(div);
			return 1;
		case 0xe7:
			div->cs = val & 3;
			div->card[0]->cs = val & 1;
			div->card[1]->cs = (val >> 1) & 1;
			return 1;
		case 0xeb:
			div_spi(div, 1, val);
			return 1;
	}
	return 0;
}

int div_in(DivMMC* div, int port, int* val) {
	if (!div->model) return 0;
	if ((port & 0xff) != 0xeb) return 0;	// #E3 and #E7 are written only: the bus is left alone
	*val = div_spi(div, 0, 0xff);
	return 1;
}

int div_nmi(DivMMC* div, int held) {
	if (!div->model) return 0;
	div->nmiBtn = held ? 1 : 0;
	int line = div->nmiBtn && !div->mapcond;
	int fire = line && !div->nmiLine;
	div->nmiLine = line;
	return fire;
}

// the EEPROM image: 8K, a shorter file leaves the rest erased
int div_load_eeprom(DivMMC* div, const char* path) {
	FILE* f = fopen(path, "rb");
	if (!f) return 0;
	memset(div->data, 0xff, DIV_EE_SIZE);
	size_t n = fread(div->data, 1, DIV_EE_SIZE, f);
	fclose(f);
	div->eeDirty = 0;
	div_map(div);
	return n > 0;
}

int div_save_eeprom(DivMMC* div, const char* path) {
	FILE* f = fopen(path, "wb");
	if (!f) return 0;
	size_t n = fwrite(div->data, 1, DIV_EE_SIZE, f);
	fclose(f);
	if (n == DIV_EE_SIZE) div->eeDirty = 0;
	return n == DIV_EE_SIZE;
}
