#pragma once

// divMMC: an SD card interface on the Spectrum's edge connector. 8K of EEPROM and
// 128K (or 512K) of SRAM paged over #0000-#3FFF by port #E3 and by an automapper
// that watches the opcode fetches; two card slots on an SPI port (#E7 selects,
// #EB moves a byte). The model is the board Mario Prato published (CPLD source
// divmmc.vhd v1.0, 2012) that the divMMC EnJOY! boards are built on.

#ifdef __cplusplus
extern "C" {
#endif

#include "memory.h"
#include "sdcard.h"

#define DIV_EE_SIZE	0x2000
#define DIV_RAM_MAX	0x80000
#define DIV_PAGES_MAX	(DIV_RAM_MAX >> 13)

// models
enum {
	DIV_NONE = 0,
	DIV_ENJOY		// divMMC EnJOY! (Prato's logic)
};

// SPI engines (the board's own, its clock and what a read sends)
enum {
	DIV_SPI_PRATO = 0	// the CPLD's, on the Spectrum's clock: 1 T to sample, 16 T a byte
};

typedef struct {
	int model;		// DIV_NONE: not fitted
	int spi;		// DIV_SPI_*
	int pages;		// 8K SRAM pages fitted: 16 or 64
	int eeWrite;		// jumper J1 open: the EEPROM takes writes, automap needs MAPRAM
	int tWC;		// the EEPROM's write cycle, us

	// port #E3, cleared at power-on only
	int conmem;
	int mapram;
	int bank;		// all six bits
	// the automapper: two flip-flops on the opcode fetches, cleared at power-on only
	int mapcond;
	int automap;
	int shown;		// what the memory map shows now: 0 the machine, 1 divMMC

	// SPI: #E7 bits 1,0 = /CS of card 1,0; the engine (div_spi in divmmc.c)
	int cs;
	long long spiS;		// the bus clock edge the transfer in progress started at
	int spiRes;		// what the card gave in it
	int spiLat;		// what an IN reads
	int spiNxt;		// what an IN sends: what the last transfer left in the shifter

	// the NMI button: /NMI is pulled while it is held and mapcond is low
	int nmiBtn;
	int nmiLine;

	// the EEPROM's own state (AT28C64B)
	struct {
		long long loadEnd;	// the page load window closes (us * 1000, ns)
		long long busyEnd;	// the internal write ends (ns)
		int busy;		// a page is loaded or being written
		int lastAdr;
		int lastVal;
		int toggle;		// I/O6 while busy
		int sdp;		// software data protection on
		int seq;		// how far a command sequence has got
		int arm;		// SDP: the 3-byte prefix seen, this load may write
		int n;			// bytes latched
		unsigned short adr[64];
		unsigned char val[64];
	} ee;
	int eeDirty;		// written since the image was saved

	unsigned char data[DIV_EE_SIZE + DIV_RAM_MAX];	// the EEPROM, then the SRAM
	SDCard* card[2];

	// the machine: its memory map, and its clocks
	Memory* mem;
	void* host;
	long long (*edge)(void*);	// bus clock edges since the machine started (no wait states)
	long long (*ns)(void*);		// time, ns
} DivMMC;

DivMMC* div_create(Memory* mem, void* host, long long (*edge)(void*), long long (*ns)(void*));
void div_destroy(DivMMC*);
void div_set_model(DivMMC*, int model, int pages);

void div_power_on(DivMMC*);	// the supply: registers, flip-flops and the cards
void div_reset(DivMMC*);	// /RESET: card selects and the SPI engine only

// an opcode fetch at adr, before its byte is read: 1 when the map changed
int div_m1(DivMMC*, int adr);
// whether a fetch at adr would change anything (the fast paths ask first)
int div_m1_acts(DivMMC*, int adr);
// a delayed switch waits for the next fetch
static inline int div_pending(DivMMC* div) {return div->model && (div->mapcond != div->automap);}

// ports, by A7..A0: 1 when the port is the board's (*val: what an IN reads)
int div_out(DivMMC*, int port, int val);
int div_in(DivMMC*, int port, int* val);

// the overlay over #0000-#3FFF as the registers say, after any change
void div_map(DivMMC*);
// the opcode-fetch pages that must reach div_m1 (#00, #04, #05, #1F, #3D)
int div_trap_page(int page);

// the button, held or not, looked at as the machine runs: 1 when /NMI has just
// gone low (an NMI to take). The line follows mapcond too.
int div_nmi(DivMMC*, int held);

// the EEPROM's 8K from a file (a shorter one leaves the rest erased) and to one
int div_load_eeprom(DivMMC*, const char* path);
int div_save_eeprom(DivMMC*, const char* path);
void div_erase_eeprom(DivMMC*);
int div_eeprom_erased(DivMMC*);

#ifdef __cplusplus
}
#endif
