#pragma once

#include <QDialog>

#include "debuga/dbg_evo.h"

// The ZX Evolution panel in a window of its own, beside the main window: it
// keeps up with the machine while it runs, which the debugger cannot.
class xEvoWin : public QDialog {
	Q_OBJECT
	public:
		xEvoWin(QWidget* = nullptr);
	signals:
		void s_detach(bool);
		void s_picture();
	public slots:
		void upd();			// one refresh per emulated frame
		void setDetached(bool);
		void updateStyle();
	protected:
		void reject();
		void closeEvent(QCloseEvent*);
		void moveEvent(QMoveEvent*);
		void resizeEvent(QResizeEvent*);
	private:
		xEvoPanel* panel;
		int lastfrm;
};
