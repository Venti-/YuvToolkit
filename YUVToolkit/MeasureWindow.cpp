#include "YT_Interface.h"

#include "MeasureWindow.h"
#include "VideoView.h"
#include "VideoViewList.h"



MeasureResultsModel::MeasureResultsModel( QObject *parent, QList<MeasureItem>& results) :
QAbstractTableModel(parent), m_Results(results)
{
	for (int i=0; i<m_Results.size(); i++)
	{
		const MeasureItem& item = m_Results.at(i);
		if (!m_ViewIdCols.contains(item.viewId))
		{
			m_ViewIdCols.append(item.viewId);
		}

		if (item.viewId == m_ViewIdCols.at(0))
		{
			m_MeasureNameRows.append(item.op.measureName);
		}
	}
}

int MeasureResultsModel::rowCount( const QModelIndex &parent /*= QModelIndex()*/ ) const
{
	return m_MeasureNameRows.size()*4;
}

int MeasureResultsModel::columnCount( const QModelIndex &parent /*= QModelIndex()*/ ) const
{
	
	return m_ViewIdCols.size();
}

QVariant MeasureResultsModel::data( const QModelIndex &index, int role /*= Qt::DisplayRole*/ ) const
{
	int row = index.row();
	int col = index.column();

	switch(role){
	case Qt::DisplayRole:
		{
			unsigned int viewId = m_ViewIdCols.at(col);
			const QString& measureName = m_MeasureNameRows.at(row/4);

			for (int i=0; i<m_Results.size(); i++)
			{
				const MeasureItem& res = m_Results.at(i);
				if (res.op.measureName == measureName && res.viewId == viewId)
				{
					int plane = row % 4;
					if (res.op.hasResults[plane])
					{
						return QString("%1").arg(res.op.results[plane], 0, 'f', 2);
					}else
					{
						return QString("");
					}
				}
			}
		}

		break;
	case Qt::FontRole:
		if (row%4 == 3)
		{
			QFont boldFont;
			boldFont.setBold(true);
			return boldFont;
		}
		break;
	case Qt::BackgroundRole:
		if (row%4 == 3)
		{
			return QBrush(QColor(224, 192, 192));
		}
		break;
	}
	return QVariant();
}

QVariant MeasureResultsModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
	switch(role){
	case Qt::DisplayRole:
		{
			if (orientation == Qt::Horizontal) {
				switch (section)
				{
				case 0:
					return QString("1");
				case 1:
					return QString("2");
				case 2:
					return QString("3");
				}
			}else {
				QString header = m_MeasureNameRows.at(section/4);
				int plane = section%4;
				switch (plane)
				{
				case 0:
					header += "(Y)";
					break;
				case 1:
					header += "(U)";
					break;
				case 2:
					header += "(V)";
					break;
				}
				return header;
			}
		}
		break;
	case Qt::FontRole:
		if (orientation == Qt::Vertical) 
		{
			if (section%4 == 3)
			{
				QFont boldFont;
				boldFont.setBold(true);
				return boldFont;
			}
		}
		break;
	}

	return QVariant();

}

Qt::ItemFlags MeasureResultsModel::flags( const QModelIndex & index ) const
{
	int row = index.row();
	int col = index.column();
	unsigned int viewId = m_ViewIdCols.at(col);
	const QString& measureName = m_MeasureNameRows.at(row/4);

	for (int i=0; i<m_Results.size(); i++)
	{
		const MeasureItem& res = m_Results.at(i);
		if (res.op.measureName == measureName && res.viewId == viewId)
		{
			int plane = row % 4;
			if (res.op.hasResults[plane])
			{
				return Qt::ItemIsEnabled;
			}
		}
	}
	return 0;
}

void MeasureResultsModel::ResultsUpdated()
{
	emit dataChanged(index(0,0), index(8, 1));
}

DistortionMapModel::DistortionMapModel( QObject *parent ) :
QAbstractTableModel(parent)
{
}

int DistortionMapModel::rowCount( const QModelIndex &parent /*= QModelIndex()*/ ) const
{
	return 2;
}

int DistortionMapModel::columnCount( const QModelIndex &parent /*= QModelIndex()*/ ) const
{
	return 1;
}

QVariant DistortionMapModel::data( const QModelIndex &index, int role /*= Qt::DisplayRole*/ ) const
{
	int row = index.row();
	int col = index.column();

	switch(role){
	case Qt::DisplayRole:
		switch (index.row())
		{
		case 0:
			return QString("PSNR map");
		case 1:
			return QString("MSE map");
		}
		break;
	case Qt::CheckStateRole:
		return Qt::Checked;
	}
	return QVariant();
}

Qt::ItemFlags DistortionMapModel::flags( const QModelIndex & /*index*/ ) const
{
	return Qt::ItemIsSelectable |  Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
}


MeasureWindow::MeasureWindow(VideoViewList* vvList, QWidget *parent, Qt::WFlags flags) : 
	QWidget(parent, flags), m_VideoViewList(vvList), m_ResultsModel(NULL), m_DistortionMapModel(this), m_UpdateTimer(NULL)
{
	ui.setupUi(this);
	// connect(ui.originalList, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboIndexChanged(int)));
	// connect(ui.processedList, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboIndexChanged(int)));

	ui.table_Distortion_Map->setModel(&m_DistortionMapModel);
	ui.table_Distortion_Map->setColumnWidth(0, 1000);
}

MeasureWindow::~MeasureWindow()
{
}

void MeasureWindow::showEvent( QShowEvent *event )
{
	UpdateRequest();
}

void MeasureWindow::hideEvent( QHideEvent *event )
{
	ClearAll();
}


void MeasureWindow::onComboIndexChanged( int )
{
	/*if (ui.originalList->count()<=1)
	{
		return;
	}

	m_Original = m_VideoViewList->at(ui.originalList->currentIndex());
	m_Processed = m_VideoViewList->at(ui.processedList->currentIndex());

	if (m_MeasureList.size()>0)
	{
		for (int i=0; i<m_MeasureList.size(); i++)
		{
			// TODO clean up.
		}
	}

	const QList<PlugInInfo*>& lst = GetHostImpl()->GetMeasurePluginList();
	for (int i=0; i<lst.size(); i++)
	{
		PlugInInfo* plugInInfo = lst[i];

		Measure* measure = plugInInfo->plugin->NewMeasure(plugInInfo->string);
		m_MeasureList.append(measure);

		SourceInfo origInfo, procInfo;
		m_Original->GetSource()->GetInfo(origInfo);
		m_Processed->GetSource()->GetInfo(procInfo);
		measure->GetSupportedModes(origInfo.format, procInfo.format, m_ViewOutItems, m_MeasureOutItems);

		for (int i=0; i<m_MeasureOutItems.size(); i++)
		{
			Measure::MeasureItem item = m_MeasureOutItems.at(i);
			m_OutputMeasureItems.insert(item, QVariant(-1));
		}
	}

	*/
}

void MeasureWindow::UpdateMeasure()
{
	/*
	if (ui.originalList->count()<=1)
	{
		return;
	}

	VideoQueue::Frame* origFrame  = m_Original->GetVideoQueue()->GetLastRenderFrame();			
	VideoQueue::Frame* procFrame  = m_Processed->GetVideoQueue()->GetLastRenderFrame();			
	if (!(origFrame && procFrame))
	{
		return;
	}

	Measure* measure = m_MeasureList.at(0);

	measure->Process(origFrame->source, procFrame->source, m_OutputViewItems, m_OutputMeasureItems);*/
}

void MeasureWindow::UpdateMeasureWindow()
{
	if (!isVisible())	
	{
		// TODO; enable timer only when needed
		return;
	}

	/*QMapIterator<Measure::MeasureItem, QVariant> i(m_OutputMeasureItems);
	while (i.hasNext()) {
		i.next();
		// cout << i.key() << ": " << i.value() << endl;
		QTableWidgetItem* item = ui.tableWidget->item(i.key().plane, i.key().measureType);
		if (item)
		{
			item->setText(i.value().toString());
		}
	}
	*/
}

QSize MeasureWindow::sizeHint() const
{
	QTableView* view = ui.table_Measure_Results;
	QAbstractItemModel* m = view->model();

	QPainter painter;
	QFontMetrics fontMetrics = painter.fontMetrics();
	int columnWidth = fontMetrics.width("WWW.WW");
	
	return QSize(columnWidth*3+ui.verticalLayout->spacing()*3, 250);
}

void MeasureWindow::on_button_Options_clicked()
{
	
}

void MeasureWindow::OnTimer()
{
	if (m_ResultsModel)
	{
		m_VideoViewList->GetProcessThread()->GetMeasureResults(m_MeasureItemList);
		m_ResultsModel->ResultsUpdated();
		ui.table_Measure_Results->resizeColumnsToContents();
	}
}

void MeasureWindow::UpdateRequest()
{
	ClearAll();

	unsigned int sourceView1 = m_SourceList.at(0);

	for (int j=1; j<m_SourceList.size(); j++)
	{
		unsigned int sourceView2 = m_SourceList.at(j);
		unsigned int viewId = m_VideoViewList->NewVideoViewId();

		const QList<PlugInInfo*>& plugins = GetHostImpl()->GetMeasurePluginList();
		for (int i=0; i<plugins.size(); i++)
		{
			PlugInInfo* info = plugins.at(i);

			MeasureItem req = {0};
			
			req.plugin = info;
			req.measure = info->plugin->NewMeasure(info->string);

			const QList<MeasureCapability>& caps = req.measure->GetCapabilities();
			for (int j=0; j<caps.size(); j++)
			{
				const MeasureCapability& c = caps.at(j);
				req.op.measureName = c.measureName;

				req.sourceViewId1 = sourceView1;
				req.sourceViewId2 = sourceView2;
				req.showDistortionMap = false;
				req.viewId = viewId;

				m_MeasureItemList.append(req);
			}
		}
	}

	m_ResultsModel = new MeasureResultsModel(this, m_MeasureItemList);
	m_VideoViewList->GetProcessThread()->SetMeasureRequests(m_MeasureItemList);
	ui.table_Measure_Results->setModel(m_ResultsModel);
	// ui.table_Measure_Results->setShowGrid(false);

	m_UpdateTimer = new QTimer(this);
	m_UpdateTimer->setInterval(200);
	connect(m_UpdateTimer, SIGNAL(timeout()), this, SLOT(OnTimer()));
	m_UpdateTimer->start();

	PlaybackControl* c = m_VideoViewList->GetControl();
	PlaybackControl::Status status;
	c->GetStatus(&status);
	if (!status.isPlaying)
	{
		c->Seek(status.lastProcessPTS, false);
	}

}

void MeasureWindow::OnVideoViewSourceListChanged()
{
	m_SourceList = m_VideoViewList->GetSourceIDList();
	while (m_SourceList.size()>3)
	{
		m_SourceList.removeLast();
	}
	
	UpdateLabels();
	if (isVisible())
	{
		UpdateRequest();
	}
}

void MeasureWindow::ClearAll()
{
	if (m_UpdateTimer)
	{
		m_UpdateTimer ->stop();
		SAFE_DELETE(m_UpdateTimer);
	}

	// Clear request list from process threads
	m_VideoViewList->GetProcessThread()->SetMeasureRequests(QList<MeasureItem>());
	ui.table_Measure_Results->setModel(NULL);

	QSet<Measure*> measureListDeleted;
	for (int i=0; i<m_MeasureItemList.size(); i++)
	{
		const MeasureItem& req = m_MeasureItemList.at(i);
		if (!measureListDeleted.contains(req.measure))
		{
			measureListDeleted.insert(req.measure);

			req.plugin->plugin->ReleaseMeasure(req.measure);
		}
	}

	SAFE_DELETE(m_ResultsModel);
	m_MeasureItemList.clear();
}

void MeasureWindow::UpdateLabels()
{
	TextLabel* labels[] = {ui.label_Original, ui.label_Processed, ui.label_Processed_2};
	QLabel* labels2[] = {ui.label_0, ui.label_1, ui.label_2};

	for (int i=0; i<sizeof(labels)/sizeof(labels[0]); i++)
	{
		if (m_SourceList.size()>i)
		{
			VideoView* vv = m_VideoViewList->find(m_SourceList.at(i));
			labels[i]->setText(vv->GetTitle());
			labels[i]->setToolTip(vv->GetTitle());
			labels[i]->show();
			labels2[i]->show();
		}else
		{
			labels[i]->setText("");
			labels[i]->setToolTip("");
			labels[i]->hide();
			labels2[i]->hide();
		}
	}
}
