#pragma once

// The ZX Evolution panel: what the TSConf and BaseConf hardware is set to, in
// the debugger's dock or in a window of its own.

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QLabel>
#include <QStackedWidget>
#include <QTabBar>
#include <QToolButton>
#include <QTreeWidget>
#include <functional>

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
		std::function<void()> onStyle;	// words or field names was picked
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

class xTsSprPage : public QWidget {
	Q_OBJECT
	public:
		xTsSprPage(QWidget* = nullptr);
		void draw();
	private:
		QCheckBox* cbActive;
		QTreeWidget* list;
		QLabel* labPic;
		QLabel* labInfo;
		int sel;			// the sprite picked, -1 none
		void draw_pick();
};

// 512x512 dots of a tile map or a graphics page, scaled to fit, with rectangles on top
class xTsMapView : public QWidget {
	public:
		xTsMapView(QWidget* = nullptr);
		QImage img;
		QList<QRect> marks;		// in image dots
		std::function<void(int, int)> onHover;	// image dot under the pointer, -1 when it left
	protected:
		void paintEvent(QPaintEvent*);
		void mouseMoveEvent(QMouseEvent*);
		void leaveEvent(QEvent*);
	private:
		QRect target() const;
};

class xTsTilePage : public QWidget {
	Q_OBJECT
	public:
		xTsTilePage(QWidget* = nullptr);
		void draw();
	private:
		QComboBox* cbView;
		QSpinBox* sbPage;
		QSpinBox* sbPal;
		QToolButton* tbT0G;
		QToolButton* tbT1G;
		QToolButton* tbSG;
		xTsMapView* view;
		QLabel* labInfo;
		int hx, hy;			// dot under the pointer, -1 none
		void hover_info();
};

// a grid of palette cells: names of the rows on the left, what uses a row on the right
class xEvoPalView : public QWidget {
	public:
		xEvoPalView(QWidget* = nullptr);
		int cols, rows;
		QVector<QRgb> cells;
		QStringList cellText;		// over each cell, when there is room; may be empty
		QStringList rowNames;
		QStringList colNames;
		QStringList rowTags;
		int border;			// the cell framed as the border, -1 none
		int pick;			// the cell framed as picked, -1 none
		std::function<void(int)> onHover;	// the cell under the pointer, -1 when it left
		std::function<void(int)> onPick;
		int widthFor(int w, int h) const;
	protected:
		void paintEvent(QPaintEvent*);
		void mouseMoveEvent(QMouseEvent*);
		void mousePressEvent(QMouseEvent*);
		void leaveEvent(QEvent*);
	private:
		QRect gridIn(int w, int h) const;
		QRect grid() const;		// the cells, without the names
		int tagWidth() const;
		int at(QPoint) const;
};

#define PAL_INFO	9	// name/value rows beside the palette, the most a machine has

// the 256 CRAM colors on TSConf, the 16 cells on BaseConf
class xEvoPalPage : public QWidget {
	Q_OBJECT
	public:
		xEvoPalPage(QWidget* = nullptr);
		void draw();
	protected:
		void resizeEvent(QResizeEvent*);
	private:
		QHBoxLayout* row;
		xEvoPalView* view;
		QWidget* infoBox;
		QLabel* swatch;
		QLabel* names[PAL_INFO];
		QLabel* vals[PAL_INFO];
		QLabel* labLegend;
		int hov;			// the cell under the pointer, -1 none
		void info();
		void fit();
};

// BaseConf
class xEvoPortsPage : public QWidget {
	Q_OBJECT
	public:
		xEvoPortsPage(QWidget* = nullptr);
		void draw();
	private:
		xEvoList* list;
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
		xTsSprPage* tsSpr;
		xTsTilePage* tsTile;
		xTsDmaPage* tsDma;
		xEvoPortsPage* evoPorts;
		xEvoPalPage* pal;
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
