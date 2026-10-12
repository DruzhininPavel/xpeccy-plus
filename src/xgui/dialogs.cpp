#include "xgui.h"
#include "../xcore/xcore.h"
#include <QMessageBox>
#include <QAbstractButton>

void shitHappens(const char* msg) {
	if (!conf.running) return;
	QMessageBox mbx(QMessageBox::Critical,"Shit happens",msg,QMessageBox::Ok);
	mbx.exec();
}

// Yes and No as OK and Cancel look elsewhere
bool areSure(const char* msg) {
	QMessageBox mbx(QMessageBox::Question,"Yes? No? What?",msg,QMessageBox::Yes | QMessageBox::No);
	mbx.button(QMessageBox::Yes)->setIcon(QIcon(":/images/ok-apply.png"));
	mbx.button(QMessageBox::No)->setIcon(QIcon(":/images/cancel.png"));
	// a style sheet's padding is not in a message box button's width
	foreach(QAbstractButton* bt, mbx.buttons())
		bt->setMinimumWidth(bt->fontMetrics().horizontalAdvance(bt->text()) + 32 + bt->iconSize().width());
	int res = mbx.exec();
	return (res == QMessageBox::Yes);
}

void showInfo(const char* msg) {
	QMessageBox mbx(QMessageBox::Information,"Message",msg,QMessageBox::Ok);
	mbx.exec();
}
