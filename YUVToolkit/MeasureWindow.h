#ifndef MEASURE_WINDOW_H
#define MEASURE_WINDOW_H

#include "YT_InterfaceImpl.h"
#include <QtGui>
#include <QtCore>
#include "ui_MeasureWindow.h"
class VideoViewList;
class VideoView;

#include <QAbstractTableModel>

class MeasureResultsModel : public QAbstractTableModel
{
	Q_OBJECT;
	QList<MeasureItem>& m_Results;
	QStringList m_MeasureNameRows;
	QList<unsigned int> m_ViewIdCols;
public:
	MeasureResultsModel(QObject *parent, QList<MeasureItem>& );
	int rowCount(const QModelIndex &parent = QModelIndex()) const ;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex & /*index*/) const;
	void ResultsUpdated();
};

class DistortionMapModel : public QAbstractTableModel
{
	Q_OBJECT;
public:
	DistortionMapModel(QObject *parent);
	int rowCount(const QModelIndex &parent = QModelIndex()) const ;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	Qt::ItemFlags flags(const QModelIndex & /*index*/) const;
};

class MeasureWindow : public QWidget
{
	Q_OBJECT;
	VideoViewList* m_VideoViewList;

	MeasureResultsModel* m_ResultsModel;
	DistortionMapModel m_DistortionMapModel;
	QTimer* m_UpdateTimer;	

	UintList m_SourceList;
	QList<MeasureItem> m_MeasureItemList;
public:
	Ui::MeasureWindow ui;

	MeasureWindow(VideoViewList* vvList, QWidget *parent = 0, Qt::WFlags flags = 0);
	~MeasureWindow();

	void UpdateMeasure();

	void UpdateMeasureWindow();

	QSize sizeHint() const;
protected:
	void showEvent(QShowEvent *event);
	void hideEvent(QHideEvent *event);
	
	void ClearAll();
	void UpdateRequest();
	void UpdateLabels();
public slots:
	void OnVideoViewSourceListChanged();
private slots:
	void onComboIndexChanged(int);
	void on_button_Options_clicked();
	void OnTimer();
};

#endif
