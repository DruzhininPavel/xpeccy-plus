#pragma once

#include <QTableView>
#include "../../libxpeccy/filetypes/filetypes.h"
#include "../xgui.h"

class xDiskCatModel : public xTableModel {
	Q_OBJECT
	public:
		xDiskCatModel(QObject* p = NULL);
		void setRows(QStringList, int, QList<QVariantList>);
		void setLive(int, QFont);
		int textColumns() const {return ntext;}
	private:
		QStringList heads;
		int ntext = 0;			// the columns before the figures, left aligned
		QList<QVariantList> rows;
		int live = -1;			// the row under the head, drawn in liveFont
		QFont liveFont;

		int rowCount(const QModelIndex& = QModelIndex()) const;
		int columnCount(const QModelIndex& = QModelIndex()) const;
		QVariant data(const QModelIndex&, int) const;
		QVariant headerData(int, Qt::Orientation, int) const;
};

class xDiskCatTable : public QTableView {
	Q_OBJECT
	public:
		xDiskCatTable(QWidget* p = NULL);
		void setRows(QStringList, int, QList<QVariantList>);
		int textColumns() const {return model->textColumns();}
		void setLive(int);
	private:
		xDiskCatModel* model;
};
