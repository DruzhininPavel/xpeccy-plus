#include "opt_diskcat.h"

// model

xDiskCatModel::xDiskCatModel(QObject* p):xTableModel(p) {}

int xDiskCatModel::rowCount(const QModelIndex&) const {
	return rows.size();
}

int xDiskCatModel::columnCount(const QModelIndex&) const {
	return heads.size();
}

QVariant xDiskCatModel::headerData(int sect, Qt::Orientation ori, int role) const {
	QVariant res;
	if (ori != Qt::Horizontal) return res;
	// a figure's heading stands over it
	if ((role == Qt::TextAlignmentRole) && (sect >= ntext))
		return int(Qt::AlignRight | Qt::AlignVCenter);
	if (role != Qt::DisplayRole) return res;
	if ((sect < 0) || (sect >= heads.size())) return res;
	return heads[sect];
}

QVariant xDiskCatModel::data(const QModelIndex& idx, int role) const {
	QVariant res;
	if (!idx.isValid()) return res;
	int row = idx.row();
	int col = idx.column();
	if (row >= rowCount()) return res;
	if (col >= columnCount()) return res;
	// the figures line up on their last digit
	if (role == Qt::TextAlignmentRole)
		return (col >= ntext) ? int(Qt::AlignRight | Qt::AlignVCenter) : int(Qt::AlignLeft | Qt::AlignVCenter);
	if (role == Qt::FontRole)
		return (row == live) ? QVariant(liveFont) : res;
	if (role != Qt::DisplayRole) return res;
	if (col < rows[row].size()) res = rows[row][col];
	return res;
}

// the headings, how many of the columns are text, and a row of values each
void xDiskCatModel::setRows(QStringList h, int text, QList<QVariantList> r) {
	beginResetModel();
	heads = h;
	ntext = text;
	rows = r;
	live = -1;
	endResetModel();
}

void xDiskCatModel::setLive(int row, QFont fnt) {
	if (row == live) return;
	int old = live;
	live = row;
	liveFont = fnt;
	if (old >= 0) emit dataChanged(index(old, 0), index(old, columnCount() - 1));
	if (row >= 0) emit dataChanged(index(row, 0), index(row, columnCount() - 1));
}

// table

xDiskCatTable::xDiskCatTable(QWidget* p):QTableView(p) {
	model = new xDiskCatModel();
	setModel(model);
}

void xDiskCatTable::setRows(QStringList heads, int text, QList<QVariantList> rows) {
	model->setRows(heads, text, rows);
}

// the file the head is over, in bold; -1 for none
void xDiskCatTable::setLive(int row) {
	QFont fnt = font();
	fnt.setBold(true);
	model->setLive(row, fnt);
}
