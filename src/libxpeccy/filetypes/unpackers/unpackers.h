#pragma once

#define	PAK_HRS	1
#define	PAK_MLZ	2

typedef unsigned short WORD;
typedef unsigned char BYTE;

/* BBStream class */

typedef struct {
	BYTE* base;
	BYTE* p;
	int   idx;
	int   len;
	int pType;
	int  eof;
	WORD  bits;
} bbStream;

void bsInit(bbStream*, BYTE*, int len, int type);
BYTE bsGetByte(bbStream*);
BYTE bsGetBit(bbStream*);
BYTE bsGetBits(bbStream*, int);

// (src, its length, dst, its room): the length unpacked. A broken stream stops
// at either end, and at a reference to before the start of dst.
WORD dehrust(BYTE*, int, BYTE*, int);
WORD demegalz(BYTE*, int, BYTE*, int);

#define UNP_PUT(v) do { if (to >= end) return to - dst; *to = (v); to++; } while (0)
#define UNP_REF() do { if ((to >= end) || ((to - dst) + offset < 0)) return to - dst; *to = to[offset]; to++; } while (0)
