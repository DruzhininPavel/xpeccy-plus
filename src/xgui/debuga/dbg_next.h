#pragma once

// The ZX Next panel's pages: the nextregs, the sprites, the copper, the layers, the
// tilemap, the palettes and the DMA, read from the core as they stand. The panel,
// its dock and its window are the ZX Evolution one's (dbg_evo.h).

#include "dbg_evo.h"

// A picture of any size, kept to its proportions and scaled in whole steps when it is
// bigger than 1:1, with rectangles on top. shown is the size it stands for: a 640 dot
// wide line of half pixels is shown as 320.
class xNxPicView : public xEvoFitView {
	public:
		xNxPicView(QWidget* = nullptr);
		QImage img;
		QSize shown;			// empty: the image's own size
		QList<QRect> marks;		// in image dots
		std::function<void(int, int)> onHover;	// image dot under the pointer, -1 when it left
		int widthFor(int w, int h) const;
	protected:
		void paintEvent(QPaintEvent*);
		void mouseMoveEvent(QMouseEvent*);
		void leaveEvent(QEvent*);
	private:
		QSize logical() const;
		QRect targetIn(int w, int h) const;
};

class xNxRegsPage : public xEvoPage {
	Q_OBJECT
	public:
		xNxRegsPage(QWidget* = nullptr);
		void draw(Computer*);
	private:
		QCheckBox* cbRaw;
		xEvoList* list;
};

class xNxSprPage : public xEvoPage {
	Q_OBJECT
	public:
		xNxSprPage(QWidget* = nullptr);
		void draw(Computer*);
	protected:
		void changeEvent(QEvent*);
	private:
		QCheckBox* cbVisible;
		QTreeWidget* list;
		QLabel* labPic;
		QLabel* labInfo;
		int sel;			// the sprite picked, -1 none
		bool fitted;
		void draw_pick();
};

class xNxCopPage : public xEvoPage {
	Q_OBJECT
	public:
		xNxCopPage(QWidget* = nullptr);
		void draw(Computer*);
	protected:
		void changeEvent(QEvent*);
	private:
		QLabel* labState;
		QCheckBox* cbFollow;
		QTreeWidget* list;
		unsigned short words[1024];	// what the rows show
		int pcRow;			// the row lit as the copper's pc, -1 none
		bool filled, fitted;
};

class xNxLayerPage : public xEvoInfoPage {
	Q_OBJECT
	public:
		xNxLayerPage(QWidget* = nullptr);
		void draw(Computer*);
	protected:
		void setup(int what);
		int fitWidth(int w, int h);
	private:
		QComboBox* cbLayer;
		QLabel* labLegend;
		xNxPicView* view;
		int mx, my;			// image dot under the pointer, -1 none
		void info();
};

class xNxTilePage : public xEvoInfoPage {
	Q_OBJECT
	public:
		xNxTilePage(QWidget* = nullptr);
		void draw(Computer*);
	protected:
		void setup(int what);
		int fitWidth(int w, int h);
	private:
		QComboBox* cbView;
		QSpinBox* sbPal;
		xNxPicView* view;
		int mx, my;
		void info();
};

class xNxPalPage : public xEvoInfoPage {
	Q_OBJECT
	public:
		xNxPalPage(QWidget* = nullptr);
		void draw(Computer*);
	protected:
		void setup(int what);
		int fitWidth(int w, int h);
	private:
		QComboBox* cbPal;
		xEvoPalView* view;
		QLabel* labLegend;
		int hov;
		void info();
};

class xNxDmaPage : public xEvoPage {
	Q_OBJECT
	public:
		xNxDmaPage(QWidget* = nullptr);
		void draw(Computer*);
	private:
		xEvoList* list;
};
