#pragma once

// The ZX Evolution panel: what the TSConf and BaseConf hardware is set to, in
// the debugger's dock or in a window of its own.

#include <QCheckBox>
#include <QLabel>
#include <QStackedWidget>
#include <QTabBar>
#include <QToolButton>
#include <QTreeWidget>

#include "../classes.h"
#include "../../xcore/xcore.h"

// A read-only list of named values in groups, the way the panel shows registers:
// a value that changed since the last look is lit like a changed register.
class xEvoList : public QTreeWidget {
	public:
		xEvoList(QStringList head, QWidget* = nullptr);
		// row = group "/" name; made on first use, in the order first used
		void put(const QString& group, const QString& name, const QStringList& cols);
		void finish();			// rows not put since the last finish() go
		void restyle();
		void fitHeight();
	private:
		QMap<QString, QTreeWidgetItem*> rows;
		QMap<QString, QTreeWidgetItem*> groups;
		QSet<QString> seen;
		QTreeWidgetItem* group(const QString&);
};

class xTsRegsPage : public QWidget {
	Q_OBJECT
	public:
		xTsRegsPage(QWidget* = nullptr);
		void draw();
	signals:
		void s_layers();		// a layer switch was flipped
	private:
		xEvoList* list;
		QCheckBox* lay[6];
		QLabel* labNote;
		bool hold;
		void layer_toggled();
};

class xTsDmaPage : public QWidget {
	Q_OBJECT
	public:
		xTsDmaPage(QWidget* = nullptr);
		void draw();
	private:
		xEvoList* regs;
		QTreeWidget* log;
		unsigned logCount;		// dmalog.count the list was filled from
		int logFrame;			// and the frame it was
};

// the tab row and the page it shows
class xEvoPanel : public QWidget {
	Q_OBJECT
	public:
		xEvoPanel(QWidget* = nullptr);
		void reload();			// take the settings back from conf
	signals:
		void s_detach(bool);
		void s_picture();		// the picture was drawn again
	public slots:
		void draw();
	private slots:
		void tab_changed(int);
	private:
		QTabBar* tabs;
		QStackedWidget* stack;
		QToolButton* tbDetach;
		QLabel* labHidden;		// says a layer is kept out of the picture
		xTsRegsPage* tsRegs;
		xTsDmaPage* tsDma;
		QLabel* noMachine;
		int hw;				// the machine the tab row was built for
		bool hold;
		void build_tabs();
};

class xEvoWidget : public xDockWidget {
	Q_OBJECT
	public:
		xEvoWidget(QString, QString, QWidget* = nullptr);
		void setDetached(bool);
	signals:
		void s_detach(bool);
		void s_picture();
	public slots:
		void draw();
	private:
		xEvoPanel* panel;
		QWidget* gone;
};

// TSConf layers in the main picture: what the switches do, for both panels
int evo_set_layers(int hide);		// 1 when the picture shows it already
QString evo_layers_note();		// why it does not, after a 0
