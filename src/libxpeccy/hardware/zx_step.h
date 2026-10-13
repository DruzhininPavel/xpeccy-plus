#pragma once

// What every instruction of a ZX ends in, inline: for the quiet step, which calls
// them straight, and for the handlers that keep their names (zx_sync, zx_irq_ack).

#include "hardware.h"
#include "../cpu/Z80/z80.h"

// INT sampled at the end of every instruction
static inline void zx_irq_ack_in(Computer* comp) {
	// an instruction that ends in an i/o cycle leaves the video at its
	// end, a tick past the one the cpu samples INT in: a pulse that
	// began inside that tick is not there yet
	int ahead = res4 - comp->cpu->t;
	// lazy: dots still counted cross no event, so they leave intFRAME
	// zero or not as it is, and a step back (ahead) walks them first
	comp_vid_to_t(comp);
	int act = comp->vid->intFRAME;
	if (act && (ahead > 0) && (comp->vid->intlen - act < ahead * comp->nsPerTickFixed / comp->vid->nsPerDotFixed))
		act = 0;
	comp->cpu->flgACK = !!act && !comp->rzx.play;
	// INT is a level: taken early in the pulse, it is taken again as soon
	// as interrupts are back on and the pulse is still there (fuse does
	// the same from EI). Butler's 128K timing tests count on it.
	if (act && !comp->rzx.play)
		comp->cpu->intrq |= Z80_INT;
}

// TSConf's flags (tslab.c names them flgVDOS, flgLINT, flgDINT)
#define TSL_FLG_VDOS	flag[1]
#define TSL_FLG_LINT	flag[2]		// 1 on vid->intLINE, 0 on line int ack
#define TSL_FLG_DINT	flag[3]		// (same for dma int)

// TSConf's (ts_irq): line and dma hold until acked; the cpu drops INT on every
// ack it takes
static inline void ts_irq_ack_in(Computer* comp) {
	zx_irq_ack_in(comp);
	if (comp->TSL_FLG_LINT || comp->TSL_FLG_DINT) {
		comp->cpu->flgACK = 1;
		comp->cpu->intrq |= Z80_INT;
	}
	if (comp->TSL_FLG_VDOS)
		comp->cpu->flgACK = 0;
}

static inline void zx_sync_in(Computer* comp, int ns) {
	// devices. This runs once per instruction, so the ones that are idle or
	// not there at all are asked by the flag they check first, not by a call
	if (comp->dif->fdc->plan || comp->dif->doors)
		difSync(comp->dif, ns);
	if (comp->gs->enable)
		gsSync(comp->gs, ns);
	if (comp->saa->enabled)
		saaSync(comp->saa, ns);
	tsSync(comp->ts, ns);
	tapSync(comp->tape, ns);
	bcSync(comp->beep, ns);
	if (comp->keyb->per) kbd_sync(comp->keyb, ns);		// a key is held: ps/2 auto-repeat
	// nmi
	if ((comp->cpu->regPC > 0x3fff) && comp->flgNMIRQ) {
		comp->cpu->intrq |= Z80_NMI;	// request nmi
		comp->flgDOS = 1;			// set dos page
		comp->flgROM = 1;
		comp->hw->mapMem(comp);
	}
}
