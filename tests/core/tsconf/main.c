// Headless TSConf bench: runs a raw Z80 binary on libxpeccy's TSConf core
// and prints the mailbox the program fills in.
//
//	tsconf-bench <prog.bin|prog.spg> [frames]
//
// XPECCY_ROOT is the source tree, for config/roms/tsconf.rom (default: the
// current folder). The machine is set up by hand after res/machines/evo-tsconf.conf
// (layout 448x320, 4 MB, Beta disk, Nemo-Evo IDE): a change there does not reach it.
//
// A .bin is loaded at #8000 (RAM page 2), windows 1..3 hold pages 5,2,0,
// SP = #BF00, DI, IM 1; a .spg goes through the real loadSPG. The program
// reports through the mailbox at #BF00..#BF7F: byte 0 = #AA ends the run,
// bytes 1.. are printed.
//
// Environment: PIX="x,y ..." prints the palette index of dots of the last whole
// frame, PIXH the same in half dots, DUMP=f.ppm writes the frame, TSU=1 prints
// the TSU state, HIDE=mask hides layers (TSL_HIDE_*), STATE=1 prints a hash of
// the RAM, the CPU registers and the T count.

#include "../bench.h"
#include "filetypes/filetypes.h"
#include "xreplay.h"

static unsigned fnv(const unsigned char* p, size_t n) {
	unsigned h = 2166136261u;
	while (n--) h = (h ^ *p++) * 16777619u;
	return h;
}

static void state_line(Computer* comp, const char* tag) {
	cpuGetRegs(comp->cpu);		// F is made from the flags when asked for
	printf("%s: ram %08X pc %04X sp %04X af %04X t %lld\n", tag, fnv(comp->mem->ramData, MEM_4M),
		comp->cpu->regPC, comp->cpu->regSP, comp->cpu->regAF, comp->tickTotal);
}

// REDRAW="frames,t,mask,dbg,ref[,steps,poke]": run whole frames, then on to T t
// of the next one, and stop there as the debugger (dbg=1) or a pause (dbg=0) does;
// in the debugger, step on steps instructions. Then hide the mask and redraw
// (xreplay), or with ref=1 print what a run that had the mask hidden from the
// start shows. The pictures and the state of the two must match. poke=1 changes a
// byte of RAM before the redraw, which has to make it refuse.
static void redraw_test(Computer* comp, const char* spec) {
	int frames = 2, t = 0, mask = 0, dbg = 0, ref = 0, steps = 0, poke = 0;
	sscanf(spec, "%d,%d,%i,%d,%d,%d,%d", &frames, &t, &mask, &dbg, &ref, &steps, &poke);
	xreplay_on = !ref;
	if (ref) comp->vid->tsconf.hide = mask;
	int fstart = comp->vid->fcnt;
	while (comp->vid->fcnt - fstart < frames)
		compExec(comp);
	while (comp->frmtCount < t)
		compExec(comp);
	if (dbg) {		// as DebugWin::start()
		vid_dark_tail(comp->vid);
		comp->vid->debug = 1;
		comp->flgDBG = 1;
		while (steps-- > 0)
			compExec(comp);
	}
	if (poke) comp->mem->ramData[0x3fff] ^= 0xff;
	if (!ref) {
		state_line(comp, "BEFORE");
		comp->vid->tsconf.hide = mask;
		int done = xreplay_redraw(comp);
		printf("REDRAW: %s\n", done ? "done" : "refused");
		if (!done) printf("WHY  : %s\n", xreplay_why());
	}
	state_line(comp, "STATE");
	size_t drawn = comp->vid->ray.ptr - scrimg;
	// what is on screen: the debugger shows scrimg, a pause the last whole frame
	printf("PIC  : ray %d,%d scr-drawn %08X", comp->vid->ray.x, comp->vid->ray.y, fnv(scrimg, drawn));
	if (dbg) printf(" scr %08X\n", fnv(scrimg, bufSize));
	else printf(" buf %08X\n", fnv(bufimg, bufSize));
	const char* dump = getenv("DUMP");
	if (dump) dump_ppm(comp, dump);
}

int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "usage: tsconf-bench prog.bin [frames]\n");
		return 2;
	}
	int frames = (argc > 2) ? atoi(argv[2]) : 50;
	const char* root = getenv("XPECCY_ROOT");
	xhost_time_fixed = 1759665600;		// Sunday 2025-10-05 12:00 UTC: runs repeat

	Computer* comp = compCreate();
	if (!compSetHardware(comp, "TSConf")) {
		fprintf(stderr, "no TSConf core\n");
		return 1;
	}
	vLayout lay = {{448, 320}, {52, 48}, {88, 32}, {256, 192}, {0, 0}, 64};
	comp_set_layout(comp, &lay);
	bytesPerLine = comp->vid->full.x * 8;	// as emulwin.cpp sets it: a whole raster row per image row
	bufSize = bytesPerLine * comp->vid->full.y;
	memSetSize(comp->mem, MEM_4M, MEM_512K);
	if (!load_rom(comp, root ? root : ".", "tsconf.rom"))
		return 1;
	difSetHW(comp->dif, DIF_BDI);
	ide_set_type(comp->ide, IDE_NEMO_EVO);
	compSetBaseFrq(comp, 3.5);
	compReset(comp, RES_DEFAULT);

	size_t n = 0;
	unsigned char* dst = comp->mem->ramData + (2 << 14);
	const char* ext = strrchr(argv[1], '.');
	if (ext && !strcmp(ext, ".spg")) {		// through the real loader
		memset(dst + 0x3f00, 0, 0x100);
		int err = loadSPG(comp, argv[1], 0);
		if (err != ERR_OK) {
			printf("%s: loadSPG error %d\n", base_name(argv[1]), err);
			return 0;
		}
	} else {
		FILE* f = fopen(argv[1], "rb");
		if (!f) {
			fprintf(stderr, "can't open %s\n", argv[1]);
			return 1;
		}
		n = fread(dst, 1, 0x3f00, f);
		fclose(f);
		memset(dst + 0x3f00, 0, 0x100);
		comp->hw->out(comp, 0x11af, 5);
		comp->hw->out(comp, 0x12af, 2);
		comp->hw->out(comp, 0x13af, 0);
		cpu_start(comp);
	}

	const char* hide = getenv("HIDE");
	if (hide) comp->vid->tsconf.hide = (unsigned char)strtol(hide, NULL, 0);
	const char* redraw = getenv("REDRAW");
	if (redraw) {
		redraw_test(comp, redraw);
		compDestroy(comp);
		return 0;
	}
	int fstart = comp->vid->fcnt;
	while ((mbox_rd(comp, MBOX) != 0xaa) && (comp->vid->fcnt - fstart < frames))
		compExec(comp);
	printf("%s: %zu bytes, %s after %d frames, pc=%04X\n", base_name(argv[1]), n,
		(mbox_rd(comp, MBOX) == 0xaa) ? "done" : "TIMEOUT",
		comp->vid->fcnt - fstart, comp->cpu->regPC);
	mbox_print(comp);
	probe(comp, "PIX", "PIX  ", 8);
	probe(comp, "PIXH", "PIXH ", 4);
	printf("BRD  : %02X\n", comp->vid->brdcol);
	if (getenv("STATE")) state_line(comp, "STATE");
	if (getenv("DMALOG")) {
		unsigned cnt = comp->tsconf.dmalog.count;
		printf("DMA  : %u started\n", cnt);
		for (unsigned k = (cnt > TS_DMALOG) ? cnt - TS_DMALOG : 0; k < cnt; k++) {
			tsDmaRec* r = &comp->tsconf.dmalog.rec[k % TS_DMALOG];
			printf("DMA  : #%u frame %d at %d:%d ctrl %02X %06X -> %06X %d x %d, %d T%s\n", k, r->fcnt,
				r->line, r->dot, r->ctrl, r->src, r->dst, r->len, r->num, r->dur, r->cut ? " cut" : "");
		}
	}
	const char* dump = getenv("DUMP");
	if (dump) {
		dump_ppm(comp, dump);
		printf("DUMP : bpl %d full %dx%d\n", bytesPerLine, comp->vid->full.x, comp->vid->full.y);
	}
	if (getenv("TSU")) {
		int nl = 0, nn = 0;
		for (int j = 0; j < 0x200; j++) {nl += !!comp->vid->line[j]; nn += !!comp->vid->tsconf.tsNext[j];}
		printf("TSU  : win %d,%d %dx%d tcfg %02X sgp %02X vconf %02X line %d next %d ray %d\n",
			comp->vid->tsconf.tsXPos, comp->vid->tsconf.tsYPos, comp->vid->tsconf.tsSize.x, comp->vid->tsconf.tsSize.y,
			comp->vid->tsconf.tconfig, comp->vid->tsconf.SGPage, comp->vid->tsconf.p00af, nl, nn, comp->vid->ray.y);
		printf("TSU  : line[96..111]:");
		for (int j = 96; j < 112; j++) printf(" %02X", comp->vid->line[j]);
		printf("  mode %d nogfx %d pal15 %08X pal3F %08X\n", comp->vid->vmode, comp->vid->nogfx, comp->vid->pal[0x15], comp->vid->pal[0x3f]);
	}
	printf("CRAM :");
	for (int j = 0; j < 16; j++) printf(" %02X", comp->vid->tsconf.cram[j]);
	printf("\nLINB :");
	for (int j = 0; j < 16; j++) printf(" %02X", comp->vid->linb[j]);
	printf("\nSFILE:");
	for (int j = 0; j < 16; j++) printf(" %02X", comp->vid->tsconf.sfile[j]);
	printf("\n");
	compDestroy(comp);
	return 0;
}
