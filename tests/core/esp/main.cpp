// Headless bench for the emulated ESP-01: the Next with the ESP on UART 0, wired as the
// program wires it (every byte the UART sends goes to esp_byte, esp_frame at each frame
// end), a guest program talking AT to it, and a TCP server on 127.0.0.1 to fetch from.
//
//	esp-bench <prog.bin> <frames>
//
// prog.bin is put at 0 of the SRAM (ROM0: the program starts at PC 0). It prints with an
// OUT of one character to 0x2003 and ends the run with an OUT to 0x0103. The server's port
// is written at 0x3F00 as decimal text; the guest's output is printed with that port as
// <port>, so it reads the same every run. The ESP keeps no flash: it boots joined.
//
// The server answers one line per connection: "big" gets a 48000-byte page, anything else
// a short line; then it closes. What it sent is printed after the guest's output.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

#include "spectrum.h"
#include "../../../src/xcore/esp.h"
#include "../../../src/xcore/espnet.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET sock_t;
#define sock_close closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int sock_t;
#define sock_close close
#define INVALID_SOCKET (-1)
#endif

#define PAGE 48000

static std::string guest;		// what the guest printed
static int done = 0;
static long srvBytes = 0, srvSum = 0;	// the "big" pages sent, as the guest should count them

static void log_out(Computer*, int port, int val) {
	if ((port & 0xffff) == 0x2003) guest += (char)(val & 0xff);
	if ((port & 0xffff) == 0x0103) done = 1;
}

static void host_tx(Computer* comp, int uart, int b, long long start) {
	if (uart == 0) esp_byte(comp, b, start);
}

static unsigned char page_byte(long i) {
	return (unsigned char)((i * 31 + (i >> 8)) & 0xff);
}

static void serve(sock_t ls) {
	for (;;) {
		sock_t c = accept(ls, nullptr, nullptr);
		if (c == INVALID_SOCKET) return;
		std::string req;
		char ch;
		while ((req.size() < 256) && (recv(c, &ch, 1, 0) == 1)) {
			req += ch;
			if (ch == '\n') break;
		}
		while (!req.empty() && ((req.back() == '\n') || (req.back() == '\r'))) req.pop_back();
		std::string body;
		if (req == "big") {
			for (long i = 0; i < PAGE; i++) body += (char)page_byte(i);
			srvBytes += PAGE;
			for (long i = 0; i < PAGE; i++) srvSum = (srvSum + page_byte(i)) & 0xffff;
		} else {
			body = "hello\r\n";
		}
		size_t at = 0;
		while (at < body.size()) {
			int n = (int)send(c, body.data() + at, (int)(body.size() - at), 0);
			if (n <= 0) break;
			at += n;
		}
#ifdef _WIN32
		shutdown(c, SD_SEND);
#else
		shutdown(c, SHUT_WR);
#endif
		while (recv(c, &ch, 1, 0) > 0) {}		// till the ESP closes its end
		sock_close(c);
	}
}

int main(int argc, char** argv) {
	if (argc < 3) {
		fprintf(stderr, "usage: esp-bench prog.bin frames\n");
		return 2;
	}
	int frames = atoi(argv[2]);
	esp_net_init();
	sock_t ls = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sa.sin_port = 0;
	socklen_t sl = sizeof(sa);
	if ((ls == INVALID_SOCKET) || bind(ls, (sockaddr*)&sa, sizeof(sa)) || listen(ls, 4) || getsockname(ls, (sockaddr*)&sa, &sl)) {
		fprintf(stderr, "no server socket\n");
		return 1;
	}
	std::string port = std::to_string(ntohs(sa.sin_port));
	std::thread(serve, ls).detach();

	Computer* comp = compCreate();
	if (!compSetHardware(comp, "ZXNext")) {
		fprintf(stderr, "no ZXNext core\n");
		return 1;
	}
	compSetBaseFrq(comp, 3.5);
	compReset(comp, RES_DEFAULT);
	comp_kbd_release(comp);
	comp->next->busRomcs = 1;
	next_reset_open = 1;
	next_ear_tied = 1;
	FILE* f = fopen(argv[1], "rb");
	if (!f) {
		fprintf(stderr, "can't open %s\n", argv[1]);
		return 1;
	}
	size_t n = fread(comp->mem->ramData, 1, 0x3f00, f);
	fclose(f);
	(void)n;
	memcpy(comp->mem->ramData + 0x3f00, port.c_str(), port.size() + 1);
	next_log_out = log_out;
	next_uart_host_tx = host_tx;
	esp_flash_file(nullptr);
	esp_attach(comp);

	int frame = comp->next->vsyncCount;
	while (!done && (comp->next->vsyncCount < frames)) {
		compExec(comp);
		if (comp->next->vsyncCount != frame) {
			frame = comp->next->vsyncCount;
			esp_frame(comp);
		}
	}
	esp_detach(comp);

	for (size_t at; (at = guest.find(port)) != std::string::npos;) guest.replace(at, port.size(), "<port>");
	printf("%s", guest.c_str());
	if (!guest.empty() && (guest.back() != '\n')) printf("\n");
	if (srvBytes) printf("server: %li bytes, sum %li\n", srvBytes, srvSum);
	printf("%s\n", done ? "ended by the program" : "frame limit");	// the frame depends on the host's network
	return 0;
}
