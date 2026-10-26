// A download's progress in a dialog (dl_follow in xgui.h)

#include <QProgressBar>
#include <QTimer>

#include "xgui.h"
#include "../xcore/download.h"

static QString dl_mb(qint64 bytes) {
	double mb = bytes / 1048576.0;
	return (mb < 10) ? QString::number(mb, 'f', 1) : QString::number(qRound64(mb));
}

// polled: the download runs on a thread of its own and tells nobody
void dl_follow(QObject* owner, xDownload* dl, QProgressBar* bar, QLabel* line, std::function<void(const dlStatus&)> done, qint64 expect) {
	QTimer* tmr = new QTimer(owner);
	tmr->setInterval(100);
	QObject::connect(tmr, &QTimer::timeout, owner, [=]() {
		dlStatus st = dl->status();
		// the size expected stands in until the download outgrows it
		qint64 total = st.total;
		bool guess = (total <= 0) && (expect > st.done);
		if (guess) total = expect;
		if (bar && (total > 0)) {
			bar->setRange(0, 1000);
			bar->setValue(int(1000.0 * st.done / total));
		} else if (bar) {
			bar->setRange(0, 0);
		}
		if (line) {
			line->setText((total > 0) ? QString(guess ? "%0 of about %1 MB" : "%0 of %1 MB").arg(dl_mb(st.done), dl_mb(total))
				: st.done ? QString("%0 MB").arg(dl_mb(st.done)) : QString());
		}
		if (st.state == DL_RUN) return;
		tmr->stop();
		tmr->deleteLater();
		done(st);
	});
	tmr->start();
}
