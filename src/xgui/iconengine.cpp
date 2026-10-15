#include "iconengine.h"

#include <QApplication>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>

static int area(const QSize& sz) {
	return sz.width() * sz.height();
}

// the picture drawn for this mode and state that suits the size best
const xIconEngine::xPic* xIconEngine::match(const QSize& size, QIcon::Mode mode, QIcon::State state) const {
	const xPic* got = nullptr;
	int want = area(size);
	for (const xPic& p : pics) {
		if ((p.mode != mode) || (p.state != state)) continue;
		if (got) {
			int a = area(p.pix.size());
			int b = area(got->pix.size());
			int pick = (qMin(a, b) >= want) ? qMin(a, b) : qMax(a, b);
			if (pick != a) continue;
		}
		got = &p;
	}
	return got;
}

// with none drawn for the mode or the state asked, another one stands in, in Qt's order
const xIconEngine::xPic* xIconEngine::best(const QSize& size, QIcon::Mode mode, QIcon::State state) const {
	QIcon::State other = (state == QIcon::On) ? QIcon::Off : QIcon::On;
	QList<QPair<QIcon::Mode, QIcon::State> > order;
	order << qMakePair(mode, state);
	if ((mode == QIcon::Disabled) || (mode == QIcon::Selected)) {
		QIcon::Mode opp = (mode == QIcon::Disabled) ? QIcon::Selected : QIcon::Disabled;
		order << qMakePair(QIcon::Normal, state) << qMakePair(QIcon::Active, state) << qMakePair(mode, other)
			<< qMakePair(QIcon::Normal, other) << qMakePair(QIcon::Active, other) << qMakePair(opp, state)
			<< qMakePair(opp, other);
	} else {
		QIcon::Mode opp = (mode == QIcon::Normal) ? QIcon::Active : QIcon::Normal;
		order << qMakePair(opp, state) << qMakePair(mode, other) << qMakePair(opp, other)
			<< qMakePair(QIcon::Disabled, state) << qMakePair(QIcon::Selected, state)
			<< qMakePair(QIcon::Disabled, other) << qMakePair(QIcon::Selected, other);
	}
	for (const auto& ms : order) {
		const xPic* p = match(size, ms.first, ms.second);
		if (p) return p;
	}
	return nullptr;
}

QSize xIconEngine::actualSize(const QSize& size, QIcon::Mode mode, QIcon::State state) {
	const xPic* p = best(size, mode, state);
	if (!p) return QSize();
	QSize sz = p->pix.size();
	if ((sz.width() > size.width()) || (sz.height() > size.height()))
		sz.scale(size, Qt::KeepAspectRatio);
	return sz;
}

QPixmap xIconEngine::pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) {
	const xPic* p = best(size, mode, state);
	if (!p) return QPixmap();
	QSize sz = actualSize(size, mode, state);
	bool grey = (p->mode != mode) && (mode != QIcon::Normal);
	if ((sz == p->pix.size()) && !grey) return p->pix;
	QString key = QString("%0x%1/%2/%3").arg(size.width()).arg(size.height()).arg(int(mode)).arg(int(state));
	if (made.contains(key)) return made.value(key);
	QPixmap pm = p->pix;
	if (pm.size() != sz)
		pm = pm.scaled(sz, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	if (grey) {
		QStyleOption opt;
		opt.palette = QApplication::palette();
		QPixmap gen = QApplication::style()->generatedIconPixmap(mode, pm, &opt);
		if (!gen.isNull()) pm = gen;
	}
	made.insert(key, pm);
	return pm;
}

void xIconEngine::paint(QPainter* pnt, const QRect& rect, QIcon::Mode mode, QIcon::State state) {
	qreal dpr = pnt->device() ? pnt->device()->devicePixelRatioF() : qApp->devicePixelRatio();
	pnt->drawPixmap(rect, pixmap(rect.size() * dpr, mode, state));
}

void xIconEngine::addPixmap(const QPixmap& pix, QIcon::Mode mode, QIcon::State state) {
	if (pix.isNull()) return;
	made.clear();
	for (xPic& p : pics) {
		if ((p.mode == mode) && (p.state == state) && (p.pix.size() == pix.size())) {
			p.pix = pix;
			return;
		}
	}
	pics.append({pix, mode, state});
}

// "x.png" brings "x-16.png" and "x-24.png" with it, where they are drawn
void xIconEngine::addFile(const QString& path, const QSize&, QIcon::Mode mode, QIcon::State state) {
	addPixmap(QPixmap(path), mode, state);
	if (!path.endsWith(".png")) return;
	QString base = path.left(path.size() - 4);
	for (int px : {16, 24})
		if (base.endsWith(QString("-%0").arg(px))) return;	// one of the small ones itself
	for (int px : {16, 24})
		addPixmap(QPixmap(QString("%0-%1.png").arg(base).arg(px)), mode, state);
}

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
QList<QSize> xIconEngine::availableSizes(QIcon::Mode mode, QIcon::State state) {
#else
QList<QSize> xIconEngine::availableSizes(QIcon::Mode mode, QIcon::State state) const {
#endif
	QList<QSize> res;
	for (const xPic& p : pics)
		if ((p.mode == mode) && (p.state == state))
			res.append(p.pix.size());
	return res;
}

QString xIconEngine::key() const {
	return "xpeccy-png";
}

QIconEngine* xIconEngine::clone() const {
	return new xIconEngine(*this);
}

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
void xIconEngine::virtual_hook(int id, void* data) {
	if (id == QIconEngine::IsNullHook) {
		*reinterpret_cast<bool*>(data) = pics.isEmpty();
	} else {
		QIconEngine::virtual_hook(id, data);
	}
}
#endif

// Qt asks for an engine by the file's suffix; its own icons (styles, dialogs) keep its own
QIconEngine* xIconPlugin::create(const QString& path) {
	return path.startsWith(":/images/") ? new xIconEngine : nullptr;
}
