#include <string.h>
#include <time.h>

#include "ds1307.h"
#include "defines.h"

// The time is kept as seconds from 2000-01-01 00:00, local time, since that is all the chip
// can say (00-99 for the year); the host's comes in the same count through localtime().

enum {DS_IDLE = 0, DS_RX, DS_ACKRX, DS_TX, DS_ACKTX, DS_WAIT};	// WAIT: released until START or STOP
enum {DS_PADDR = 0, DS_PPTR, DS_PDATA, DS_PREAD};

#define DS_DAY		86400LL

// days from 1970-01-01 of a civil date (H. Hinnant's algorithm)
static long long days_from_civil(long long y, int m, int d) {
	long long era, yoe, doy, doe;
	y -= (m <= 2);
	era = ((y >= 0) ? y : (y - 399)) / 400;
	yoe = y - era * 400;
	doy = (153 * (m + ((m > 2) ? -3 : 9)) + 2) / 5 + d - 1;
	doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
	return era * 146097 + doe - 719468;
}

static void civil_from_days(long long z, int* y, int* m, int* d) {
	long long era, doe, yoe, doy, mp;
	z += 719468;
	era = ((z >= 0) ? z : (z - 146096)) / 146097;
	doe = z - era * 146097;
	yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
	doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
	mp = (5 * doy + 2) / 153;
	*d = (int)(doy - (153 * mp + 2) / 5 + 1);
	*m = (int)(mp + ((mp < 10) ? 3 : -9));
	*y = (int)(yoe + era * 400 + (*m <= 2));
}

#define DS_EPOCH	10957LL		// days_from_civil(2000, 1, 1)

static long long ds_secs(int y, int mo, int d, int h, int mi, int s) {
	return (days_from_civil(y, mo, d) - DS_EPOCH) * DS_DAY + h * 3600LL + mi * 60LL + s;
}

static long long ds_host(void) {
	time_t t = xhost_time_fixed ? (time_t)xhost_time_fixed : time(NULL);
	struct tm* tm = localtime(&t);
	if (!tm) return 0;
	return ds_secs(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
}

static long long ds_floor_div(long long a, long long b) {
	return (a >= 0) ? (a / b) : -((-a + b - 1) / b);
}

static int bcd(int v) {
	return ((v / 10) << 4) | (v % 10);
}

static int unbcd(int v) {
	return ((v >> 4) & 15) * 10 + (v & 15);
}

void ds_init(DS1307* d) {
	memset(d, 0x00, sizeof(DS1307));
	d->dow = 6;			// 2000-01-01 was a Saturday: day 7 with Sunday as 1, as the host's week has it
	d->scl = d->sda = d->out = 1;
}

// the time registers into the user buffer, as an I2C START does
static void ds_latch(DS1307* d) {
	long long t = d->halted ? d->frozen : (ds_host() + d->ofs);
	long long days = ds_floor_div(t, DS_DAY);
	int sec = (int)(t - days * DS_DAY);
	int y, m, dd, h = sec / 3600;
	civil_from_days(days + DS_EPOCH, &y, &m, &dd);
	d->reg[0] = bcd(sec % 60) | (d->halted ? 0x80 : 0);
	d->reg[1] = bcd((sec / 60) % 60);
	if (d->reg[2] & 0x40) {				// 12-hour mode, as the hours were last written
		int h12 = h % 12;
		d->reg[2] = 0x40 | ((h >= 12) ? 0x20 : 0) | bcd(h12 ? h12 : 12);
	} else {
		d->reg[2] = bcd(h);
	}
	d->reg[3] = (unsigned char)(((days + d->dow) % 7 + 7) % 7 + 1);
	d->reg[4] = bcd(dd);
	d->reg[5] = bcd(m);
	d->reg[6] = bcd(((y % 100) + 100) % 100);
}

// the user buffer as the clock, after a write to 00-06: the day of the week stays what it says
static void ds_apply(DS1307* d) {
	int h, mo, dd;
	long long t, days;
	if (d->reg[2] & 0x40) {
		h = unbcd(d->reg[2] & 0x1f) % 12;
		if (d->reg[2] & 0x20) h += 12;
	} else {
		h = unbcd(d->reg[2] & 0x3f);
	}
	mo = unbcd(d->reg[5] & 0x1f);
	dd = unbcd(d->reg[4] & 0x3f);
	if (mo < 1) mo = 1;
	if (mo > 12) mo = 12;
	if (dd < 1) dd = 1;
	t = ds_secs(2000 + unbcd(d->reg[6]), mo, dd, h, unbcd(d->reg[1] & 0x7f), unbcd(d->reg[0] & 0x7f));
	days = ds_floor_div(t, DS_DAY);
	d->dow = (unsigned char)((((d->reg[3] & 7) - 1 - days) % 7 + 7) % 7);
	d->halted = (d->reg[0] & 0x80) ? 1 : 0;
	if (d->halted) d->frozen = t;
	else d->ofs = t - ds_host();
}

static void ds_write(DS1307* d, int adr, int v) {
	static const unsigned char mask[8] = {0xff, 0x7f, 0x7f, 0x07, 0x3f, 0x1f, 0xff, 0x93};
	if (adr < 8) {
		d->reg[adr] = v & mask[adr];
		if (adr < 7) ds_apply(d);
	} else {
		d->reg[adr] = v;
	}
}

static void ds_load(DS1307* d) {
	d->sr = d->reg[d->ptr];
	d->ptr = (d->ptr + 1) & 0x3f;
	d->bits = 0;
	d->out = (d->sr >> 7) & 1;
	d->st = DS_TX;
}

// SCL falling: what the chip drives next changes only here, with the clock low
static void ds_fall(DS1307* d) {
	switch (d->st) {
		case DS_RX:
			if (d->bits < 8) break;
			if (d->phase == DS_PADDR) {
				if ((d->sr >> 1) != DS_ADDR) {		// not this chip's
					d->st = DS_WAIT;
					break;
				}
				if (d->sr & 1) d->phase = DS_PREAD;
			} else if (d->phase == DS_PPTR) {
				d->ptr = d->sr & 0x3f;
			} else {
				ds_write(d, d->ptr, d->sr);
				d->ptr = (d->ptr + 1) & 0x3f;
			}
			d->out = 0;				// ACK
			d->st = DS_ACKRX;
			break;
		case DS_ACKRX:
			d->out = 1;
			if (d->phase == DS_PREAD) {
				ds_load(d);
			} else {
				d->phase = (d->phase == DS_PADDR) ? DS_PPTR : DS_PDATA;
				d->bits = 0;
				d->sr = 0;
				d->st = DS_RX;
			}
			break;
		case DS_TX:
			d->bits++;
			if (d->bits < 8) {
				d->out = (d->sr >> (7 - d->bits)) & 1;
			} else {
				d->out = 1;			// the master's ACK
				d->st = DS_ACKTX;
			}
			break;
		case DS_ACKTX:
			if (d->ack) {
				ds_load(d);
			} else {
				d->out = 1;
				d->st = DS_WAIT;
			}
			break;
	}
}

int ds_bus(DS1307* d, int scl, int sda) {
	int oscl = d->scl;
	int osda = d->sda;
	scl = scl ? 1 : 0;
	sda = (sda ? 1 : 0) & d->out;
	if (oscl && scl) {
		if (osda && !sda) {			// START, or a repeated one
			d->st = DS_RX;
			d->phase = DS_PADDR;
			d->bits = 0;
			d->sr = 0;
			d->out = 1;
			ds_latch(d);
		} else if (!osda && sda) {		// STOP
			d->st = DS_IDLE;
			d->out = 1;
		}
	} else if (!oscl && scl) {			// SCL rising: the bit is taken
		if (d->st == DS_RX) {
			d->sr = (d->sr << 1) | sda;
			d->bits++;
		} else if (d->st == DS_ACKTX) {
			d->ack = !sda;
		}
	} else if (oscl && !scl) {
		ds_fall(d);
	}
	d->scl = scl;
	d->sda = (sda ? 1 : 0) & d->out;
	// the master's level is kept apart in the caller; the line with the chip's put on it
	return d->sda;
}

int ds_sda(DS1307* d) {
	return d->sda;
}

static void put64(unsigned char* p, long long v) {
	int i;
	for (i = 0; i < 8; i++) p[i] = (unsigned char)((unsigned long long)v >> (8 * i));
}

static long long get64(const unsigned char* p) {
	unsigned long long v = 0;
	int i;
	for (i = 0; i < 8; i++) v |= (unsigned long long)p[i] << (8 * i);
	return (long long)v;
}

// "DS13", a version, CH and the day offset, then the registers, the offset and the frozen time
void ds_nv_get(DS1307* d, unsigned char* buf) {
	memset(buf, 0x00, DS_NV_SIZE);
	memcpy(buf, "DS13", 4);
	buf[4] = 1;
	buf[5] = d->halted;
	buf[6] = d->dow;
	memcpy(buf + 8, d->reg, 64);
	put64(buf + 72, d->ofs);
	put64(buf + 80, d->frozen);
}

void ds_nv_set(DS1307* d, const unsigned char* buf) {
	if (memcmp(buf, "DS13", 4) || (buf[4] != 1)) return;
	d->halted = buf[5] ? 1 : 0;
	d->dow = buf[6] % 7;
	memcpy(d->reg, buf + 8, 64);
	d->ofs = get64(buf + 72);
	d->frozen = get64(buf + 80);
}
