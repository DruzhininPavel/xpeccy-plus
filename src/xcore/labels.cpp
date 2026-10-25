#include "xcore.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>

#include <QDebug>

// labels

xLabelSet* createLabelSet(QString name) {
	xLabelSet* set = new xLabelSet;
	set->name = name;
	set->list.clear();
	conf.labsets.append(set);
	return set;
}

// return pointer to existing labelset for given name, or nullptr if doesn't exist
xLabelSet* findLabelSet(QString name) {
	xLabelSet* ptr = nullptr;
	xLabelSet* res = nullptr;
	foreach (ptr, conf.labsets) {
		if (ptr->name == name) {
			res = ptr;
		}
	}
	return res;
}

// create conf.labmap from current labset
void map_labels() {
	xLabelSet* set = conf.curlabset;
	conf.labmap.clear();
	if (set) {
		xAdr xadr;
		QString nm;
		foreach(nm, set->list.keys()) {
			xadr = set->list[nm];
			conf.labmap[xadr.type][xadr.abs] = nm;
		}
	}
}

int delLabelSet(QString name) {
	int res = 0;
	xLabelSet* cur = conf.curlabset;
	xLabelSet* ptr;
	for (int i = conf.labsets.size() - 1; i >= 0; i--) {
		ptr = conf.labsets.at(i);
		if (ptr->name == name) {
			conf.labsets.removeAt(i);
			if (cur == ptr) {		// delete current?
				conf.curlabset = conf.labsets.size() ? conf.labsets.at(0) : nullptr;
			}
			delete(ptr);
			res++;
		}
	}
	map_labels();
	return res;
}

// returns pointer to existing labels set for given name, or add new labelset and return its pointer
xLabelSet* newLabelSet(QString name) {
	xLabelSet* res = findLabelSet(name);
	if (!res) {
		res = createLabelSet(name);
	}
	return res;
}

void setLabelSet(xLabelSet* set) {
	conf.curlabset = set;
	map_labels();			// recreate labmap
}

xLabelSet* setLabelSet(QString path) {
	xLabelSet* set = findLabelSet(path);
	if (set) {
		setLabelSet(set);
	}
	return set;
}

void clear_labels() {
	xLabelSet* set = conf.curlabset;
	if (set) {
		set->list.clear();
		conf.labmap.clear();
	}
}

void clear_all_labels() {
	while(!conf.labsets.isEmpty()) {
		free(conf.labsets.first());
		conf.labsets.takeFirst();
	}
}

void del_label(QString name) {
	xLabelSet* set = conf.curlabset;
	if (set) {
		if (set->list.contains(name)) {
			xAdr xadr = set->list[name];
			set->list.remove(name);
			conf.labmap[xadr.type].remove(xadr.abs);
		}
	}
}

void add_label(xAdr xadr, QString name, xLabelSet* set) {
	if (!set) {
		set = conf.curlabset;
	}
	if (!set) {				// if no current labset
		set = newLabelSet("noname");	// new internal labset
		setLabelSet(set);
	}
	if (set) {
		if (set->list.contains(name))
			del_label(name);
		set->list[name] = xadr;
		conf.labmap[xadr.type][xadr.abs] = name;
	}
}

QString find_label(xAdr xadr) {
	QString lab;
	xLabelSet* set = conf.curlabset;
	if (set) {
		if (conf.labmap.contains(xadr.type)) {
			if (conf.labmap[xadr.type].contains(xadr.abs)) {
				lab = conf.labmap[xadr.type][xadr.abs];
			}
		}
		if (lab.isEmpty() && conf.labmap.contains(MEM_IO)) lab = conf.labmap[MEM_IO].value(xadr.adr);
	}
	return lab;
}

xAdr find_label(QString nm) {
	xAdr xadr = {-1, -1, -1, -1};
	xLabelSet* set = conf.curlabset;
	if (set) {
		if (set->list.contains(nm)) {
			xadr = set->list.value(nm);
			if (conf.zx && (conf.zx->hw->id == HW_ZXNEXT) && (xadr.type == MEM_RAM)) {
				xadr.adr = comp_find_xadr(conf.zx, xadr.type, xadr.abs, MEM_VIEW_READ);
			}
		}
	}
	return xadr;
}

// One sjasmplus LABELSLIST file into the set named after it, which is then
// the one in effect. 0: it could not be opened, and nothing changed.
static int labels_read(const QString& path) {
	QString line;
	QString name;
	QStringList arr;
	xAdr xadr;
	QFile file(path);
	if (!file.open(QFile::ReadOnly)) return 0;
	bool isNext = conf.zx && (conf.zx->hw->id == HW_ZXNEXT);
	xLabelSet* set = newLabelSet(QFileInfo(path).fileName());
	set->list.clear();		// before it is made current, or its old map is built for nothing
	setLabelSet(set);
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
	static const QRegularExpression sep("[: \t\r\n]");
#else
	static const QRegExp sep("[: \t\r\n]");
#endif
	while(!file.atEnd()) {
		line = file.readLine();
		if (line.startsWith(":"))
			line.prepend("FF");
		arr = line.split(sep, X_SkipEmptyParts);
		if (arr.size() > 2) {
			bool pageOk, adrOk;
			xadr.type = MEM_RAM;
			xadr.bank = arr.at(0).toInt(&pageOk, 16);
			xadr.adr = arr.at(1).toInt(&adrOk, 16);
			if (!pageOk || !adrOk || (xadr.adr < 0) || (xadr.adr > 0xffff)) continue;
			if (isNext) {
				if (xadr.bank == 0xff) {
					xadr.type = MEM_IO;
					xadr.abs = xadr.adr;
					xadr.bank = -1;
				} else {
					if ((xadr.bank < 0) || (xadr.bank >= 0xe0)) continue;
					// SjASMPlus uses 8K Next RAM pages; ramData includes the ROM/SRAM area before them.
					xadr.abs = 0x40000 + (xadr.bank << 13) + (xadr.adr & 0x1fff);
					xadr.bank = xadr.abs >> 14;
					xadr.adr = comp_find_xadr(conf.zx, xadr.type, xadr.abs, MEM_VIEW_READ);
				}
				add_label(xadr, arr.at(2));
				continue;
			}
			if (xadr.bank == 0xff) {
				switch (xadr.adr & 0xc000) {
					case 0x0000: xadr.bank = 0; break;
					case 0x4000: xadr.bank = 5; break;
					case 0x8000: xadr.bank = 2; break;
					case 0xc000: xadr.bank = 0; break;
				}
			}
			xadr.adr &= 0x3fff;
			xadr.abs = (xadr.bank << 14) | xadr.adr;
			name = arr.at(2);
			switch (xadr.bank) {
				case 0xff:
					xadr.type = -1;		// cpu
					xadr.bank = -1;
					break;
				case 0x05:
					xadr.adr |= 0x4000;
					break;
				case 0x02:
					xadr.adr |= 0x8000;
					break;
				default:
					xadr.adr |= 0xc000;
					break;
			}
			//if (xadr.bank > 0)
			//	xadr.bank = xadr.abs >> 8;
			add_label(xadr, name);
			//
		}
	}
	xlog(XLG_FILE, XLL_INFO, "labels from %s", path.toLocal8Bit().constData());
	return 1;
}

// A file the user named, on the command line or in the debugger: the one
// Reload reads again. NULL asks for it.
int loadLabels(const char* fn) {
	QString path(fn);
	if (path.isEmpty())
		path = QFileDialog::getOpenFileName(NULL, "Load SJASM labels",QString(),QString(),nullptr,QFileDialog::DontUseNativeDialog);
	if (path.isEmpty()) return 1;
	if (!labels_read(path)) return 0;
	conf.labpath = path;
	return 1;
}

bool labels_named_on() {
	return !conf.labpath.isEmpty() && conf.curlabset
		&& (conf.curlabset->name == QFileInfo(conf.labpath).fileName());
}

// the labels an image came with last, and the set in effect before them
static QString imgSet;
static QString imgBefore;

void labels_image(const QString& image) {
	if (!imgSet.isEmpty()) {
		// a set picked by hand since then stays
		if (conf.curlabset && (conf.curlabset->name == imgSet))
			setLabelSet(findLabelSet(imgBefore));
		imgSet.clear();
	}
	if (!conf.dbg.imglabels || image.isEmpty()) return;
	QFileInfo fi(image);
	QString path = fi.dir().filePath(fi.completeBaseName() + ".labels");
	if (!QFileInfo(path).isFile()) return;
	QString before = conf.curlabset ? conf.curlabset->name : QString();
	if (!labels_read(path)) return;
	imgBefore = before;
	imgSet = conf.curlabset->name;
}

int saveLabels(const char* fn) {
	int res = 1;
	QStringList keys;
	QString key;
	xAdr xadr;
	QString line;
	QFile file;
	QString path(fn);
	if (path.isEmpty())
		path = QFileDialog::getSaveFileName(NULL, "Save labels",QString(),QString(),nullptr,QFileDialog::DontUseNativeDialog);
	if (path.isEmpty()) {
		res = 0;
	} else {
		file.setFileName(path);
		if (file.open(QFile::WriteOnly)) {
			xLabelSet* set = conf.curlabset;
			if (set) {
				keys = set->list.keys();
				foreach(key, keys) {
					xadr = set->list.value(key);
					if (conf.zx && (conf.zx->hw->id == HW_ZXNEXT)) {
						if ((xadr.type == MEM_RAM) && (xadr.abs >= 0x40000) && (xadr.abs < 0x200000)) {
							line = gethexbyte((xadr.abs - 0x40000) >> 13);
							line.append(QString(":%0 %1\n").arg(gethexword(xadr.abs & 0x1fff), key));
						} else {
							int adr = comp_find_xadr(conf.zx, xadr.type, xadr.abs, MEM_VIEW_READ);
							if (adr < 0) continue;
							line = QString(":%0 %1\n").arg(gethexword(adr), key);
						}
					} else {
						line = (xadr.type == MEM_RAM) ? gethexbyte(xadr.abs >> 14) : "FF";
						line.append(QString(":%0 %1\n").arg(gethexword(xadr.abs & 0x3fff), key));
					}
					file.write(line.toUtf8());
				}
			}
			file.close();
		} else {
			res = 0;
		}
	}
	return res;
}

// comments

void add_comment(xAdr xadr, QString str) {
	conf.commap[xadr.type][xadr.abs] = str;
}

void del_comment(xAdr xadr) {
	if (conf.commap.contains(xadr.type)) {
		conf.commap[xadr.type].remove(xadr.abs);
	}
}

QString find_comment(xAdr xadr) {
	QString str;
	if (conf.commap.contains(xadr.type)) {
		if (conf.commap[xadr.type].contains(xadr.abs)) {
			str = conf.commap[xadr.type][xadr.abs];
		}
	}
	return str;
}

void clear_comments() {
	conf.commap.clear();
}
