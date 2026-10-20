// virtual keyboard

#include "vkeyboard.h"
#include "xcore/xcore.h"
#include "xgui/xgui.h"

#include <QActionGroup>
#include <QApplication>
#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QPainter>
#include <QMenu>
#include <QScreen>
#include <QTimer>
#include <QWindow>

#if defined(_WIN32)
#include <windows.h>
#ifndef SPI_GETFONTSMOOTHINGORIENTATION
#define SPI_GETFONTSMOOTHINGORIENTATION 0x2012
#endif
#ifndef FE_FONTSMOOTHINGORIENTATIONRGB
#define FE_FONTSMOOTHINGORIENTATIONRGB 1
#endif
#endif

// the skins there are, as named in the config and in res/vkbd/zx-<name>.json
static const struct {
	const char* name;
	const char* title;
} vkSkins[] = {{"black", "Black"}, {"white", "White"}};

// Up to this size subpixel text is worth it; above it the triads are too small
// against the letters to see, and only fringe the colours
#define VK_LCD_MAX	1.5

// Subpixel text needs a screen with its subpixels in RGB stripes, and only
// Windows says what the screen has: ClearType on, RGB order. A screen turned on
// its side has its stripes running the other way.
static bool hostLcd(QWidget* w) {
#if defined(_WIN32)
	BOOL on = FALSE;
	UINT type = 0;
	UINT order = 0;
	if (!SystemParametersInfo(SPI_GETFONTSMOOTHING, 0, &on, 0) || !on) return false;
	if (!SystemParametersInfo(SPI_GETFONTSMOOTHINGTYPE, 0, &type, 0) || (type != FE_FONTSMOOTHINGCLEARTYPE)) return false;
	if (!SystemParametersInfo(SPI_GETFONTSMOOTHINGORIENTATION, 0, &order, 0) || (order != FE_FONTSMOOTHINGORIENTATIONRGB)) return false;
	// QWidget::screen() is Qt 5.14 and later
	QScreen* scr = w->windowHandle() ? w->windowHandle()->screen() : QGuiApplication::primaryScreen();
	return !scr || (scr->geometry().width() >= scr->geometry().height());
#else
	(void)w;
	return false;
#endif
}

keyWindow::keyWindow(QWidget* p):QDialog(p) {
	kb = NULL;
	dock = 0;
	picScale = 0;
	memset(&xent, 0, sizeof(xent));
	xent.key = ENDKEY;
	drawTimer.setSingleShot(true);
	drawTimer.setInterval(60);
	connect(&drawTimer, SIGNAL(timeout()), this, SLOT(redraw()));
	connect(&watcher, SIGNAL(fileChanged(QString)), this, SLOT(loadSkin()));
	connect(&watcher, SIGNAL(directoryChanged(QString)), this, SLOT(loadSkin()));
	loadSkin();
	setModal(false);
	setWindowModality(Qt::NonModal);
	setSizeGripEnabled(true);
	setMinimumSize(qRound(cnv.width() / 2), qRound(cnv.height() / 2));
	setZoom(storedZoom());
	setWindowIcon(QIcon(":/images/vkeys.png"));
	setWindowTitle("Virtual Keyboard - ZX Spectrum");
	if (conf.keywin.dock)
		setDock(true);
}

// The skin from the config. A file of the same name in the config folder's vkbd/
// is taken instead of the built-in one, and taken again whenever it is saved, so
// a layout can be tuned while the window shows it.
void keyWindow::loadSkin() {
	QString name = QString::fromStdString(conf.keywin.skin);
	bool known = false;
	for (const auto& sk : vkSkins)
		known |= (name == sk.name);
	if (!known) name = vkSkins[0].name;
	QString dir = QString::fromLocal8Bit(conf.path.confDir.c_str()) + "/vkbd";
	QString own = dir + "/zx-" + name + ".json";
	if (!watcher.files().isEmpty()) watcher.removePaths(watcher.files());
	if (!watcher.directories().isEmpty()) watcher.removePaths(watcher.directories());
	if (QFile::exists(dir)) watcher.addPath(dir);	// a file put there later
	VkSkin sk;
	QString err;
	QFile file(own);
	if (file.exists()) {
		watcher.addPath(own);
		if (!file.open(QIODevice::ReadOnly) || !sk.load(file.readAll(), &err)) {
			qWarning("vkbd: %s: %s", own.toLocal8Bit().constData(), err.toLocal8Bit().constData());
			sk = VkSkin();
		}
	}
	if (sk.isNull()) {
		QFile res(":/res/vkbd/zx-" + name + ".json");
		if (res.open(QIODevice::ReadOnly) && !sk.load(res.readAll(), &err))
			qWarning("vkbd: built-in %s: %s", name.toLocal8Bit().constData(), err.toLocal8Bit().constData());
	}
	if (sk.isNull() && skin.isNull()) return;	// nothing to show at all
	if (!sk.isNull()) skin = sk;
	cnv = skin.canvas();
	picScale = 0;
	pic = QPixmap();
	update();
}

// window pixels per canvas unit. The picture keeps its shape, so a window of
// another aspect gets an empty strip beside it
double keyWindow::scale() {
	if (cnv.isEmpty()) return 0;
	double sx = width() / cnv.width();
	double sy = height() / cnv.height();
	return (sx < sy) ? sx : sy;
}

// the same in the screen's own pixels: what the picture is drawn at
double keyWindow::devScale() {
	double dpr = devicePixelRatioF();
	return qRound(cnv.width() * scale() * dpr) / cnv.width();
}

// The picture's top left corner, on a whole device pixel, so it is put down 1:1
QPointF keyWindow::picOrigin() {
	double dpr = devicePixelRatioF();
	double sd = devScale();
	int x = (int)((width() * dpr - qRound(cnv.width() * sd)) / 2);
	int y = (int)((height() * dpr - qRound(cnv.height() * sd)) / 2);
	return QPointF(x / dpr, y / dpr);
}

// the picture's shape, from one side to the other
int keyWindow::higFor(int wid) {
	return qRound(wid * cnv.height() / cnv.width());
}

int keyWindow::widFor(int hig) {
	return qRound(hig * cnv.width() / cnv.height());
}

// the size the window was left at last time
double keyWindow::storedZoom() {
	return (conf.keywin.width > 0) ? (conf.keywin.width / cnv.width()) : 1.0;
}

// a point of the window in canvas units
QPointF keyWindow::imgPos(QPoint pos) {
	double sc = devScale() / devicePixelRatioF();
	if (sc <= 0.0) return QPointF(-1, -1);
	QPointF org = picOrigin();
	return QPointF((pos.x() - org.x()) / sc, (pos.y() - org.y()) / sc);
}

void keyWindow::setZoom(double z) {
	resize(qRound(cnv.width() * z), qRound(cnv.height() * z));
}

// the picture at the size the window has now
void keyWindow::redraw() {
	double sd = devScale();
	if (sd <= 0) return;
	QImage img = skin.render(sd, (sd <= VK_LCD_MAX + 1e-6) && hostLcd(this));
	pic = QPixmap::fromImage(img);
	pic.setDevicePixelRatio(devicePixelRatioF());
	picScale = sd;
	update();
}

// docked: no frame of its own, the emulator window drags it around
void keyWindow::setDock(bool d) {
	bool vis = isVisible();
	dock = d;
	conf.keywin.dock = dock;
	setWindowFlags(d ? (Qt::Tool | Qt::FramelessWindowHint) : Qt::Dialog);
	setSizeGripEnabled(!d);
	// the geometry is set before the window comes up as well as after: set
	// beforehand it goes to the window manager as the position asked for, which
	// is the one thing that can stop it placing the window somewhere of its own
	snap();
	if (vis) show();		// changing the flags takes the window off screen
	if (!d)
		setZoom(storedZoom());
}

void keyWindow::snap() {
	if (!dock) return;
	QWidget* par = parentWidget() ? parentWidget()->window() : nullptr;
	// A window that is not on screen yet has no geometry worth reading: this
	// window is built before the emulator window is shown, and taking its size
	// then put the keyboard off the screen entirely. Whatever is missed here
	// comes back from showEvent and from every move of the emulator window.
	if (!par || !par->isVisible()) return;
	QRect rc = par->frameGeometry();
	int wid = rc.width();
	int hig = higFor(wid);
	int y = rc.bottom() + 1;
	// fullscreen, or the window sits too low: lay the keyboard over the picture
	if (par->isFullScreen() || (y + hig > SCREENSIZE.height()))
		y = rc.bottom() - hig + 1;
	QRect want(rc.left(), y, wid, hig);
	if (geometry() != want)
		setGeometry(want);
}

// A window manager puts the window where it wants when it maps it, and that
// happens after show() has returned - so while docked, every move it makes is
// answered by putting the window back. snap() does nothing once the geometry
// is right, which is what stops this from bouncing.
void keyWindow::moveEvent(QMoveEvent* ev) {
	QDialog::moveEvent(ev);
	if (dock)
		snap();
}

void keyWindow::showMenu(QPoint gpos) {
	QMenu mnu(this);
	QAction* act;
	const double zval[] = {1.0, 1.5, 2.0, 3.0};
	for (int i = 0; i < 4; i++) {
		act = mnu.addAction(QString("Zoom x%1").arg(zval[i]));
		act->setData(zval[i]);
		act->setDisabled(dock);
	}
	mnu.addSeparator();
	QMenu* skm = mnu.addMenu("Skin");
	QActionGroup* skg = new QActionGroup(skm);
	for (const auto& sk : vkSkins) {
		act = skm->addAction(sk.title);
		act->setCheckable(true);
		act->setChecked(conf.keywin.skin == sk.name);
		act->setData(QString(sk.name));
		act->setActionGroup(skg);
	}
	QAction* dck = mnu.addAction("Dock under the emulator");
	dck->setCheckable(true);
	dck->setChecked(dock);
	mnu.addSeparator();
	QAction* rall = mnu.addAction("Release all keys");
	act = mnu.exec(gpos);
	if (act == NULL) return;
	if (act == dck) {
		setDock(!dock);
	} else if (act == rall) {
		if (kb) x_kbd_release_all(kb);
		xent.zxKey[0] = 0;
		update();
	} else if (act->actionGroup() == skg) {
		conf.keywin.skin = act->data().toString().toStdString();
		loadSkin();
	} else if (act->data().isValid()) {
		setZoom(act->data().toDouble());
	}
}

// The window is free to take any shape here; the picture keeps its own and sits
// in the middle of its ground. Only Windows squares the window itself, in
// nativeEvent below, where it can be done before anything is drawn - putting the
// size back afterwards means fighting the window manager for the frame, and the
// manager wins: the picture ends up drawn for one size while the frame is
// another, so the keyboard is cut off or a stale one is left behind.
void keyWindow::resizeEvent(QResizeEvent* ev) {
	QDialog::resizeEvent(ev);
	if (!dock)
		conf.keywin.width = ev->size().width();
	// drawing the picture anew on every step of a drag would hold the drag up:
	// the old one is stretched meanwhile, the exact one comes when it settles
	if (!pic.isNull())
		drawTimer.start();
}

void keyWindow::showEvent(QShowEvent* ev) {
	QDialog::showEvent(ev);
#if defined(__APPLE__)
	// the shape has to be told to the window itself, and a new one is made
	// whenever the flags change, so it is set every time it comes up
	vkbd_set_aspect(this, qRound(cnv.width()), qRound(cnv.height()));
#endif
	snap();
	// see moveEvent: a window manager can also map the window in its own place
	// without a move worth reporting, so the geometry is put back a few times
	// while the mapping settles - how long that takes is the manager's business
	if (dock) {
		QTimer::singleShot(0, this, SLOT(snap()));
		QTimer::singleShot(150, this, SLOT(snap()));
		QTimer::singleShot(400, this, SLOT(snap()));
	}
}

#if defined(_WIN32)
// Windows asks what size the window may take while the frame is still being
// dragged, so the shape is fixed here, before anything is drawn. Correcting it
// afterwards in resizeEvent works too, but the window then flickers between the
// size the mouse asked for and the one it is put back to for the whole drag.
// The rectangle is in real pixels and Qt's own sizes are not, so the frame is
// measured off the window itself.
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
bool keyWindow::nativeEvent(const QByteArray& type, void* msg, qintptr* res) {
#else
bool keyWindow::nativeEvent(const QByteArray& type, void* msg, long* res) {
#endif
	MSG* wmsg = (MSG*)msg;
	if (wmsg->message == WM_SIZING) {
		RECT* rct = (RECT*)wmsg->lParam;
		RECT wrc, crc;
		GetWindowRect(wmsg->hwnd, &wrc);
		GetClientRect(wmsg->hwnd, &crc);
		int fw = (wrc.right - wrc.left) - crc.right;	// frame around the picture
		int fh = (wrc.bottom - wrc.top) - crc.bottom;
		int wid = (rct->right - rct->left) - fw;
		int hig = (rct->bottom - rct->top) - fh;
		// the side being dragged stays where the mouse is, the other one follows
		switch(wmsg->wParam) {
			case WMSZ_TOP:
			case WMSZ_BOTTOM:
				wid = widFor(hig);
				rct->right = rct->left + wid + fw;
				break;
			default:
				hig = higFor(wid);
				if ((wmsg->wParam == WMSZ_TOPLEFT) || (wmsg->wParam == WMSZ_TOPRIGHT)) {
					rct->top = rct->bottom - hig - fh;
				} else {
					rct->bottom = rct->top + hig + fh;
				}
				break;
		}
		if (res) *res = TRUE;
		return true;
	}
	// the shape fixed above leaves the mouse inside the window, not on the frame,
	// and Qt's window class has no cursor to put back over the frame's sizing arrows
	if ((wmsg->message == WM_SETCURSOR) && (LOWORD(wmsg->lParam) == HTCLIENT)
			&& !QApplication::overrideCursor() && !childAt(mapFromGlobal(QCursor::pos()))) {
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		if (res) *res = TRUE;
		return true;
	}
	return QDialog::nativeEvent(type, msg, res);
}
#endif

void keyWindow::switcher() {
	if (isVisible())
		hide();
	else
		show();
}

void keyWindow::upd(Keyboard* k) {
	kb = k;
	if (isVisible())
		repaint();
}

void keyWindow::rall(Keyboard* k) {
	if (!isVisible()) {
		x_kbd_release_all(k);
	}
}

// TODO: untide from ZX-keyboard (row,pos calculation)
void keyWindow::paintEvent(QPaintEvent*) {
	QPainter pnt;
	unsigned char val;
	int row, pos;
	double sd = devScale();
	// the first picture is drawn at once; a stale one is stretched until the
	// timer draws the exact one (a drag, or the window moved to another screen)
	if (pic.isNull() && (sd > 0))
		redraw();
	else if ((sd != picScale) && !drawTimer.isActive())
		drawTimer.start();
	const QVector<VkKey>& keys = skin.keys();
	pnt.begin(this);
	// The picture is drawn to sit on a ground of its own: the keys are opaque and
	// only a thin contour around each one is clear, so what shows through it is
	// the pressed colour - under a light theme the window colour would outline
	// every key all the time
	pnt.fillRect(rect(), skin.ground());
	// The picture drawn for this size is in device pixels, and so are its
	// contours - the lit keys are put there in the same pixels. A stale one being
	// stretched takes them in canvas units.
	bool exact = (sd == picScale);
	double dpr = devicePixelRatioF();
	pnt.translate(picOrigin());
	if (exact) {
		pnt.scale(1 / dpr, 1 / dpr);
	} else {
		pnt.scale(sd / dpr, sd / dpr);
	}
	if (kb && (keys.size() >= 40)) {
		for(int i = 0; i < 8; i++) {
			pos = (i & 4) ? 0 : 9;
			row = (i & 4) ? (i & 3) : (~i & 3);
			val = ~kb->map[i] & 0x1f;
			while(val) {
				if (val & 1) {
					int n = row * 10 + pos;
					if (exact) {
						pnt.fillRect(skin.fillAt(n, sd), QColor(0, 200, 255));
					} else {
						pnt.fillRect(keys[n].fill, QColor(0, 200, 255));
					}
				}
				val >>= 1;
				pos += (i & 4) ? 1 : -1;
			}
		}
	}
	if (exact) {
		pnt.resetTransform();		// 1:1, the picture is in device pixels already
		pnt.drawPixmap(picOrigin(), pic);
	} else if (!pic.isNull()) {
		pnt.setRenderHint(QPainter::SmoothPixmapTransform, true);
		pnt.drawPixmap(QRectF(0, 0, cnv.width(), cnv.height()), pic, QRectF(pic.rect()));
	}
	pnt.end();
}

void keyWindow::mousePressEvent(QMouseEvent* ev) {
	if (!kb) return;

	bool hit = false;
	QPointF ipos = imgPos(QPoint(ev->xEventX, ev->xEventY));
	foreach(const VkKey& key, skin.keys()) {
		if (key.ch && key.hit.contains(ipos)) {
			hit = true;
			xent.zxKey[0] = key.ch;
		}
	}
	if (!hit) {			// no hit: nothing to release on mouse up either
		xent.zxKey[0] = 0;
		// the right button holds a key down, so the menu is on a miss
		if (ev->button() == Qt::RightButton)
			showMenu(mapToGlobal(QPoint(ev->xEventX, ev->xEventY)));
		return;
	}
	xent.zxKey[1] = 0;
	switch(ev->button()) {
		case Qt::LeftButton:		// the lock: see x_key()
			emu_lock();
			kbd_press(kb, &xent);
			emu_unlock();
			update();
			break;
		case Qt::RightButton:
			emu_lock();
			kbdTrigger(kb, &xent);
			emu_unlock();
			update();
			break;
		case Qt::MiddleButton:
			x_kbd_release_all(kb);
			xent.zxKey[0] = 0;
			update();
			break;
		default:
			break;
	}
}

void keyWindow::mouseReleaseEvent(QMouseEvent* ev) {
	if (!kb) return;
	if (ev->button() == Qt::LeftButton) {
		emu_lock();
		kbd_release(kb, &xent);
		emu_unlock();
	}
	update();
}

void keyWindow::keyPressEvent(QKeyEvent* ev) {
	if (ev->key() == Qt::Key_Escape) {
		hide();
	} else {
		emit s_key_press(ev);
	}
}

void keyWindow::keyReleaseEvent(QKeyEvent* ev) {
	if (ev->key() != Qt::Key_Escape)
		emit s_key_release(ev);
}
