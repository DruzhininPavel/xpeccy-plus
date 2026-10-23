#pragma once

// The host's network for the emulated ESP: non-blocking sockets polled from the emulation
// thread, and name lookups on a thread of their own so a slow DNS never holds a frame.

#include <memory>
#include <string>
#include <vector>

struct xEspLookup;

class xEspSock {
	public:
		enum {S_NONE = 0, S_RESOLVING, S_CONNECTING, S_OPEN, S_FAILED, S_CLOSED};
		enum {F_NONE = 0, F_DNS, F_REFUSED};
		~xEspSock();
		void open(bool udp, const std::string& host, int port);
		int poll();			// moves the state on; returns it
		int state() const {return st;}
		int failure() const {return why;}
		std::string peerIp() const {return ip;}		// the far end, once it is looked up
		int peerPort() const {return port;}
		int send(const unsigned char* b, int n);	// bytes taken, -1 the link is gone
		int recv(unsigned char* b, int n);		// bytes read, 0 none yet, -1 closed
		void close();
		bool udp = false;
	private:
		int st = S_NONE;
		int why = F_NONE;
		long long fd = -1;
		int port = 0;
		std::string ip;
		std::shared_ptr<xEspLookup> look;
		void connectTo();
};

bool esp_net_init();
