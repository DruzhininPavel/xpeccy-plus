#pragma once

// The ZX Evolution panel: what the TSConf and BaseConf hardware is set to, in
// the debugger's dock or in a window of its own. The ZX Next panel is the same
// panel with tabs of its own (dbg_next.cpp).

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

// which of the two panels: each has its tabs, its dock and window, and its settings
enum {EVO_PANEL = 0, NEXT_PANEL};

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
		void wordsOnly();		// no field names to switch to: no menu, the header stays
		std::function<void()> onStyle;	// words or field names was picked
	protected:
		void changeEvent(QEvent*);
	private:
		QMap<QString, QTreeWidgetItem*> rows;
		QMap<QString, QTreeWidgetItem*> groups;
		QSet<QString> seen;
		QColor chgBg, chgFg;		// a changed value, as conf.pal has it this draw
		bool grown;
		bool named;			// the last column follows the words/field names switch
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

// A picture asking for the width its page fitted, and taking less on a narrower page.
class xEvoFitView : public QWidget {
	public:
		xEvoFitView(QWidget* = nullptr);
		void setFitWidth(int);		// no less than the minimum width
		QSize sizeHint() const;
	private:
		int fitW;
};

// A page with a picture at the left and beside it a column of name/value rows of a fixed
// width, so neither moves as the values change. The page fits the picture on a resize.
class xEvoInfoPage : public xEvoPage {
	Q_OBJECT
	public:
		xEvoInfoPage(int hw, int rows, QWidget* p = nullptr);
	protected:
		QVBoxLayout* lay;		// the row of the two in it; pages add above and below
		int shown;			// what setup() was last made for, -1 nothing yet
		virtual void setup(int what) = 0;
		virtual int fitWidth(int w, int h) = 0;	// the picture's width in w x h
		void setPicture(xEvoFitView*);
		void setNames(const QStringList& names, const QStringList& all, const QStringList& widest);
		void setValues(const QStringList&);	// "-" for the rows past the list
		void fit();
		void resizeEvent(QResizeEvent*);
		void changeEvent(QEvent*);
	private:
		QHBoxLayout* row;
		xEvoFitView* pic;
		QWidget* infoBox;
		QVector<QLabel*> names;
		QVector<QLabel*> vals;
};

// 512x512 dots of a tile map or a graphics page, scaled to fit, with rectangles on top
class xTsMapView : public xEvoFitView {
	public:
		xTsMapView(QWidget* = nullptr);
		QImage img;
		QList<QRect> marks;		// in image dots
		std::function<void(int, int)> onHover;	// image dot under the pointer, -1 when it left
		int sideFor(int w, int h) const;	// the picture's side in w x h
	protected:
		void paintEvent(QPaintEvent*);
		void mouseMoveEvent(QMouseEvent*);
		void leaveEvent(QEvent*);
	private:
		QRect target() const;
};

class xTsTilePage : public xEvoInfoPage {
	Q_OBJECT
	public:
		xTsTilePage(QWidget* = nullptr);
		void draw(Computer*);
	protected:
		void setup(int mode);
		int fitWidth(int w, int h);
	private:
		QComboBox* cbView;
		QSpinBox* sbPage;
		QSpinBox* sbPal;
		QToolButton* tbT0G;
		QToolButton* tbT1G;
		QToolButton* tbSG;
		xTsMapView* view;
		int hx, hy;			// dot under the pointer, -1 none
		void hover_info();
};

// A grid of square palette cells, sized as a 16x16 one would be so that a machine with
// fewer gets cells of the same size: names of the rows on the left, and on the right a
// column for each tag, marked on the rows it is on.
class xEvoPalView : public xEvoFitView {
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
		int widthFor(int w, int h);	// and the cells take the size that fits there
	protected:
		void paintEvent(QPaintEvent*);
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

// the 256 CRAM colors on TSConf, the 16 cells on BaseConf
class xEvoPalPage : public xEvoInfoPage {
	Q_OBJECT
	public:
		xEvoPalPage(QWidget* = nullptr);
		void draw(Computer*);
	protected:
		void setup(int hw);
		int fitWidth(int w, int h);
	private:
		xEvoPalView* view;
		QLabel* labLegend;
		int hov;			// the cell under the pointer, -1 none
		void info();
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
		xEvoPanel(int kind, QWidget* = nullptr);
		const int kind;			// EVO_PANEL, NEXT_PANEL
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
		QStringList names;		// and their tab names
		QList<xEvoPage*> tabPages;	// the ones in the tab row now
		QLabel* noMachine;
		int hw;				// the machine the tab row was built for
		void build_tabs();
};

class xEvoWidget : public xDockWidget {
	Q_OBJECT
	public:
		xEvoWidget(int kind, QString, QString, QWidget* = nullptr);
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

// what the pages of both panels share
QBrush evo_checker(const QWidget*);
void evo_tree_setup(QTreeWidget*, const QStringList& head);
void evo_fit_column(QTreeWidget*, int col, QStringList samples);
bool evo_field_names();
// A register's bits: the field name the docs give it, and the same in words (none: a
// bit the words leave to the rest of the line)
struct evoBit {
	int mask;
	const char* field;
	const char* words;
};
QStringList evo_bit_list(int v, const evoBit*);
QString evo_join(const QStringList&, const char* none);
QString evo_bits(int v, const evoBit*, const char* none);
// the Next panel's pages and their tab names (dbg_next.cpp)
QList<xEvoPage*> next_panel_pages(QStringList* names);
