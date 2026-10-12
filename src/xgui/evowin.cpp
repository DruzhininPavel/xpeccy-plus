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
	setWindowIcon(QIcon(":/images/grp-board.png"));
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

// the debugger hands its own font out child by child; this window is not one
// of its children, so it takes the font here
void xEvoWin::updateStyle() {
	QFont uiFont = QApplication::font();
	foreach (QWidget* wid, findChildren<QWidget*>())
		wid->setFont(qobject_cast<QMenu*>(wid) ? uiFont : conf.dbg.font);
	setFont(conf.dbg.font);
	panel->reload();
}

void xEvoWin::setDetached(bool on) {
	if (on) {
		// geometry(), not move/resize: the same rectangle both ways, where
		// pos()/move() would walk the window down the screen by the height
		// of its own title bar on every restore
		if ((conf.dbg.evopos.x() >= 0) && (conf.dbg.evopos.y() >= 0)) {
			setGeometry(QRect(conf.dbg.evopos, conf.dbg.evosiz));
		} else {
			resize(conf.dbg.evosiz);
		}
		show();
		raise();
		activateWindow();
		evo_arm_replay();
		panel->reload();
	} else {
		hide();
	}
}

// One refresh per emulated frame, so the readout keeps the machine's own time
// instead of a timer's. Fast mode makes ten times as many frames and no more to
// read out of each, so there it takes every tenth.
void xEvoWin::upd() {
	if (!isVisible() || isMinimized()) return;
	if (conf.emu.fast && ((conf.vid.fcount - lastfrm) < XSCR_FAST_EVERY)) return;
	lastfrm = conf.vid.fcount;
	panel->draw();
}

// where the window is left is where it comes back
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

// Escape on a dialog is reject(), which hides the window on its own without a
// close event: the debugger would go on thinking the panel is detached and the
// next toggle would do nothing at all. Send it the same road as the X button.
void xEvoWin::reject() {
	close();
}

// closing it is another way of saying "put it back in the debugger"
void xEvoWin::closeEvent(QCloseEvent* ev) {
	emit s_detach(false);
	ev->accept();
}
