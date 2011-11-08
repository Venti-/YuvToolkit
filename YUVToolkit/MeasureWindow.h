#ifndef MEASURE_WINDOW_H
#define MEASURE_WINDOW_H

#include "YT_Interface.h"
#include <QtGui>
#include "ui_MeasureWindow.h"
class VideoViewList;
class VideoView;

#include <QAbstractTableModel>

class MeasureResultsModel : public QAbstractTableModel
{
	Q_OBJECT;
public:
	MeasureResultsModel(QObject *parent);
	int rowCount(const QModelIndex &parent = QModelIndex()) const ;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex & /*index*/) const;
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

	VideoView* m_Original;
	VideoView* m_Processed;
	QList<Measure*> m_MeasureList;
	/*
	QList<Measure::MeasureItem> m_MeasureOutItems;
	QList<Measure::MeasureItem> m_ViewOutItems;

	QMap<Measure::MeasureItem, FramePtr> m_OutputViewItems;
	QMap<Measure::MeasureItem, QVariant> m_OutputMeasureItems;*/

	MeasureResultsModel m_ResultsModel;
	DistortionMapModel m_DistortionMapModel;
public:
	Ui::MeasureWindow ui;

	MeasureWindow(VideoViewList* vvList, QWidget *parent = 0, Qt::WFlags flags = 0);
	~MeasureWindow();

	void UpdateMeasure();

	void UpdateMeasureWindow();

	QSize sizeHint() const;
protected:
	void showEvent(QShowEvent *event);

public slots:
	void onComboIndexChanged(int);
};

#endif
