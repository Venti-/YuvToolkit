#include "VideoViewList.h"
#include "VideoView.h"
#include "RendererWidget.h"
#include "RenderThread.h"
#include "ProcessThread.h"

#include "SourceThread.h"
#include "Layout.h"
#include "ColorConversion.h"
#include "MeasureWindow.h"

#include <assert.h>

VideoViewList::VideoViewList(QMainWindow* mainWindow, RendererWidget* rw) : m_RenderThread(NULL),
	m_ProcessThread(NULL), m_Duration(0), m_LongestVideoView(0), m_VideoCount(0),
	m_CurrentPTS(0), m_SeekingPTS(INVALID_PTS), m_SeekingPTSNext(INVALID_PTS), 
	m_EndOfFile(false)
{
	m_RenderWidget = rw;
	m_MainWindow = mainWindow;
	
	connect(this, SIGNAL(ResolutionDurationChanged()), this, SLOT(UpdateDuration()));

	for (int i=0; i<4; i++)
	{
		QDockWidget* dock;
		
		QString str;
		QTextStream(&str) << "Quality Measures Window " << i+1;

		dock = new QDockWidget(str, m_MainWindow );
		dock->setAllowedAreas(NULL);
		dock->setVisible(false);
		dock->setFloating(true);
		m_DockWidgetList.append(dock);

		MeasureWindow* win = new MeasureWindow(this, dock);
		dock->setWidget(win);
		m_MeasureWindowList.append(win);
	}
}

VideoViewList::~VideoViewList()
{
}

VideoView* VideoViewList::NewVideoView( const char* title )
{
	if (m_ProcessThread == NULL)
	{
		m_ProcessThread = new ProcessThread();
		m_ProcessThread->Start();

		connect(this, SIGNAL(seek(unsigned int, bool)), 
			m_ProcessThread, SLOT(Seek(unsigned int, bool)));
	}

	if (m_RenderWidget->GetRenderer() == NULL)
	{
		QSettings settings;
		QString renderType = settings.value("main/renderer", "D3D").toString();
		m_RenderWidget->Init(renderType);

		m_RenderThread = new RenderThread(m_RenderWidget->GetRenderer(), this);
		
		connect(m_ProcessThread, SIGNAL(sceneReady(QList<YT_Frame_Ptr>, unsigned int, bool)), 
			m_RenderThread, SLOT(RenderScene(QList<YT_Frame_Ptr>, unsigned int, bool)));
		connect(this, SIGNAL(layoutUpdated(QList<unsigned int>, QList<QRect>, QList<QRect>)), 
			m_RenderThread, SLOT(SetLayout(QList<unsigned int>, QList<QRect>, QList<QRect>)));
		connect(m_RenderThread, SIGNAL(sceneRendered(QList<YT_Frame_Ptr>, unsigned int, bool)), 
			this, SLOT(OnSceneRendered(QList<YT_Frame_Ptr>, unsigned int, bool)));
	}

	m_RenderThread->Stop();

	VideoView* vv = new VideoView(m_MainWindow, m_RenderWidget, m_ProcessThread);
	INFO_LOG("VideoViewList::NewVideoView %X", vv);

	m_RenderWidget->layout->AddView(vv);
	m_VideoList.append(vv);	
	
	vv->SetTitle( title );

	OnUpdateRenderWidgetPosition();

	m_RenderThread->Start();

	connect(vv, SIGNAL(Close(VideoView*)), this, SLOT(CloseVideoView(VideoView*)));
	connect(vv, SIGNAL(TransformTriggered(QAction*, VideoView*, TransformActionData*)), this, SLOT(OnVideoViewTransformTriggered(QAction*, VideoView*, TransformActionData*)));

	emit VideoViewCreated(vv);

	return vv;
}

void VideoViewList::OnUpdateRenderWidgetPosition()
{
	QRect rcClient = m_RenderWidget->rect();
	int width = rcClient.width();
	m_RenderWidget->layout->UpdateGeometry();

	QList<unsigned int> ids;
	QList<QRect> srcRects, dstRects;
	for (int i=0; i<m_VideoList.size(); ++i) 
	{
		VideoView* vv = m_VideoList.at(i);

		ids.append(vv->GetID());
		srcRects.append(vv->srcRect);
		dstRects.append(vv->dstRect);
	}

	if (m_VideoList.size()>0)
	{
		emit layoutUpdated(ids, srcRects, dstRects);
	}
}

void VideoViewList::CloseVideoView( VideoView* vv)
{
	INFO_LOG("VideoViewList::CloseVideoView %X", vv);
	for (int i=0; i<m_VideoList.size(); ++i) 
	{
		VideoView* vv2 = m_VideoList.at(i);
		/*
		if (vv2->GetRefVideoQueue() == vv->GetVideoQueue())
		{
			CloseVideoView(vv2);
			i = 0; 
		}*/
	}

	m_RenderThread->Stop();
	m_ProcessThread->Stop();
	
	vv->UnInit();
	m_RenderWidget->layout->RemoveView(vv);
	m_VideoList.removeOne(vv);
	
	UpdateDuration();

	if (m_VideoList.isEmpty())
	{
		m_RenderThread->Stop();
		SAFE_DELETE(m_RenderThread);

		m_RenderWidget->UnInit();
		m_RenderWidget->repaint();

		m_CurrentPTS = 0;
		m_SeekingPTS = m_SeekingPTSNext = INVALID_PTS;

		SAFE_DELETE(m_ProcessThread);
		SAFE_DELETE(m_ProcessThread);
	}else
	{
		m_RenderThread->Start();
		m_ProcessThread->Start();
	}

	emit VideoViewClosed(vv);

	SAFE_DELETE(vv);
}

void VideoViewList::CheckRenderReset()
{
	if (m_RenderThread && m_RenderWidget->GetRenderer()->NeedReset())
	{
		m_RenderThread->Stop();

		YT_RESULT res = m_RenderWidget->GetRenderer()->Reset();
		assert(res == YT_OK);

		if (res == YT_OK)
		{
			m_RenderThread->Start();
		}
	}
}
/*
bool VideoViewList::GetRenderFrameList( QList<YT_Render_Frame>& frameList, unsigned int& renderPTS )
{
	QTime time;
	time.restart();
	// INFO_LOG("VideoViewList::GetRenderFrameList");

	QMutexLocker locker(&m_MutexAddRemoveAndSeeking);

	while (frameList.size()>m_VideoList.size())
	{
		frameList.removeLast();
	}

	while (frameList.size() < m_VideoList.size())
	{
		YT_Render_Frame rf;
		frameList.append(rf);
	}


	unsigned int pts = INVALID_PTS; // Next pts to read from video queue
	// If paused, get the most recent item in the queue (flush queue)
	// If playing, find the closest frame in the future
	if (m_SeekingPTS != INVALID_PTS)
	{
		pts = m_SeekingPTS;
	}else if (m_Paused)
	{
		pts = m_CurrentPTS;
	}else
	{
		// Get next pts to render
		unsigned int minPTS = INVALID_PTS;
		unsigned int maxPTS = 0;
		for (int i=0; i<m_VideoList.size(); ++i) 
		{
			VideoView* vv = m_VideoList.at(i);
			if (vv->GetType() == YT_PLUGIN_SOURCE)
			{
				SourceThread* st = vv->GetSourceThread();
				VideoQueue* vq = vv->GetVideoQueue();
				
				bool isLastFrame = false;
				unsigned int currentPTS = vv->GetSource()->SeekPTS(m_CurrentPTS);
				unsigned int nextPts = vq->GetNextPTS(currentPTS, isLastFrame);
				if (nextPts!=INVALID_PTS &&  nextPts<minPTS && !isLastFrame)
				{
					minPTS = nextPts;
				}

				if (nextPts!=INVALID_PTS &&  nextPts>maxPTS)
				{
					maxPTS = nextPts;
				}
			}
		}
		if (minPTS < INVALID_PTS)
		{
			pts = minPTS;
		}else
		{
			pts = maxPTS;
		}
		assert(pts < INVALID_PTS);
	}

	m_CurrentPTS = pts;

	bool shouldRender = true;
	bool seekingDone = true;
	for (int i=0; i<m_VideoList.size(); ++i) 
	{
		VideoView* vv = m_VideoList.at(i);
		if (!(vv->dstRect.isValid() && vv->srcRect.isValid()))
		{
			shouldRender = false;
		}

		if (vv->GetType() == YT_PLUGIN_UNKNOWN)
		{
			shouldRender = false;
		}else if (vv->GetType() == YT_PLUGIN_SOURCE)
		{
			VideoQueue::Frame* vqf = NULL;
			YT_Source* source = vv->GetSource();
			unsigned int sourcePTS = pts;
			
			if (source)
			{
 				sourcePTS = source->SeekPTS(pts);
			
				vqf = vv->GetVideoQueue()->GetRenderFrame(sourcePTS);
			}

			if (vqf)
			{
				YT_Render_Frame& rf = frameList[i];
				rf.frame = vqf->render;
				COPY_RECT(rf.srcRect, vv->srcRect);
				COPY_RECT(rf.dstRect, vv->dstRect);

				if (m_SeekingPTS != INVALID_PTS)
				{
					if (vqf->source->PTS() != sourcePTS)
					{
						seekingDone = false;
					}
				}
			}else
			{
				// Some frames are black, will cause flickering
				shouldRender = false;
				seekingDone = false;

				if (m_NeedSeekingRequest)
				{
					SourceThread* st = vv->GetSourceThread();
					st->Seek(sourcePTS);
					// QMetaObject::invokeMethod(st, "Seek", Qt::QueuedConnection, Q_ARG(unsigned int, sourcePTS));	
				}
			}
		}
	}

	m_NeedSeekingRequest = false;

	if (seekingDone && m_SeekingPTS != INVALID_PTS)
	{
		m_SeekingPTS = INVALID_PTS;
	}

	for (int i=0; i<m_VideoList.size(); ++i) 
	{
		VideoView* vv = m_VideoList.at(i);
		if (vv->GetType() == YT_PLUGIN_TRANSFORM)
		{
			VideoQueue::Frame* ref  = vv->GetRefVideoQueue()->GetLastRenderFrame();			
			if (!ref)
			{
				continue;
			}

			YT_Format_Ptr transformFormat = YT_Format_Ptr(new YT_FormatImpl);
			vv->GetTransform()->GetFormat(ref->source->Format(), vv->GetOutputName(), transformFormat);
			
			VideoQueue::Frame* vqf = vv->GetVideoQueue()->GetSourceFrame(transformFormat);
			if (vqf != NULL && vqf->source != NULL && vqf->render != NULL )
			{
				QMap<QString, YT_Frame_Ptr> outputs;
				QMap<QString, QVariant> stats;
				outputs.insert(vv->GetOutputName(), vqf->source);
				vv->GetTransform()->Process(ref->source, outputs, stats);

				VideoQueue::RenderFrame(vqf);

				vv->GetVideoQueue()->ReleaseSourceFrame(vqf);

				/// Just to flush queue and make more source buffer available
				bool b;
				unsigned int ptsNext = vv->GetVideoQueue()->GetNextPTS(INVALID_PTS, b); // find the PTS of next frame in the queue

				YT_Render_Frame& rf = frameList[i];
				rf.frame = vqf->render;
				COPY_RECT(rf.srcRect, vv->srcRect);
				COPY_RECT(rf.dstRect, vv->dstRect);
			}
		}
	}

	if (m_Paused || m_SeekingPTS != INVALID_PTS)
	{
		renderPTS  = INVALID_PTS;
	}else
	{
		renderPTS  = pts;
	}

	INFO_LOG("GetRenderFrameList took %d ms", time.elapsed());

	return shouldRender;
}*/

class QThread3 : public QThread
{
public:
	static void msleep(unsigned long msecs) {
		QThread::msleep(msecs);
	}
};

void VideoViewList::Seek( unsigned int pts, bool playAfterSeek)
{
	INFO_LOG("VideoViewList::Seek %d - m_SeekingPTS %d", pts, m_SeekingPTS);

	assert(m_VideoList.size()>0);
	
	if (pts != INVALID_PTS)
	{
		if (m_SeekingPTS == INVALID_PTS)
		{
			m_SeekingPTS = pts;
			emit seek(pts, playAfterSeek);
		}else
		{
			// Already seeking
			m_SeekingPTSNext = pts;
		}
	} else
	{
		emit seek(pts, playAfterSeek);
	}
}

void VideoViewList::CheckLoopFromStart()
{
	if (m_SeekingPTS == INVALID_PTS && m_SeekingPTSNext == INVALID_PTS)
	{
		VideoView* vv = m_LongestVideoView;
		
		YT_Source* src = vv->GetSource();
		YT_Frame_Ptr frame = vv->GetLastFrame();

		bool endOfFile = (frame && frame->Info(IS_LAST_FRAME).toBool());

		if (!m_EndOfFile && endOfFile)
		{
			Seek(0, true);
		}

		m_EndOfFile = endOfFile;
	}
}

void VideoViewList::UpdateDuration()
{
	m_Duration = 0;
	m_LongestVideoView = NULL;
	for (int i=0; i<m_VideoList.size(); ++i) 
	{
		VideoView* vv = m_VideoList.at(i);
		SourceThread* st = vv->GetSourceThread();
		if (st)
		{
			YT_Source* source = st->GetSource();

			YT_Source_Info info;
			source->GetInfo(info);

			if (info.duration>m_Duration)
			{
				m_Duration = info.duration;
				m_LongestVideoView = vv;
			}
		}
	}
}

void VideoViewList::CheckResolutionChanged()
{
	bool changed = false;
	for (int i=0; i<m_VideoList.size(); ++i) 
	{
		if (m_VideoList.at(i)->CheckResolutionDurationChanged())
		{
			changed = true;
		}
	}

	if (changed)
	{
		emit ResolutionDurationChanged();
	}
}

void VideoViewList::OnVideoViewTransformTriggered( QAction* action, VideoView* vv, TransformActionData *data)
{
	bool hasView = false;
	for (int i=0; i<m_VideoList.size(); ++i) 
	{
		VideoView* vv2 = m_VideoList.at(i);
		/*
		if (vv2->GetType() == YT_PLUGIN_TRANSFORM && vv2->GetRefVideoQueue() == 
			vv->GetVideoQueue() && vv2->GetOutputName() == data->outputName)
		{
			hasView = true;
			CloseVideoView(vv2);
		}*/
	}

	if (!hasView)
	{
		VideoView* planeVv = NewVideoView(data->outputName.toAscii());

		YT_Transform* transform = data->transformPlugin->NewTransform(data->transformName);

		planeVv->Init(transform, vv->GetVideoQueue(), data->outputName);
	}

	action->setChecked(!hasView);
}

bool VideoViewList::IsPlaying()
{
	if (m_ProcessThread)
	{
		return m_ProcessThread->IsPlaying();
	}else
	{
		return false;
	}
}

VideoView* VideoViewList::longest() const
{
	return m_LongestVideoView;
}

void VideoViewList::UpdateMeasureWindows()
{
	for (int i=0; i<m_MeasureWindowList.size(); i++)
	{
		MeasureWindow* win = m_MeasureWindowList.at(i);
		if (win->isVisible())
		{
			win->UpdateMeasureWindow();
		}
	}
}

void VideoViewList::OnSceneRendered( QList<YT_Frame_Ptr> scene, unsigned int pts, bool seeking )
{
	for (int i=0; i<m_VideoList.size(); ++i) 
	{
		VideoView* vv = m_VideoList.at(i);
		vv->ClearLastFrame();
	}

	for (int i=0; i<scene.size(); i++)
	{
		YT_Frame_Ptr frame = scene.at(i);
		VideoView* vv = find(frame->Info(VIEW_ID).toUInt());
		if (vv)
		{
			vv->SetLastFrame(frame);
		}
	}

	m_CurrentPTS = pts;

	if (seeking)
	{
		// assert(pts == m_SeekingPTS);
		m_SeekingPTS = INVALID_PTS;

		if (m_SeekingPTS == INVALID_PTS && m_SeekingPTSNext != INVALID_PTS)
		{
			unsigned int pts = m_SeekingPTSNext;
			m_SeekingPTSNext = INVALID_PTS;

			Seek(pts, m_ProcessThread->IsPlaying());
		}
	}
}

VideoView* VideoViewList::find( unsigned int id ) const
{
	for (int i=0; i<m_VideoList.size(); ++i) 
	{
		VideoView* vv = m_VideoList.at(i);
		if (vv->GetID() == id)
		{
			return vv;
		}
	}	
	return NULL;
}
