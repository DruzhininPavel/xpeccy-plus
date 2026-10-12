#include <QCursor>
#include <QHBoxLayout>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QSplitter>
#include <QHeaderView>
#include <QTreeWidgetItemIterator>
#include <QPushButton>
#include <QVBoxLayout>

#include "dbg_evo.h"
#include "../../libxpeccy/xreplay.h"

// LIST

xEvoList::xEvoList(QStringList head, QWidget* p):QTreeWidget(p) {
	setColumnCount(head.size());
	setHeaderLabels(head);
	setRootIsDecorated(false);
	setItemsExpandable(false);
	setIndentation(0);
	setUniformRowHeights(true);
	setSelectionMode(QAbstractItemView::NoSelection);
	setFocusPolicy(Qt::NoFocus);
	setAlternatingRowColors(true);		// as the dumps and the listing have them
	header()->setStretchLastSection(true);
	header()->setSectionsMovable(false);
	header()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	header()->setToolTip("Right click: in words or by field names");
	// the last column in plain words or as the docs name the fields, for both lists
	setContextMenuPolicy(Qt::CustomContextMenu);
	header()->setContextMenuPolicy(Qt::CustomContextMenu);
	auto menu = [this](const QPoint&) {
		QMenu mnu;
		QAction* words = mnu.addAction("In words");
		QAction* names = mnu.addAction("Field names");
		words->setCheckable(true);
		names->setCheckable(true);
		words->setChecked(!conf.dbg.evonames);
		names->setChecked(conf.dbg.evonames);
		QAction* got = mnu.exec(QCursor::pos());
		if (!got) return;
		conf.dbg.evonames = (got == names) ? 1 : 0;
		if (onStyle) onStyle();
	};
	connect(this, &QWidget::customContextMenuRequested, this, menu);
	connect(header(), &QWidget::customContextMenuRequested, this, menu);
}

// A column as wide as the widest of its samples, and never narrower than it has
// been: a value that grows for a step and shrinks back must not move the table.
// A font change (the debugger hands its own out after the panel is made) starts
// the widths over.
static void evo_fit_column(QTreeWidget* view, int col, QStringList samples) {
	QString fkey = QString("fitFont%0").arg(col);
	if (view->property(fkey.toUtf8().constData()).toString() != view->font().toString()) {
		view->setProperty(fkey.toUtf8().constData(), view->font().toString());
		view->setColumnWidth(col, 0);
	}
	samples << view->headerItem()->text(col);
	QTreeWidgetItemIterator it(view);
	for (; *it; ++it)
		if (!(*it)->isFirstColumnSpanned()) samples << (*it)->text(col);
	QFontMetrics fm(view->font());
	int w = 0;
	foreach (QString smp, samples)
		w = qMax(w, fm.horizontalAdvance(smp) + fm.averageCharWidth());
	if (w > view->columnWidth(col))
		view->setColumnWidth(col, w);
}

// group rows are colored as the debugger's panel headers are
static void evo_header_colors(QTreeWidgetItem* it, const QFont& base) {
	QFont fnt = base;
	fnt.setBold(true);
	it->setFont(0, fnt);
	QColor bg = conf.pal.value("dbg.header.bg");
	QColor fg = conf.pal.value("dbg.header.txt");
	for (int i = 0; i < it->columnCount(); i++) {
		if (bg.isValid()) it->setBackground(i, bg);
		if (fg.isValid()) it->setForeground(i, fg);
	}
}

QTreeWidgetItem* xEvoList::group(const QString& name) {
	QTreeWidgetItem* it = groups.value(name, nullptr);
	if (it) return it;
	it = new QTreeWidgetItem(this, QStringList(name));
	it->setFirstColumnSpanned(true);
	evo_header_colors(it, font());
	it->setExpanded(true);
	groups[name] = it;
	return it;
}

void xEvoList::put(const QString& grp, const QString& name, const QStringList& cols) {
	QString key = grp + "/" + name;
	seen.insert(key);
	QTreeWidgetItem* it = rows.value(key, nullptr);
	bool fresh = !it;
	if (fresh) {
		it = new QTreeWidgetItem(group(grp));
		rows[key] = it;
	}
	// a value that moved since the last look is lit, as the registers are
	QColor bg = conf.pal.value(DBG_PAL_CHG_BG);
	QColor fg = conf.pal.value(DBG_PAL_CHG_TXT);
	for (int i = 0; i < cols.size(); i++) {
		bool moved = !fresh && (it->text(i) != cols[i]);
		it->setText(i, cols[i]);
		if (i != 2) continue;
		it->setTextAlignment(i, Qt::AlignRight | Qt::AlignVCenter);
		if (moved) {
			if (bg.isValid()) it->setBackground(i, bg);
			if (fg.isValid()) it->setForeground(i, fg);
		} else {
			it->setData(i, Qt::BackgroundRole, QVariant());
			it->setData(i, Qt::ForegroundRole, QVariant());
		}
	}
}

void xEvoList::finish() {
	foreach (QString key, rows.keys()) {
		if (seen.contains(key)) continue;
		delete rows.take(key);
	}
	foreach (QString key, groups.keys()) {
		if (groups[key]->childCount()) continue;
		delete groups.take(key);
	}
	seen.clear();
}

void xEvoList::restyle() {
	if (columnCount() > 3)
		headerItem()->setText(3, conf.dbg.evonames ? "Fields" : "Means");
	foreach (QTreeWidgetItem* it, groups)
		evo_header_colors(it, font());
	for (int i = 0; i < columnCount() - 1; i++)
		evo_fit_column(this, i, (i == 2) ? QStringList("000 > 000") : QStringList());
}

// exactly as tall as its rows, for a list that shares the page with another
void xEvoList::fitHeight() {
	int rows = 0;
	int rh = 0;
	foreach (QTreeWidgetItem* g, groups) {
		rh = rowHeight(indexFromItem(g));
		rows += 1 + g->childCount();
	}
	setFixedHeight(header()->sizeHint().height() + rows * rh + 2 * frameWidth());
}

// THE LAYER SWITCHES

static QString layersNote;

QString evo_layers_note() {
	return layersNote;
}

// The machine's own lock keeps the emulation out while the mask changes. A
// running machine draws the next line with it; a stopped one is drawn again.
int evo_set_layers(int hide) {
	Computer* comp = conf.zx;
	if (!comp || (comp->hw->id != HW_TSLAB)) return 0;
	int res = 1;
	layersNote.clear();
	emu_lock();
	comp->vid->tsconf.hide = hide & 0x3f;
	if (conf.emu.pause) {
		res = xreplay_redraw(comp);
		xlog(XLG_GUI, XLL_INFO, "layers %.2X: %s", hide & 0x3f, xreplay_why());
		if (res) {
			frame_publish();
		} else {
			layersNote = QString("Shown from the next frame: %0.").arg(xreplay_why());
		}
	} else {
		tsl_recompose(comp->vid);
	}
	emu_unlock();
	return res;
}

// HELPERS

static QString hex2(int v) {return QString::number(v & 0xff, 16).toUpper().rightJustified(2, '0');}
static QString hex3(int v) {return QString::number(v & 0x1ff, 16).toUpper().rightJustified(3, '0');}

// a 22-bit byte address as the DMA registers set it: page and offset in it
static QString ts_adr(int a) {
	return QString("%0:%1").arg(hex2(a >> 14)).arg(QString::number(a & 0x3fff, 16).toUpper().rightJustified(4, '0'));
}

static const char* ts_dev_name(int ctrl) {
	switch (((ctrl & 0x80) >> 4) | (ctrl & 7)) {
		case 0x1: return "RAM > RAM";
		case 0x9: return "blit > RAM";
		case 0x6: return "add-blit > RAM";
		case 0x2: return "SPI > RAM";
		case 0xa: return "RAM > SPI";
		case 0x3: return "IDE > RAM";
		case 0xb: return "RAM > IDE";
		case 0x4: return "fill > RAM";
		case 0xc: return "RAM > CRAM";
		case 0xd: return "RAM > SFILE";
	}
	return "?";
}

// the Means column in plain words, or in the names the docs and tsconfig.asm give the fields
static bool fieldNames() {return conf.dbg.evonames;}

// DDEV with W/R as tsconfig.asm names it (DMA_ dropped); 6 is not in it
static QString ts_ddev_name(int ctrl) {
	switch (((ctrl & 0x80) >> 4) | (ctrl & 7)) {
		case 0x1: return "RAM";
		case 0x9: return "BLT";
		case 0x2: return "SPI_RAM";
		case 0xa: return "RAM_SPI";
		case 0x3: return "IDE_RAM";
		case 0xb: return "RAM_IDE";
		case 0x4: return "FILL";
		case 0xc: return "RAM_CRAM";
		case 0xd: return "RAM_SFILE";
	}
	return QString("%0:%1").arg((ctrl >> 7) & 1).arg(ctrl & 7);
}

static QString plural(int n, const char* one, const char* many) {
	return QString("%0 %1").arg(n).arg((n == 1) ? one : many);
}

static void dma_regs(xEvoList* l, const tsRegs& r) {
	const QString g("DMA");
	bool nm = fieldNames();
	l->put(g, "src", {"1A..1CAF", "DMASAddr", ts_adr(r.dmasrc), ""});
	l->put(g, "dst", {"1D..1FAF", "DMADAddr", ts_adr(r.dmadst), ""});
	l->put(g, "len", {"26AF", "DMALen", hex2(r.dmalen), plural(r.dmalen + 1, "word a burst", "words a burst")});
	l->put(g, "num", {"28AF", "DMANum", hex2(r.dmanum), plural(r.dmanum + 1, "burst", "bursts")});
	QString ctl = nm ? "-" : "nothing started yet";
	if ((r.dmactrl >= 0) && nm) {
		QStringList f("DDEV=" + ts_ddev_name(r.dmactrl));
		if (r.dmactrl & 0x20) f << "S_ALGN";
		if (r.dmactrl & 0x10) f << "D_ALGN";
		if (r.dmactrl & 0x08) f << "A_SZ";
		if (r.dmactrl & 0x40) f << "OPT";
		ctl = f.join(" ");
	} else if (r.dmactrl >= 0) {
		ctl = ts_dev_name(r.dmactrl);
		if (r.dmactrl & 0x20) ctl += QString(", src align %0").arg((r.dmactrl & 8) ? 512 : 256);
		if (r.dmactrl & 0x10) ctl += QString(", dst align %0").arg((r.dmactrl & 8) ? 512 : 256);
		if ((r.dmactrl & 0x40) && ((r.dmactrl & 0x87) == 0x06)) ctl += ", saturate";
	}
	l->put(g, "ctrl", {"27AF", "DMACtrl", (r.dmactrl < 0) ? "-" : hex2(r.dmactrl), ctl});
	l->put(g, "status", {"27AF", "DMAStatus", r.dmaact ? "80" : "00", r.dmaact ? (nm ? "DMA_ACT" : "running") : (nm ? "-" : "idle")});
	// where a running transfer is: the address registers are put back only at its end
	bool run = (r.dmaCurSrc >= 0);
	l->put(g, "nsrc", {"", "Source now", run ? ts_adr(r.dmaCurSrc) : "-", ""});
	l->put(g, "ndst", {"", "Dest now", run ? ts_adr(r.dmaCurDst) : "-", ""});
	l->put(g, "prog", {"", "Done", run ? QString("%0/%1").arg(r.dmaBurst).arg(r.dmaBursts) : "-",
		run ? QString("burst %0 of %1, word %2 of %3").arg(r.dmaBurst).arg(r.dmaBursts).arg(r.dmaWord).arg(r.dmaWords) : ""});
}

// REGISTERS PAGE

static const char* layNames[6] = {"S0", "T0", "S1", "T1", "S2", "Bitmap"};
static const char* layTips[6] = {
	"Sprites under tile layer 0", "Tile layer 0", "Sprites between the tile layers",
	"Tile layer 1", "Sprites over both tile layers", "The bitmap screen"
};

xTsRegsPage::xTsRegsPage(QWidget* p):QWidget(p) {
	hold = false;
	QVBoxLayout* lay = new QVBoxLayout(this);
	lay->setContentsMargins(0, 0, 0, 0);
	QHBoxLayout* row = new QHBoxLayout;
	row->addWidget(new QLabel("Show"));
	for (int i = 0; i < 6; i++) {
		this->lay[i] = new QCheckBox(layNames[i]);
		this->lay[i]->setToolTip(layTips[i]);
		this->lay[i]->setChecked(true);
		row->addWidget(this->lay[i]);
		connect(this->lay[i], &QCheckBox::toggled, this, &xTsRegsPage::layer_toggled);
	}
	row->addStretch(1);
	lay->addLayout(row);
	labNote = new QLabel;
	labNote->setWordWrap(true);
	labNote->hide();
	lay->addWidget(labNote);
	list = new xEvoList({"Port", "Register", "Value", "Means"});
	list->onStyle = [this]() {draw();};
	lay->addWidget(list, 1);
}

void xTsRegsPage::layer_toggled() {
	if (hold) return;
	int hide = 0;
	for (int i = 0; i < 6; i++)
		if (!lay[i]->isChecked()) hide |= 1 << i;
	evo_set_layers(hide);
	emit s_layers();
}

static QString vconf_means(int v) {
	static const char* modes[4] = {"ZX", "16c", "256c", "text"};
	static const char* res[4] = {"256x192", "320x200", "320x240", "360x288"};
	if (fieldNames()) {
		QStringList f;
		f << QString("VM=%0").arg(modes[v & 3]) << QString("RRES=%0").arg(res[(v >> 6) & 3]);
		if (v & 0x20) f << "NOGFX";
		if (v & 0x10) f << "NOTSU";
		if (v & 0x08) f << "GFXOVR";
		if (v & 0x04) f << "FT_EN";
		return f.join(" ");
	}
	QString s = QString("%0 %1").arg(modes[v & 3]).arg(res[(v >> 6) & 3]);
	if (v & 0x20) s += ", no bitmap";
	if (v & 0x10) s += ", no tiles or sprites";
	if (v & 0x08) s += ", bitmap over them";
	if (v & 0x04) s += ", VDAC2";
	return s;
}

static QString tsconf_means(int v) {
	if (fieldNames()) {
		QStringList f;
		if (v & 0x80) f << "S_EN";
		if (v & 0x40) f << "T1_EN";
		if (v & 0x20) f << "T0_EN";
		if (v & 0x08) f << "T1Z_EN";
		if (v & 0x04) f << "T0Z_EN";
		if (v & 0x01) f << "TS_EXT";
		return f.isEmpty() ? "-" : f.join(" ");
	}
	QStringList on;
	if (v & 0x80) on << "sprites";
	if (v & 0x20) on << ((v & 0x04) ? "tiles 0 (with tile 0)" : "tiles 0");
	if (v & 0x40) on << ((v & 0x08) ? "tiles 1 (with tile 0)" : "tiles 1");
	QString s = on.isEmpty() ? "all off" : on.join(", ");
	if (v & 0x01) s += "; over 360x288";
	return s;
}

static QString memconf_means(int v) {
	static const char* lck[4] = {"7FFD 512K", "7FFD 128K", "7FFD auto", "7FFD 1024K"};
	static const char* lckf[4] = {"512", "128", "AUTO", "1024"};
	QStringList s;
	if (fieldNames()) {
		s << QString("LCK=%0").arg(lckf[(v >> 6) & 3]);
		if (v & 8) s << "W0_RAM";
		if (v & 4) s << "!W0_MAP";
		if (v & 2) s << "W0_WE";
		if (v & 1) s << "ROM128";
		return s.join(" ");
	}
	s << ((v & 8) ? "window 0 RAM" : "window 0 ROM");
	if (v & 2) s << "writable";
	s << ((v & 4) ? "Page0 as is" : ((v & 1) ? "mapped, ROM 48" : "mapped, ROM 128"));
	s << lck[(v >> 6) & 3];
	return s.join(", ");
}

static QString pending(int now, int next, bool on, QString (*fmt)(int)) {
	QString s = fmt(now);
	if (on && (next != now)) s += " > " + fmt(next);
	return s;
}

void xTsRegsPage::draw() {
	Computer* comp = conf.zx;
	if (!comp || (comp->hw->id != HW_TSLAB)) return;
	tsRegs r;
	ts_get_regs(comp, &r);
	// the switches follow the machine: another panel or a reset may have moved them
	hold = true;
	for (int i = 0; i < 6; i++)
		lay[i]->setChecked(!(comp->vid->tsconf.hide & (1 << i)));
	hold = false;
	QString note = evo_layers_note();
	labNote->setText(note);
	labNote->setVisible(!note.isEmpty());

	QString g = "Video";
	list->put(g, "vconf", {"00AF", "VConfig", pending(r.vconf, r.latVconf, r.lat & TSL_LAT_VCONF, hex2),
		vconf_means((r.lat & TSL_LAT_VCONF) ? r.latVconf : r.vconf)});
	list->put(g, "vpage", {"01AF", "VPage", pending(r.vpage, r.latVpage, r.lat & TSL_LAT_VPAGE, hex2), ""});
	list->put(g, "gx", {"02/03AF", "GXOffs", pending(r.gxoffs, r.latGx, r.lat & (TSL_LAT_GXL | TSL_LAT_GXH), hex3),
		QString::number((r.lat & (TSL_LAT_GXL | TSL_LAT_GXH)) ? r.latGx : r.gxoffs)});
	list->put(g, "gy", {"04/05AF", "GYOffs", pending(r.gyoffs, r.latGy, r.lat & (TSL_LAT_GYL | TSL_LAT_GYH), hex3),
		QString::number((r.lat & (TSL_LAT_GYL | TSL_LAT_GYH)) ? r.latGy : r.gyoffs)});
	bool nm = fieldNames();
	int pal = (r.lat & TSL_LAT_PAL) ? r.latPal : r.palsel;
	list->put(g, "pal", {"07AF", "PalSel", pending(r.palsel, r.latPal, r.lat & TSL_LAT_PAL, hex2),
		QString(nm ? "GPAL=%0 T0PAL=%1 T1PAL=%2" : "bitmap %0, tiles 0 %1, tiles 1 %2").arg(pal & 15).arg((pal >> 4) & 3).arg((pal >> 6) & 3)});
	list->put(g, "brd", {"0FAF", "Border", hex2(r.border), ""});

	g = "Tiles and sprites";
	list->put(g, "tsconf", {"06AF", "TSConfig", hex2(r.tsconf), tsconf_means(r.tsconf)});
	list->put(g, "tmp", {"16AF", "TMPage", hex2(r.tmpage), "tile maps"});
	list->put(g, "t0g", {"17AF", "T0GPage", pending(r.t0gpage, r.latT0g, r.lat & TSL_LAT_T0G, hex2), "tiles 0 graphics"});
	list->put(g, "t1g", {"18AF", "T1GPage", pending(r.t1gpage, r.latT1g, r.lat & TSL_LAT_T1G, hex2), "tiles 1 graphics"});
	list->put(g, "sg", {"19AF", "SGPage", hex2(r.sgpage), "sprite graphics"});
	list->put(g, "t0x", {"40/41AF", "T0XOffs", pending(r.t0xoffs, r.latT0x, r.lat & (TSL_LAT_T0XL | TSL_LAT_T0XH), hex3),
		QString::number((r.lat & (TSL_LAT_T0XL | TSL_LAT_T0XH)) ? r.latT0x : r.t0xoffs)});
	list->put(g, "t0y", {"42/43AF", "T0YOffs", hex3(r.t0yoffs), QString::number(r.t0yoffs)});
	list->put(g, "t1x", {"44/45AF", "T1XOffs", pending(r.t1xoffs, r.latT1x, r.lat & (TSL_LAT_T1XL | TSL_LAT_T1XH), hex3),
		QString::number((r.lat & (TSL_LAT_T1XL | TSL_LAT_T1XH)) ? r.latT1x : r.t1xoffs)});
	list->put(g, "t1y", {"46/47AF", "T1YOffs", hex3(r.t1yoffs), QString::number(r.t1yoffs)});

	g = "Memory";
	for (int i = 0; i < 4; i++) {
		QString means;
		if (i == 0) {
			MemPage* pg = &comp->mem->map[0];
			means = QString("%0 page %1 in window 0").arg((pg->type == MEM_RAM) ? "RAM" : "ROM").arg(hex2(pg->num >> 6));
		}
		list->put(g, QString("page%0").arg(i), {QString("%0AF").arg(0x10 + i, 2, 16, QChar('0')).toUpper(),
			QString("Page%0").arg(i), hex2(r.page[i]), means});
	}
	list->put(g, "memconf", {"21AF", "MemConfig", hex2(r.memconf), memconf_means(r.memconf)});
	QString fmat = QString("#%0000").arg(QString::number(r.fmaddr, 16).toUpper());
	list->put(g, "fm", {"15AF", "FMAddr", hex2(r.fmaddr | (r.fmen ? 0x10 : 0)), nm ? QString("FM_EN=%0 FM_ADDR=%1").arg(r.fmen).arg(fmat)
		: (r.fmen ? QString("CRAM and SFILE at %0").arg(fmat) : "off")});
	QStringList cw;
	for (int i = 0; i < 4; i++)
		if (r.cacheconf & (1 << i)) cw << QString(nm ? "EN_%0" : "#%0").arg(QString::number(i << 14, 16).toUpper().rightJustified(4, '0'));
	list->put(g, "cache", {"2BAF", "CacheConfig", hex2(r.cacheconf), cw.isEmpty() ? (nm ? "-" : "off") : cw.join(" ")});
	QStringList fd;
	for (int i = 0; i < 4; i++)
		if (r.fddvirt & (1 << i)) fd << QString(nm ? "VIRT_%0" : "%0").arg(QChar('A' + i));
	QString fds;
	if (nm) {
		if (r.fddvirt & 0x80) fd << "OPEN_VG";
		fds = fd.isEmpty() ? "-" : fd.join(" ");
	} else {
		fds = fd.isEmpty() ? "none virtual" : fd.join(" ") + " virtual";
		if (r.fddvirt & 0x80) fds += ", ports open outside DOS";
	}
	list->put(g, "fdd", {"29AF", "FDDVirt", hex2(r.fddvirt), fds});

	g = "System";
	static const char* clk[3] = {"3.5 MHz", "7 MHz", "14 MHz"};
	static const char* clkf[3] = {"3.5", "7", "14"};
	list->put(g, "sys", {"20AF", "SysConfig", "-", nm ? QString("ZCLK=%0").arg(clkf[r.clock])
		: QString("%0, cache %1").arg(clk[r.clock]).arg(r.cacheconf ? "on" : "off")});
	QStringList ints;
	if (r.intmask & 1) ints << (nm ? "FRAME" : "frame");
	if (r.intmask & 2) ints << (nm ? "LINE" : "line");
	if (r.intmask & 4) ints << "DMA";
	list->put(g, "intmask", {"2AAF", "INTMask", hex2(r.intmask), ints.isEmpty() ? (nm ? "-" : "none") : ints.join(nm ? " " : ", ")});
	list->put(g, "hsint", {"22AF", "HSINT", hex2(r.hsint), QString("dot %0 of the line").arg(r.hsint * 2)});
	list->put(g, "vsint", {"23/24AF", "VSINT", hex3(r.vsint), r.vsinc ? QString("line %0, then %1 lines on").arg(r.vsint).arg(r.vsinc)
		: QString("line %0").arg(r.vsint)});

	dma_regs(list, r);
	list->finish();
	list->restyle();
}

// SPRITES PAGE

#define SPR_PIC		192	// the picked sprite is drawn this big at most

xTsSprPage::xTsSprPage(QWidget* p):QWidget(p) {
	sel = -1;
	QVBoxLayout* lay = new QVBoxLayout(this);
	lay->setContentsMargins(0, 0, 0, 0);
	cbActive = new QCheckBox("Active only");
	cbActive->setChecked(true);
	lay->addWidget(cbActive);
	list = new QTreeWidget;
	list->setRootIsDecorated(false);
	list->setUniformRowHeights(true);
	list->setAlternatingRowColors(true);
	list->setSelectionMode(QAbstractItemView::SingleSelection);
	list->setHeaderLabels({"#", "Layer", "X", "Y", "Size", "Tile", "Pal", "Flags"});
	list->header()->setStretchLastSection(true);
	list->header()->setSectionsMovable(false);
	// the list over the picked sprite, the line between them movable
	QSplitter* split = new QSplitter(Qt::Vertical);
	split->setChildrenCollapsible(false);
	split->addWidget(list);
	QWidget* bot = new QWidget;
	QHBoxLayout* blay = new QHBoxLayout(bot);
	blay->setContentsMargins(0, 0, 0, 0);
	labPic = new QLabel;
	labPic->setMinimumSize(64, 64);
	labPic->setMaximumWidth(SPR_PIC);
	labPic->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	labPic->setAlignment(Qt::AlignCenter);
	labInfo = new QLabel;
	labInfo->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	labInfo->setWordWrap(true);
	blay->addWidget(labPic);
	blay->addWidget(labInfo, 1);
	split->addWidget(bot);
	split->setStretchFactor(0, 2);
	split->setStretchFactor(1, 1);
	lay->addWidget(split, 1);
	connect(cbActive, &QCheckBox::toggled, this, [this]() {draw();});
	connect(list, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* it) {
		sel = it ? it->data(0, Qt::UserRole).toInt() : -1;
		draw_pick();
	});
}

static const char* sprLayer[4] = {"S0", "S1", "S2", "-"};

void xTsSprPage::draw() {
	Computer* comp = conf.zx;
	if (!comp || (comp->hw->id != HW_TSLAB)) return;
	Video* vid = comp->vid;
	if (list->topLevelItemCount() == 0) {
		for (int i = 0; i < TSL_SPRITES; i++) {
			QTreeWidgetItem* it = new QTreeWidgetItem(list);
			it->setData(0, Qt::UserRole, i);
			for (int c = 0; c < 7; c++)
				it->setTextAlignment(c, Qt::AlignRight | Qt::AlignVCenter);
		}
	}
	// S0 runs from the first entry to the first LEAP, S1 on to the next, S2 to the
	// third: what comes after that is never drawn
	int layer = 0;
	QColor dim = palette().color(QPalette::Disabled, QPalette::Text);
	for (int i = 0; i < TSL_SPRITES; i++) {
		tsSprite s;
		tsl_get_sprite(vid, i, &s);
		QTreeWidgetItem* it = list->topLevelItem(i);
		QStringList fl;
		if (s.act) fl << "ACT";
		if (s.xf) fl << "XF";
		if (s.yf) fl << "YF";
		if (s.leap) fl << "LEAP";
		QStringList cols = {QString::number(i), sprLayer[layer], QString::number(s.x), QString::number(s.y),
			QString("%0x%1").arg(s.w).arg(s.h), QString::number(s.tile), QString::number(s.pal), fl.join(" ")};
		for (int c = 0; c < cols.size(); c++) {
			it->setText(c, cols[c]);
			if (s.act && (layer < 3)) it->setData(c, Qt::ForegroundRole, QVariant());
			else it->setForeground(c, dim);
		}
		it->setHidden(cbActive->isChecked() && !s.act);
		if (s.leap && (layer < 3)) layer++;
	}
	static const QStringList widest[7] = {{"84"}, {"Layer", "S0"}, {"511"}, {"511"}, {"64x64"}, {"4095"}, {"Pal", "15"}};
	for (int i = 0; i < 7; i++)
		evo_fit_column(list, i, widest[i]);
	draw_pick();
}

// the picked sprite as the TSU draws it, through the colors as they are now;
// a transparent dot shows as a checker
void xTsSprPage::draw_pick() {
	Computer* comp = conf.zx;
	if (!comp || (comp->hw->id != HW_TSLAB) || (sel < 0)) {
		labPic->clear();
		labInfo->setText(sel < 0 ? "Pick a sprite to see it" : "");
		return;
	}
	Video* vid = comp->vid;
	tsSprite s;
	tsl_get_sprite(vid, sel, &s);
	static unsigned char dots[64 * 64];
	tsl_sprite_dots(vid, sel, dots);
	QImage img(s.w, s.h, QImage::Format_RGB32);
	for (int y = 0; y < s.h; y++) {
		for (int x = 0; x < s.w; x++) {
			int d = dots[y * s.w + x];
			if (d) {
				uint32_t c = vid->pal[d];
				img.setPixel(x, y, qRgb(c & 0xff, (c >> 8) & 0xff, (c >> 16) & 0xff));
			} else {
				img.setPixel(x, y, ((x ^ y) & 4) ? qRgb(0x60, 0x60, 0x60) : qRgb(0x40, 0x40, 0x40));
			}
		}
	}
	int zoom = qMax(1, qMin(qMin(labPic->width(), SPR_PIC) / s.w, qMin(labPic->height(), SPR_PIC) / s.h));
	labPic->setPixmap(QPixmap::fromImage(img.scaled(s.w * zoom, s.h * zoom)));
	int sg = vid->tsconf.SGPage;
	labInfo->setText(QString("Sprite %0, %1x%2 at %3,%4\nTile %5: row %6, column %7 of page %8\nPalette %9 (colors %10..%11)")
		.arg(sel).arg(s.w).arg(s.h).arg(s.x).arg(s.y)
		.arg(s.tile).arg(s.tile >> 6).arg(s.tile & 63).arg(hex2(sg))
		.arg(s.pal).arg(hex2(s.pal << 4)).arg(hex2((s.pal << 4) | 15)));
}

// a TSConf color as the picture shows it; 0 (a hole) as a checker
static QRgb ts_rgb(Video* vid, int idx, int x, int y) {
	if (!idx) return ((x ^ y) & 4) ? qRgb(0x60, 0x60, 0x60) : qRgb(0x40, 0x40, 0x40);
	uint32_t c = vid->pal[idx & 0xff];
	return qRgb(c & 0xff, (c >> 8) & 0xff, (c >> 16) & 0xff);
}

// MAP VIEW

xTsMapView::xTsMapView(QWidget* p):QWidget(p) {
	setMouseTracking(true);
	setMinimumSize(128, 128);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

// the image as big as fits, square, in whole steps when it is bigger than 1:1
QRect xTsMapView::target() const {
	if (img.isNull()) return QRect();
	int side = qMin(width(), height());
	if (side >= img.width()) side = (side / img.width()) * img.width();
	return QRect((width() - side) / 2, (height() - side) / 2, side, side);
}

void xTsMapView::paintEvent(QPaintEvent*) {
	QPainter pnt(this);
	QRect t = target();
	if (t.isEmpty()) return;
	pnt.drawImage(t, img);
	double k = (double)t.width() / img.width();
	QColor col = conf.pal.value("dbg.header.bg");
	if (!col.isValid()) col = Qt::yellow;
	pnt.setPen(QPen(col, 2));
	pnt.setBrush(Qt::NoBrush);
	foreach (QRect r, marks)
		pnt.drawRect(QRectF(t.x() + r.x() * k, t.y() + r.y() * k, r.width() * k, r.height() * k));
}

void xTsMapView::mouseMoveEvent(QMouseEvent* ev) {
	QRect t = target();
	if (t.isEmpty() || !t.contains(ev->pos())) {
		if (onHover) onHover(-1, -1);
		return;
	}
	double k = (double)img.width() / t.width();
	if (onHover) onHover((int)((ev->pos().x() - t.x()) * k), (int)((ev->pos().y() - t.y()) * k));
}

void xTsMapView::leaveEvent(QEvent*) {
	if (onHover) onHover(-1, -1);
}

// TILES PAGE

enum {TV_MAP0 = 0, TV_MAP1, TV_GFX};

xTsTilePage::xTsTilePage(QWidget* p):QWidget(p) {
	hx = hy = -1;
	QVBoxLayout* lay = new QVBoxLayout(this);
	lay->setContentsMargins(0, 0, 0, 0);
	QHBoxLayout* row = new QHBoxLayout;
	cbView = new QComboBox;
	cbView->addItem("Map, layer 0", TV_MAP0);
	cbView->addItem("Map, layer 1", TV_MAP1);
	cbView->addItem("Graphics", TV_GFX);
	cbView->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	row->addWidget(cbView);
	row->addWidget(new QLabel("Page"));
	sbPage = new QSpinBox;
	sbPage->setDisplayIntegerBase(16);
	sbPage->setRange(0, 255);
	sbPage->setSingleStep(8);		// a graphics page is 8 pages long: 512x512 dots
	row->addWidget(sbPage);
	auto preset = [this, row](const char* name, const char* tip) {
		QToolButton* tb = new QToolButton;
		tb->setText(name);
		tb->setToolTip(tip);
		row->addWidget(tb);
		return tb;
	};
	tbT0G = preset("T0G", "The page T0GPage points at");
	tbT1G = preset("T1G", "The page T1GPage points at");
	tbSG = preset("SG", "The page SGPage points at");
	row->addWidget(new QLabel("Palette"));
	sbPal = new QSpinBox;
	sbPal->setRange(0, 15);
	row->addWidget(sbPal);
	row->addStretch(1);
	lay->addLayout(row);
	view = new xTsMapView;
	lay->addWidget(view, 1);
	labInfo = new QLabel;
	labInfo->setWordWrap(true);
	lay->addWidget(labInfo);
	view->onHover = [this](int x, int y) {
		hx = x;
		hy = y;
		hover_info();
	};
	auto redraw = [this]() {draw();};
	connect(cbView, QOverload<int>::of(&QComboBox::currentIndexChanged), this, redraw);
	connect(sbPage, QOverload<int>::of(&QSpinBox::valueChanged), this, redraw);
	connect(sbPal, QOverload<int>::of(&QSpinBox::valueChanged), this, redraw);
	auto pick = [this](int which) {
		if (!conf.zx) return;
		Video* vid = conf.zx->vid;
		sbPage->setValue(which == 0 ? vid->tsconf.T0GPage : (which == 1) ? vid->tsconf.T1GPage : vid->tsconf.SGPage);
		cbView->setCurrentIndex(TV_GFX);
	};
	connect(tbT0G, &QToolButton::clicked, this, [pick]() {pick(0);});
	connect(tbT1G, &QToolButton::clicked, this, [pick]() {pick(1);});
	connect(tbSG, &QToolButton::clicked, this, [pick]() {pick(2);});
}

void xTsTilePage::draw() {
	Computer* comp = conf.zx;
	if (!comp || (comp->hw->id != HW_TSLAB)) return;
	Video* vid = comp->vid;
	int mode = cbView->currentData().toInt();
	bool gfx = (mode == TV_GFX);
	sbPage->setEnabled(gfx);
	sbPal->setEnabled(gfx);
	QImage img(512, 512, QImage::Format_RGB32);
	unsigned char dots[64];
	view->marks.clear();
	for (int ty = 0; ty < 64; ty++) {
		for (int tx = 0; tx < 64; tx++) {
			if (gfx) {
				tsl_tile_dots(vid, sbPage->value(), (ty << 6) | tx, sbPal->value() << 4, dots);
			} else {
				tsl_map_dots(vid, mode, tsl_map_entry(vid, mode, ty, tx), dots);
			}
			for (int y = 0; y < 8; y++)
				for (int x = 0; x < 8; x++)
					img.setPixel((tx << 3) + x, (ty << 3) + y, ts_rgb(vid, dots[(y << 3) + x], (tx << 3) + x, (ty << 3) + y));
		}
	}
	if (!gfx) {
		// the window the offsets show, in up to four pieces where it wraps round
		int ox = (mode ? vid->tsconf.T1XOffset : vid->tsconf.T0XOffset) & 0x1ff;
		int oy = (mode ? vid->tsconf.T1YOffset : vid->tsconf.T0YOffset) & 0x1ff;
		int w = vid->tsconf.tsSize.x;
		int h = vid->tsconf.tsSize.y;
		for (int sy = 0; sy < 2; sy++) {
			for (int sx = 0; sx < 2; sx++) {
				QRect r = QRect(ox - sx * 512, oy - sy * 512, w, h).intersected(QRect(0, 0, 512, 512));
				if (!r.isEmpty()) view->marks << r;
			}
		}
	}
	view->img = img;
	view->update();
	hover_info();
}

void xTsTilePage::hover_info() {
	Computer* comp = conf.zx;
	if (!comp || (comp->hw->id != HW_TSLAB)) return;
	Video* vid = comp->vid;
	int mode = cbView->currentData().toInt();
	if ((hx < 0) || (hy < 0)) {
		labInfo->setText((mode == TV_GFX) ? "64x64 tiles of 8x8 dots" : QString("TMPage %0; the frame shows the outlined part").arg(hex2(vid->tsconf.TMPage)));
		return;
	}
	int tx = hx >> 3;
	int ty = hy >> 3;
	if (mode == TV_GFX) {
		int tile = (ty << 6) | tx;
		int ofs = ((tile & 0xfc0) << 5) | ((tile & 0x3f) << 2);
		labInfo->setText(QString("Tile %0 (row %1, column %2): page %3, lines at %4 + n * 100")
			.arg(tile).arg(ty).arg(tx).arg(hex2(sbPage->value() + (ofs >> 14))).arg(QString::number(ofs & 0x3fff, 16).toUpper().rightJustified(4, '0')));
		return;
	}
	int e = tsl_map_entry(vid, mode, ty, tx);
	int ofs = (ty << 8) | (mode << 7) | (tx << 1);
	int palhi = ((e >> 8) & 0x30) | (mode ? vid->tsconf.T1Pal76 : vid->tsconf.T0Pal76);
	QStringList fl;
	if (e & 0x4000) fl << "XF";
	if (e & 0x8000) fl << "YF";
	bool hole = !(e & 0xfff) && !(vid->tsconf.tconfig & (mode ? 8 : 4));
	labInfo->setText(QString("Cell %0,%1 at %2:%3 = %4: tile %5 (row %6, column %7), colors %8..%9%10%11")
		.arg(tx).arg(ty).arg(hex2(vid->tsconf.TMPage)).arg(QString::number(ofs, 16).toUpper().rightJustified(4, '0'))
		.arg(QString::number(e, 16).toUpper().rightJustified(4, '0')).arg(e & 0xfff).arg((e & 0xfff) >> 6).arg(e & 63)
		.arg(hex2(palhi)).arg(hex2(palhi | 15)).arg(fl.isEmpty() ? "" : " " + fl.join(" ")).arg(hole ? ", a hole (tile 0)" : ""));
}

// BASECONF PORTS PAGE

xEvoPortsPage::xEvoPortsPage(QWidget* p):QWidget(p) {
	QVBoxLayout* lay = new QVBoxLayout(this);
	lay->setContentsMargins(0, 0, 0, 0);
	list = new xEvoList({"Port", "Register", "Value", "Means"});
	list->onStyle = [this]() {draw();};
	lay->addWidget(list, 1);
}

// the set bits of v by name, b0 first; nullptr for a bit with no name
static QStringList bit_names(int v, const char* const* names) {
	QStringList s;
	for (int i = 0; i < 8; i++)
		if ((v & (1 << i)) && names[i]) s << names[i];
	return s;
}

static QString hex4(int v) {return QString::number(v & 0xffff, 16).toUpper().rightJustified(4, '0');}

void xEvoPortsPage::draw() {
	Computer* comp = conf.zx;
	if (!comp || (comp->hw->id != HW_PENTEVO)) return;
	evoRegs r;
	evo_get_regs(comp, &r);
	bool nm = fieldNames();

	QString g = "Ports";
	static const char* const p7N[8] = {0, 0, 0, "SCR", "ROM", "LOCK", 0, 0};
	list->put(g, "7ffd", {"7FFD", "7FFD", hex2(r.p7ffd), nm
		? QString("PG=%0 %1").arg(r.p7ffd & 7).arg(bit_names(r.p7ffd, p7N).join(" "))
		: QString("RAM page %0 at #C000, screen %1, ROM %2%3").arg(r.p7ffd & 7).arg((r.p7ffd & 8) ? 7 : 5)
			.arg((r.p7ffd >> 4) & 1).arg((r.p7ffd & 0x20) ? ", locked" : "")});
	static const char* const effN[8] = {"P16C", 0, "BLOCK1M", "RAM0_0", "TURBOOFF", "HWMC", 0, "GLUCLOCK"};
	static const char* const effW[8] = {"16 colors", 0, "7FFD as 128K", "RAM 0 at #0000", "turbo off", "multicolor", 0, "clock chip ports"};
	QStringList ew = bit_names(r.peff7, nm ? effN : effW);
	list->put(g, "eff7", {"EFF7", "EFF7", hex2(r.peff7), ew.isEmpty() ? "-" : ew.join(nm ? " " : ", ")});
	static const char* atmModes[8] = {"EGA", "?", "hardware multicolor", "ZX", "?", "?", "text", "Evo text"};
	bool pager = r.p77 & 0x20;
	list->put(g, "77", {"xx77", "77", hex2(r.p77), nm
		? QString("RG=%0%1 PEN=%2 CPM_N=%3 PEN2=%4").arg(r.p77 & 7).arg((r.p77 & 8) ? " TURBO" : "")
			.arg(pager ? 0 : 1).arg((r.p77 >> 6) & 1).arg((r.p77 & 0x80) ? 0 : 1)
		: QString("mode %0 (%1)%2, pager %3%4, palette %5").arg(r.p77 & 7).arg(atmModes[r.p77 & 7])
			.arg((r.p77 & 8) ? ", turbo" : "").arg(pager ? "on" : "off").arg((r.p77 & 0x40) ? "" : ", DOS held on")
			.arg((r.p77 & 0x80) ? "closed" : "open")});
	static const char* const bfN[8] = {"SHADOW_EN", "ROMRW_EN", "FNTW_EN", "SET_NMI", "BRK_ENA", "PAL444_ENA", 0, 0};
	static const char* const bfW[8] = {"shadow ports", "ROM writable", "font writable", "NMI", "trap on", "4-4-4 palette", 0, 0};
	QStringList bw = bit_names(r.pbf, nm ? bfN : bfW);
	list->put(g, "bf", {"xxBF", "BF", hex2(r.pbf), bw.isEmpty() ? "-" : bw.join(nm ? " " : ", ")});

	g = "Windows";
	for (int i = 0; i < 4; i++) {
		MemPage* pg = &comp->mem->map[i << 6];
		list->put(g, QString("w%0").arg(i), {"", QString("#%0").arg(hex4(i << 14)), hex2(pg->num >> 6),
			QString("%0 page %1").arg((pg->type == MEM_RAM) ? "RAM" : "ROM").arg(hex2(pg->num >> 6))});
	}

	g = "Pager";
	for (int i = 0; i < 8; i++) {
		int f = r.pgflag[i];
		QStringList m;
		if (nm) {
			m << ((f & 0x40) ? "RAM" : "ROM");
			if (f & 0x80) m << "MIX";
			if (f & 0x20) m << "WP";
		} else {
			m << ((f & 0x40) ? "RAM" : "ROM");
			if (f & 0x80) m << ((f & 0x40) ? "7FFD bits mixed in" : "DOS bit mixed in");
			if (f & 0x20) m << "write protected";
		}
		bool live = ((i & 4) == r.pgset);
		list->put(g, QString("p%0").arg(i), {QString("%0FF7").arg((i & 3) << 2, 1, 16).toUpper(),
			QString("W%0 ROM=%1%2").arg(i & 3).arg(i >> 2).arg(live ? " *" : ""), hex2(r.pg[i]), m.join(nm ? " " : ", ")});
	}

	g = "System";
	QStringList st;
	if (r.dos) st << "DOS";
	if (r.vdos) st << (nm ? "VDOS" : "TR-DOS emulation");
	if (r.vnmi) st << (nm ? "VNMI" : "in NMI");
	static const char* clk[3] = {"3.5 MHz", "7 MHz", "14 MHz"};
	list->put(g, "state", {"", "State", "", st.isEmpty() ? "-" : st.join(", ")});
	list->put(g, "clock", {"", "Clock", "", clk[r.clock]});
	list->put(g, "trap", {"00/01BD", "Trap", hex4(r.trap), (r.pbf & 0x10) ? "on" : "off"});
	list->put(g, "brd", {"", "Border", QString::number(r.border, 16).toUpper(), "palette cell"});
	QStringList vd;
	for (int i = 0; i < 4; i++)
		if (r.virt & (1 << i)) vd << QString(QChar('A' + i));
	list->put(g, "virt", {"13BD", "Virtual", QString::number(r.virt, 16).toUpper(), vd.isEmpty() ? "-" : vd.join(" ")});
	list->put(g, "ext", {"2F..8F", "2F 4F 6F 8F", "", QString("%0 %1 %2 %3").arg(hex2(r.ext[0])).arg(hex2(r.ext[1]))
		.arg(hex2(r.ext[2])).arg(hex2(r.ext[3]))});

	g = "Palette";
	for (int h = 0; h < 2; h++) {
		QStringList cells;
		for (int i = 0; i < 8; i++) cells << hex2(r.pal[h * 8 + i]);
		list->put(g, QString("pal%0").arg(h), {"FF", QString("Cells %0..%1").arg(h * 8).arg(h * 8 + 7), "", cells.join(" ")});
	}
	list->finish();
	list->restyle();
}

// DMA PAGE

xTsDmaPage::xTsDmaPage(QWidget* p):QWidget(p) {
	logCount = ~0u;
	logFrame = -1;
	QVBoxLayout* lay = new QVBoxLayout(this);
	lay->setContentsMargins(0, 0, 0, 0);
	regs = new xEvoList({"Port", "Register", "Value", "Means"});
	regs->onStyle = [this]() {draw();};
	lay->addWidget(regs, 0);
	lay->addWidget(new QLabel("Started this frame and the last"));
	log = new QTreeWidget;
	log->setRootIsDecorated(false);
	log->setUniformRowHeights(true);
	log->setAlternatingRowColors(true);
	log->setSelectionMode(QAbstractItemView::NoSelection);
	log->setFocusPolicy(Qt::NoFocus);
	log->setHeaderLabels({"Frame", "Line:dot", "Transfer", "From", "To", "Size", "Took"});
	log->header()->setStretchLastSection(true);
	lay->addWidget(log, 1);
}

void xTsDmaPage::draw() {
	Computer* comp = conf.zx;
	if (!comp || (comp->hw->id != HW_TSLAB)) return;
	tsRegs r;
	ts_get_regs(comp, &r);
	dma_regs(regs, r);
	regs->finish();
	regs->restyle();
	regs->fitHeight();
	// the list only when it can have changed: a transfer started or ended, or a frame went
	unsigned cnt = comp->tsconf.dmalog.count;
	tsDmaRec* last = cnt ? &comp->tsconf.dmalog.rec[(cnt - 1) % TS_DMALOG] : nullptr;
	if ((cnt == logCount) && (comp->vid->fcnt == logFrame) && !(last && (last->dur < 0))) return;
	logCount = cnt;
	logFrame = comp->vid->fcnt;
	log->clear();
	// T to lines: the line is 448 dots whatever the cpu clock is
	double tPerLine = comp->vid->full.x * (double)comp->vid->nsPerDotFixed / (double)comp->nsPerTickFixed;
	unsigned first = (cnt > TS_DMALOG) ? cnt - TS_DMALOG : 0;
	for (unsigned k = first; k < cnt; k++) {
		tsDmaRec* d = &comp->tsconf.dmalog.rec[k % TS_DMALOG];
		int age = comp->vid->fcnt - d->fcnt;
		if (age > 1) continue;
		QString took = (d->dur < 0) ? "running" : QString("%0 T, %1 lines%2").arg(d->dur)
			.arg(d->dur / tPerLine, 0, 'f', 1).arg(d->cut ? ", cut short" : "");
		QTreeWidgetItem* it = new QTreeWidgetItem(log, {age ? "last" : "this", QString("%0:%1").arg(d->line).arg(d->dot),
			ts_dev_name(d->ctrl), ts_adr(d->src), ts_adr(d->dst), QString("%0 x %1").arg(d->len).arg(d->num), took});
		it->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
	}
	static const QStringList widest[6] = {{"Frame", "last"}, {"Line:dot", "319:447"}, {"add-blit > RAM"},
		{"FF:3FFE"}, {"FF:3FFE"}, {"256 x 256"}};
	for (int i = 0; i < 6; i++)
		evo_fit_column(log, i, widest[i]);
	log->scrollToBottom();
}

// PANEL

xEvoPanel::xEvoPanel(QWidget* p):QWidget(p) {
	hw = -1;
	hold = false;
	QVBoxLayout* lay = new QVBoxLayout(this);
	lay->setContentsMargins(0, 0, 0, 0);
	QHBoxLayout* head = new QHBoxLayout;
	head->setContentsMargins(0, 0, 0, 0);
	tabs = new QTabBar;
	tabs->setDrawBase(false);
	labHidden = new QLabel("Layers hidden");
	labHidden->setToolTip("Some of the layers are kept out of the picture (Registers tab)");
	labHidden->hide();
	tbDetach = new QToolButton;
	tbDetach->setIcon(QIcon(":/images/display.png"));
	tbDetach->setToolTip("Show in its own window");
	tbDetach->setCheckable(true);
	head->addWidget(tabs);
	head->addStretch(1);
	head->addWidget(labHidden);
	head->addWidget(tbDetach);
	lay->addLayout(head);

	stack = new QStackedWidget;
	tsRegs = new xTsRegsPage;
	tsSpr = new xTsSprPage;
	tsTile = new xTsTilePage;
	tsDma = new xTsDmaPage;
	evoPorts = new xEvoPortsPage;
	noMachine = new QLabel("Nothing to show on this machine");
	noMachine->setAlignment(Qt::AlignCenter);
	stack->addWidget(tsRegs);
	stack->addWidget(tsSpr);
	stack->addWidget(tsTile);
	stack->addWidget(tsDma);
	stack->addWidget(evoPorts);
	stack->addWidget(noMachine);
	lay->addWidget(stack, 1);

	connect(tabs, &QTabBar::currentChanged, this, &xEvoPanel::tab_changed);
	connect(tbDetach, &QToolButton::clicked, this, &xEvoPanel::s_detach);
	connect(tsRegs, &xTsRegsPage::s_layers, this, [this]() {
		draw();
		emit s_picture();
	});
}

void xEvoPanel::build_tabs() {
	hold = true;
	while (tabs->count() > 0)
		tabs->removeTab(0);
	hw = conf.zx ? conf.zx->hw->id : -1;
	if (hw == HW_TSLAB) {
		tabs->setTabData(tabs->addTab("Registers"), QVariant::fromValue((void*)tsRegs));
		tabs->setTabData(tabs->addTab("Sprites"), QVariant::fromValue((void*)tsSpr));
		tabs->setTabData(tabs->addTab("Tiles"), QVariant::fromValue((void*)tsTile));
		tabs->setTabData(tabs->addTab("DMA"), QVariant::fromValue((void*)tsDma));
	} else if (hw == HW_PENTEVO) {
		tabs->setTabData(tabs->addTab("Ports"), QVariant::fromValue((void*)evoPorts));
	}
	tabs->setVisible(tabs->count() > 0);
	int idx = qBound(0, conf.dbg.evotab, qMax(0, tabs->count() - 1));
	tabs->setCurrentIndex(idx);
	stack->setCurrentWidget(tabs->count() ? (QWidget*)tabs->tabData(idx).value<void*>() : noMachine);
	hold = false;
}

void xEvoPanel::tab_changed(int idx) {
	if (hold || (idx < 0)) return;
	conf.dbg.evotab = idx;
	stack->setCurrentWidget((QWidget*)tabs->tabData(idx).value<void*>());
	draw();
}

void xEvoPanel::reload() {
	tbDetach->setChecked(conf.dbg.evodetach);
	build_tabs();
	draw();
}

void xEvoPanel::draw() {
	if (!conf.zx) return;
	if (conf.zx->hw->id != hw)
		build_tabs();
	labHidden->setVisible((hw == HW_TSLAB) && (conf.zx->vid->tsconf.hide != 0));
	QWidget* cur = stack->currentWidget();
	if (cur == tsRegs) tsRegs->draw();
	if (cur == tsSpr) tsSpr->draw();
	if (cur == tsTile) tsTile->draw();
	if (cur == tsDma) tsDma->draw();
	if (cur == evoPorts) evoPorts->draw();
}

// DOCK

xEvoWidget::xEvoWidget(QString i, QString t, QWidget* p):xDockWidget(i,t,p) {
	QWidget* wid = new QWidget;
	setWidget(wid);
	setObjectName("EVOWIDGET");	// the name a saved dock layout knows it by

	QVBoxLayout* lay = new QVBoxLayout(wid);
	lay->setContentsMargins(5, 5, 5, 5);
	panel = new xEvoPanel;
	lay->addWidget(panel);

	gone = new QWidget;
	QVBoxLayout* glay = new QVBoxLayout(gone);
	QLabel* lab = new QLabel("The ZX Evolution panel is in a window of its own.");
	lab->setAlignment(Qt::AlignCenter);
	lab->setWordWrap(true);
	QPushButton* btn = new QPushButton("Bring it back");
	glay->addStretch(1);
	glay->addWidget(lab);
	glay->addWidget(btn, 0, Qt::AlignCenter);
	glay->addStretch(1);
	lay->addWidget(gone);
	gone->hide();

	connect(panel, &xEvoPanel::s_detach, this, &xEvoWidget::s_detach);
	connect(panel, &xEvoPanel::s_picture, this, &xEvoWidget::s_picture);
	connect(btn, &QPushButton::clicked, this, [this](){emit s_detach(false);});
}

void xEvoWidget::setDetached(bool on) {
	panel->setVisible(!on);
	gone->setVisible(on);
	if (!on) panel->reload();
}

void xEvoWidget::draw() {
	if (panel->isVisible())
		panel->draw();
}
