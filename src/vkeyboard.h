#pragma once

#include <QWidget>
#include <QDialog>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPixmap>
#include <QColor>
#include <QResizeEvent>
#include <QShowEvent>
#include <QMoveEvent>

#include <QTimer>
#include <QFileSystemWatcher>

#include "libxpeccy/input/input.h"
#include "xgui/vkbdskin.h"

#if defined(__APPLE__)
// vkeyboard_mac.mm: let the window keep its shape by itself
void vkbd_set_aspect(QWidget*, int, int);
#endif

class keyWindow : public QDialog {
	Q_OBJECT
	public:
		keyWindow(QWidget* = NULL);
	signals:
		void s_key_press(QKeyEvent*);
		void s_key_release(QKeyEvent*);
	public slots:
		void switcher();
		void upd(Keyboard*);
		void rall(Keyboard*);
		void snap();		// sit under the emulator window
	private slots:
		void redraw();		// the picture at the window's size, now
		void loadSkin();
	private:
		Keyboard* kb;
		keyEntry xent;
		VkSkin skin;
		QSizeF cnv;		// the layout's canvas, in its own units
		QPixmap pic;		// the picture as last drawn
		double picScale;	// device pixels per canvas unit it was drawn at
		QTimer drawTimer;	// a size being dragged is drawn when it settles
		QFileSystemWatcher watcher;	// a layout of the user's own, edited
		unsigned dock:1;	// glued under the emulator window
		int hover;		// the key under the pointer, -1 for none
		int keyAt(QPointF);
		double scale();
		double devScale();
		QPointF picOrigin();
		double storedZoom();
		int higFor(int);
		int widFor(int);
		QPointF imgPos(QPoint);
		void setZoom(double);
		void setDock(bool);
		void showMenu(QPoint);
		void paintEvent(QPaintEvent*);
		void mousePressEvent(QMouseEvent*);
		void mouseReleaseEvent(QMouseEvent*);
		void mouseMoveEvent(QMouseEvent*);
		void leaveEvent(QEvent*);
		void keyPressEvent(QKeyEvent*);
		void keyReleaseEvent(QKeyEvent*);
		void resizeEvent(QResizeEvent*);
		void moveEvent(QMoveEvent*);
		void showEvent(QShowEvent*);
#if defined(_WIN32)
	#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
		bool nativeEvent(const QByteArray&, void*, qintptr*);
	#else
		bool nativeEvent(const QByteArray&, void*, long*);
	#endif
#endif
};
