#include <mutex>
#include <thread>
#include <cstring>

#include "espnet.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET sock_t;
#define SOCK_BAD INVALID_SOCKET
#define sock_close closesocket
static bool sock_wouldblock() {
	int e = WSAGetLastError();
	return (e == WSAEWOULDBLOCK) || (e == WSAEINPROGRESS);
}
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
typedef int sock_t;
#define SOCK_BAD (-1)
#define sock_close ::close
static bool sock_wouldblock() {
	return (errno == EWOULDBLOCK) || (errno == EAGAIN) || (errno == EINPROGRESS) || (errno == EINTR);
}
#endif

// a send to a socket the far end has reset raises SIGPIPE on POSIX, which ends the program:
// Linux is told per call, macOS per socket (SO_NOSIGPIPE, set when it is made)
#ifdef MSG_NOSIGNAL
#define SEND_FLAGS MSG_NOSIGNAL
#else
#define SEND_FLAGS 0
#endif

bool esp_net_init() {
#ifdef _WIN32
	static bool done = false;
	if (!done) {
		WSADATA wd;
		done = (WSAStartup(MAKEWORD(2, 2), &wd) == 0);
	}
	return done;
#else
	return true;
#endif
}

// one lookup, shared by the socket and the thread doing it: either may be gone first
struct xEspLookup {
	std::mutex lock;
	bool done = false;
	bool ok = false;
	sockaddr_storage addr;
	int len = 0;
};

static void lookup_run(std::shared_ptr<xEspLookup> lk, std::string host, bool udp) {
	addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;		// the ESP8266 AT firmware is IPv4 only
	hints.ai_socktype = udp ? SOCK_DGRAM : SOCK_STREAM;
	addrinfo* res = nullptr;
	bool ok = (getaddrinfo(host.c_str(), nullptr, &hints, &res) == 0) && res;
	std::lock_guard<std::mutex> g(lk->lock);
	if (ok) {
		memcpy(&lk->addr, res->ai_addr, res->ai_addrlen);
		lk->len = (int)res->ai_addrlen;
	}
	if (res) freeaddrinfo(res);
	lk->ok = ok;
	lk->done = true;
}

xEspSock::~xEspSock() {
	close();
}

void xEspSock::open(bool u, const std::string& host, int p) {
	close();
	esp_net_init();
	udp = u;
	port = p;
	ip.clear();
	why = F_NONE;
	look = std::make_shared<xEspLookup>();
	st = S_RESOLVING;
	std::thread(lookup_run, look, host, u).detach();
}

void xEspSock::connectTo() {
	sockaddr_in sa;
	{
		std::lock_guard<std::mutex> g(look->lock);
		memcpy(&sa, &look->addr, sizeof(sa));
	}
	sa.sin_port = htons((unsigned short)port);
	const unsigned char* a = (const unsigned char*)&sa.sin_addr;
	ip = std::to_string(a[0]) + "." + std::to_string(a[1]) + "." + std::to_string(a[2]) + "." + std::to_string(a[3]);
	sock_t s = socket(AF_INET, udp ? SOCK_DGRAM : SOCK_STREAM, udp ? IPPROTO_UDP : IPPROTO_TCP);
	if (s == SOCK_BAD) {
		st = S_FAILED;
		return;
	}
#ifdef _WIN32
	u_long nb = 1;
	ioctlsocket(s, FIONBIO, &nb);
#else
	fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
#ifdef SO_NOSIGPIPE
	int nosig = 1;
	setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, &nosig, sizeof(nosig));
#endif
#endif
	if (!udp) {
		int one = 1;
		setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char*)&one, sizeof(one));
	}
	fd = (long long)s;
	if (::connect(s, (sockaddr*)&sa, sizeof(sa)) == 0) {
		st = S_OPEN;
	} else if (sock_wouldblock()) {
		st = udp ? S_OPEN : S_CONNECTING;
	} else {
		why = F_REFUSED;
		st = S_FAILED;
	}
}

int xEspSock::poll() {
	if (st == S_RESOLVING) {
		bool done, ok;
		{
			std::lock_guard<std::mutex> g(look->lock);
			done = look->done;
			ok = look->ok;
		}
		if (done) {
			if (ok) {
				connectTo();
			} else {
				why = F_DNS;
				st = S_FAILED;
			}
		}
	}
	if (st == S_CONNECTING) {
		sock_t s = (sock_t)fd;
		fd_set wr, ex;
		FD_ZERO(&wr);
		FD_ZERO(&ex);
		FD_SET(s, &wr);
		FD_SET(s, &ex);
		timeval tv = {0, 0};
		if (select((int)s + 1, nullptr, &wr, &ex, &tv) > 0) {
			int err = 0;
			socklen_t len = sizeof(err);
			getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&err, &len);
			if (FD_ISSET(s, &ex) || err) {
				why = F_REFUSED;
				st = S_FAILED;
			} else {
				st = S_OPEN;
			}
		}
	}
	return st;
}

int xEspSock::send(const unsigned char* b, int n) {
	if (st != S_OPEN) return -1;
	int r = (int)::send((sock_t)fd, (const char*)b, n, SEND_FLAGS);
	if (r >= 0) return r;
	return sock_wouldblock() ? 0 : -1;
}

int xEspSock::recv(unsigned char* b, int n) {
	if (st != S_OPEN) return -1;
	int r = (int)::recv((sock_t)fd, (char*)b, n, 0);
	if (r > 0) return r;
	if (r == 0) {
		if (udp) return 0;
		st = S_CLOSED;
		return -1;
	}
	if (sock_wouldblock()) return 0;
#ifdef _WIN32
	if (udp && (WSAGetLastError() == WSAECONNRESET)) return 0;	// an ICMP port unreachable
#endif
	st = S_CLOSED;
	return -1;
}

void xEspSock::close() {
	if (fd >= 0) sock_close((sock_t)fd);
	fd = -1;
	look.reset();
	if (st != S_NONE) st = S_CLOSED;
}
