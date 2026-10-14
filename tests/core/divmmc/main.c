// Headless divMMC bench: a ZX Spectrum 48K (as res/machines/zx48.conf has it)
// with a divMMC on its edge connector, running a raw Z80 binary.
//
//	divmmc-bench <prog.bin> [frames]
//
// XPECCY_ROOT is the source tree, for config/roms/48.rom (default: the current
// folder). The program is loaded at #8000, SP = #BF00, DI, IM 1, and reports
// through the mailbox at #BF00..#BF7F: byte 0 = #AA ends the run, bytes 1.. are
// printed, then the board's state.
//
// Environment: EEPROM=file the 8K image (default: erased); JUMPER=1 opens J1 (the
// EEPROM takes writes, automap needs MAPRAM); RAM512=1 fits 512K; SDCARD=img and
// SDCARD1=img put cards in the slots (a name ending in .img is made blank, 1 MB);
// NMIAT=n holds the NMI button through frame n; RESETAFTER=1 the state a reset
// leaves, POWERAFTER=1 the state a power cycle leaves; EEOUT=file writes the
// EEPROM after the run; TICKS=1 the T the program took.

#include "../bench.h"

// the screen as text: each 8x8 cell matched against the 48K ROM's font (ink on
// paper or inverted), '?' for a cell that is no character
static void scr_text(Computer* comp) {
	const unsigned char* scr = comp->mem->ramData + (5 << 14);
	const unsigned char* fnt = comp->mem->romData + 0x3d00;
	for (int y = 0; y < 24; y++) {
		char line[33];
		for (int x = 0; x < 32; x++) {
			unsigned char cell[8];
			for (int r = 0; r < 8; r++)
				cell[r] = scr[((y & 0x18) << 8) | (r << 8) | ((y & 7) << 5) | x];
			int ch = '?';
			for (int c = 0; (c < 96) && (ch == '?'); c++) {
				int same = 1, inv = 1;
				for (int r = 0; r < 8; r++) {
					same &= (cell[r] == fnt[c * 8 + r]);
					inv &= (cell[r] == (unsigned char)~fnt[c * 8 + r]);
				}
				if (same || inv) ch = 32 + c;
			}
			line[x] = (char)ch;
		}
		line[32] = 0;
		int end = 32;
		while ((end > 0) && (line[end - 1] == ' ')) line[--end] = 0;
		printf("|%s\n", line);
	}
}

static int rd16(const unsigned char* p) {return p[0] | (p[1] << 8);}
static unsigned int rd32(const unsigned char* p) {return rd16(p) | ((unsigned int)rd16(p + 2) << 16);}

// After the run: a directory in the root of the image's first FAT16 partition,
// and what its first cluster's sectors hold - entries, zeros and FFs.
static void dir_dump(const char* img, const char* name) {
	FILE* f = fopen(img, "rb");
	if (!f) return;
	unsigned char s[512];
	char n11[12];
	snprintf(n11, sizeof(n11), "%-11s", name);
	if (fread(s, 512, 1, f) != 1) {fclose(f); return;}
	unsigned int part = rd32(s + 0x1c6);
	x_fseek(f, (long long)part * 512, SEEK_SET);
	if (fread(s, 512, 1, f) != 1) {fclose(f); return;}
	int spc = s[13], res = rd16(s + 14), nfat = s[16], rootn = rd16(s + 17), fatsz = rd16(s + 22);
	unsigned int root = part + res + nfat * fatsz;
	unsigned int data = root + rootn * 32 / 512;
	int clu = -1;
	for (int i = 0; (i < rootn * 32 / 512) && (clu < 0); i++) {
		x_fseek(f, (long long)(root + i) * 512, SEEK_SET);
		if (fread(s, 512, 1, f) != 1) break;
		for (int e = 0; e < 16; e++)
			if (!memcmp(s + e * 32, n11, 11)) clu = rd16(s + e * 32 + 26);
	}
	if (clu < 0) {
		printf("DIR  : %s not found\n", name);
		fclose(f);
		return;
	}
	printf("DIR  : %s cluster %d, %d sectors\n", name, clu, spc);
	for (int k = 0; k < spc; k++) {
		x_fseek(f, (long long)(data + (clu - 2) * spc + k) * 512, SEEK_SET);
		if (fread(s, 512, 1, f) != 1) break;
		int z = 0, ff = 0, used = 0;
		for (int i = 0; i < 512; i++) {
			z += !s[i];
			ff += (s[i] == 0xff);
		}
		for (int e = 0; e < 16; e++)
			used += (s[e * 32] != 0x00) && (s[e * 32] != 0xe5);
		printf("DIR  : sector %d: %d entries, %d x 00, %d x FF, first half %s, second half %s\n", k, used, z, ff,
			memchr(s, 0xff, 256) ? "has FF" : "no FF", memchr(s + 256, 0xff, 256) ? "has FF" : "no FF");
	}
	fclose(f);
}

static void div_state(Computer* comp, const char* tag) {
	DivMMC* div = comp->div;
	printf("%s: conmem %d mapram %d bank %02X mapcond %d automap %d shown %d cs %d #0000 %02X #2000 %02X\n",
		tag, div->conmem, div->mapram, div->bank, div->mapcond, div->automap, div->shown, div->cs,
		mem_rd_fast(comp->mem, 0x0000) & 0xff, mem_rd_fast(comp->mem, 0x2000) & 0xff);
}

int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "usage: divmmc-bench prog.bin [frames]\n");
		return 2;
	}
	int frames = (argc > 2) ? atoi(argv[2]) : 50;
	const char* root = getenv("XPECCY_ROOT");
	if (!root) root = ".";

	Computer* comp = compCreate();
	if (!compSetHardware(comp, "ZX48")) {
		fprintf(stderr, "no ZX48 core\n");
		return 1;
	}
	// set up by hand after res/machines/zx48.conf and ULA.48: a change there does not reach it
	vLayout lay = {{448, 312}, {64, 56}, {64, 16}, {256, 192}, {120, 8}, 64};
	comp_set_layout(comp, &lay);
	bytesPerLine = comp->vid->full.x * 8;
	memSetSize(comp->mem, MEM_64K, MEM_32K);
	if (!load_rom(comp, root, "48.rom"))
		return 1;
	compSetBaseFrq(comp, 3.5);
	comp->resbank = RES_48;
	comp->flgCNTI = 1;
	comp_set_cont(comp, 1);
	comp_set_cont_pattern(comp, 1);
	comp->vid->ula->early = 1;
	comp->fbus = FBUS_NONE;
	xhost_time_fixed = 1759665600;		// runs repeat
	div_set_model(comp->div, DIV_ENJOY, getenv("RAM512") ? 64 : 16);
	comp->div->eeWrite = getenv("JUMPER") ? 1 : 0;
	const char* ee = getenv("EEPROM");
	if (ee && !div_load_eeprom(comp->div, ee)) {
		char path[4096];		// a name beside the program
		snprintf(path, sizeof(path), "%.*s%s", (int)(base_name(argv[1]) - argv[1]), argv[1], ee);
		if (!div_load_eeprom(comp->div, path)) {
			fprintf(stderr, "can't read %s\n", ee);
			return 1;
		}
	}
	div_power_on(comp->div);
	compReset(comp, RES_DEFAULT);
	comp_kbd_release(comp);
	const char* sd0 = getenv("SDCARD");
	if (sd0) sdcSetImage(comp->div->card[0], sd0);
	const char* sd1 = getenv("SDCARD1");
	if (sd1) sdcSetImage(comp->div->card[1], sd1);

	// "-": no program, the machine starts from its reset (and the board's power-on).
	// PROGAT=n: so it does, and at frame n the program is put in and run.
	int boot = !strcmp(argv[1], "-");
	int progat = getenv("PROGAT") ? atoi(getenv("PROGAT")) : 0;
	size_t n = 0;
	unsigned char prog[0x3f00];
	if (!boot) {
		FILE* f = fopen(argv[1], "rb");
		if (!f) {
			fprintf(stderr, "can't open %s\n", argv[1]);
			return 1;
		}
		n = fread(prog, 1, sizeof(prog), f);
		fclose(f);
	}
	unsigned char* dst = comp->mem->ramData + (2 << 14);
	if (!boot && !progat) {
		memcpy(dst, prog, n);
		memset(dst + 0x3f00, 0, 0x100);
		cpu_start(comp);
	}

	int nmiat = getenv("NMIAT") ? atoi(getenv("NMIAT")) : -1;
	int fstart = comp->vid->fcnt;
	long long tick0 = comp->tickTotal;
	while ((boot || (mbox_rd(comp, MBOX) != 0xaa)) && (comp->vid->fcnt - fstart < frames)) {
		if (progat && (comp->vid->fcnt - fstart == progat)) {
			progat = 0;
			memcpy(dst, prog, n);
			memset(dst + 0x3f00, 0, 0x100);
			cpu_start(comp);
			tick0 = comp->tickTotal;
		}
		comp->flgNMIRQ = ((comp->vid->fcnt - fstart) == nmiat) ? 1 : 0;
		compExec(comp);
	}
	comp->flgNMIRQ = 0;
	printf("%s: %zu bytes, %s after %d frames, pc=%04X\n", base_name(argv[1]), n,
		(mbox_rd(comp, MBOX) == 0xaa) ? "done" : "TIMEOUT",
		comp->vid->fcnt - fstart, comp->cpu->regPC);
	if (getenv("TICKS")) printf("TICKS: %lld\n", comp->tickTotal - tick0);
	if (!boot) mbox_print(comp);
	div_state(comp, "DIV  ");
	if (getenv("SCREEN")) scr_text(comp);
	const char* dump = getenv("DUMP");
	if (dump) dump_ppm(comp, dump);
	if (getenv("RESETAFTER")) {
		compReset(comp, RES_DEFAULT);
		div_state(comp, "RESET");
	}
	if (getenv("POWERAFTER")) {
		div_power_on(comp->div);
		compReset(comp, RES_DEFAULT);
		div_state(comp, "POWER");
	}
	const char* dd = getenv("DIRDUMP");
	if (dd && sd0) {
		sdcSetImage(comp->div->card[0], "");	// written out and closed
		dir_dump(sd0, dd);
	}
	const char* eeout = getenv("EEOUT");
	if (eeout) div_save_eeprom(comp->div, eeout);
	compDestroy(comp);
	return 0;
}
