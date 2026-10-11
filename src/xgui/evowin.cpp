// the ZX Evolution panel, detached

#include <QApplication>
#include <QCloseEvent>
#include <QMenu>
#include <QMoveEvent>
#include <QResizeEvent>
#include <QVBoxLayout>

#include "evowin.h"
#include "xgui.h"

xEvoWin::xEvoWin(QWidget* p):QDialog(p) {
	setWindowTitle("ZX Evolution");
	setSizeGripEnabled(true);
	lastfrm = 0;
	QVBoxLayout* lay = new QVBoxLayout(this);
	lay->setContentsMargins(5, 5, 5, 5);
	panel = new xEvoPanel;
	lay->addWidget(panel);
	updateStyle();
	connect(panel, &xEvoPanel::s_detach, this, &xEvoWin::s_detach);
	connect(panel, &xEvoPanel::s_picture, this, &xEvoWin::s_picture);
}

// the debugger hands its font out to its own children only
void xEvoWin::updateStyle() {
	QFont uiFont = QApplication::font();
	foreach (QWidget* wid, findChildren<QWidget*>())
		wid->setFont(qobject_cast<QMenu*>(wid) ? uiFont : conf.dbg.font);
	setFont(conf.dbg.font);
	panel->reload();
}

void xEvoWin::setDetached(bool on) {
	if (on) {
		if ((conf.dbg.evopos.x() >= 0) && (conf.dbg.evopos.y() >= 0)) {
			setGeometry(QRect(conf.dbg.evopos, conf.dbg.evosiz));
		} else {
			resize(conf.dbg.evosiz);
		}
		show();
		raise();
		activateWindow();
		panel->reload();
	} else {
		hide();
	}
}

void xEvoWin::upd() {
	if (!isVisible() || isMinimized()) return;
	if (conf.emu.fast && ((conf.vid.fcount - lastfrm) < XSCR_FAST_EVERY)) return;
	lastfrm = conf.vid.fcount;
	panel->draw();
}

void xEvoWin::moveEvent(QMoveEvent* ev) {
	QDialog::moveEvent(ev);
	if (!isVisible()) return;
	conf.dbg.evopos = geometry().topLeft();
}

void xEvoWin::resizeEvent(QResizeEvent* ev) {
	QDialog::resizeEvent(ev);
	if (!isVisible()) return;
	conf.dbg.evosiz = geometry().size();
}

// Escape hides a dialog without a close event: send it the same road as the X
void xEvoWin::reject() {
	close();
}

void xEvoWin::closeEvent(QCloseEvent* ev) {
	emit s_detach(false);
	ev->accept();
}
