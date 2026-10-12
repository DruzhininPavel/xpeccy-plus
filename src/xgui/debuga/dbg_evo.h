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
		void begin();			// before the put()s of a draw
		// row = group "/" name; made on first use, in the order first used
		void put(const QString& group, const QString& name, const QStringList& cols);
		bool finish();			// rows not put since begin() go; true when rows came
		void restyle();			// header colors and column widths
		void fitHeight();
		std::function<void()> onStyle;	// words or field names was picked
	protected:
		void changeEvent(QEvent*);
	private:
		QMap<QString, QTreeWidgetItem*> rows;
		QMap<QString, QTreeWidgetItem*> groups;
		QSet<QString> seen;
		QColor chgBg, chgFg;		// a changed value, as conf.pal has it this draw
		bool grown;
		QTreeWidgetItem* group(const QString&);
};

// A tab of the panel. The panel draws it only on the machine it is for.
class xEvoPage : public QWidget {
	Q_OBJECT
	public:
		xEvoPage(int hw, QWidget* p = nullptr):QWidget(p), hw(hw) {}
		const int hw;			// HW_* it is for, -1 any
		virtual void draw(Computer*) = 0;
};

class xTsRegsPage : public xEvoPage {
	Q_OBJECT
	public:
		xTsRegsPage(QWidget* = nullptr);
		void draw(Computer*);
	signals:
		void s_layers();		// a layer switch was flipped
	private:
		xEvoList* list;
		QCheckBox* lay[6];
		QLabel* labNote;
		void layer_toggled();
};

class xTsSprPage : public xEvoPage {
	Q_OBJECT
	public:
		xTsSprPage(QWidget* = nullptr);
		void draw(Computer*);
	protected:
		void changeEvent(QEvent*);
	private:
		QCheckBox* cbActive;
		QTreeWidget* list;
		QLabel* labPic;
		QLabel* labInfo;
		int sel;			// the sprite picked, -1 none
		bool fitted;			// the columns are as wide as this font needs
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

class xTsTilePage : public xEvoPage {
	Q_OBJECT
	public:
		xTsTilePage(QWidget* = nullptr);
		void draw(Computer*);
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

// A grid of square palette cells, sized as a 16x16 one would be so that a machine with
// fewer gets cells of the same size: names of the rows on the left, and on the right a
// column for each tag, marked on the rows it is on.
class xEvoPalView : public QWidget {
	public:
		xEvoPalView(QWidget* = nullptr);
		int cols, rows;
		QVector<QRgb> cells;
		QStringList rowNames;
		QStringList colNames;
		QStringList tagNames;		// none: no tag columns
		QVector<int> rowTags;		// a bit for each of tagNames
		int border;			// the cell framed as the border, -1 none
		int pick;			// the cell framed as picked, -1 none
		std::function<void(int)> onHover;	// the cell under the pointer, -1 when it left
		std::function<void(int)> onPick;
		std::function<void()> onResize;
		int widthFor(int w, int h);	// and the cells take the size that fits there
	protected:
		void paintEvent(QPaintEvent*);
		void resizeEvent(QResizeEvent*);
		void mouseMoveEvent(QMouseEvent*);
		void mousePressEvent(QMouseEvent*);
		void leaveEvent(QEvent*);
	private:
		int side;			// a cell's side from the last widthFor, 0 none yet
		QRect gridIn(int w, int h) const;
		QRect grid() const;		// the cells, without the names
		int tagColumn() const;		// one tag column's width
		int at(QPoint) const;
};

#define PAL_INFO	9	// name/value rows beside the palette, the most a machine has

// the 256 CRAM colors on TSConf, the 16 cells on BaseConf
class xEvoPalPage : public xEvoPage {
	Q_OBJECT
	public:
		xEvoPalPage(QWidget* = nullptr);
		void draw(Computer*);
	protected:
		void resizeEvent(QResizeEvent*);
		void changeEvent(QEvent*);
	private:
		QHBoxLayout* row;
		xEvoPalView* view;
		QWidget* infoBox;
		QLabel* names[PAL_INFO];
		QLabel* vals[PAL_INFO];
		QLabel* labLegend;
		int hov;			// the cell under the pointer, -1 none
		int shown;			// the machine the names were set up for, -1 none
		void setup(int hw);
		void info();
		void fit();
};

// BaseConf
class xEvoPortsPage : public xEvoPage {
	Q_OBJECT
	public:
		xEvoPortsPage(QWidget* = nullptr);
		void draw(Computer*);
	private:
		xEvoList* list;
};

class xTsDmaPage : public xEvoPage {
	Q_OBJECT
	public:
		xTsDmaPage(QWidget* = nullptr);
		void draw(Computer*);
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
		void restyle();
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
		QList<xEvoPage*> pages;		// all of them, in the order of their tabs
		QList<xEvoPage*> tabPages;	// the ones in the tab row now
		QLabel* noMachine;
		int hw;				// the machine the tab row was built for
		void build_tabs();
};

class xEvoWidget : public xDockWidget {
	Q_OBJECT
	public:
		xEvoWidget(QString, QString, QWidget* = nullptr);
		void setDetached(bool);
		void restyle();
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
// start the frame snapshots a redraw needs, on TSConf: the debugger or the panel is shown
void evo_arm_replay();
