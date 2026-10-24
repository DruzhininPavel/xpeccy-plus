// The emulated ESP-01's basic commands (the AT document's chapter 3): echo, restart, version,
// sleep, the UART, RF power, the pins and the system messages, as NONOS AT 1.7.6 answers them.
// Arguments are read the firmware's way: digits only, no spaces, and the byte right after the
// name is skipped unseen (so AT+GSLP5000 reads as AT+GSLP=000).

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "esp_int.h"

#define IO_PINS		16
#define BAUD_MAX	5000000
#define LS_IDLE_MS	100		// quiet this long and joined, light sleep sets in (about a DTIM beacon)

// fixed values a real ESP-01 would measure: plausible, not taken from a module
#define FREE_HEAP	51336		// free RAM after boot
#define VDD33_NONE	65535		// the AT image reads TOUT as the ADC, so VDD33 cannot be measured
#define ADC_FLOAT	5		// TOUT is not brought out on an ESP-01: a floating input

static struct {
	int sleepType = 2;		// modem sleep
	uint32_t sysmsg = 0;		// SYSMSG_CUR
	int stop = 1;			// UART_CUR as set: 1, 2 (1.5 bits) or 3 (2 bits)
	int flow = 0;
	// the pins: IO_MUX function and pull-up, GPIO output enable and latch
	int func[IO_PINS];
	bool pull[IO_PINS];
	uint32_t gpioOut = 0;
	uint32_t gpioLatch = 0;
	// light sleep and its wake-up pin (pins -1 when not set)
	bool wake = false;
	int wakePin = -1;
	int wakeActive = 0;		// the level that wakes
	int awakePin = -1;
	int awakeLevel = 0;		// what the awake pin shows while awake
	bool asleep = false;
	long long wokeAt = 0;
} B;

// == reading arguments ==

struct espCur {
	const std::string& s;
	size_t i;
	int at(size_t k) const {return (i + k < s.size()) ? (unsigned char)s[i + k] : 0;}
	int c() const {return at(0);}
	int next() {int v = c(); i++; return v;}
};

static uint32_t u32(long v) {return (uint32_t)v;}

// The firmware's integer reader: at most 10 characters, digits and a '-' first (a later '-'
// is skipped). err: 1 an empty field, 2 a non-digit as the 10th character, 3 a lone '-'.
// Returns whether more follows: the next byte is neither the end nor \r.
static bool rd_int(espCur& p, long& val, int& err) {
	uint32_t acc = 0;
	bool neg = false;
	int i = 0;
	err = 0;
	val = 0;
	while (++i < 11) {
		int ch = p.c();
		if ((ch >= '0') && (ch <= '9')) {
			acc = acc * 10 + (uint32_t)(ch - '0');
			val = (int32_t)acc;
			p.i++;
			continue;
		}
		if (ch == '-') {
			if (i == 1) neg = true;
			p.i++;
			continue;
		}
		if (i == 10) {err = 2; return false;}
		if (i == 1) {err = 1; return true;}
		break;
	}
	if (neg && (i == 2)) {err = 3; return false;}
	val = neg ? -(long)(int32_t)acc : (long)(int32_t)acc;
	return (p.c() != 0) && (p.c() != '\r');
}

// SYSMSG's reader: unsigned, decimal or 0x hex, 32 bits; stops at the first non-digit
static bool rd_uint(espCur& p, uint32_t& val) {
	int ch = p.c();
	if ((ch < '0') || (ch > '9')) return false;
	uint32_t base = 10;
	if (ch == '0') {
		if ((p.at(1) & ~0x20) == 'X') {
			p.i += 2;
			base = 16;
		} else {
			p.i++;
		}
	}
	uint32_t acc = 0, cut = 0xffffffffu / base, lim = 0xffffffffu % base;
	for (;;) {
		ch = p.c();
		uint32_t d;
		if ((ch >= '0') && (ch <= '9')) d = (uint32_t)(ch - '0');
		else if ((base == 16) && ((uint32_t)((ch & ~0x20) - 'A') < 6)) d = (uint32_t)((ch & ~0x20) - 'A' + 10);
		else break;
		if ((acc > cut) || ((acc == cut) && (d > lim))) return false;
		acc = acc * base + d;
		p.i++;
	}
	val = acc;
	return true;
}

static uint32_t fl_uint(const char* key, uint32_t def) {
	std::string v = esp_fl_get(key);
	if (v.empty()) return def;
	char* e = nullptr;
	unsigned long n = strtoul(v.c_str(), &e, 10);
	return (e && !*e) ? (uint32_t)n : def;
}

static void put_line(const std::string& name, const char* fmt, long v) {
	char s[96];
	std::string f = "%s:" + std::string(fmt);
	snprintf(s, sizeof(s), f.c_str(), name.c_str(), v);
	esp_put(s);
}

// == the pins ==

// pins 0, 2, 4, 5 are GPIOs as IO_MUX function 0, the rest as function 3
static int gpio_func(int pin) {
	return (((pin & ~2) == 0) || (pin == 4) || (pin == 5)) ? 0 : 3;
}

static bool is_gpio(int pin) {
	return B.func[pin] == gpio_func(pin);
}

// GPIO0 and GPIO2 go to the Next; the other pads go nowhere on an ESP-01
static void pin_sync(int pin) {
	if ((pin != 0) && (pin != 2)) return;
	bool out = is_gpio(pin) && ((B.gpioOut >> pin) & 1);
	esp_gpio_drive(pin, out, (B.gpioLatch >> pin) & 1);
}

static int pin_in(Computer* comp, int pin) {
	if ((pin == 0) || (pin == 2)) return esp_gpio_level(comp, pin);
	if ((pin == 1) || (pin == 3)) return 1;		// the UART lines idle high
	return B.pull[pin] ? 1 : 0;
}

static void pin_output(int pin, int level) {
	B.gpioOut |= 1u << pin;
	if (level) B.gpioLatch |= 1u << pin;
	else B.gpioLatch &= ~(1u << pin);
	pin_sync(pin);
}

// what SYSGPIODIR/WRITE/READ refuse when the pin is not a GPIO
static bool gpio_check(int pin) {
	if (is_gpio(pin)) return true;
	esp_put("NOT GPIO MODE!\r\n");
	esp_err();
	return false;
}

// == ATE, RST, GMR, GSLP, RESTORE ==

// ATE<n>: the digit ends the name, so the number starts right here
static void ate_set(espCmdCall& c) {
	espCur p{c.p, 0};
	long v;
	int err;
	if (rd_int(p, v, err) || ((v != 0) && (v != 1))) {esp_err(); return;}
	esp_set_echo(v == 1);
	esp_ok();
}

static void rst_exec(espCmdCall& c) {
	esp_ok();
	esp_reboot(c.comp, c.at);
}

static void gmr_exec(espCmdCall& c) {
	(void)c;
	esp_put("AT version:1.7.6.0(Jan 24 2022 08:56:02)\r\nSDK version:3.0.6-dev(072755c)\r\n"
		"compile time:Jun 17 2024 07:38:00\r\nBin version(Wroom 02):1.7.6");
	esp_ok();
}

// asleep after the OK; GPIO16 does not reach RST on an ESP-01, so only a reset wakes it
static void gslp_set(espCmdCall& c) {
	espCur p{c.p, 1};
	long t;
	int err;
	if (rd_int(p, t, err) || (t < 0)) {esp_err(); return;}
	esp_ok();
	esp_deep_sleep(c.comp, t);
}

static void restore_exec(espCmdCall& c) {
	esp_fl_clear();
	esp_ok();
	esp_reboot(c.comp, c.at);
}

// == the UART ==

static int frame_of(long bits, long stop, long parity) {
	int fr = (int)((bits - 5) << 3);
	if (parity) fr |= 4;
	if (parity == 1) fr |= 2;
	if (stop == 3) fr |= 1;			// 1.5 stop bits go as 1: a receiver takes either
	return fr;
}

static int frame_bits(int fr) {return 5 + ((fr >> 3) & 3);}
static int frame_parity(int fr) {return !(fr & 4) ? 0 : (fr & 2) ? 1 : 2;}

static void uart_put(const std::string& name, long baud, int bits, int stop, int parity, int flow) {
	char s[96];
	snprintf(s, sizeof(s), "%s:%li,%i,%i,%i,%i", name.c_str(), baud, bits, stop, parity, flow);
	esp_put(s);				// no \r\n of its own: the OK's comes next
	esp_ok();
}

// what the UART runs at: the divider's rate, the field 20 bits wide
static void uart_cur_query(espCmdCall& c) {
	long b = esp_baud();
	uint32_t div = (b >= 1) ? ((uint32_t)(80000000L / b) & 0xfffff) : 0;
	int fr = esp_frame_bits();
	uart_put(c.name, div ? (long)(80000000u / div) : 0, frame_bits(fr), B.stop, frame_parity(fr), B.flow);
}

// the record as saved; an erased or bad one reads as all zeros
static void uart_def_query(espCmdCall& c) {
	long b = esp_fl_num("baud", -1);
	if ((b < 1) || (b > BAUD_MAX)) {uart_put(c.name, 0, 0, 0, 0, 0); return;}
	long fr = esp_fl_num("frame", -1);
	long stop = esp_fl_num("stop", (fr < 0) ? 0 : (fr & 1) ? 3 : 1);
	long flow = esp_fl_num("flow", 0);
	uart_put(c.name, b, (fr < 0) ? 8 : frame_bits((int)fr), ((stop >= 1) && (stop <= 3)) ? (int)stop : 0,
		(fr < 0) ? 0 : frame_parity((int)fr), ((flow >= 1) && (flow <= 3)) ? (int)flow : 0);
}

// one field and the comma after it
static bool uart_field(espCur& p, long& v) {
	int err;
	bool more = rd_int(p, v, err);
	return !err && more && (p.next() == ',');
}

// <baud>,<databits>,<stopbits>,<parity>,<flow>; OK at the old rate, then the switch
static void uart_apply(espCmdCall& c, bool save) {
	espCur p{c.p, 1};
	long b, d, s, par, f;
	int err;
	bool more = rd_int(p, b, err);
	if (err || !more || (b > BAUD_MAX) || (b == 0) || (p.next() != ',')) {esp_err(); return;}	// a negative rate passes, as on the module
	if (!uart_field(p, d) || (d < 5) || (d > 8)) {esp_err(); return;}
	if (!uart_field(p, s) || (s < 1) || (s > 3)) {esp_err(); return;}
	if (!uart_field(p, par) || (par < 0) || (par > 2)) {esp_err(); return;}
	rd_int(p, f, err);
	if (err || (f < 0) || (f > 3) || (p.c() != '\r')) {esp_err(); return;}
	int fr = frame_of(d, s, par);
	esp_ok();
	esp_set_uart(b, fr);
	B.stop = (int)s;
	B.flow = (int)f;
	if (save) {
		esp_fl_set_num("baud", b);
		esp_fl_set_num("frame", fr);
		esp_fl_set_num("stop", s);
		esp_fl_set_num("flow", f);
	}
}

static void uart_cur_set(espCmdCall& c) {uart_apply(c, false);}
static void uart_def_set(espCmdCall& c) {uart_apply(c, true);}

// the rate alone, kept as the default; the OK already at the new rate
static void ipr_set(espCmdCall& c) {
	espCur p{c.p, 1};
	long b;
	int err;
	if (rd_int(p, b, err) || (b > BAUD_MAX) || (b == 0)) {esp_err(); return;}
	esp_set_uart(b, esp_frame_bits());
	esp_fl_set_num("baud", b);
	esp_ok();
}

// == sleep and RF ==

static void sleep_query(espCmdCall& c) {
	put_line(c.name, "%li\r\n", B.sleepType);
	esp_ok();
}

static void sleep_set(espCmdCall& c) {
	espCur p{c.p, 1};
	long v;
	int err;
	if (rd_int(p, v, err) || (u32(v) >= 3)) {esp_err(); return;}
	B.sleepType = (int)v;
	esp_ok();
}

// <enable>,<trigger_GPIO>,<trigger_level>[,<awake_GPIO>,<awake_level>]
static void wakeupgpio_set(espCmdCall& c) {
	espCur p{c.p, 1};
	long en, trig = -1, lvl = -1, awake = -1, alvl = -1;
	int err;
	rd_int(p, en, err);
	if (err) {esp_err(); return;}
	if (en == 1) {
		if (p.next() != ',') {esp_err(); return;}
		rd_int(p, trig, err);
		if (err || (u32(trig) > 15) || (p.next() != ',')) {esp_err(); return;}
		rd_int(p, lvl, err);
		if (err || ((lvl != 0) && (lvl != 1))) {esp_err(); return;}
		if (p.c() == ',') {
			p.i++;
			rd_int(p, awake, err);
			// the firmware compares the awake pin with its level code (4 low, 5 high), not
			// with the trigger pin
			if (err || (u32(awake) > 15) || (awake == lvl + 4) || (p.next() != ',')) {esp_err(); return;}
			rd_int(p, alvl, err);
			if (err || (u32(alvl) >= 2)) {esp_err(); return;}
		}
	} else if (en != 0) {
		esp_err();
		return;
	}
	if (p.c() != '\r') {esp_err(); return;}
	B.wake = (en == 1);
	B.wakePin = (int)trig;
	B.wakeActive = (int)lvl;
	B.awakePin = (int)awake;
	B.awakeLevel = (int)alvl;
	B.asleep = false;
	if (B.wake) {
		B.func[trig] = gpio_func((int)trig);
		pin_sync((int)trig);
		if (awake >= 0) {
			B.func[awake] = gpio_func((int)awake);
			pin_output((int)awake, (int)alvl);
		}
	}
	esp_ok();
}

static void rfpower_set(espCmdCall& c) {
	espCur p{c.p, 1};
	long v;
	int err;
	if (rd_int(p, v, err) || (u32(v) > 82)) {esp_err(); return;}
	esp_ok();
}

static void rfvdd_query(espCmdCall& c) {
	put_line(c.name, "%li\r\n", VDD33_NONE);
	esp_ok();
}

static void rfvdd_set(espCmdCall& c) {
	espCur p{c.p, 1};
	long v;
	int err;
	if (rd_int(p, v, err)) {esp_err(); return;}
	if (u32(v - 1900) > 1400) {
		esp_put("range:1900~3300\r\n");
		esp_err();
		return;
	}
	esp_ok();
}

static void rfvdd_exec(espCmdCall& c) {
	(void)c;
	esp_ok();
}

static void rfautotrace_query(espCmdCall& c) {
	put_line(c.name, "%li\r\n", (esp_fl_num("rfautotrace", 1) == 1) ? 1 : 0);
	esp_ok();
}

static void rfautotrace_set(espCmdCall& c) {
	espCur p{c.p, 1};
	long v;
	int err;
	if (rd_int(p, v, err) || (u32(v) >= 2)) {esp_err(); return;}
	esp_fl_set_num("rfautotrace", v);
	esp_ok();
}

static void sysram_query(espCmdCall& c) {
	put_line(c.name, "%li\r\n", FREE_HEAP);
	esp_ok();
}

static void sysadc_query(espCmdCall& c) {
	put_line(c.name, "%li\r\n", ADC_FLOAT);
	esp_ok();
}

// == the pins' commands ==

// <pin>,<mode>,<pull-up>; mode is the IO_MUX function, checked as a byte
static void sysiosetcfg_set(espCmdCall& c) {
	espCur p{c.p, 1};
	long pin, mode, pull;
	int err;
	if (!rd_int(p, pin, err) || err || (u32(pin) > 15)) {esp_err(); return;}
	p.i++;
	if (!rd_int(p, mode, err) || err) {esp_err(); return;}
	int m = (int)(mode & 0xff);
	bool fine = (m < 2) || ((m == 2) && ((pin > 11) || (pin == 2))) || ((m == 3) && ((pin & ~2) != 0) && (pin != 4) && (pin != 5))
		|| ((m == 4) && (pin != 4) && (pin != 5));
	if (!fine) {esp_err(); return;}
	p.i++;
	if (rd_int(p, pull, err) || err || (u32(pull) >= 2) || (p.c() != '\r')) {esp_err(); return;}
	B.func[pin] = m & 7;
	B.pull[pin] = (pull == 1);
	pin_sync((int)pin);
	esp_ok();
}

static void sysiogetcfg_set(espCmdCall& c) {
	espCur p{c.p, 1};
	long pin;
	int err;
	if (rd_int(p, pin, err) || err || (u32(pin) > 15) || (p.c() != '\r')) {esp_err(); return;}
	char s[64];
	snprintf(s, sizeof(s), "%s:%li,%i,%i\r\n", c.name.c_str(), pin, B.func[pin], B.pull[pin] ? 1 : 0);
	esp_put(s);
	esp_ok();
}

// <pin>,<0|1>: the pin's number and a bit, as DIR and WRITE take them
static bool pin_and_bit(espCmdCall& c, long& pin, long& bit) {
	espCur p{c.p, 1};
	int err;
	if (!rd_int(p, pin, err) || err || (u32(pin) > 15)) return false;
	p.i++;
	return !rd_int(p, bit, err) && !err && (u32(bit) < 2) && (p.c() == '\r');
}

static void sysgpiodir_set(espCmdCall& c) {
	long pin, dir;
	if (!pin_and_bit(c, pin, dir)) {esp_err(); return;}
	if (!gpio_check((int)pin)) return;
	if (dir) B.gpioOut |= 1u << pin;
	else B.gpioOut &= ~(1u << pin);
	pin_sync((int)pin);
	esp_ok();
}

static void sysgpiowrite_set(espCmdCall& c) {
	long pin, level;
	if (!pin_and_bit(c, pin, level)) {esp_err(); return;}
	if (!gpio_check((int)pin)) return;
	if (!((B.gpioOut >> pin) & 1)) {
		esp_put("NOT OUTPUT!\r\n");
		esp_err();
		return;
	}
	pin_output((int)pin, (int)level);
	esp_ok();
}

// an output reads its latch, an input the pin
static void sysgpioread_set(espCmdCall& c) {
	espCur p{c.p, 1};
	long pin;
	int err;
	if (rd_int(p, pin, err) || err || (u32(pin) > 15) || (p.c() != '\r')) {esp_err(); return;}
	if (!gpio_check((int)pin)) return;
	int dir = (B.gpioOut >> pin) & 1;
	int level = dir ? (int)((B.gpioLatch >> pin) & 1) : pin_in(c.comp, (int)pin);
	char s[64];
	snprintf(s, sizeof(s), "%s:%li,%i,%i\r\n", c.name.c_str(), pin, dir, level);
	esp_put(s);
	esp_ok();
}

// == system messages ==

static void sysmsg_put(const std::string& name, uint32_t v) {
	char s[64];
	snprintf(s, sizeof(s), "%s:0x%lx\r\n", name.c_str(), (unsigned long)v);
	esp_put(s);
	esp_ok();
}

static void sysmsg_cur_query(espCmdCall& c) {sysmsg_put(c.name, B.sysmsg);}
static void sysmsg_def_query(espCmdCall& c) {sysmsg_put(c.name, fl_uint("sysmsg", 0));}

// any 32-bit value is taken: only bits 0 and 1 mean something
static void sysmsg_apply(espCmdCall& c, bool save) {
	espCur p{c.p, 1};
	uint32_t v;
	if (!rd_uint(p, v) || (p.c() != '\r')) {esp_err(); return;}
	B.sysmsg = v;
	if (save) esp_fl_set("sysmsg", std::to_string((unsigned long)v));
	esp_ok();
}

static void sysmsg_cur_set(espCmdCall& c) {sysmsg_apply(c, false);}
static void sysmsg_def_set(espCmdCall& c) {sysmsg_apply(c, true);}

int esp_sysmsg() {
	return (int)(B.sysmsg & 3);
}

// == +TEST: the SDK's example command, left in the image ==

static void test_test(espCmdCall& c) {(void)c; esp_put("at_testCmdTest\r\n"); esp_ok();}
static void test_query(espCmdCall& c) {(void)c; esp_put("at_queryCmdTest\r\n"); esp_ok();}
static void test_exec(espCmdCall& c) {(void)c; esp_put("at_exeCmdTest\r\n"); esp_ok();}

// The string goes over the buffer the first line was printed in, NUL-ended only when its
// closing quote comes within 10 characters: what is left of that line shows after it.
static std::string test_string(espCur& p, std::string buf) {
	int ch = p.c();
	if ((ch == ',') || (ch == '\r')) return "";
	if (ch != '"') return buf;
	buf.resize(32, '\0');
	size_t q = p.i + 1, n = 0;
	bool esc = false;
	for (;;) {
		if (n >= 10) {
			if (p.at(q - p.i) == '"') p.i = q + 1;
			break;
		}
		ch = p.at(q - p.i);
		if (ch == '\r') {p.i = q; break;}
		if (!esc && (ch == '\\')) {
			esc = true;
		} else if (!esc && (ch == '"')) {
			buf[n] = '\0';
			p.i = q + 1;
			break;
		} else {
			buf[n++] = (char)ch;
			esc = false;
		}
		q++;
	}
	return std::string(buf.c_str());
}

static void test_set(espCmdCall& c) {
	espCur p{c.p, 1};
	long v;
	int err;
	if (!rd_int(p, v, err) || (p.next() != ',')) {esp_err(); return;}
	char s[64];
	snprintf(s, sizeof(s), "the first parameter:%li\r\n", v);
	esp_put(s);
	esp_put("the second parameter:" + test_string(p, s) + "\r\n");
	if (p.c() == ',') {
		p.i++;
		rd_int(p, v, err);
		snprintf(s, sizeof(s), "the third parameter:%li\r\n", v);
		esp_put(s);
	}
	if (p.c() == '\r') esp_ok();
	else esp_err();
}

// == light sleep ==

// Light sleep sets in when the station is joined and the module is idle. Before it sleeps the
// firmware looks at the trigger pin: still at its waking level, it stays awake. The awake pin
// shows its level while awake and the other one while asleep. The UART is left as it is.
static void light_sleep(Computer* comp, long long now) {
	bool idle = (B.sleepType == 1) && esp_wifi_joined() && !esp_held() && !esp_out_queued()
		&& (now - esp_last_input() >= esp_ms(comp, LS_IDLE_MS)) && (now - B.wokeAt >= esp_ms(comp, LS_IDLE_MS));
	bool trig = B.wake && (pin_in(comp, B.wakePin) == B.wakeActive);
	if (B.asleep) {
		if (idle && !trig) return;
		B.asleep = false;
		B.wokeAt = now;
		if (B.wake && (B.awakePin >= 0)) pin_output(B.awakePin, B.awakeLevel);
		return;
	}
	if (!idle) return;
	if (trig) {
		if (B.awakePin >= 0) pin_output(B.awakePin, B.awakeLevel);
		return;
	}
	B.asleep = true;
	if (B.wake && (B.awakePin >= 0)) pin_output(B.awakePin, !B.awakeLevel);
}

// == the hooks ==

void esp_basic_boot(Computer* comp) {
	(void)comp;
	B.sleepType = 2;
	B.sysmsg = fl_uint("sysmsg", 0);
	long b = esp_fl_num("baud", -1);
	bool rec = (b >= 1) && (b <= BAUD_MAX);
	long fr = esp_fl_num("frame", 0x18);
	long stop = esp_fl_num("stop", (fr & 1) ? 3 : 1);
	long flow = esp_fl_num("flow", 0);
	B.stop = (rec && (stop >= 1) && (stop <= 3)) ? (int)stop : 1;
	B.flow = (rec && (flow >= 0) && (flow <= 3)) ? (int)flow : 0;
	for (int i = 0; i < IO_PINS; i++) {
		B.func[i] = 0;
		B.pull[i] = false;
	}
	B.func[2] = 2;			// GPIO2 is UART1's TX, where the firmware's debug lines go
	B.gpioOut = 0;
	B.gpioLatch = 0;
	B.wake = false;
	B.wakePin = B.awakePin = -1;
	B.asleep = false;
	B.wokeAt = 0;
	pin_sync(0);
	pin_sync(2);
}

void esp_basic_frame(Computer* comp, long long now) {
	light_sleep(comp, now);
}

// == the table, in the firmware's order ==

const espCmd esp_basic_cmds[] = {
	{"E", nullptr, nullptr, ate_set, nullptr},
	{"+RST", nullptr, nullptr, nullptr, rst_exec},
	{"+GMR", nullptr, nullptr, nullptr, gmr_exec},
	{"+GSLP", nullptr, nullptr, gslp_set, nullptr},
	{"+SLEEP", nullptr, sleep_query, sleep_set, nullptr},
	{"+IPR", nullptr, nullptr, ipr_set, nullptr},
	{"+RESTORE", nullptr, nullptr, nullptr, restore_exec},
	{"+RFPOWER", nullptr, nullptr, rfpower_set, nullptr},
	{"+RFVDD", nullptr, rfvdd_query, rfvdd_set, rfvdd_exec},
	{"+RFAUTOTRACE", nullptr, rfautotrace_query, rfautotrace_set, nullptr},
	{"+UART", nullptr, uart_cur_query, uart_def_set, nullptr},
	{"+UART_DEF", nullptr, uart_def_query, uart_def_set, nullptr},
	{"+UART_CUR", nullptr, uart_cur_query, uart_cur_set, nullptr},
	{"+WAKEUPGPIO", nullptr, nullptr, wakeupgpio_set, nullptr},
	{"+SYSRAM", nullptr, sysram_query, nullptr, nullptr},
	{"+SYSIOSETCFG", nullptr, nullptr, sysiosetcfg_set, nullptr},
	{"+SYSIOGETCFG", nullptr, nullptr, sysiogetcfg_set, nullptr},
	{"+SYSGPIODIR", nullptr, nullptr, sysgpiodir_set, nullptr},
	{"+SYSGPIOWRITE", nullptr, nullptr, sysgpiowrite_set, nullptr},
	{"+SYSGPIOREAD", nullptr, nullptr, sysgpioread_set, nullptr},
	{"+SYSADC", nullptr, sysadc_query, nullptr, nullptr},
	{"+SYSMSG_CUR", nullptr, sysmsg_cur_query, sysmsg_cur_set, nullptr},
	{"+SYSMSG_DEF", nullptr, sysmsg_def_query, sysmsg_def_set, nullptr},
	{"+TEST", test_test, test_query, test_set, test_exec},
	{nullptr, nullptr, nullptr, nullptr, nullptr}
};
