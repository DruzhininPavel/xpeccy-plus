#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "defines.h"
#include <stddef.h>

// mempage type
enum {
	MEM_RAM	= 1,
	MEM_ROM,
	MEM_SLOT,
	MEM_EXT,
	MEM_IO
};

typedef struct {
    int type;
    int bank;
    int adr;
    int abs;
} xAdr;

typedef int(*extmrd)(int, void*);
typedef void(*extmwr)(int, int, void*);

typedef struct {
	int type;			// type of page (ram/rom/slt/io/ext...)
	int num;			// page number
	void* data;			// ptr for rd/wr func
	extmrd rd;			// external rd
	extmwr wr;			// external wr
} MemPage;

typedef struct {
	unsigned char* data;
	unsigned int size;
	unsigned int mask;
} xMemItem;

typedef struct {
	MemPage map[256];			// 256 x 256 | 256 x 64K
	// map[] as plain bytes, for the cpu to read and write straight (CPU.rdp/m1p/wrp):
	// a page memSetBank() gave memStdRd/memStdWr, NULL where its callback has to run.
	// Worked out from map[] (mem_derive), so no snapshot holds them.
	unsigned char* rdp[256];
	unsigned char* wrp[256];
	unsigned char* m1p[256];		// rdp less the rom at #3Dxx, whose fetch pages TR-DOS in
	unsigned char* m1dos[256];		// ...and less the ram, whose fetch pages it out
	unsigned char slow[256];		// ram pages in an odd 16K bank: where the ULA snows from
	short bank[256];			// the 16K bank of a ram page, -1 for any other
	unsigned char ramData[MEM_4M];		// 4M
	unsigned char romData[MEM_512K];	// 512K
	int ramSize;
	int ramMask;
	int romSize;
	int romMask;
	int pgsize;	// size of page in bytes
	int pgmask;	// number of LSBits in address = page offset (FF or FFFF)
	int pgshift;	// = log2(page size), 8 for 256-pages, 16 for 64K-pages
	int busmask;	// cpu addr bus mask (todo: move to CPU)
	// map[] pages in the ram the ULA contends: a ram page whose address has
	// contBit set (mem_set_cont). Kept by memSetBank(); the cpu only reports a
	// bus cycle to the machine where one of these is on the bus (CPU.contp).
	unsigned char cont[256];
	int contBit;
} Memory;

Memory* memCreate(void);
void memDestroy(Memory*);

int memRd(Memory*, int);
void memWr(Memory*, int, int);
int memStdRd(int, void*);		// the page callbacks memSetBank() gives plain ram and rom
void memStdWr(int, int, void*);

// A byte of a page, and the page an address is in. A page of plain ram or rom
// is read and written straight, which is what memStdRd/memStdWr would do.
static inline MemPage* mem_get_page(Memory* mem, int adr) {
	return &mem->map[(adr >> mem->pgshift) & 0xff];
}
static inline int mem_page_rd(MemPage* pg, int adr) {
	if (pg->rd == memStdRd)
		return ((unsigned char*)pg->data)[adr & 0xff];
	return pg->rd ? (pg->rd(adr & 0xffff, pg->data) & 0xff) : 0xff;
}
static inline void mem_page_wr(MemPage* pg, int adr, int val) {
	if (pg->wr == memStdWr)
		((unsigned char*)pg->data)[adr & 0xff] = val & 0xff;
	else if (pg->wr)
		pg->wr(adr, val, pg->data);
}
// Both calls in one: for the machines that bypass memRd/memWr - these have no
// -1 answer and no bus mask of their own, which is where the two differ.
static inline int mem_rd_fast(Memory* mem, int adr) {
	return mem_page_rd(mem_get_page(mem, adr), adr);
}
static inline void mem_wr_fast(Memory* mem, int adr, int val) {
	mem_page_wr(mem_get_page(mem, adr), adr, val);
}

void memSetSize(Memory*, int, int);
// How much of ramData a machine can reach. Not ramSize: memSetBank puts a page
// at ramData + (bank << pgshift & ramMask), and ZX48 runs with 64K of ram behind
// a 128K mask (see the hand-set ramMask in profiles.cpp), so its screen bank
// alone sits past ramSize.
size_t mem_ram_extent(Memory*);
void mem_cold_fill(Memory*, const unsigned char*, int, int);
void memSetBank(Memory* mem, int page, int type, int bank, int siz, extmrd rd, extmwr wr, void* data);
void mem_set_cont(Memory*, int bit);	// the address bit of a contended page, 0: none
void mem_derive(Memory*);		// rdp..slow and cont again, after map[] was written behind memSetBank()

void memPutData(Memory*,int,int,int,char*);

xAdr mem_get_xadr(Memory*, int);
int memFindAdr(Memory*, int, int);

void mem_set_bus(Memory*, int);
int mem_get_phys_adr(Memory*, int);

#ifdef __cplusplus
}
#endif
