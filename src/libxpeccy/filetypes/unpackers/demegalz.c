#include "unpackers.h"

WORD demegalz(BYTE* src, int slen, BYTE* dst, int dlen) {
//	BYTE* from = src;
	BYTE* to = dst;
	BYTE* end = dst + dlen;
	bbStream strm;
	bbStream* bs = &strm;
	bsInit(bs, src, slen, PAK_MLZ);

	signed short offset = 0;
	UNP_PUT(bsGetByte(bs));
	unsigned char len;
	int work = 1;
	while (!bs->eof && work) {
		while (bsGetBit(bs)) {
			UNP_PUT(bsGetByte(bs));
		}
		len = bsGetBits(bs, 2);
		switch (len) {
			case 0x00:
				offset = bsGetBits(bs, 3) - 8;
				len = 1;
				break;
			case 0x01:
				offset = bsGetByte(bs) - 256;
				len = 2;
				break;
			case 0x02:
				if (bsGetBit(bs)) {
					offset = (0xf0 + bsGetBits(bs, 4) - 1) << 8;
					offset += bsGetByte(bs);
				} else {
					offset = bsGetByte(bs) - 256;
				}
				len = 3;
				break;
			default:
				len = 1;
				while (!bsGetBit(bs) && (len < 9)) {
					len++;
				}
				switch (len) {
					case 1: len = 4 + bsGetBit(bs); break;
					case 2: len = 6 + bsGetBits(bs,2); break;
					case 3: len = 10 + bsGetBits(bs,3); break;
					case 4: len = 18 + bsGetBits(bs,4); break;
					case 5: len = 34 + bsGetBits(bs,5); break;
					case 6: len = 66 + bsGetBits(bs,6); break;
					case 7: offset = 130 + bsGetBits(bs,7);
						len = (offset < 255) ? (offset & 0xff) : 0xff;
						break;
					default: len = 0; work = 0; break;
				}
				if (bsGetBit(bs)) {
					offset = (0xf0 + bsGetBits(bs,4) - 1) << 8;
					offset += bsGetByte(bs);
				} else {
					offset = bsGetByte(bs) - 256;
				}
				break;
		}
		while (len > 0) {
			UNP_REF();
			len--;
		}

	}
	return (to - dst);
}
