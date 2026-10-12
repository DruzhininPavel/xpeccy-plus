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

void bsInit(bbStream*, BYTE*, int);
BYTE bsGetByte(bbStream*);
BYTE bsGetBit(bbStream*);
BYTE bsGetBits(bbStream*, int);

WORD dehrust(BYTE*, BYTE*);
WORD demegalz(BYTE*, BYTE*);
