#pragma once

// The on-screen keyboard's picture, drawn from a layout file (res/vkbd/zx-*.json,
// the format of Volutar's layout_svg.py) at any size, sharp at every one of them.

#include <QByteArray>
#include <QColor>
#include <QImage>
#include <QMap>
#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QString>
#include <QVector>

struct VkKey {
	unsigned char ch;	// the Spectrum key, as keyEntry::zxKey takes it
	QRectF box;		// the key as drawn, in canvas units
	QRectF fill;		// what shows through its contour when it is pressed
	QRectF hit;		// what a click on it takes
	int row, col;		// where it is in the layout
};

class VkSkin {
	public:
		bool load(const QByteArray& json, QString* err = nullptr);
		bool isNull() const {return rows.isEmpty();}
		QSizeF canvas() const {return QSizeF(cw, ch);}
		// what the window puts under the picture, so what the contours show
		QColor ground() const {return grnd;}
		const QVector<VkKey>& keys() const {return keytab;}
		// what to light for key n in a picture drawn at that scale, in its pixels:
		// its contour as the picture has it, which on a small one is wider than the
		// contour scaled down
		QRect fillAt(int n, double scale) const;
		// the Spectrum key at that place of the keyboard's matrix: half-row 0-3
		// from the top, key 0-9 from the left; 0 for none
		static unsigned char zxKeyAt(int row, int pos);
		// the picture at that many pixels per canvas unit, premultiplied ARGB;
		// lcd: subpixel (RGB) text on its opaque parts
		QImage render(double scale, bool lcd) const;
	private:
		struct Label {
			QString text;		// a text label...
			QString icon;		// ...or an icon
			QColor col;
			double sz, dx, dy, tl, th, ebx, eby;
			bool hasTl;
			QString anchor;		// "", start, middle, end
			QString sn;		// "", off, px, half
		};
		struct Key {
			double x, w, ax, ay;
			QString shape, sn;
			QVector<Label> labels;
		};
		struct Row {
			double y, h;
			QVector<Key> keys;
		};
		struct Icon {
			QVector<QVector<QPointF>> polys;	// filled even-odd, or stroked with sw
			QVector<QRectF> cuts;			// holes in box
			QVector<QVector<QPointF>> cutPolys;
			QRectF box;
			bool isBox = false;
			bool isLine = false;
			QPointF p1, p2;
			double sw = 0;
		};
		double cw = 0, ch = 0;
		QColor bg, grnd;
		QString anchor, sn;
		QVector<QPair<double, QColor>> stops;
		double inset = 1, outLR = 0, outTB = 1, corner = 3;
		double rbW = 11, rbSlope = -0.45;
		QMap<QString, QColor> colors;
		QMap<QString, Icon> icons;
		QVector<Row> rows;
		QVector<VkKey> keytab;
		void makeKeys();
};
