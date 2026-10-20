// The on-screen keyboard's picture, drawn from its layout file.
//
// The layout is Volutar's (res/vkbd/zx-*.json), in the format his layout_svg.py
// turns into an SVG, and it is drawn by the same rules: a key is anchored at its
// centre, a label sits at anchor + (dx, dy), "sn" snaps either to the pixel grid,
// "tl" squeezes a text run to a width and "th" stretches it to a height, the
// contour around a key is cut out of the ground so the pressed colour shows
// through it. What the SVG cannot say is done here at the size the window has:
// key edges and text baselines land on whole pixels of that size, and the text
// is hinted (FreeType, vertical-only) and may be subpixel.

#include "vkbdskin.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>

#include <cmath>
#include <cstring>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_ADVANCES_H

// ---------------------------------------------------------------- helpers

static int fr(double v) {
	return (int)std::floor(v + 0.5);
}

static double snapTo(double v, const QString& mode) {
	if (mode == "px") return fr(v);
	if (mode == "half") return fr(v * 2) / 2.0;
	return v;
}

static double num(const QJsonObject& o, const char* key, double def) {
	QJsonValue v = o.value(key);
	return v.isDouble() ? v.toDouble() : def;
}

// a field under its name or under the one older layouts used
static QJsonValue field(const QJsonObject& o, const char* key, const char* old) {
	return o.contains(key) ? o.value(key) : o.value(old);
}

static QPointF point(const QJsonValue& v) {
	QJsonArray a = v.toArray();
	return QPointF(a.at(0).toDouble(), a.at(1).toDouble());
}

static QVector<QPointF> points(const QJsonValue& v) {
	QVector<QPointF> res;
	foreach(QJsonValue p, v.toArray())
		res.append(point(p));
	return res;
}

static bool isRect(const QJsonArray& a) {
	return (a.size() == 4) && a.at(0).isDouble() && a.at(1).isDouble() && a.at(2).isDouble() && a.at(3).isDouble();
}

static QPainterPath rrect(double x, double y, double w, double h, double r) {
	QPainterPath p;
	r = std::max(0.0, std::min(r, std::min(w / 2.0, h / 2.0)));
	p.addRoundedRect(QRectF(x, y, w, h), r, r);
	return p;
}

// key shapes of layout_svg.py: corner radius, how the body is filled, how far it
// is inset from the key box, and whether it has a contour
enum {FILL_NONE = 0, FILL_CAP, FILL_WHITE};
struct Preset {
	double corner;
	int fill;
	double inset;
	bool contour;
};

static const Preset* preset(const QString& name) {
	static const Preset key = {3, FILL_CAP, 1, true};
	static const Preset plate = {0, FILL_WHITE, 0, false};
	static const Preset ghost = {0, FILL_NONE, 0, false};
	if (name.isEmpty() || (name == "key")) return &key;
	if (name == "plate") return &plate;
	if (name == "ghost") return &ghost;
	return nullptr;
}

// the rainbow of layout_svg.py: colour and x of each band at the top edge
static const struct {
	const char* col;
	double x;
} rainbow[] = {{"#e21430", 569}, {"#e9b71d", 579}, {"#75c150", 589}, {"#379eee", 599}};

// ---------------------------------------------------------------- the font

// One face for the whole program, from the resource: the keyboard never uses a
// system font, so it looks the same everywhere
static FT_Library ftlib = nullptr;
static FT_Face ftface = nullptr;
static QByteArray ftdata;

static FT_Face keysFace() {
	static bool tried = false;
	if (tried) return ftface;
	tried = true;
	QFile file(":/res/vkbd/keys-font.ttf");
	if (!file.open(QIODevice::ReadOnly)) return nullptr;
	ftdata = file.readAll();		// FreeType reads it from here for good
	if (FT_Init_FreeType(&ftlib)) return nullptr;
	if (FT_New_Memory_Face(ftlib, (const FT_Byte*)ftdata.constData(), ftdata.size(), 0, &ftface))
		ftface = nullptr;
	return ftface;
}

// ---------------------------------------------------------------- loading

bool VkSkin::load(const QByteArray& json, QString* err) {
	QJsonParseError perr;
	QJsonDocument doc = QJsonDocument::fromJson(json, &perr);
	if (!doc.isObject()) {
		if (err) *err = perr.errorString();
		return false;
	}
	QJsonObject cfg = doc.object();
	QJsonObject canv = cfg.value("canvas").toObject();
	cw = field(canv, "w", "width").toDouble(530);
	ch = field(canv, "h", "height").toDouble(248);
	bg = QColor(field(canv, "bg", "background").toString("#000000"));
	// under the contours: the ground of the picture unless the layout says
	// otherwise - a light keyboard needs a darker one, or its contours vanish
	grnd = canv.contains("ground") ? QColor(canv.value("ground").toString()) : bg;
	if (!grnd.isValid()) grnd = bg;
	anchor = cfg.value("anchor").toString();
	sn = cfg.value("sn").toString("off");
	stops.clear();
	QJsonArray stp = cfg.value("gradient").toObject().value("stops").toArray();
	if (stp.isEmpty())
		stp = QJsonDocument::fromJson("[[0,\"#4e5868\"],[0.3,\"#2f384a\"],[1,\"#44545e\"]]").array();
	foreach(QJsonValue v, stp)
		stops.append(qMakePair(v.toArray().at(0).toDouble(), QColor(v.toArray().at(1).toString())));
	QJsonObject cnt = cfg.value("contour").toObject();
	inset = num(cnt, "inset", 1);
	outLR = num(cnt, "out_lr", 0);
	outTB = num(cnt, "out_tb", 1);
	corner = num(cnt, "corner", 3);
	QJsonObject rb = cfg.value("rainbow").toObject();
	rbW = field(rb, "w", "width").toDouble(11);
	rbSlope = num(rb, "slope", -0.45);
	colors.clear();
	QJsonObject cols = cfg.value("colors").toObject();
	foreach(QString k, cols.keys())
		colors[k] = QColor(cols.value(k).toString());
	icons.clear();
	QJsonObject ics = cfg.value("icons").toObject();
	foreach(QString k, ics.keys()) {
		QJsonObject g = ics.value(k).toObject();
		Icon ic;
		ic.sw = num(g, "sw", 0);
		if (g.contains("line")) {
			QJsonArray ln = g.value("line").toArray();
			ic.isLine = true;
			ic.p1 = point(ln.at(0));
			ic.p2 = point(ln.at(1));
		} else if (g.contains("box") || g.contains("cut")) {
			QJsonArray b = g.value("box").toArray();
			ic.isBox = true;
			ic.box = (b.size() == 4) ? QRectF(b.at(0).toDouble(), b.at(1).toDouble(), b.at(2).toDouble(), b.at(3).toDouble()) : QRectF(0, 0, 8, 8);
			foreach(QJsonValue c, g.value("cut").toArray()) {
				QJsonArray ca = c.toArray();
				if (isRect(ca)) {
					ic.cuts.append(QRectF(ca.at(0).toDouble(), ca.at(1).toDouble(), ca.at(2).toDouble(), ca.at(3).toDouble()));
				} else {
					ic.cutPolys.append(points(c));
				}
			}
		} else if (!g.value("polys").toArray().isEmpty()) {
			foreach(QJsonValue p, g.value("polys").toArray())
				ic.polys.append(points(p));
		} else {
			ic.polys.append(points(g.value("pts")));
		}
		icons[k] = ic;
	}
	rows.clear();
	foreach(QJsonValue rv, cfg.value("rows").toArray()) {
		QJsonObject ro = rv.toObject();
		Row row;
		row.y = num(ro, "y", 0);
		row.h = num(ro, "h", 0);
		foreach(QJsonValue kv, ro.value("keys").toArray()) {
			QJsonObject ko = kv.toObject();
			Key key;
			key.x = num(ko, "x", 0);
			key.w = num(ko, "w", 0);
			key.ax = num(ko, "anchor_x", 0);
			key.ay = num(ko, "anchor_y", 0);
			key.shape = ko.value("shape").toString();
			key.sn = ko.value("sn").toString();
			if (!preset(key.shape)) {
				if (err) *err = QString("unknown key shape: %0").arg(key.shape);
				rows.clear();
				return false;
			}
			foreach(QJsonValue lv, ko.value("labels").toArray()) {
				QJsonObject lo = lv.toObject();
				Label lab;
				lab.icon = field(lo, "i", "icon").toString();
				lab.text = lo.value("t").toVariant().toString();
				QString col = field(lo, "col", "color").toString("white");
				lab.col = colors.contains(col) ? colors[col] : QColor(col);
				if (!lab.col.isValid()) lab.col = Qt::white;
				lab.sz = field(lo, "sz", "size").toDouble(9) + num(lo, "sy", 0);
				lab.dx = num(lo, "dx", 0);
				lab.dy = num(lo, "dy", 0);
				lab.hasTl = lo.contains("tl");
				lab.tl = num(lo, "tl", 0) + num(lo, "sx", 0);
				lab.th = num(lo, "th", 0);
				// eb: thicker by that many canvas pixels, both ways or [x, y]
				QJsonValue eb = lo.value("eb");
				lab.ebx = eb.isArray() ? eb.toArray().at(0).toDouble() : eb.toDouble(0);
				lab.eby = eb.isArray() ? eb.toArray().at(1).toDouble() : eb.toDouble(0);
				lab.anchor = lo.value("a").toString();
				lab.sn = lo.value("sn").toString();
				if (!lab.icon.isEmpty() && !icons.contains(lab.icon)) {
					if (err) *err = QString("unknown icon id: %0").arg(lab.icon);
					rows.clear();
					return false;
				}
				key.labels.append(lab);
			}
			row.keys.append(key);
		}
		rows.append(row);
	}
	if (rows.isEmpty() || (cw <= 0) || (ch <= 0)) {
		if (err) *err = "no keys";
		rows.clear();
		return false;
	}
	makeKeys();
	return true;
}

// Which Spectrum key each one is comes from its place: the rows are the
// keyboard's, left to right. A key whose first label says otherwise is a sign the
// layout was rearranged, and it is said so.
void VkSkin::makeKeys() {
	static const char* zxRows[] = {"1234567890", "qwertyuiop", "asdfghjklE", "CzxcvbnmS "};
	static const char* zxNames[] = {"ENTER", "CAPS", "SYMBOL", "BREAK"};
	static const char zxNamed[] = "ECS ";
	keytab.clear();
	for (int r = 0; r < rows.size(); r++) {
		const Row& row = rows[r];
		double top = (r == 0) ? 0 : (rows[r - 1].y + rows[r - 1].h + row.y) / 2.0;
		double bot = (r == rows.size() - 1) ? ch : (row.y + row.h + rows[r + 1].y) / 2.0;
		for (int i = 0; i < row.keys.size(); i++) {
			const Key& key = row.keys[i];
			VkKey vk;
			vk.row = r;
			vk.col = i;
			vk.ch = ((r < 4) && (i < 10)) ? zxRows[r][i] : 0;
			vk.box = QRectF(key.x, row.y, key.w, row.h);
			vk.fill = vk.box.adjusted(-outLR - 0.5, -outTB - 0.5, outLR + 0.5, outTB + 0.5);
			// as wide as the key, from row to row: the gaps between the keys and the
			// ground around them are left as misses, which is where the window's
			// menu opens (the right button on a key holds the key down)
			vk.hit = QRectF(key.x, top, key.w, bot - top);
			foreach(const Label& lab, key.labels) {
				if (lab.text.isEmpty()) continue;
				const char* nm = strchr(zxNamed, vk.ch);
				QString want = nm ? zxNames[nm - zxNamed] : QString(QChar(vk.ch)).toUpper();
				if (!lab.text.startsWith(want))
					qWarning("vkbd: key %i of row %i is '%c' by its place, its label says '%s'",
						 i, r, vk.ch, lab.text.toUtf8().constData());
				break;
			}
			keytab.append(vk);
		}
	}
}

// ---------------------------------------------------------------- drawing

// a label's coverage, one byte per channel (R, G, B) - all three the same for gray
struct Mask {
	int x = 0, y = 0, w = 0, h = 0;
	std::vector<uchar> c;
};

// put a label down in its colour. A subpixel triad needs an opaque pixel under
// it, so over the contour (or anything not opaque) the three are averaged
static void composite(QImage& img, const Mask& m, const QColor& col) {
	int cr = col.red();
	int cg = col.green();
	int cb = col.blue();
	for (int y = 0; y < m.h; y++) {
		int dy = m.y + y;
		if ((dy < 0) || (dy >= img.height())) continue;
		QRgb* line = (QRgb*)img.scanLine(dy);
		const uchar* src = m.c.data() + y * m.w * 3;
		for (int x = 0; x < m.w; x++, src += 3) {
			int dx = m.x + x;
			if ((dx < 0) || (dx >= img.width())) continue;
			int kr = src[0];
			int kg = src[1];
			int kb = src[2];
			if (!(kr | kg | kb)) continue;
			QRgb d = line[dx];
			int a = qAlpha(d);
			int km = (kr + kg + kb + 1) / 3;
			if (a < 255) kr = kg = kb = km;
			line[dx] = qRgba((cr * kr + qRed(d) * (255 - kr) + 127) / 255,
					 (cg * kg + qGreen(d) * (255 - kg) + 127) / 255,
					 (cb * kb + qBlue(d) * (255 - kb) + 127) / 255,
					 (255 * km + a * (255 - km) + 127) / 255);
		}
	}
}

// an icon is drawn as a shape into a mask of its own, then laid down like text
static Mask iconMask(const QPainterPath& path, double sw, bool stroke, bool flatCap) {
	Mask m;
	QPen pen(Qt::black, sw, Qt::SolidLine, flatCap ? Qt::FlatCap : Qt::SquareCap, Qt::MiterJoin);
	pen.setMiterLimit(4);		// SVG's default
	QPainterPathStroker sk(pen);
	QRectF br = stroke ? sk.createStroke(path).boundingRect() : path.boundingRect();
	QRect box = br.toAlignedRect().adjusted(-1, -1, 1, 1);
	if (box.isEmpty()) return m;
	QImage msk(box.size(), QImage::Format_Alpha8);
	msk.fill(0);
	QPainter p(&msk);
	p.setRenderHint(QPainter::Antialiasing, true);
	p.translate(-box.topLeft());
	if (stroke) {
		p.setPen(pen);
		p.setBrush(Qt::NoBrush);
	} else {
		p.setPen(Qt::NoPen);
		p.setBrush(Qt::black);
	}
	p.drawPath(path);
	p.end();
	m.x = box.x();
	m.y = box.y();
	m.w = box.width();
	m.h = box.height();
	m.c.resize(m.w * m.h * 3);
	for (int y = 0; y < m.h; y++) {
		const uchar* s = msk.constScanLine(y);
		uchar* d = m.c.data() + y * m.w * 3;
		for (int x = 0; x < m.w; x++, d += 3)
			d[0] = d[1] = d[2] = s[x];
	}
	return m;
}

// a width in the picture's pixels: whole, and at least one if it is not none
static int pxWidth(double v, double s) {
	return (v == 0) ? 0 : std::max(1, fr(v * s));
}

// a key box in the picture's pixels, its edges on whole ones
static QRect keyBoxAt(double y, double h, double x, double w, double s) {
	int x0 = fr(x * s);
	int y0 = fr(y * s);
	return QRect(x0, y0, fr((x + w) * s) - x0, fr((y + h) * s) - y0);
}

// The contour's outer edge as render() cuts it, and a pixel more all round: the
// excess is under the opaque ground, so no rounding can leave a dark seam
QRect VkSkin::fillAt(int n, double s) const {
	if ((n < 0) || (n >= keytab.size())) return QRect();
	const Row& row = rows[keytab[n].row];
	const Key& key = row.keys[keytab[n].col];
	QRect kb = keyBoxAt(row.y, row.h, key.x, key.w, s);
	int olr = pxWidth(outLR, s) + 1;
	int otb = pxWidth(outTB, s) + 1;
	return kb.adjusted(-olr, -otb, olr, otb);
}

QImage VkSkin::render(double s, bool lcd) const {
	int W = std::max(1, fr(cw * s));
	int H = std::max(1, fr(ch * s));
	QImage img(W, H, QImage::Format_ARGB32_Premultiplied);
	img.fill(Qt::transparent);
	if (isNull()) return img;
	auto px = [s](double v) {return pxWidth(v, s);};
	auto keyBox = [s](const Row& row, const Key& key) {return keyBoxAt(row.y, row.h, key.x, key.w, s);};

	QPainter p(&img);
	p.fillRect(img.rect(), bg);
	p.setRenderHint(QPainter::Antialiasing, true);
	p.setPen(Qt::NoPen);
	for (const auto& band : rainbow) {
		QPolygonF poly;
		poly << QPointF(band.x * s, 0) << QPointF((band.x + rbW) * s, 0)
		     << QPointF((band.x + rbSlope * ch + rbW) * s, ch * s) << QPointF((band.x + rbSlope * ch) * s, ch * s);
		p.setBrush(QColor(band.col));
		p.drawPolygon(poly);
	}
	// the contours, cut out of the ground and the rainbow alike
	p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
	p.setBrush(Qt::black);
	int olr = px(outLR);
	int otb = px(outTB);
	double rr = (corner + std::max(outTB, outLR)) * s;
	foreach(const Row& row, rows) {
		foreach(const Key& key, row.keys) {
			if (!preset(key.shape)->contour) continue;
			QRect kb = keyBox(row, key);
			p.drawPath(rrect(kb.x() - olr, kb.y() - otb, kb.width() + 2 * olr, kb.height() + 2 * otb, rr));
		}
	}
	// the caps over them: the edge pixel is the cap's colour with what is left of
	// its alpha, so a pressed key shows through exactly where the contour is
	p.setCompositionMode(QPainter::CompositionMode_SourceOver);
	foreach(const Row& row, rows) {
		foreach(const Key& key, row.keys) {
			const Preset* pr = preset(key.shape);
			if (pr->fill == FILL_NONE) continue;
			QRect kb = keyBox(row, key);
			int ins = px(pr->inset);
			QRectF body(kb.x() + ins, kb.y() + ins, kb.width() - 2 * ins, kb.height() - 2 * ins);
			if (pr->fill == FILL_CAP) {
				QLinearGradient gr(0, body.top(), 0, body.bottom());
				for (const auto& st : stops)
					gr.setColorAt(st.first, st.second);
				p.setBrush(gr);
			} else {
				p.setBrush(Qt::white);
			}
			p.drawPath(rrect(body.x(), body.y(), body.width(), body.height(), std::max(0.0, pr->corner - pr->inset) * s));
		}
	}
	p.end();

	FT_Face face = keysFace();
	foreach(const Row& row, rows) {
		foreach(const Key& key, row.keys) {
			QRect kb = keyBox(row, key);
			QString kmode = key.sn.isEmpty() ? sn : key.sn;
			double rx = snapTo((kb.left() + kb.left() + kb.width()) / 2.0 + key.ax * s, kmode);
			double ry = snapTo((kb.top() + kb.top() + kb.height()) / 2.0 + key.ay * s, kmode);
			foreach(const Label& lab, key.labels) {
				QString mode = lab.sn.isEmpty() ? kmode : lab.sn;
				double x = snapTo(rx + lab.dx * s, mode);
				double y = snapTo(ry + lab.dy * s, mode);
				if (!lab.icon.isEmpty()) {
					const Icon& ic = icons[lab.icon];
					QTransform tr;
					tr.translate(x, y);
					tr.scale(s, s);
					QPainterPath path;
					path.setFillRule(Qt::OddEvenFill);
					if (ic.isLine) {
						path.moveTo(ic.p1);
						path.lineTo(ic.p2);
						composite(img, iconMask(tr.map(path), (ic.sw > 0 ? ic.sw : 1) * s, true, true), lab.col);
					} else if (ic.isBox) {
						path.addRect(ic.box);
						foreach(QRectF c, ic.cuts)
							path.addRect(c);
						foreach(const QVector<QPointF>& c, ic.cutPolys)
							path.addPolygon(QPolygonF(c)), path.closeSubpath();
						composite(img, iconMask(tr.map(path), 0, false, false), lab.col);
					} else {
						foreach(const QVector<QPointF>& c, ic.polys) {
							QPainterPath pp;
							pp.setFillRule(Qt::OddEvenFill);
							pp.addPolygon(QPolygonF(c));
							pp.closeSubpath();
							composite(img, iconMask(tr.map(pp), ic.sw * s, ic.sw > 0, false), lab.col);
						}
					}
					continue;
				}
				if (!face || lab.text.isEmpty()) continue;
				// the run's glyphs and their places in font units, kerned
				QVector<uint> cps = lab.text.toUcs4();
				QVector<FT_UInt> gid;
				QVector<long> pos;
				long pen = 0;
				for (int i = 0; i < cps.size(); i++)
					gid.append(FT_Get_Char_Index(face, cps[i]));
				for (int i = 0; i < gid.size(); i++) {
					pos.append(pen);
					FT_Fixed adv = 0;
					FT_Get_Advance(face, gid[i], FT_LOAD_NO_SCALE, &adv);
					pen += adv;
					if (i + 1 < gid.size()) {
						FT_Vector kv;
						if (!FT_Get_Kerning(face, gid[i], gid[i + 1], FT_KERNING_UNSCALED, &kv))
							pen += kv.x;
					}
				}
				double upem = face->units_per_EM;
				double natural = pen * lab.sz / upem;
				double kx = (lab.hasTl && (lab.tl >= 4) && (natural > 0)) ? lab.tl / natural : 1.0;
				double ky = (lab.th > 0) ? lab.th / lab.sz : 1.0;
				QString anc = !lab.anchor.isEmpty() ? lab.anchor : (!anchor.isEmpty() ? anchor : QString("middle"));
				if (anc == "middle") x -= natural * kx * s / 2.0;
				else if (anc == "end") x -= natural * kx * s;
				int base = fr(y);		// the baseline on a whole pixel, always
				double ppx = lab.sz * s * kx;	// pixels per em, each way
				double ppy = lab.sz * s * ky;
				FT_Set_Char_Size(face, (FT_F26Dot6)std::lround(ppx * 64), (FT_F26Dot6)std::lround(ppy * 64), 72, 72);
				// every glyph rendered at its own subpixel x, then laid into one mask
				struct Glyph {int x, y, w, h; std::vector<uchar> c;};
				QVector<Glyph> gl;
				for (int i = 0; i < gid.size(); i++) {
					double gx = x + pos[i] * ppx / upem;
					int ix = (int)std::floor(gx);
					FT_Vector delta = {(FT_Pos)std::lround((gx - ix) * 64), 0};
					FT_Set_Transform(face, nullptr, &delta);
					if (FT_Load_Glyph(face, gid[i], lcd ? FT_LOAD_TARGET_LCD : FT_LOAD_TARGET_LIGHT)) continue;
					if ((lab.ebx != 0) || (lab.eby != 0))
						FT_Outline_EmboldenXY(&face->glyph->outline, std::lround(lab.ebx * s * 64), std::lround(lab.eby * s * 64));
					if (FT_Render_Glyph(face->glyph, lcd ? FT_RENDER_MODE_LCD : FT_RENDER_MODE_NORMAL)) continue;
					const FT_Bitmap& bm = face->glyph->bitmap;
					if (!bm.width || !bm.rows) continue;
					Glyph g;
					g.w = lcd ? bm.width / 3 : bm.width;
					g.h = bm.rows;
					g.x = ix + face->glyph->bitmap_left;
					g.y = base - face->glyph->bitmap_top;
					g.c.resize(g.w * g.h * 3);
					for (int yy = 0; yy < g.h; yy++) {
						const uchar* s = bm.buffer + yy * bm.pitch;
						uchar* d = g.c.data() + yy * g.w * 3;
						for (int xx = 0; xx < g.w; xx++, d += 3) {
							if (lcd) {
								d[0] = s[xx * 3];
								d[1] = s[xx * 3 + 1];
								d[2] = s[xx * 3 + 2];
							} else {
								d[0] = d[1] = d[2] = s[xx];
							}
						}
					}
					gl.append(g);
				}
				FT_Set_Transform(face, nullptr, nullptr);
				if (gl.isEmpty()) continue;
				QRect box(gl[0].x, gl[0].y, gl[0].w, gl[0].h);
				foreach(const Glyph& g, gl)
					box |= QRect(g.x, g.y, g.w, g.h);
				Mask m;
				m.x = box.x();
				m.y = box.y();
				m.w = box.width();
				m.h = box.height();
				m.c.assign(m.w * m.h * 3, 0);
				foreach(const Glyph& g, gl) {
					for (int yy = 0; yy < g.h; yy++) {
						const uchar* s = g.c.data() + yy * g.w * 3;
						uchar* d = m.c.data() + ((g.y - m.y + yy) * m.w + (g.x - m.x)) * 3;
						for (int k = 0; k < g.w * 3; k++)	// overlapping glyphs: one over the other
							d[k] = 255 - (255 - d[k]) * (255 - s[k]) / 255;
					}
				}
				composite(img, m, lab.col);
			}
		}
	}
	return img;
}
