#pragma once

// Built into the exe, not loaded from a folder: moc has to see this before qtplugin.h
#ifndef QT_STATICPLUGIN
#define QT_STATICPLUGIN
#endif

#include <QIconEngine>
#include <QIconEnginePlugin>
#include <QHash>
#include <QList>

// An icon out of :/images, with the pictures drawn for small sizes beside it.
// Whatever makes a QIcon of "x.png" - code, a .ui form - gets "x-16.png" and
// "x-24.png" as well where they are drawn, so a picture drawn for 16 px is the
// one shown at 16 px everywhere, and not the big one scaled down. A size is
// picked as Qt picks it: the smallest picture at least as big, else the biggest.
class xIconEngine : public QIconEngine {
	public:
		xIconEngine() {}
		xIconEngine(const xIconEngine&) = default;
		void paint(QPainter*, const QRect&, QIcon::Mode, QIcon::State) override;
		QPixmap pixmap(const QSize&, QIcon::Mode, QIcon::State) override;
		QSize actualSize(const QSize&, QIcon::Mode, QIcon::State) override;
		void addPixmap(const QPixmap&, QIcon::Mode, QIcon::State) override;
		void addFile(const QString&, const QSize&, QIcon::Mode, QIcon::State) override;
		QString key() const override;
		QIconEngine* clone() const override;
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
		QList<QSize> availableSizes(QIcon::Mode, QIcon::State) override;
		bool isNull() override {return pics.isEmpty();}
#else
		QList<QSize> availableSizes(QIcon::Mode, QIcon::State) const override;
		void virtual_hook(int, void*) override;
#endif
	private:
		struct xPic {
			QPixmap pix;
			QIcon::Mode mode;
			QIcon::State state;
		};
		QList<xPic> pics;
		QHash<QString, QPixmap> made;	// scaled or greyed, by size, mode and state
		const xPic* match(const QSize&, QIcon::Mode, QIcon::State) const;
		const xPic* best(const QSize&, QIcon::Mode, QIcon::State) const;
};

class xIconPlugin : public QIconEnginePlugin {
	Q_OBJECT
	// QIconEngineFactoryInterface_iid, spelled out: moc is not given Qt's headers to expand it
	Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QIconEngineFactoryInterface" FILE "iconengine.json")
	public:
		QIconEngine* create(const QString& = QString()) override;
};
