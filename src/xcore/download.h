#pragma once

#include <QByteArray>
#include <QString>
#include <atomic>
#include <mutex>
#include <thread>

// Downloads over HTTPS with libcurl, one per object, on a thread of its own: the
// caller starts it, polls status() from its timer and reads the result. Linked on
// Windows (our static Schannel build) and macOS (the system's); loaded at run time
// on Linux, where dl_available() says whether there is one.

enum {DL_IDLE = 0, DL_RUN, DL_DONE, DL_FAIL, DL_CANCEL};

struct dlStatus {
	int state = DL_IDLE;
	qint64 done = 0;	// bytes of the whole file, a resumed part included
	qint64 total = 0;	// 0 while the server has not said
	QString error;		// DL_FAIL: short, for the user
};

class xDownload {
	public:
		xDownload() = default;
		~xDownload();		// cancels and waits for the thread
		// whole into memory; timeout in seconds, 0 for none; more than limit bytes fails
		void toMemory(const QString& url, int timeout = 0, qint64 limit = 4 << 20);
		// into part, continued from its end where the server allows it, else from the start;
		// the part stays after a failure or a cancel, for the next try to continue
		void toFile(const QString& url, const QString& part);
		void cancel();
		dlStatus status() const;
		QByteArray data() const;	// toMemory, once DL_DONE
	private:
		friend struct dlTransfer;
		mutable std::mutex lock;
		std::thread worker;
		std::atomic<bool> stop{false};
		dlStatus st;
		QByteArray mem;
		void start(const QString& url, const QString& part, int timeout, qint64 limit);
		void run(QString url, QString part, int timeout, qint64 limit);
		void progress(qint64 done, qint64 total);
		void finish(int state, const QString& error);
};

// false when no libcurl can be had here; why says what to install
bool dl_available(QString* why = nullptr);
