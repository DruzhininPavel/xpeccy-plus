#include "video.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

// tsconf sprites & tiles

static int adr,xscr,yscr;
static unsigned char col, ink, pap, scrbyte, nxtbyte;

void vidDrawByteDD(Video*);

static int fadr;
static int tile;
static int sadr;	// adr in sprites dsc
static int xadr;	// = pos with XFlip
static unsigned char* tsl;	// the line buffer the tiles and sprites go to

// The layers in use over each other in their order, S0 at the bottom. A layer's
// dot is 0 where it is transparent. Eight dots at a time: a byte of the mask is
// FF where the layer's dot is not 0.
static void ts_compose(unsigned char* dst, unsigned char lay[5][0x200], int use) {
	int i = 0;
	while ((i < 5) && !(use & (1 << i))) i++;
	if (i == 5) {
		memset(dst, 0x00, 0x200);
		return;
	}
	memcpy(dst, lay[i], 0x200);
	for (i++; i < 5; i++) {
		if (!(use & (1 << i))) continue;
		const unsigned char* src = lay[i];
		for (int x = 0; x < 0x200; x += 8) {
			uint64_t s, d;
			memcpy(&s, src + x, 8);
			if (!s) continue;
			memcpy(&d, dst + x, 8);
			uint64_t m = (((s & 0x7f7f7f7f7f7f7f7fULL) + 0x7f7f7f7f7f7f7f7fULL) | s) & 0x8080808080808080ULL;
			m = (m >> 7) * 0xff;
			d = (d & ~m) | (s & m);
			memcpy(dst + x, &d, 8);
		}
	}
}

int tslLayersApart = 0;

// a line drawn whole keeps what it has: its layers were never apart
void tsl_recompose(Video* vid) {
	int n = vid->tsconf.layNext;
	unsigned char* dst[2] = {vid->line, vid->tsconf.tsNext};
	for (int k = 0; k < 2; k++) {
		int used = vid->tsconf.layUsed[n ^ k ^ 1];
		if (!(used & TSL_LAY_WHOLE))
			ts_compose(dst[k], vid->tsconf.lay[n ^ k ^ 1], used & ~vid->tsconf.hide);
	}
}

// Where the entry of tile layer lay at row, col of the map is: a row of the map is 256
// bytes, layer 0's 64 entries in the first half and layer 1's in the second
static inline int ts_map_adr(int page, int lay, int row, int col) {
	return (page << 14) | ((row & 63) << 8) | ((lay & 1) << 7) | ((col & 63) << 1);
}

// The tile map is read 16 lines ahead of the line it is for, 8 entries a line for each layer,
// into 4 row buffers: the coarse Y offset and TMPage count when a row is read, the fine
// offset when it is drawn (video_ts.v). ln is the line of the tile/sprite window.
static int vidTSLReadMap(Video* vid, int ln) {
	int res = 0;
	if ((ln < -16) || (ln >= vid->tsconf.tsSize.y - 8)) return 0;
	int tml = ln + 16;
	for (int lay = 0; lay < 2; lay++) {
		if (!(vid->tsconf.tconfig & (lay ? 0x40 : 0x20))) continue;
		int row = (tml >> 3) + ((lay ? vid->tsconf.T1YOffset : vid->tsconf.T0YOffset) >> 3);
		for (int n = 0; n < 8; n++) {
			int c = ((tml & 7) << 3) | n;
			int a = ts_map_adr(vid->tsconf.TMPage, lay, row, c);
			vid->tsconf.tmb[lay][(tml >> 3) & 3][c] = vid->mrd(a, vid->xptr) | (vid->mrd(a | 1, vid->xptr) << 8);
			res++;
		}
	}
	return res;
}

// Where line `line` of graphics tile `tile` starts. A graphics page is 64x64 tiles of
// 8x8 dots, two dots a byte: 4 bytes a tile line, 256 bytes a line across the page.
static inline int ts_gfx_adr(int page, int tile, int line) {
	return (page << 14) + (((tile & 0xfc0) << 5) | ((line & 7) << 8) | ((tile & 0x3f) << 2));
}

// A tile map entry: tile b11..0, palette b13..12, XFlip b14, YFlip b15. Where line `line`
// of the cell starts, YFlip taken in.
static inline int ts_cell_line_adr(int page, int entry, int line) {
	int fa = ts_gfx_adr(page, entry, line);
	return (entry & 0x8000) ? (fa ^ 0x0700) : fa;
}

// the color's high nibble a map entry gets: PalSel b7..6 for the layer, b5..4 its own
static inline int ts_cell_pal(int entry, int palhi) {
	return palhi | ((entry >> 8) & 0x30);
}

// tile 0 is a hole in the layer unless its Z_EN is on
static inline int ts_cell_hole(Video* vid, int lay, int entry) {
	return !(entry & 0xfff) && !(vid->tsconf.tconfig & (lay ? 8 : 4));
}

// render tiles
int vidTSLRenderTiles(Video* vid, int ln, int lay, unsigned short yoffs, unsigned short xoffs, unsigned char gpage, unsigned char palhi) {
	int j;
	int res = 0;
	yscr = (ln & 31) + (yoffs & 7);							// t_line: the row buffer and the line in it
	unsigned short* row = vid->tsconf.tmb[lay][(yscr >> 3) & 3];
	xscr = (0x200 - xoffs) & 0x1ff;								// pos in line buf
	xadr = vid->tsconf.tconfig & (lay ? 8 : 4);
	// the TSU only fetches the tiles across its window, plus two (video_mode.v x_tiles)
	int ntiles = (vid->tsconf.tsSize.x >> 3) + 2;
	for (adr = 0; adr < 64; adr++) {							// 64 tiles in row
		int vis = ((adr - (xoffs >> 3)) & 63) < ntiles;
		tile = row[adr];									// tile dsc

		if ((tile & 0xfff) || xadr) {							// !0 or (0 enabled)
			fadr = ts_cell_line_adr(gpage, tile, yscr);				// full addr of row of this tile
			if (vis) res += 2;		// 8 dots, 2 memory readings
			col = ts_cell_pal(tile, palhi);						// palette (b7..4 of color)
			if (tile & 0x4000) {							// XFlip
				xscr += 8;
				for (j = 0; j < 4; j++) {
					int b = vid->mrd(fadr, vid->xptr);	// both pixels of the byte
					col = (col & 0xf0) | ((b & 0xf0) >> 4);			// left pixel
					xscr--;
					if (col & 0x0f) tsl[xscr & 0x1ff] = col;
					col = (col & 0xf0) | (b & 0x0f);				// right pixel
					xscr--;
					if (col & 0x0f) tsl[xscr & 0x1ff] = col;
					fadr++;
				}
				xscr += 8;
			} else {								// no XFlip
				for (j = 0; j < 4; j++) {
					int b = vid->mrd(fadr, vid->xptr);	// both pixels of the byte
					col = (col & 0xf0) | ((b & 0xf0) >> 4);					// left pixel
					if (col & 0x0f) {
						tsl[xscr & 0x1ff] = col;
					}
					xscr++;
					col = (col & 0xf0) | (b & 0x0f);						// right pixel
					if (col & 0x0f) {
						tsl[xscr & 0x1ff] = col;
					}
					xscr++;
					fadr++;
				}
			}
		} else {
			xscr += 8;
		}
	}
	return res;
}

// render sprites
typedef struct {
	unsigned y:9;		// 0[0:7], 1:0
	unsigned ys:3;		// 1[1:3]
	unsigned res1:1;	// 1:4
	unsigned act:1;		// 1:5
	unsigned leap:1;	// 1:6
	unsigned yf:1;		// 1:7
	unsigned x:9;		// 2[0:7], 3:0
	unsigned xs:3;		// 3[1:3]
	unsigned res2:3;	// 3[4:6]
	unsigned xf:1;		// 3:7
	unsigned tnum:12;	// 4[0:7], 5[0:3]
	unsigned pal:4;		// 5[4:7]
} TSpr;

static void ts_spr_parse(const unsigned char* ptr, TSpr* spr) {
	spr->y = ptr[0];
	spr->y |= (ptr[1] & 1) << 8;
	spr->ys = (ptr[1] & 0x0e) >> 1;
	spr->res1 = (ptr[1] & 0x10) ? 1 : 0;
	spr->act = (ptr[1] & 0x20) ? 1 : 0;
	spr->leap = (ptr[1] & 0x40) ? 1 : 0;
	spr->yf = (ptr[1] & 0x80) ? 1 : 0;
	spr->x = ptr[2];
	spr->x |= (ptr[3] & 1) << 8;
	spr->xs = (ptr[3] & 0x0e) >> 1;
	spr->res2 = (ptr[3] & 0x70) >> 4;
	spr->xf = (ptr[3] & 0x80) ? 1 : 0;
	spr->tnum = ptr[4];
	spr->tnum |= (ptr[5] & 0x0f) << 8;
	spr->pal = (ptr[5] & 0xf0) >> 4;
}

// where line `line` of a sprite (counted in the graphics, after YFlip) starts
static int ts_spr_line_adr(Video* vid, const TSpr* spr, int line) {
	return ts_gfx_adr(vid->tsconf.SGPage, spr->tnum + ((line & 0x1f8) << 3), line);
}

// the next byte of a sprite line: it wraps round in its line across the page
static inline int ts_spr_next(int fadr) {
	return (fadr & ~0xff) | ((fadr + 1) & 0xff);
}

int vidTSLRenderSprites(Video* vid, int ln) {
	TSpr spr;
	int res = 0;
	while (sadr < (0x200 - 6)) {
		ts_spr_parse(&vid->tsconf.sfile[sadr], &spr);
		if (spr.act) {
			adr = spr.y;
			xscr = (spr.ys + 1) << 3;		// Ysize - 000:8; 001:16; 010:24; ...
			yscr = ln;				// line on screen
			if (((yscr - adr) & 0x1ff) < xscr) {	// if sprite visible on current line
				res += (spr.xs + 1) << 1;	// a word for every 4 dots across
				yscr -= adr;			// line inside sprite;
				if (spr.yf) yscr = xscr - yscr - 1;	// YFlip (Ysize - norm.pos - 1)
				fadr = ts_spr_line_adr(vid, &spr, yscr);	// adr of pix line to put in buf

				col = spr.pal << 4;
				xadr = (spr.xs + 1) << 3;	// xsoze
				adr = spr.x;			// xpos
				if (spr.xf) adr += xadr - 1;	// xpos of right pixel (xflip)
				for (xscr = xadr; xscr > 0; xscr -= 2) {
					int b = vid->mrd(fadr, vid->xptr);	// both pixels of the byte
					col = (col & 0xf0) | ((b & 0xf0) >> 4);			// left pixel
					if (col & 0x0f) tsl[adr & 0x1ff] = col;
					if (spr.xf) adr--; else adr++;
					col = (col & 0xf0) | (b & 0x0f);				// right pixel
					if (col & 0x0f) tsl[adr & 0x1ff] = col;
					if (spr.xf) adr--; else adr++;
					fadr = ts_spr_next(fadr);
				}
			}
		}
		sadr += 6;
		if (spr.leap) break;		// LEAP
	}
	return res;
}

// A line of an undrawn frame: what the two above fetch, counted and not drawn.
// The DRAM cycles they take are what the cpu and the DMA see of the TSU.
static int ts_tiles_count(Video* vid, int ln, int lay, unsigned short yoffs, unsigned short xoffs) {
	int ys = (ln & 31) + (yoffs & 7);
	unsigned short* row = vid->tsconf.tmb[lay][(ys >> 3) & 3];
	int zen = vid->tsconf.tconfig & (lay ? 8 : 4);
	int ntiles = (vid->tsconf.tsSize.x >> 3) + 2;
	int res = 0;
	for (int a = 0; a < 64; a++)
		if ((((a - (xoffs >> 3)) & 63) < ntiles) && ((row[a] & 0xfff) || zen))
			res += 2;
	return res;
}

// the bits ts_spr_parse() takes them from: Y, YS, ACT and LEAP in bytes 0-1, XS in byte 3
static int ts_sprites_count(Video* vid, int ln) {
	int res = 0;
	while (sadr < (0x200 - 6)) {
		const unsigned char* p = &vid->tsconf.sfile[sadr];
		if (p[1] & 0x20) {
			int y = p[0] | ((p[1] & 1) << 8);
			if (((ln - y) & 0x1ff) < ((((p[1] >> 1) & 7) + 1) << 3))
				res += (((p[3] >> 1) & 7) + 1) << 1;
		}
		sadr += 6;
		if (p[1] & 0x40) break;
	}
	return res;
}

// for the debugger: a sprite and its dots, the way the TSU reads and draws them

void tsl_get_sprite(Video* vid, int idx, tsSprite* s) {
	TSpr spr;
	ts_spr_parse(&vid->tsconf.sfile[(idx % TSL_SPRITES) * 6], &spr);
	s->x = spr.x;
	s->y = spr.y;
	s->w = (spr.xs + 1) << 3;
	s->h = (spr.ys + 1) << 3;
	s->tile = spr.tnum;
	s->pal = spr.pal;
	s->act = spr.act;
	s->leap = spr.leap;
	s->xf = spr.xf;
	s->yf = spr.yf;
}

void tsl_sprite_dots(Video* vid, int idx, unsigned char* out) {
	TSpr spr;
	ts_spr_parse(&vid->tsconf.sfile[(idx % TSL_SPRITES) * 6], &spr);
	int w = (spr.xs + 1) << 3;
	int h = (spr.ys + 1) << 3;
	for (int r = 0; r < h; r++) {
		int fa = ts_spr_line_adr(vid, &spr, spr.yf ? (h - r - 1) : r);
		unsigned char* row = out + r * w;
		for (int c = 0; c < w; c += 2) {
			int b = vid->mrd(fa, vid->xptr);
			int d0 = (b >> 4) & 15;
			int d1 = b & 15;
			// the left dot of a byte goes first, from the right edge with XFlip
			row[spr.xf ? (w - 1 - c) : c] = d0 ? (spr.pal << 4) | d0 : 0;
			row[spr.xf ? (w - 2 - c) : (c + 1)] = d1 ? (spr.pal << 4) | d1 : 0;
			fa = ts_spr_next(fa);
		}
	}
}

// Which of S0, S1, S2 each SFILE entry is drawn in, the way vidTSRender's three passes
// share sadr: each goes on from where the one before stopped, to the next LEAP
void tsl_sprite_layers(Video* vid, signed char* layer) {
	int l = 0;
	for (int i = 0; i < TSL_SPRITES; i++) {
		layer[i] = (l < 3) ? l : -1;
		if (vid->tsconf.sfile[i * 6 + 1] & 0x40) l++;
	}
}

int tsl_gfx_adr(int page, int tile, int line) {
	return ts_gfx_adr(page, tile, line);
}

int tsl_map_entry(Video* vid, int lay, int row, int col) {
	int a = ts_map_adr(vid->tsconf.TMPage, lay, row, col);
	return vid->mrd(a, vid->xptr) | (vid->mrd(a | 1, vid->xptr) << 8);
}

void tsl_map_cell(Video* vid, int lay, int row, int col, tsMapCell* c) {
	c->adr = ts_map_adr(vid->tsconf.TMPage, lay, row, col);
	c->entry = tsl_map_entry(vid, lay, row, col);
	c->pal = ts_cell_pal(c->entry, lay ? vid->tsconf.T1Pal76 : vid->tsconf.T0Pal76);
	c->hole = ts_cell_hole(vid, lay, c->entry);
}

void tsl_map_dots(Video* vid, int lay, int entry, unsigned char* out) {
	if (ts_cell_hole(vid, lay, entry)) {
		memset(out, 0x00, 64);
		return;
	}
	int page = lay ? vid->tsconf.T1GPage : vid->tsconf.T0GPage;
	int pal = ts_cell_pal(entry, lay ? vid->tsconf.T1Pal76 : vid->tsconf.T0Pal76);
	int xf = entry & 0x4000;
	for (int r = 0; r < 8; r++) {
		int fa = ts_cell_line_adr(page, entry, r);
		for (int c = 0; c < 8; c += 2) {
			int b = vid->mrd(fa++, vid->xptr);
			int d0 = b >> 4;
			int d1 = b & 15;
			out[r * 8 + (xf ? 7 - c : c)] = d0 ? pal | d0 : 0;
			out[r * 8 + (xf ? 6 - c : c + 1)] = d1 ? pal | d1 : 0;
		}
	}
}

void tsl_tile_dots(Video* vid, int page, int tile, int palhi, unsigned char* out) {
	for (int r = 0; r < 8; r++) {
		int fa = ts_gfx_adr(page, tile, r);
		for (int c = 0; c < 8; c += 2) {
			int b = vid->mrd(fa++, vid->xptr);
			out[r * 8 + c] = (b & 0xf0) ? (palhi & 0xf0) | (b >> 4) : 0;
			out[r * 8 + c + 1] = (b & 0x0f) ? (palhi & 0xf0) | (b & 15) : 0;
		}
	}
}

//render 4bpp mode line
int vidTSLRender16c(Video* vid) {
	xscr = vid->tsconf.xOffset & 0x1ff;
	yscr = (vid->tsconf.scrLine + vid->tsconf.yOffset) & 0x1ff;
	xadr = ((vid->vidPage & 0xf8) << 14) + (yscr << 8);
	// pixel by pixel: an odd X offset starts on the right nibble of a byte
	for (fadr = 0; fadr < vid->scrsize.x; fadr++) {
		if (!fadr || !(xscr & 1))
			scrbyte = vid->mrd(xadr | (xscr >> 1), vid->xptr);
		vid->linb[fadr] = vid->tsconf.scrPal | ((xscr & 1) ? (scrbyte & 0x0f) : (scrbyte >> 4));
		xscr = (xscr + 1) & 0x1ff;
	}
	return vid->scrsize.x >> 2;		// 1/4
}

// render 8bpp mode line
int vidTSLRender256c(Video* vid) {
	xscr = vid->tsconf.xOffset & 0x1ff;
	yscr = (vid->tsconf.scrLine + vid->tsconf.yOffset) & 0x1ff;
	adr = ((vid->vidPage & 0xf0) << 14) + (yscr << 9) + xscr;
	xadr = adr & ~0x1ff;
	fadr = 0;
	while (fadr < vid->scrsize.x) {
		vid->linb[fadr] = vid->mrd(adr, vid->xptr);
		fadr++;
		adr = ((adr + 1) & 0x1ff) | xadr;
	}
	return vid->scrsize.x >> 1;		// 1/2
}

// render text mode line
int vidTSLRenderText(Video* vid) {
	xscr = vid->tsconf.xOffset & 0x1ff;
	yscr = (vid->tsconf.scrLine + vid->tsconf.yOffset) & 0x1ff;
	adr = (vid->vidPage << 14) + ((yscr & 0x1f8) << 5) + (xscr >> 2);
	xadr = adr & ~0x7f;
	fadr = 0;
	while (fadr < (vid->scrsize.x << 1)) {
		tile = vid->mrd(adr, vid->xptr);		// char nr
		col = vid->mrd(adr | 0x80, vid->xptr);
		adr = ((adr + 1) & 0x7f) | xadr;
		ink = (col & 0x0f) | 0x10;			// b4 marks ink for the mixer; PalSel goes on at the output
		pap = (col & 0xf0) >> 4;
		scrbyte = vid->mrd(MADR(vid->vidPage ^ 1, (tile << 3) | (yscr & 7)), vid->xptr);	// char line data (8 dots)
		do {
			vid->linb[fadr & 0x3ff] = (scrbyte & 0x80) ? ink : pap;
			scrbyte <<= 1;
			fadr++;
		} while (fadr & 7);
	}
	return vid->scrsize.x >> 1;		// 4 of every 8 cycles
}

// the TSConfig bit each layer is on by: S0 T0 S1 T1 S2
static const unsigned char tsLayerOn[5] = {0x80, 0x20, 0x80, 0x40, 0x80};

// DRAM cycles (one a dot) the line takes: bitmap fetch as video_mode.v sets it, tile map
// prefetch, a word per 4 tile/sprite pixels
int vidTSRender(Video* vid) {
	int res = 0;
	// In an undrawn frame the fetches are only counted. The tiles and sprites are
	// for the line after this one, which is the next frame's first at the end of
	// one: drawn when that frame is.
	int draw = !vid->nodraw;
	int next = draw;
	if ((vid->ray.y + 1 >= vid->full.y) && (vid->nodrawNext >= 0))
		next = !vid->nodrawNext;
// this line's tiles and sprites were drawn during the line before
	if (draw) memcpy(vid->line, vid->tsconf.tsNext, 0x200);
	vid->tsconf.layNext ^= 1;			// and so were its layers
// the bitmap has a window of its own
	int gfx = (vid->ray.y >= vid->tsconf.yPos) && (vid->ray.y < vid->tsconf.yPos + vid->scrsize.y);
	if (draw) memset(vid->linb,0x00,0x200);
// bitplane/text (render to vid->linb)
	if (gfx) switch(vid->vmode) {
		case VID_TSL_16:
			res += draw ? vidTSLRender16c(vid) : (vid->scrsize.x >> 2);
			break;
		case VID_TSL_256:
			res += draw ? vidTSLRender256c(vid) : (vid->scrsize.x >> 1);
			break;
		case VID_TSL_TEXT:
			res += draw ? vidTSLRenderText(vid) : (vid->scrsize.x >> 1);	// rendered as double-density
			break;
	}
	if (gfx && (vid->vmode == VID_TSL_NORMAL)) res += 32;	// ZX: 1 of 8 over 256 dots
	if (gfx) vid->tsconf.scrLine++;
// the TSU works a line ahead (video_ts.v renders line + 1), with the registers as they are now.
// ln runs from -16: the map of the window's first lines is read at the end of the frame before
	int ln = (vid->ray.y + 1 - vid->tsconf.tsYPos + vid->full.y) % vid->full.y;
	if (ln >= vid->full.y - 16) ln -= vid->full.y;
	res += vidTSLReadMap(vid, ln);
	// The layers each into a buffer of their own while one is hidden or a redraw
	// may come, put together after; else all into tsNext, over each other as they
	// go. The set the next line goes to: only what was drawn into it is cleared.
	int apart = tslLayersApart || vid->tsconf.hide;
	int n = vid->tsconf.layNext;
	unsigned char (*lay)[0x200] = vid->tsconf.lay[n];
	for (int i = 0; i < 5; i++)
		if (vid->tsconf.layUsed[n] & (1 << i)) memset(lay[i], 0x00, 0x200);
	vid->tsconf.layUsed[n] = apart ? 0 : TSL_LAY_WHOLE;
	tsl = vid->tsconf.tsNext;
	if (next && (!apart || (ln < 0) || (ln >= vid->tsconf.tsSize.y))) memset(tsl, 0x00, 0x200);
	if ((ln < 0) || (ln >= vid->tsconf.tsSize.y)) return res;
	sadr = 0x000;					// adr inside SFILE
	int used = 0;					// a sprite layer with no sprite on the line stays clear
	for (int i = 0; i < 5; i++) {			// S0 T0 S1 T1 S2, each a TSL_HIDE_* bit
		if (!(vid->tsconf.tconfig & tsLayerOn[i])) continue;
		if (apart) tsl = lay[i];
		int t = i >> 1;
		int r;
		if (!next) {
			r = (i & 1) ? ts_tiles_count(vid, ln, t, t ? vid->tsconf.T1YOffset : vid->tsconf.T0YOffset,
					t ? vid->tsconf.T1XOffset : vid->tsconf.T0XOffset)
				: ts_sprites_count(vid, ln);
		} else {
			r = (i & 1) ? vidTSLRenderTiles(vid, ln, t, t ? vid->tsconf.T1YOffset : vid->tsconf.T0YOffset,
					t ? vid->tsconf.T1XOffset : vid->tsconf.T0XOffset, t ? vid->tsconf.T1GPage : vid->tsconf.T0GPage,
					t ? vid->tsconf.T1Pal76 : vid->tsconf.T0Pal76)
				: vidTSLRenderSprites(vid, ln);
		}
		res += r;
		if (r || (i & 1)) used |= 1 << i;
	}
	if (apart && next) {
		vid->tsconf.layUsed[n] = used;
		ts_compose(vid->tsconf.tsNext, lay, used & ~vid->tsconf.hide);
	}
	return res;
}

static const int tslXRes[4] = {256,320,320,360};
static const int tslYRes[4] = {192,200,240,288};
static const int tslModeTab[4] = {VID_TSL_NORMAL, VID_TSL_16, VID_TSL_256, VID_TSL_TEXT};

// Everything below derives from these ports and the layout alone, so a line
// where none of them moved skips the whole recompute (vid_set_mode's search
// and unlazy among it). The guard checks the derived side too - a snapshot or
// an spg loader pokes nogfx/vmode directly, and the next line puts them back
// the way the ports say, as the unconditional rerun used to.
void tslUpdatePorts(Video* vid) {
	unsigned char val = vid->tsconf.p00af;
	int key = val | (vid->tsconf.p07af << 8) | (vid->tsconf.tconfig << 16);
	if (key == vid->tslKey)
		return;
	vid->tslKey = key;
	vid->scrsize.x = tslXRes[(val >> 6) & 3];
	vid->scrsize.y = tslYRes[(val >> 6) & 3];
	vid->tsconf.xPos = (vid->vend.x - vid->scrsize.x) / 2;
	vid->tsconf.yPos = (vid->vend.y - vid->scrsize.y) / 2;
	int m = (vid->tsconf.tconfig & 1) ? 3 : (val >> 6) & 3;	// TSConfig b0: tiles and sprites over the whole 360x288
	vid->tsconf.tsSize.x = tslXRes[m];
	vid->tsconf.tsSize.y = tslYRes[m];
	vid->tsconf.tsXPos = (vid->vend.x - vid->tsconf.tsSize.x) / 2;
	vid->tsconf.tsYPos = (vid->vend.y - vid->tsconf.tsSize.y) / 2;
	vid_set_mode(vid, tslModeTab[val & 3]);
	vid->nogfx = !!(val & 0x20);
	val = vid->tsconf.p07af;
	vid->tsconf.scrPal = (val << 4) & 0xf0;
	vid->tsconf.T0Pal76 = (val << 2) & 0xc0;
	vid->tsconf.T1Pal76 = (val & 0xc0);
}

// HBlank start
void vts_hblk(Video* vid) {
	// the line starts here for the controller: what the cpu wrote to the latched
	// registers is taken now, and the line INT raised after it, so a handler's
	// writes wait for the line after this one (video_ports.v, video_sync.v)
	unsigned short m = vid->tsconf.lat.mask;
	if (m) {
		if (m & TSL_LAT_VCONF) vid->tsconf.p00af = vid->tsconf.lat.vconf;
		if (m & TSL_LAT_VPAGE) vid->vidPage = vid->tsconf.lat.vpage;
		if (m & TSL_LAT_GXL) vid->tsconf.soxl = vid->tsconf.lat.gxl;
		if (m & TSL_LAT_GXH) vid->tsconf.soxh = vid->tsconf.lat.gxh;
		if (m & TSL_LAT_PAL) vid->tsconf.p07af = vid->tsconf.lat.palsel;
		if (m & TSL_LAT_T0XL) vid->tsconf.t0xl = vid->tsconf.lat.t0xl;
		if (m & TSL_LAT_T0XH) vid->tsconf.t0xh = vid->tsconf.lat.t0xh;
		if (m & TSL_LAT_T1XL) vid->tsconf.t1xl = vid->tsconf.lat.t1xl;
		if (m & TSL_LAT_T1XH) vid->tsconf.t1xh = vid->tsconf.lat.t1xh;
		if (m & TSL_LAT_T0G) vid->tsconf.T0GPage = vid->tsconf.lat.t0g;
		if (m & TSL_LAT_T1G) vid->tsconf.T1GPage = vid->tsconf.lat.t1g;
		if (m & TSL_LAT_GYL) vid->tsconf.soyl = vid->tsconf.lat.gyl;
		if (m & TSL_LAT_GYH) vid->tsconf.soyh = vid->tsconf.lat.gyh;
		if (m & (TSL_LAT_GYL | TSL_LAT_GYH)) vid->tsconf.scrLine = 0;
		vid->tsconf.lat.mask = 0;
	}
	if (vid->inten & 2) {
		vid->intLINE = 1;
		vid->xirq(IRQ_VID_LINE, vid->xptr);
	}
}

// Line start
void vts_line(Video* vid) {
	tslUpdatePorts(vid);
	int load = vidTSRender(vid);
	vid->tsconf.dramLoad = (load > 448) ? 448 : load;
}

// Frame start
void vts_frame(Video* vid) {
	vid->tsconf.scrLine = 0;
}

// What a dot shows (video_render.v). gfx is the bitmap's colour, or -1 outside its window;
// vis says it is not a hole, which is what GFXOVR (VConfig b3) puts it over the tiles on.
// Only the tile/sprite layer is transparent without it: bitmap colour 0 is a colour.
static int ts_mix(Video* vid, int gfx, int vis) {
	if (vid->tsconf.hide & TSL_HIDE_GFX) gfx = -1;		// as outside its window
	int tx = vid->ray.x - vid->tsconf.tsXPos;
	int ty = vid->ray.y - vid->tsconf.tsYPos;
	int ts = -1;
	if (!(vid->tsconf.p00af & 0x10) && (tx >= 0) && (tx < vid->tsconf.tsSize.x) && (ty >= 0) && (ty < vid->tsconf.tsSize.y)
			&& (vid->line[tx] & 0x0f))		// NOTSU is VConfig b4
		ts = vid->line[tx];
	int under = (ts < 0) ? vid->brdcol : ts;		// what shows through the bitmap
	if (gfx < 0) return under;
	if (vid->tsconf.p00af & 0x08) return (vis && !vid->nogfx) ? gfx : under;
	return (ts >= 0) ? ts : (vid->nogfx ? vid->brdcol : gfx);
}

// tsconf normal screen (separated 'cuz of palette)

void vidDrawTSLNormal(Video* vid) {
	int gfx = -1;
	int vis = 0;
	xscr = vid->ray.x - vid->bord.x;
	yscr = vid->ray.y - vid->bord.y;
	if ((yscr >= 0) && (yscr < vid->scrn.y) && !vid->nogfx) {
		if ((xscr & 7) == 4) {
			adr = zx_pix_adr_xy(xscr + 4, yscr);
			nxtbyte = vid->mrd(MADR(vid->vidPage, adr), vid->xptr);
		}
		if ((xscr >= 0) && (xscr < vid->scrn.x)) {
			if ((xscr & 7) == 0) {
				scrbyte = nxtbyte;
				adr = zx_atr_adr_xy(xscr + 4, yscr);
				vid->atrbyte = vid->mrd(MADR(vid->vidPage, adr), vid->xptr);
				if ((vid->atrbyte & 0x80) && vid->flash) scrbyte ^= 0xff;
				ink = (vid->atrbyte & 0x07) | ((vid->atrbyte & 0x40) >> 3);
				pap = (vid->atrbyte & 0x78) >> 3;
			}
			vis = scrbyte & 0x80;
			gfx = vid->tsconf.scrPal | (vis ? ink : pap);
			scrbyte <<= 1;
		}
	}
	vid_dot_full(vid, ts_mix(vid, gfx, vis));
}

// tsconf extend mode (out pre-rendered bitmap/TSU layers)

void vidDrawTSLExt(Video* vid) {
	int gfx = -1;
	int vis = 0;
	xscr = vid->ray.x - vid->tsconf.xPos;
	yscr = vid->ray.y - vid->tsconf.yPos;
	if ((yscr >= 0) && (yscr < vid->scrsize.y) && (xscr >= 0) && (xscr < vid->scrsize.x)) {
		gfx = vid->linb[xscr];
		vis = (vid->vmode == VID_TSL_16) ? (gfx & 0x0f) : gfx;
	}
	vid_dot_full(vid, ts_mix(vid, gfx, vis));
}

// tsconf text: two text pixels to a dot, each mixed on its own. Hires puts out only the low
// nibble of the mix under PalSel, so the tiles, sprites and border lose their palette bits too
// (video_render.v vplex_out, video_out.v vdata)

void vidDrawTSLText(Video* vid) {
	xscr = vid->ray.x - vid->tsconf.xPos;
	yscr = vid->ray.y - vid->tsconf.yPos;
	if ((xscr < 0) || (xscr >= vid->scrsize.x) || (yscr < 0) || (yscr >= vid->scrsize.y)) {
		vid_dot_full(vid, vid->tsconf.scrPal | (ts_mix(vid, -1, 0) & 0x0f));
		return;
	}
	xscr <<= 1;
	for (int h = 0; h < 2; h++) {
		int t = vid->linb[xscr + h];
		vid_dot_half(vid, vid->tsconf.scrPal | (ts_mix(vid, t & 0x0f, t & 0x10) & 0x0f));
	}
}
