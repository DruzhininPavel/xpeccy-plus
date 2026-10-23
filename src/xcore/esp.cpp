// The emulated ESP-01. Its replies are those of the AT 1.x firmware, byte for byte where the
// Next's programs parse them (.HTTP, NXTP, NXtel, .UART, .ESPBAUD, .TCPPING, NBNGET,
// ESPAT/WIFI.BAS): the echo, the status line after it, +IPD,<link>,<length>: before data.
// It talks at its own rate - a guest at another one reads garbage, as on the board - and
// a reply goes onto the line whole, back to back. Its flash (the AP, the default rate,
// the addresses) is nvram/esp01.conf. Time is the guest's: CLK_28 edges.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

#include "esp.h"
#include "espline.h"
#include "espnet.h"
#include "xcore.h"
#include "../libxpeccy/spectrum.h"
#include "../libxpeccy/hardware/hardware.h"
#include "../libxpeccy/xlog.h"

#define ESP_UART	0
#define ESP_LINKS	5
#define ESP_LINE_MAX	512
#define ESP_SEND_MAX	2048
#define ESP_IPD_MAX	1460		// a segment: the first +IPD of a reply carries its header
#define ESP_OUT_MAX	16384		// queued for the line before the sockets are left to wait
#define ESP_RX_MAX	65536
#define ESP_FRAME	0x18		// 8N1, as uart_framing_r

enum {ST_OFF = 0, ST_HELD, ST_LOADER, ST_BOOT, ST_READY};
enum {MD_CMD = 0, MD_SEND, MD_PASS};
enum {PD_NONE = 0, PD_START, PD_JOIN, PD_LAP};

typedef struct {
	std::string b;
	double baud;
	int frame;
	long long at;			// not on the line before this edge (0: whenever)
} espMsg;

typedef struct {
	xEspSock sock;
	bool used = false;
	bool udp = false;
	std::string host;
	int port = 0;
	std::string rx;			// received, not handed on yet
	size_t rxSeen = 0;		// how much of it the last frame end saw: the quiet test
	std::string tx;			// to the socket
	int sendOk = 0;			// a CIPSEND of this many bytes waits for its SEND OK
	bool peerGone = false;
} espLink;

// what the module keeps in its flash
typedef struct {
	std::string ssid = "Xpeccy";
	std::string pass;
	int cwmode = 1;
	long baud = 115200;
	int frame = ESP_FRAME;
	bool dhcp = true;
	std::string ip = "192.168.1.70", gw = "192.168.1.1", mask = "255.255.255.0";
	bool dnsSet = false;
	std::string dns1 = "8.8.8.8", dns2 = "8.8.4.4";
} espFlash;

static struct {
	int st = ST_OFF;
	espFlash fl;
	bool loaded = false;
	// run time
	long baud = 115200;		// requested; the line runs at esp_actual() of it
	int frame = ESP_FRAME;
	int cwmode = 1;
	bool echo = true;
	int mux = 0;
	int cipmode = 0;
	bool joined = false;
	bool dhcp = true;
	std::string ip, gw, mask, dns1, dns2;
	std::string ssid, pass;
	// boot
	int bootStage = 0;
	long long bootAt = 0;
	long long t0 = 0;
	// input
	xEspLine line;
	std::string cmd;
	bool lastCR = false;
	int mode = MD_CMD;
	int sendLink = 0, sendLeft = 0;
	bool sendEx = false;
	std::string sendBuf;
	// passthrough
	std::string passTx;
	int plus = 0;
	long long lastIn = 0;		// the edge the last byte from the guest ended on
	// a command still running
	int pend = PD_NONE;
	int pendLink = 0;
	long long pendAt = 0;
	// out
	std::deque<espMsg> out;
	size_t outBytes = 0;
	double farBaud = 0;
	int farFrame = -1;
	long long replyAt = 0;		// the reply to the line just ended starts here
	espLink link[ESP_LINKS];
	bool rst0 = false;		// NR 02 b7 as last seen
} E;

// == time and rates ==

static double esp_clk(Computer* comp) {
	return nx_clk28_hz(comp);
}

static long long esp_ms(Computer* comp, double ms) {
	return (long long)(esp_clk(comp) * ms / 1000.0);
}

// the ESP8266's UART runs from 80 MHz through a whole divider
static double esp_actual(long baud) {
	if (baud < 1) baud = 1;
	long div = 80000000L / baud;
	if (div < 1) div = 1;
	return 80000000.0 / div;
}

// not nx_uart_sync(): that may hand the next byte in while one is being taken
static long long esp_now(Computer* comp) {
	return nx_now(comp);
}

// == flash ==

static std::string esp_flash_path() {
	return conf.path.nvDir + SLASH + "esp01.conf";
}

static void esp_flash_load() {
	E.loaded = true;
	FILE* f = fopen(esp_flash_path().c_str(), "rb");
	if (!f) return;
	char buf[512];
	while (fgets(buf, sizeof(buf), f)) {
		std::string s(buf);
		while (!s.empty() && ((s.back() == '\n') || (s.back() == '\r'))) s.pop_back();
		size_t eq = s.find(" = ");
		if (eq == std::string::npos) continue;
		std::string k = s.substr(0, eq), v = s.substr(eq + 3);
		if (k == "ssid") E.fl.ssid = v;
		else if (k == "pass") E.fl.pass = v;
		else if (k == "cwmode") E.fl.cwmode = atoi(v.c_str());
		else if (k == "baud") E.fl.baud = atol(v.c_str());
		else if (k == "frame") E.fl.frame = atoi(v.c_str()) & 0x1f;
		else if (k == "dhcp") E.fl.dhcp = (v == "yes");
		else if (k == "ip") E.fl.ip = v;
		else if (k == "gw") E.fl.gw = v;
		else if (k == "mask") E.fl.mask = v;
		else if (k == "dns") E.fl.dnsSet = (v == "yes");
		else if (k == "dns1") E.fl.dns1 = v;
		else if (k == "dns2") E.fl.dns2 = v;
	}
	fclose(f);
	if ((E.fl.cwmode < 1) || (E.fl.cwmode > 3)) E.fl.cwmode = 1;
	if ((E.fl.baud < 110) || (E.fl.baud > 4608000)) E.fl.baud = 115200;
}

static void esp_flash_save() {
	FILE* f = fopen(esp_flash_path().c_str(), "wb");
	if (!f) return;
	fprintf(f, "ssid = %s\npass = %s\ncwmode = %i\nbaud = %li\nframe = %i\ndhcp = %s\n", E.fl.ssid.c_str(),
		E.fl.pass.c_str(), E.fl.cwmode, E.fl.baud, E.fl.frame, E.fl.dhcp ? "yes" : "no");
	fprintf(f, "ip = %s\ngw = %s\nmask = %s\ndns = %s\ndns1 = %s\ndns2 = %s\n", E.fl.ip.c_str(), E.fl.gw.c_str(),
		E.fl.mask.c_str(), E.fl.dnsSet ? "yes" : "no", E.fl.dns1.c_str(), E.fl.dns2.c_str());
	fclose(f);
}

// == the line out ==

static void esp_trace(const char* dir, const std::string& s) {
	if (!xlog_on(XLG_NET, XLL_DEBUG)) return;
	std::string t;
	for (size_t i = 0; (i < s.size()) && (t.size() < 160); i++) {
		unsigned char c = s[i];
		if (c == '\r') t += "\\r";
		else if (c == '\n') t += "\\n";
		else if ((c < 0x20) || (c > 0x7e)) {
			char h[8];
			snprintf(h, sizeof(h), "\\x%02X", c);
			t += h;
		} else t += (char)c;
	}
	if (t.size() >= 160) t += "...";
	xlog(XLG_NET, XLL_DEBUG, "esp %s %s", dir, t.c_str());
}

static void esp_put_at(const std::string& s, long long at, double baud = 0, int frame = -1) {
	if (s.empty()) return;
	espMsg m;
	m.b = s;
	m.baud = baud ? baud : esp_actual(E.baud);
	m.frame = (frame < 0) ? E.frame : frame;
	m.at = at;
	E.out.push_back(m);
	E.outBytes += s.size();
}

static void esp_put(const std::string& s) {
	esp_trace(">", s);
	esp_put_at(s, E.replyAt);
}

// whole messages onto the line, each at its own rate; a new rate waits for the line
static void esp_pump(Computer* comp) {
	while (!E.out.empty()) {
		espMsg& m = E.out.front();
		if ((m.baud != E.farBaud) || (m.frame != E.farFrame)) {
			if (nx_uart_feed_left(comp, ESP_UART)) break;
			E.farBaud = m.baud;
			E.farFrame = m.frame;
		}
		if (nx_uart_feed_room(comp, ESP_UART) < (int)m.b.size()) break;
		// asked again every time: a power-on of the machine clears it
		nx_uart_far(comp, ESP_UART, (long long)(esp_clk(comp) / E.farBaud * 65536.0), E.farFrame);
		nx_uart_feed_at(comp, ESP_UART, (const unsigned char*)m.b.data(), (int)m.b.size(), m.at);
		E.outBytes -= m.b.size();
		E.out.pop_front();
	}
}

// == links ==

static std::string lid(int id) {
	return E.mux ? (std::to_string(id) + ",") : std::string();
}

static void link_drop(int id) {
	espLink& l = E.link[id];
	if (l.used) xlog(XLG_NET, XLL_INFO, "esp: link %i to %s:%i closed", id, l.host.c_str(), l.port);
	l.sock.close();
	l.used = false;
	l.rx.clear();
	l.tx.clear();
	l.rxSeen = 0;
	l.sendOk = 0;
	l.peerGone = false;
}

static void links_drop() {
	for (int i = 0; i < ESP_LINKS; i++) link_drop(i);
}

// == boot ==

static const char* esp_rom_log =
	"\r\n ets Jan  8 2013,rst cause:2, boot mode:(3,6)\r\n\r\nload 0x40100000, len 1856, room 16 \r\n"
	"tail 0\r\nchksum 0x63\r\nload 0x3ffe8000, len 776, room 8 \r\ntail 0\r\nchksum 0x02\r\n"
	"load 0x3ffe8310, len 552, room 8 \r\ntail 0\r\nchksum 0x79\r\ncsum 0x79\r\n\r\n"
	"2nd boot version : 1.5\r\n  SPI Speed      : 40MHz\r\n  SPI Mode       : DIO\r\n"
	"  SPI Flash Size & Map: 8Mbit(512KB+512KB)\r\njump to run user1 @ 1000\r\n\r\n";

static void esp_boot(Computer* comp, long long now) {
	links_drop();				// no CLOSED: the module is gone
	E.st = ST_BOOT;
	E.bootStage = 0;
	E.t0 = now;
	E.bootAt = now + esp_ms(comp, 60);
	E.echo = true;
	E.mux = 0;
	E.cipmode = 0;
	E.mode = MD_CMD;
	E.pend = PD_NONE;
	E.cmd.clear();
	E.lastCR = false;
	E.plus = 0;
	E.passTx.clear();
	E.joined = false;
	E.baud = E.fl.baud;
	E.frame = E.fl.frame;
	E.cwmode = E.fl.cwmode;
	E.ssid = E.fl.ssid;
	E.pass = E.fl.pass;
	E.dhcp = E.fl.dhcp;
	E.ip = E.fl.ip;
	E.gw = E.fl.gw;
	E.mask = E.fl.mask;
	E.dns1 = E.fl.dns1;
	E.dns2 = E.fl.dns2;
	E.replyAt = 0;
	xlog(XLG_NET, XLL_INFO, "esp: boot at %li", E.baud);
}

static void esp_boot_step(Computer* comp, long long now) {
	while ((E.bootStage < 4) && (now >= E.bootAt)) {
		E.replyAt = 0;
		switch (E.bootStage) {
			case 0:			// the ROM's own log, at 74880: garbage at any other rate
				esp_put_at(esp_rom_log, 0, 74880.0, ESP_FRAME);
				E.bootAt = now + esp_ms(comp, 240);
				break;
			case 1:
				esp_put("\r\nready\r\n");
				E.st = ST_READY;
				E.bootAt = now + esp_ms(comp, 1000);
				break;
			case 2:
				if (E.ssid.empty() || (E.cwmode == 2)) {
					E.bootStage = 4;
					return;
				}
				esp_put("WIFI CONNECTED\r\n");
				E.bootAt = now + esp_ms(comp, 300);
				break;
			case 3:
				esp_put("WIFI GOT IP\r\n");
				E.joined = true;
				break;
		}
		E.bootStage++;
	}
}

// == arguments ==

// comma separated, a "quoted" field with \ escaping , " and \ in it
static std::vector<std::string> esp_args(const std::string& s, std::vector<bool>* quoted = nullptr) {
	std::vector<std::string> v;
	std::string cur;
	bool q = false, wasq = false;
	for (size_t i = 0; i < s.size(); i++) {
		char c = s[i];
		if (q) {
			if ((c == '\\') && (i + 1 < s.size())) cur += s[++i];
			else if (c == '"') q = false;
			else cur += c;
		} else if (c == '"') {
			q = wasq = true;
		} else if (c == ',') {
			v.push_back(cur);
			if (quoted) quoted->push_back(wasq);
			cur.clear();
			wasq = false;
		} else {
			cur += c;
		}
	}
	v.push_back(cur);
	if (quoted) quoted->push_back(wasq);
	return v;
}

static bool esp_num(const std::string& s, long* n) {
	if (s.empty()) return false;
	char* e = nullptr;
	long v = strtol(s.c_str(), &e, 10);
	if (*e) return false;
	*n = v;
	return true;
}

// == commands ==

static void ok() {esp_put("\r\nOK\r\n");}
static void err() {esp_put("\r\nERROR\r\n");}

static std::string esp_mac() {return "5c:cf:7f:58:50:2b";}
static std::string esp_bssid() {return "5c:cf:7f:00:00:01";}

// NAME, NAME_CUR, NAME_DEF: which one, and whether it is kept in flash
static bool esp_is(const std::string& name, const char* base, bool* def) {
	std::string b(base);
	if (name == b) {*def = true; return true;}
	if (name == b + "_DEF") {*def = true; return true;}
	if (name == b + "_CUR") {*def = false; return true;}
	return false;
}

static void cmd_cipstart(Computer* comp, const std::string& arg) {
	std::vector<std::string> a = esp_args(arg);
	int id = 0, k = 0;
	if (E.mux) {
		long n;
		if (!esp_num(a[0], &n) || (n < 0) || (n >= ESP_LINKS)) {err(); return;}
		id = (int)n;
		k = 1;
	}
	if ((int)a.size() < k + 3) {err(); return;}
	std::string type = a[k], host = a[k + 1];
	long port;
	if (!esp_num(a[k + 2], &port) || (port < 1) || (port > 65535) || host.empty()) {err(); return;}
	if ((type != "TCP") && (type != "UDP")) {err(); return;}	// no TLS
	if (E.cipmode && E.mux) {err(); return;}
	if (!E.joined) {esp_put("no ip\r\n\r\nERROR\r\n"); return;}
	espLink& l = E.link[id];
	if (l.used) {esp_put("ALREADY CONNECTED\r\n\r\nERROR\r\n"); return;}
	l.used = true;
	l.udp = (type == "UDP");
	l.host = host;
	l.port = (int)port;
	l.rx.clear();
	l.tx.clear();
	l.rxSeen = 0;
	l.sendOk = 0;
	l.peerGone = false;
	l.sock.open(l.udp, host, (int)port);
	xlog(XLG_NET, XLL_INFO, "esp: link %i %s to %s:%li", id, type.c_str(), host.c_str(), port);
	E.pend = PD_START;
	E.pendLink = id;
	E.pendAt = esp_now(comp) + esp_ms(comp, 10000);	// the module gives up
}

static void cmd_cipsend(const std::string& arg, bool ex) {
	std::vector<std::string> a = esp_args(arg);
	int id = 0;
	long n;
	if (E.mux) {
		long i;
		if ((a.size() < 2) || !esp_num(a[0], &i) || (i < 0) || (i >= ESP_LINKS)) {err(); return;}
		id = (int)i;
		if (!esp_num(a[1], &n)) {err(); return;}
	} else {
		if (!esp_num(a[0], &n)) {err(); return;}
	}
	if (E.cipmode || (n < 1) || (n > ESP_SEND_MAX)) {err(); return;}
	if (!E.link[id].used || (E.link[id].sock.state() != xEspSock::S_OPEN)) {
		esp_put("link is not valid\r\n\r\nERROR\r\n");
		return;
	}
	esp_put("\r\nOK\r\n> ");
	E.mode = MD_SEND;			// from the dispatch: NXtel does not wait for the >
	E.sendLink = id;
	E.sendLeft = (int)n;
	E.sendEx = ex;
	E.sendBuf.clear();
}

static void cmd_uart(const std::string& name, const std::string& arg, bool query, bool def) {
	if (query) {
		long b = def ? E.fl.baud : (long)esp_actual(E.baud);	// 115200 reads 115273
		int fr = def ? E.fl.frame : E.frame;
		int bits = 5 + ((fr >> 3) & 3);
		int par = !(fr & 4) ? 0 : (fr & 2) ? 1 : 2;
		char s[96];
		snprintf(s, sizeof(s), "\r\n+%s:%li,%i,%i,%i,0\r\n\r\nOK\r\n", name.c_str(), b, bits, (fr & 1) ? 3 : 1, par);
		esp_put(s);
		return;
	}
	std::vector<std::string> a = esp_args(arg);
	long b, d, s, p, f;
	if ((a.size() != 5) || !esp_num(a[0], &b) || !esp_num(a[1], &d) || !esp_num(a[2], &s) || !esp_num(a[3], &p)
		|| !esp_num(a[4], &f) || (b < 110) || (b > 4608000) || (d < 5) || (d > 8) || (s < 1) || (s > 3)
		|| (p < 0) || (p > 2) || (f < 0) || (f > 3)) {
		err();
		return;
	}
	int fr = (int)(((d - 5) << 3) | (p ? 4 : 0) | ((p == 1) ? 2 : 0) | ((s == 3) ? 1 : 0));
	ok();				// at the old rate
	E.baud = b;
	E.frame = fr;
	if (def) {
		E.fl.baud = b;
		E.fl.frame = fr;
		esp_flash_save();
	}
	xlog(XLG_NET, XLL_INFO, "esp: rate %li (%.0f)", b, esp_actual(b));
}

static void esp_exec(Computer* comp, const std::string& line) {
	if (line.compare(0, 2, "AT") != 0) {err(); return;}
	std::string rest = line.substr(2);
	if (rest.empty()) {ok(); return;}
	if (rest == "E0") {ok(); E.echo = false; return;}
	if (rest == "E1") {ok(); E.echo = true; return;}
	if (rest[0] != '+') {err(); return;}
	size_t p = rest.find_first_of("=?");
	std::string name = rest.substr(1, (p == std::string::npos) ? std::string::npos : p - 1);
	bool query = (p != std::string::npos) && (rest.substr(p) == "?");
	bool set = (p != std::string::npos) && (rest[p] == '=');
	std::string arg = set ? rest.substr(p + 1) : std::string();
	if (set && (arg == "?")) {err(); return;}		// no test forms
	bool def = true;
	char s[256];
	if (name == "RST" && !set && !query) {
		ok();
		esp_boot(comp, esp_now(comp) + esp_ms(comp, 2));
		return;
	}
	if (name == "RESTORE" && !set && !query) {
		ok();
		E.fl = espFlash();
		E.fl.ssid.clear();		// factory: no AP stored
		esp_flash_save();
		esp_boot(comp, esp_now(comp) + esp_ms(comp, 2));
		return;
	}
	if (name == "GMR" && !set && !query) {
		esp_put("\r\nAT version:1.2.0.0(Jul  1 2016 20:04:45)\r\nSDK version:1.5.4.1(39cb9a32)\r\n"
			"Xpeccy+ emulated ESP-01\r\nOct 22 2026 00:00:00\r\n\r\nOK\r\n");
		return;
	}
	if (esp_is(name, "CWMODE", &def)) {
		long m;
		if (query) {
			snprintf(s, sizeof(s), "\r\n+%s:%i\r\n\r\nOK\r\n", name.c_str(), def ? E.fl.cwmode : E.cwmode);
			esp_put(s);
		} else if (set && esp_num(arg, &m) && (m >= 1) && (m <= 3)) {
			E.cwmode = (int)m;
			if (def) {E.fl.cwmode = (int)m; esp_flash_save();}
			ok();
		} else err();
		return;
	}
	if (esp_is(name, "CWJAP", &def)) {
		if (query) {
			if (E.joined) {
				snprintf(s, sizeof(s), "\r\n+%s:\"%s\",\"%s\",6,-55\r\n\r\nOK\r\n", name.c_str(), E.ssid.c_str(), esp_bssid().c_str());
				esp_put(s);
			} else esp_put("\r\nNo AP\r\n\r\nOK\r\n");
			return;
		}
		std::vector<std::string> a = esp_args(arg);
		if (!set || (a.size() < 2) || a[0].empty() || (E.cwmode == 2)) {err(); return;}
		if (E.joined) esp_put("WIFI DISCONNECT\r\n");
		links_drop();
		E.joined = false;
		E.ssid = a[0];
		E.pass = a[1];
		if (def) {E.fl.ssid = a[0]; E.fl.pass = a[1]; esp_flash_save();}
		xlog(XLG_NET, XLL_INFO, "esp: joining \"%s\"", a[0].c_str());
		E.pend = PD_JOIN;
		E.pendAt = esp_now(comp) + esp_ms(comp, 1500);
		return;
	}
	if ((name == "CWLAP") && !query) {
		E.pend = PD_LAP;
		E.pendAt = esp_now(comp) + esp_ms(comp, 2000);
		return;
	}
	if ((name == "CWQAP") && !set && !query) {
		ok();
		if (E.joined) esp_put("WIFI DISCONNECT\r\n");
		links_drop();
		E.joined = false;
		return;
	}
	if (esp_is(name, "CWDHCP", &def)) {
		std::vector<std::string> a = esp_args(arg);
		long m, en;
		if (query) {
			snprintf(s, sizeof(s), "\r\n+%s:%i\r\n\r\nOK\r\n", name.c_str(), E.dhcp ? 3 : 0);
			esp_put(s);
		} else if (set && (a.size() == 2) && esp_num(a[0], &m) && esp_num(a[1], &en) && (m >= 0) && (m <= 2) && (en >= 0) && (en <= 1)) {
			if (m != 1) {
				E.dhcp = (en != 0);
				if (def) {E.fl.dhcp = E.dhcp; esp_flash_save();}
			}
			ok();
		} else err();
		return;
	}
	if (esp_is(name, "CIPSTA", &def)) {
		if (query) {
			snprintf(s, sizeof(s), "\r\n+%s:ip:\"%s\"\r\n+%s:gateway:\"%s\"\r\n+%s:netmask:\"%s\"\r\n\r\nOK\r\n",
				name.c_str(), E.joined ? E.ip.c_str() : "0.0.0.0", name.c_str(), E.joined ? E.gw.c_str() : "0.0.0.0",
				name.c_str(), E.joined ? E.mask.c_str() : "0.0.0.0");
			esp_put(s);
			return;
		}
		std::vector<std::string> a = esp_args(arg);
		if (!set || a[0].empty() || (a.size() == 2) || (a.size() > 3)) {err(); return;}
		E.ip = a[0];
		if (a.size() == 3) {E.gw = a[1]; E.mask = a[2];}
		E.dhcp = false;
		if (def) {E.fl.ip = E.ip; E.fl.gw = E.gw; E.fl.mask = E.mask; E.fl.dhcp = false; esp_flash_save();}
		ok();
		return;
	}
	if ((name == "CIPDNS_CUR") || (name == "CIPDNS_DEF")) {
		def = (name == "CIPDNS_DEF");
		if (query) {
			snprintf(s, sizeof(s), "\r\n+%s:%s\r\n+%s:%s\r\n\r\nOK\r\n", name.c_str(), E.dns1.c_str(), name.c_str(), E.dns2.c_str());
			esp_put(s);
			return;
		}
		std::vector<std::string> a = esp_args(arg);
		long en;
		if (!set || !esp_num(a[0], &en) || (en < 0) || (en > 1) || (en && (a.size() < 2)) || (a.size() > 3)) {err(); return;}
		if (en) {
			E.dns1 = a[1];
			if (a.size() > 2) E.dns2 = a[2];
		} else {
			E.dns1 = "8.8.8.8";
			E.dns2 = "8.8.4.4";
		}
		if (def) {E.fl.dnsSet = (en != 0); E.fl.dns1 = E.dns1; E.fl.dns2 = E.dns2; esp_flash_save();}
		ok();
		return;
	}
	if ((name == "CIFSR") && !set && !query) {
		std::string r = "\r\n";
		if (E.cwmode != 1) r += "+CIFSR:APIP,\"192.168.4.1\"\r\n+CIFSR:APMAC,\"5e:cf:7f:58:50:2b\"\r\n";
		if (E.cwmode != 2) {
			r += "+CIFSR:STAIP,\"" + (E.joined ? E.ip : std::string("0.0.0.0")) + "\"\r\n";
			r += "+CIFSR:STAMAC,\"" + esp_mac() + "\"\r\n";
		}
		esp_put(r + "\r\nOK\r\n");
		return;
	}
	if (name == "CIPMUX") {
		long m;
		if (query) {
			snprintf(s, sizeof(s), "\r\n+CIPMUX:%i\r\n\r\nOK\r\n", E.mux);
			esp_put(s);
		} else if (set && esp_num(arg, &m) && ((m == 0) || (m == 1))) {
			bool any = false;
			for (int i = 0; i < ESP_LINKS; i++) any |= E.link[i].used;
			if (any) esp_put("link is builded\r\n\r\nERROR\r\n");
			else if (m && E.cipmode) err();
			else {E.mux = (int)m; ok();}
		} else err();
		return;
	}
	if (name == "CIPMODE") {
		long m;
		if (query) {
			snprintf(s, sizeof(s), "\r\n+CIPMODE:%i\r\n\r\nOK\r\n", E.cipmode);
			esp_put(s);
		} else if (set && esp_num(arg, &m) && ((m == 0) || (m == 1)) && !(m && E.mux)) {
			E.cipmode = (int)m;
			ok();
		} else err();
		return;
	}
	if ((name == "CIPSTATUS") && !set && !query) {
		bool any = false;
		std::string r;
		for (int i = 0; i < ESP_LINKS; i++) {
			espLink& l = E.link[i];
			if (!l.used || (l.sock.state() != xEspSock::S_OPEN)) continue;
			any = true;
			snprintf(s, sizeof(s), "+CIPSTATUS:%i,\"%s\",\"%s\",%i,%i,0\r\n", i, l.udp ? "UDP" : "TCP", l.host.c_str(), l.port, 1024 + i);
			r += s;
		}
		snprintf(s, sizeof(s), "\r\nSTATUS:%i\r\n", !E.joined ? 5 : any ? 3 : 2);
		esp_put(s + r + "\r\nOK\r\n");
		return;
	}
	if ((name == "CIPSTART") && set) {cmd_cipstart(comp, arg); return;}
	if ((name == "CIPSEND") && set) {cmd_cipsend(arg, false); return;}
	if ((name == "CIPSENDEX") && set) {cmd_cipsend(arg, true); return;}
	if ((name == "CIPSEND") && !set && !query) {
		// passthrough: everything the guest sends goes to the link, +++ ends it
		if (!E.cipmode || E.mux || !E.link[0].used || (E.link[0].sock.state() != xEspSock::S_OPEN)) {err(); return;}
		esp_put("\r\nOK\r\n\r\n>");
		E.mode = MD_PASS;
		E.passTx.clear();
		E.plus = 0;
		return;
	}
	if ((name == "CIPCLOSE") && !query) {
		long id = 0;
		if (E.mux && (!set || !esp_num(arg, &id) || (id < 0) || (id > ESP_LINKS))) {err(); return;}
		if (!E.mux && set) {err(); return;}
		bool any = false;
		std::string r;
		for (int i = 0; i < ESP_LINKS; i++) {
			if (!E.link[i].used || ((id != ESP_LINKS) && (i != id))) continue;
			any = true;
			r += lid(i) + "CLOSED\r\n";
			link_drop(i);
		}
		if (!any) {err(); return;}
		esp_put(r + "\r\nOK\r\n");
		return;
	}
	if (name == "CIPSTO") {
		if (query) esp_put("\r\n+CIPSTO:180\r\n\r\nOK\r\n");
		else err();			// refused with no server running
		return;
	}
	if ((name == "UART_CUR") || (name == "UART_DEF") || (name == "UART")) {
		if ((name == "UART") && query) {err(); return;}
		if (!set && !query) {err(); return;}
		cmd_uart(name, arg, query, name != "UART_CUR");
		return;
	}
	if ((name == "CWAUTOCONN") && set) {ok(); return;}
	err();
}

// == input ==

static void esp_dispatch(Computer* comp, long long end) {
	std::string line = E.cmd;
	E.cmd.clear();
	if (E.echo) {
		E.replyAt = end;
		esp_put("\r\n");
	}
	E.replyAt = end;
	if (E.pend != PD_NONE) {		// the line is lost
		esp_put("busy p...\r\n");
		return;
	}
	if (!line.empty() && (line.back() == '\r')) line.pop_back();
	esp_trace("<", (line.compare(0, 9, "AT+CWJAP=") == 0) ? line.substr(0, 9) + "..." : line);
	if (line.size() > ESP_LINE_MAX) {err(); return;}
	esp_exec(comp, line);
}

static void esp_send_done() {
	espLink& l = E.link[E.sendLink];
	char s[64];
	snprintf(s, sizeof(s), "\r\nRecv %i bytes\r\n", (int)E.sendBuf.size());
	esp_put(s);
	if (l.used) {
		l.tx += E.sendBuf;
		l.sendOk = (int)E.sendBuf.size();
	} else {
		esp_put("\r\nSEND FAIL\r\n");
	}
	E.sendBuf.clear();
	E.mode = MD_CMD;
}

static void esp_input(Computer* comp, int c, long long end) {
	long long gap = end - E.lastIn;
	E.lastIn = end;
	if (E.st != ST_READY) return;			// booting, held or in its loader: lost
	E.replyAt = end;
	if (E.mode == MD_SEND) {
		E.sendBuf += (char)c;
		if (E.sendEx && (E.sendBuf.size() >= 2) && (E.sendBuf.compare(E.sendBuf.size() - 2, 2, "\\0") == 0)) {
			E.sendBuf.resize(E.sendBuf.size() - 2);
			E.sendLeft = 0;
		} else {
			E.sendLeft--;
		}
		if (E.sendLeft <= 0) esp_send_done();
		return;
	}
	if (E.mode == MD_PASS) {
		long long guard = esp_ms(comp, 20);
		if ((c == '+') && (E.plus < 3) && ((E.plus > 0) || (gap >= guard))) {
			E.plus++;			// perhaps the escape: held until the guard after it
			return;
		}
		if (E.plus) {
			E.passTx.append(E.plus, '+');
			E.plus = 0;
		}
		E.passTx += (char)c;
		return;
	}
	if (c == '\n') {
		if (E.lastCR) esp_dispatch(comp, end);	// a stray LF is nothing
		E.lastCR = false;
		return;
	}
	E.lastCR = (c == '\r');
	if (E.cmd.size() <= ESP_LINE_MAX) E.cmd += (char)c;
	if (E.echo) {
		E.replyAt = end;
		esp_put_at(std::string(1, (char)c), end);
	}
}

// == the frame end ==

static void esp_pending(Computer* comp, long long now) {
	E.replyAt = 0;
	if (E.pend == PD_START) {
		espLink& l = E.link[E.pendLink];
		int st = l.sock.poll();
		if (st == xEspSock::S_OPEN) {
			E.pend = PD_NONE;
			esp_put(lid(E.pendLink) + "CONNECT\r\n\r\nOK\r\n");
		} else if ((st == xEspSock::S_FAILED) || (now >= E.pendAt)) {
			E.pend = PD_NONE;
			bool dns = (l.sock.failure() == xEspSock::F_DNS);
			xlog(XLG_NET, XLL_INFO, "esp: %s:%i %s", l.host.c_str(), l.port, dns ? "not found" : "refused");
			link_drop(E.pendLink);
			esp_put(dns ? "DNS Fail\r\n\r\nERROR\r\n" : "\r\nERROR\r\nCLOSED\r\n");
		}
	} else if ((E.pend == PD_JOIN) && (now >= E.pendAt)) {
		E.pend = PD_NONE;
		E.joined = true;
		esp_put("WIFI CONNECTED\r\nWIFI GOT IP\r\n\r\nOK\r\n");
	} else if ((E.pend == PD_LAP) && (now >= E.pendAt)) {
		E.pend = PD_NONE;
		std::string ap = E.ssid.empty() ? std::string("Xpeccy") : E.ssid;
		esp_put("+CWLAP:(3,\"" + ap + "\",-55,\"" + esp_bssid() + "\",6,-12,0)\r\n\r\nOK\r\n");
	}
}

static void esp_net(Computer* comp, long long now) {
	E.replyAt = 0;
	unsigned char buf[4096];
	for (int i = 0; i < ESP_LINKS; i++) {
		espLink& l = E.link[i];
		if (!l.used || ((E.pend == PD_START) && (E.pendLink == i))) continue;
		l.sock.poll();
		// what the guest sent
		if (!l.tx.empty() && (l.sock.state() == xEspSock::S_OPEN)) {
			int n = l.sock.send((const unsigned char*)l.tx.data(), (int)l.tx.size());
			if (n > 0) l.tx.erase(0, n);
			if (n < 0) l.tx.clear();
		}
		if (l.sendOk && l.tx.empty()) {
			l.sendOk = 0;
			esp_put("\r\nSEND OK\r\n");	// before any +IPD it brings
		}
		// what came back, none of it before its SEND OK
		bool pass = (E.mode == MD_PASS) && (i == 0);
		if (!l.sendOk && !l.peerGone && (E.outBytes < ESP_OUT_MAX)) {
			while (l.rx.size() < ESP_RX_MAX) {
				int n = l.sock.recv(buf, sizeof(buf));
				if (n < 0) {l.peerGone = true; break;}
				if (n == 0) break;
				if (l.udp) {		// a datagram is a packet of its own
					char h[32];
					snprintf(h, sizeof(h), "\r\n+IPD,%s%i:", lid(i).c_str(), n);
					esp_put_at(h + std::string((const char*)buf, n), 0);
					continue;
				}
				l.rx.append((const char*)buf, n);
			}
		}
		// a packet when the wire is quiet or a segment is full
		while (!l.rx.empty() && (E.outBytes < ESP_OUT_MAX) && (pass || (l.rx.size() >= ESP_IPD_MAX) || (l.rx.size() == l.rxSeen) || l.peerGone)) {
			size_t n = (l.rx.size() > ESP_IPD_MAX) ? ESP_IPD_MAX : l.rx.size();
			if (pass) {
				esp_put_at(l.rx.substr(0, n), 0);
			} else {
				char h[32];
				snprintf(h, sizeof(h), "\r\n+IPD,%s%i:", lid(i).c_str(), (int)n);
				esp_trace(">", h);
				esp_put_at(h + l.rx.substr(0, n), 0);
			}
			l.rx.erase(0, n);
		}
		l.rxSeen = l.rx.size();
		if (l.peerGone && l.rx.empty()) {
			if (!pass) esp_put(lid(i) + "CLOSED\r\n");
			link_drop(i);
		}
	}
	// passthrough out: a packet after 20 ms of quiet or at 2048 bytes; +++ alone ends it
	if (E.mode == MD_PASS) {
		long long guard = esp_ms(comp, 20);
		bool quiet = (now - E.lastIn) >= guard;
		if (E.plus == 3 && quiet) {
			E.plus = 0;
			E.mode = MD_CMD;
			xlog(XLG_NET, XLL_DEBUG, "esp: passthrough ends");
		} else if (E.plus && quiet) {
			E.passTx.append(E.plus, '+');
			E.plus = 0;
		}
		if (!E.passTx.empty() && (quiet || (E.passTx.size() >= ESP_SEND_MAX))) {
			E.link[0].tx += E.passTx;
			E.passTx.clear();
		}
	}
}

// == the outside ==

void esp_attach(Computer* comp) {
	if (!E.loaded) esp_flash_load();
	E.line.reset();
	E.out.clear();
	E.outBytes = 0;
	E.farBaud = 0;
	E.farFrame = -1;
	E.lastIn = 0;
	E.rst0 = false;
	long long now = nx_uart_sync(comp);
	esp_boot(comp, now);
	next_esp_fitted = 1;
	xlog(XLG_NET, XLL_INFO, "esp: on UART 0");
}

void esp_detach(Computer* comp) {
	if (E.st == ST_OFF) return;
	links_drop();
	E.out.clear();
	E.outBytes = 0;
	E.st = ST_OFF;
	next_esp_fitted = 0;
	if (comp && comp->hw && (comp->hw->id == HW_ZXNEXT)) nx_uart_far(comp, ESP_UART, 0, 0);
	xlog(XLG_NET, XLL_INFO, "esp: off");
}

static void esp_line_in(Computer* comp, long long now) {
	long presc;
	int frame, rtr;
	nx_uart_cfg(comp, ESP_UART, &presc, &frame, &rtr);
	E.line.setRate(esp_clk(comp) / esp_actual(E.baud), E.frame);
	std::vector<int> got;
	std::vector<long long> ends;
	E.line.take(now, got, &ends);
	for (size_t k = 0; k < got.size(); k++) esp_input(comp, got[k], ends[k]);
}

void esp_byte(Computer* comp, int b, long long start) {
	if ((E.st == ST_OFF) || comp->rzx.play) return;
	long presc;
	int frame, rtr;
	nx_uart_cfg(comp, ESP_UART, &presc, &frame, &rtr);
	E.line.put(b, start, presc, frame & 0x1f);
	esp_line_in(comp, 0);
	esp_pump(comp);
}

void esp_frame(Computer* comp) {
	if ((E.st == ST_OFF) || comp->rzx.play) return;
	long long now = nx_uart_sync(comp);	// first: the bytes it hands in are taken here
	// NR 02 b7 holds the peripherals in reset; GPIO0 low as it lets go is the loader
	bool rst = nx_esp_reset(comp);
	if (rst && !E.rst0) {
		links_drop();
		E.out.clear();
		E.outBytes = 0;
		E.st = ST_HELD;
		xlog(XLG_NET, XLL_INFO, "esp: held in reset");
	} else if (!rst && E.rst0) {
		if (!nx_esp_gpio0(comp)) {
			E.st = ST_LOADER;
			xlog(XLG_NET, XLL_INFO, "esp: GPIO0 low, its ROM loader (not emulated)");
		} else {
			E.out.clear();
			E.outBytes = 0;
			esp_boot(comp, now);
		}
	}
	E.rst0 = rst;
	esp_line_in(comp, now);
	if ((E.st == ST_BOOT) || (E.st == ST_READY)) esp_boot_step(comp, now);
	if (E.st == ST_READY) {
		esp_pending(comp, now);
		esp_net(comp, now);
	}
	esp_pump(comp);
}
