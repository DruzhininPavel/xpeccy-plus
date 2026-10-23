#pragma once

// DS1307, a real-time clock on an I2C bus (Maxim's datasheet, 19-4998 rev 10/08):
// slave address 1101000, 00-06 the time in BCD, 07 control, 08-3F 56 bytes of RAM.
// The time is the host's, moved by what the guest set it to.

#define DS_ADDR		0x68
#define DS_NV_SIZE	88		// what the battery keeps, as ds_nv_get() packs it

typedef struct {
	// what the battery keeps
	unsigned char reg[64];		// 00-07 the user buffer the bus reads and writes, 08-3F RAM
	long long ofs;			// the clock minus the host's local time, in seconds
	long long frozen;		// with CH set: the time it stands at (seconds from 2000-01-01)
	unsigned char dow;		// day of the week minus the days since 2000-01-01, mod 7
	unsigned char halted;		// CH: the oscillator stopped
	// the bus
	unsigned char scl, sda;		// the lines as they stand: the master's AND the chip's
	unsigned char out;		// what the chip puts on SDA: 1 released, 0 pulled low
	unsigned char st;		// DS_IDLE ...
	unsigned char phase;		// what the byte being taken is: address, pointer, data
	unsigned char bits;		// bits of the byte so far
	unsigned char sr;		// the byte shifting in or out
	unsigned char ack;		// the master took a byte read (its ACK)
	unsigned char ptr;		// the register pointer
} DS1307;

void ds_init(DS1307*);			// a chip that keeps the host's time
// the master's lines after a write of either (1 released, 0 pulled low); returns SDA as it stands
int ds_bus(DS1307*, int scl, int sda);
int ds_sda(DS1307*);			// the SDA line, the master's and the chip's
void ds_nv_get(DS1307*, unsigned char* buf);		// DS_NV_SIZE bytes
void ds_nv_set(DS1307*, const unsigned char* buf);	// ignored if it is not ours
