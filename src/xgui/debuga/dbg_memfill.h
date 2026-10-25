#pragma once

#include <QDialog>

#include "../../xgui/xgui.h"
#include "../../libxpeccy/spectrum.h"
#include "ui_filler.h"

class xMemFiller : public QDialog {
	Q_OBJECT
	public:
		xMemFiller(QWidget* = NULL);
		void start(Computer*, int, int);
	signals:
		void rqRefill();
	private:
		Ui::Filler ui;
		Computer* comp;
		int mrd(int);
		void mwr(int, int);
	private slots:
		void metChange();
		void fill();
};
