#include "filetypes.h"
#include "../xlog.h"

int loadSlot(Computer* comp, const char* name, int drv) {
	xlog(XLG_FILE, XLL_DEBUG, "loadSlot %s", name);
	xCartridge* slot = comp->slot;
	FILE* file = fopen(name, "rb");
	if (!file) return ERR_CANT_OPEN;
	fseek(file,0,SEEK_END);
	long siz = ftell(file);
	rewind(file);
	int err = ERR_OK;
	if (siz > MEM_4M) {
		err = ERR_RAW_LONG;
		fclose(file);
	} else {
		int tsiz = 1;
		while (tsiz < siz) {tsiz <<= 1;}		// get nearest 2^n >= siz
		slot->data = realloc(slot->data, tsiz);
		slot->brkMap = realloc(slot->brkMap, tsiz);
		memset(slot->brkMap, 0x00, tsiz);
		slot->memMask = tsiz - 1;
		sltSetPath(slot, name);
		// the image fills the slot only when its size is a power of two: the
		// rest reads as an empty bus
		size_t got = fread(slot->data, 1, tsiz, file);
		if (got < (size_t)tsiz) memset(slot->data + got, 0xff, tsiz - got);
		fclose(file);
		err = ERR_OK;
		compReset(comp, RES_DEFAULT);
	}
	return err;
}
