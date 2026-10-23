// Headless keyboard bench: bin/poll.bin (asm/poll.asm) on a machine, fed . and Delete
// - with their ps/2 codes as the host's keys come (KEYS=host), or as autostart and the
// virtual keyboard press them (KEYS=ours, no code) - and what the poll counted.
//
//	kbd-bench <poll.bin> <presses>	MACHINE=ZX48|ZXNext|... KEYS=host|ours
//
// Keys change at 20 ms ticks, as the host's come at frame ends: down for 3, up for 3,
// <presses> times . and then as many Delete.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spectrum.h"
#include "cpu/Z80/z80.h"		// the register names

static const char* cntName[6] = {"sep_m", "sep_0", "edge_m", "bare_m", "edge_0", "bare_0"};

int main(int argc, char** argv) {
	if (argc < 3) {
		fprintf(stderr, "usage: kbd-bench poll.bin presses\n");
		return 2;
	}
	int presses = atoi(argv[2]);
	const char* mach = getenv("MACHINE") ? getenv("MACHINE") : "ZX48";
	int host = !getenv("KEYS") || strcmp(getenv("KEYS"), "ours");
	xhost_time_fixed = 1759665600;
	Computer* comp = compCreate();
	if (!compSetHardware(comp, mach)) {
		fprintf(stderr, "no %s core\n", mach);
		return 1;
	}
	compSetBaseFrq(comp, 3.5);
	comp_power_on(comp);		// the Next's ps/2 keymap comes in with the power
	compReset(comp, RES_DEFAULT);
	comp_kbd_release(comp);
	FILE* f = fopen(argv[1], "rb");
	if (!f) {
		fprintf(stderr, "can't open %s\n", argv[1]);
		return 1;
	}
	int c, adr = 0x8000;
	while ((c = fgetc(f)) != EOF)
		comp->hw->mwr(comp, adr++, c);
	fclose(f);
	comp->cpu->regPC = 0x8000;
	comp->cpu->flgIFF1 = comp->cpu->flgIFF2 = 0;

	keyEntry dot, del;
	memset(&dot, 0, sizeof(dot));
	memset(&del, 0, sizeof(del));
	strcpy((char*)dot.zxKey, "Sm");
	strcpy((char*)del.zxKey, "C0");
	if (host) {
		dot.atCode = 0x49;		// set 2, as keymap.cpp has them
		del.atCode = 0x66;
	}
	const long long tick = 20000000;
	int ticks = 10 + presses * 2 * 6 + 10;
	long long t = 0;
	for (int k = 0; k < ticks; k++) {
		int n = k - 10;
		if ((n >= 0) && (n / 6 < 2 * presses)) {
			keyEntry* e = (n / 6 < presses) ? &dot : &del;
			if (n % 6 == 0) comp->hw->keyp(comp, e);
			else if (n % 6 == 3) comp->hw->keyr(comp, e);
		}
		while (t < (k + 1) * tick)
			t += compExec(comp);
	}
	for (int i = 0; i < 6; i++)
		printf("%s %d\n", cntName[i], comp->hw->mrd(comp, 0x9000 + i, 0));
	compDestroy(comp);
	return 0;
}
