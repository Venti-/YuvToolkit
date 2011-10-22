#include "YT_Interface.h"
#include "YT_InterfaceImpl.h"
#include "MainWindow.h"

#include <stdint.h>
#include <string.h>

HostImpl* GetHostImpl()
{
	return (HostImpl*)GetHost();
}

FormatImpl::FormatImpl() : color(NODATA), width(0), height(0), format_changed(true)
{
	memset(stride, 0, sizeof(stride));
	memset(plane_size, 0, sizeof(plane_size));
}

FormatImpl::~FormatImpl()
{
}


COLOR_FORMAT FormatImpl::Color() const
{
	return color;
}

void FormatImpl::SetColor( COLOR_FORMAT value )
{
	color = value;
	format_changed = true;

	name[0][0] = name[1][0] = name[2][0] = name[3][0] = '\0';

	switch(color)
	{
	case I420:
	case IYUV:
		strcpy(name[0], "Y");
		strcpy(name[1], "U");
		strcpy(name[2], "V");
		break;
	case YV12:
		strcpy(name[0], "Y");
		strcpy(name[1], "V");
		strcpy(name[2], "U");
		break;
	case NV12:
		strcpy(name[0], "Y");
		strcpy(name[1], "UV");
		break;
	case YUY2:
	case UYVY:
	case NODATA:
	case GRAYSCALE8:
	case RGB24:
	case RGBX32:
	case XRGB32:
		break;
	}
}

int FormatImpl::Width() const
{
	return width;
}

void FormatImpl::SetWidth( int value )
{
	width = value;
	format_changed = true;
}

int FormatImpl::Height() const
{
	return height;
}

void FormatImpl::SetHeight( int value )
{
	height = value;
	format_changed = true;
}

int FormatImpl::Stride( int plane ) const
{
	if (plane>=0 && plane<4)
	{
		return stride[plane];
	}else
	{
		return 0;
	}
}

void FormatImpl::SetStride( int plane, int value )
{
	if (plane>=0 && plane<4)
	{
		stride[plane] = value;
		format_changed = true;
	}
}

int* FormatImpl::Stride() const
{
	return (int*)stride;
}


size_t FormatImpl::PlaneSize( int plane )
{
	if (format_changed)
	{
		if (stride[0] == 0)
		{
			stride[0] = stride[1] = stride[2] = stride[3] = 0;
			switch(color)
			{
			case I420:
			case IYUV:
			case YV12:
				stride[0] = width;
				stride[1] = width/2;
				stride[2] = width/2;
				break;
			case NV12:
				stride[0] = width;
				stride[1] = width;
				break;
			case YUY2:
			case UYVY:
				stride[0] = width*2;
				break;
			case RGB24:
				stride[0] = width*3;
				break;
			case XRGB32:
				stride[0] = width*4;
				break;
			case RGBX32:
				stride[0] = width*4;
				break;
			case GRAYSCALE8:
				stride[0] = width;
				break;
			case NODATA:
				break;
			}
		}

		int vstride[4] = {height, 0, 0, 0};
		switch(color)
		{
		case I420:
		case IYUV:
		case YV12:
		case NV12:
			vstride[1] = height/2;
			vstride[2] = height/2;
			break;
		case NODATA:
		case GRAYSCALE8:
		case RGB24:
		case RGBX32:
		case XRGB32:
		case YUY2:
		case UYVY:
			break;
		}

		size_t data_length = 0;
		for (int i=0; i<4; i++)
		{
			if (stride[i] != 0)
			{
				plane_size[i] = stride[i]*vstride[i];
			}
		}

		format_changed = false;
	}

	if (plane>=0 && plane<4)
	{
		return plane_size[plane];
	}else
	{
		return 0;
	}
}

bool FormatImpl::operator==( const Format &f )
{

	return (this->color == f.Color()) && (this->width == f.Width()) &&
		(this->height == f.Height()) && (this->stride[0] == f.Stride(0)) &&
		(this->stride[1] == f.Stride(1)) && (this->stride[2] == f.Stride(2)) && 
		(this->stride[3] == f.Stride(3));
}

bool FormatImpl::operator!=( const Format &f )
{
	return !(*this == f);
}

Format& FormatImpl::operator=( const Format &f )
{
	this->SetColor(f.Color()); 
	this->width = f.Width();
	this->height = f.Height();
	this->stride[0] = f.Stride(0);
	this->stride[1] = f.Stride(1);
	this->stride[2] = f.Stride(2); 
	this->stride[3] = f.Stride(3);
	this->format_changed = true;

	return *this;
}

Format& FormatImpl::operator=( Format &f )
{
	*this = (const Format &)f;

	return *this;
}

bool FormatImpl::IsPlanar( int plane )
{
	switch(color)
	{
	case I420:
	case IYUV:
	case YV12:
		return plane<=2;
	case NV12:
		return plane<=0;
	case YUY2:
	case UYVY:
		return false;
	}
	return false;
}

const char* FormatImpl::PlaneName( int plane )
{
	if (!IsPlanar(plane))
	{
		return NULL;
	}
	return name[plane];
}

int FormatImpl::PlaneWidth( int plane )
{
	if (!IsPlanar(plane))
	{
		return 0;
	}

	switch(color)
	{
	case I420:
	case IYUV:
	case YV12:
	case NV12:
		if (plane == 0)
		{
			return width;
		}else
		{
			return width/2;
		}
	}
	return 0;
}

int FormatImpl::PlaneHeight( int plane )
{
	if (!IsPlanar(plane))
	{
		return 0;
	}

	switch(color)
	{
	case I420:
	case IYUV:
	case YV12:
	case NV12:
		if (plane == 0)
		{
			return height;
		}else
		{
			return height/2;
		}
	}
	return 0;
}

FrameImpl::FrameImpl(FramePool* p) : pool(p), pts(0), frame_num(0), externData(0),
	allocated_data(0), allocated_size(0), 
	format(new FormatImpl)
{
	memset(data, 0, sizeof(data));
}

FrameImpl::~FrameImpl()
{
	Deallocate();

	format.clear();
}

void FrameImpl::Deallocate()
{
	if (allocated_data)
	{
		delete []allocated_data;

		memset(data, 0, sizeof(data));

		allocated_data = 0;
		allocated_size = 0;
	}
}

FormatPtr FrameImpl::Format()
{
	return format;
}


void FrameImpl::SetFormat( const FormatPtr f)
{
	*format = *f;
}

const FormatPtr FrameImpl::Format() const
{
	return format;
}
unsigned char* FrameImpl::Data( int plane ) const
{
	if (plane>=0 && plane<4)
	{
		return data[plane];
	}else
	{
		return 0;
	}
}

void FrameImpl::SetData( int plane, unsigned char* value )
{
	// Set external buffer
	if (plane>=0 && plane<4)
	{
		if (allocated_data)
		{
			Deallocate();
		}

		data[plane] = value;
	}
}

unsigned char** FrameImpl::Data() const
{
	return (unsigned char**)data;
}

unsigned int FrameImpl::PTS() const
{
	return pts;
}

void FrameImpl::SetPTS( unsigned int value)
{
	pts = value;
}

unsigned int FrameImpl::FrameNumber() const
{
	return frame_num;
}

void FrameImpl::SetFrameNumber( unsigned int value )
{
	frame_num = value;
}

#define DEFAULT_ALIGNMENT	32
RESULT FrameImpl::Allocate()
{
	size_t data_length = format->PlaneSize(0)+format->PlaneSize(1)+
		format->PlaneSize(2)+format->PlaneSize(3);

	size_t align_margin = DEFAULT_ALIGNMENT*4;

	if (data_length+align_margin>allocated_size)
	{
		if (allocated_data)
		{
			Deallocate();
		}

		allocated_data = new unsigned char[data_length+align_margin];
		allocated_size = data_length+align_margin;
	}

	unsigned char* d = allocated_data;
	for (int i=0; i<4; i++)
	{
		while (((unsigned long)d) & (DEFAULT_ALIGNMENT-1))
			d ++;
		data[i] = d;
	
		d += format->PlaneSize(i);
	}

	return OK;
}

RESULT FrameImpl::Reset()
{
	Deallocate();

	for (int i=0; i<4; i++)
	{
		data[i] = 0;
	}

	externData = NULL;

	return OK;
}

void FrameImpl::SetExternData( void* data )
{
	externData = data;
}

void* FrameImpl::ExternData() const
{
	return externData;
}

void FrameImpl::Recyle( Frame *obj )
{
	FrameImpl* frame = static_cast<FrameImpl*> (obj);
	if (frame->pool)
	{
		frame->pool->Recycle(frame);
	}else
	{
		delete obj;
	}
}

QVariant FrameImpl::Info( INFO_KEY key) const
{
	return info.value(key);
}

void FrameImpl::SetInfo( INFO_KEY key, QVariant value)
{
	info.insert(key, value);
}

bool FrameImpl::HasInfo( INFO_KEY key ) const
{
	return info.contains(key);
}

FramePtr HostImpl::NewFrame()
{
	return FramePtr(new FrameImpl);
}

FormatPtr HostImpl::NewFormat()
{
	return FormatPtr(new FormatImpl);
}

HostImpl::HostImpl() : m_LogFile(this)
{
	QCoreApplication::setOrganizationName("YUVToolkit");
	QCoreApplication::setOrganizationDomain("YUVToolkit");
	QCoreApplication::setApplicationName("YUVToolkit");


	QDir pluginsDir(qApp->applicationDirPath());
	QStringList files;
#if defined(Q_WS_WIN)
	files.append(QString("YT*.dll"));
#elif defined(Q_WS_MACX)
	files.append(QString("libYT*.dylib"));
#elif defined(Q_OS_LINUX)
	files.append(QString("libYT*.so"));
#else
#	error Unsupported platform
#endif
	foreach (QString fileName, pluginsDir.entryList(files, QDir::Files)) {
		QPluginLoader loader(pluginsDir.absoluteFilePath(fileName));

		QObject *plugin = loader.instance();

		QString err(loader.errorString());

		YTPlugIn* ytPlugin = qobject_cast<YTPlugIn*>(plugin);

		if (ytPlugin)
		{
			if (ytPlugin->Init(this) == OK)
			{
				m_PlugInList.append(ytPlugin);
			}
		}
	}
}

RESULT HostImpl::RegisterPlugin( YTPlugIn* plugin, PLUGIN_TYPE type, const QString& name )
{
	PlugInInfo* info = new PlugInInfo;

	info->plugin = plugin;
	info->string = name;
	info->type = type;

	switch (type)
	{
	case PLUGIN_SOURCE:
		m_SourceList.append(info);
		break;
	case PLUGIN_RENDERER:
		m_RendererList.append(info);
		break;
	case PLUGIN_TRANSFORM:
		m_TransformList.append(info);
		break;
	case PLUGIN_MEASURE:
		m_MeasureList.append(info);
		break;
	case PLUGIN_UNKNOWN:
		break;
	}

	return OK;
}

QMainWindow* HostImpl::NewMainWindow(int argc, char *argv[])
{
	MainWindow* win = new MainWindow;

	if (argc == 2)
	{
		QStringList fileList;
		fileList.append(argv[1]);
        win->openFiles(fileList);
		win->EnableButtons(true);
	}

	m_MainWindowList.append(win);

	return win;
}

HostImpl::~HostImpl()
{
	while (!m_MainWindowList.isEmpty())
	{
		delete m_MainWindowList.takeFirst();
	}
}
/*
Source* HostImpl::NewSource( const QString file_ext )
{
	PlugInInfo* info = m_SourceList.first();

	return info->plugin->NewSource(info->string);
}

void HostImpl::ReleaseSource( Source* )
{

}
*/
YTPlugIn* HostImpl::FindSourcePlugin( const QString file_ext )
{
	PlugInInfo* info = m_SourceList.first();

	return info->plugin;
}

YTPlugIn* HostImpl::FindRenderPlugin( const QString& type )
{
	for (int i = 0; i < m_RendererList.size(); ++i) 
	{
		PlugInInfo* info = m_RendererList.at(i);
		if (info->string == type)
		{
			return info->plugin;
		}
	}

	if (m_RendererList.size()>0)
	{
		return m_RendererList.at(0)->plugin;
	}

	return NULL;
}

void HostImpl::Logging(void* ptr,  LOGGING_LEVELS level, const char* fmt, ... )
{
	if (m_LoggingEnabled)
	{
		QMutexLocker locker(&m_MutexLogging);
		if (m_LogFile.isOpen())
		{
			QString msg;
	
			va_list list;
			va_start(list, fmt);
			msg.vsprintf(fmt, list);

			QTextStream stream(&m_LogFile);

			stream << QTime::currentTime().toString();
			switch (level)	 
			{
				case LL_INFO:
					stream << " I ";
					break;
				case LL_DEBUG:
					stream << " D ";
					break;
				case LL_WARNING:
					stream << " W ";
					break;
				case LL_ERROR:
					stream << " E ";
					break;
			}

			showbase(hex(stream)) << (uintptr_t)ptr;
			stream << " " << msg << "\n";
		}
	}
}

void HostImpl::InitLogging()
{
	QSettings ini(QSettings::IniFormat, QSettings::UserScope,
		QCoreApplication::organizationName(),
		QCoreApplication::applicationName());
	QString dir = QFileInfo(ini.fileName()).absolutePath();

	if (!QDir().exists(dir))
	{
		QDir().mkdir(dir);
	}
	
	QSettings settings;
	if (QDir().exists(dir) && settings.value("main/logging", false).toBool())
	{
		// Clean up old log files
		QDir qdir(dir);
		qdir.setFilter(QDir::Files | QDir::NoSymLinks);
		qdir.setSorting(QDir::Time | QDir::Reversed);

		QFileInfoList list = qdir.entryInfoList(QStringList("YUVToolkit-[1-9][0-9][0-1][0-9][0-3][0-9]-[0-2][0-9][0-6][0-9][0-6][0-9].log"));
		for (int i = 0; i <= list.size()-3; ++i) 
		{
			// Keep 3 logs
			QFileInfo fileInfo = list.at(i);
			QFile::remove(fileInfo.filePath());
		}

		// Create new 
		QString filename;

		QTextStream stream(&filename); 
		stream << dir << "/" << "YUVToolkit-";
		stream << QDateTime::currentDateTime().toString("yyMMdd-hhmmss");
		stream << ".log";

		m_LogFile.setFileName(filename);
		m_LogFile.open(QIODevice::WriteOnly | QIODevice::Text);

		INFO_LOG("YUVToolkit Compiled %s %s", __DATE__, __TIME__);
	}

	m_LoggingEnabled = m_LogFile.isOpen();
}

void HostImpl::UnInitLogging()
{
	if (m_LogFile.isOpen())
	{
		m_LogFile.close();
	}
}

void HostImpl::EnableLogging( bool enable )
{
	QSettings settings;
	settings.setValue("main/logging", enable);

	UnInitLogging();

	if (enable)
	{
		InitLogging();
	}
}

bool HostImpl::IsLoggingEnabled()
{
	return m_LoggingEnabled && m_LogFile.isOpen();
}

void HostImpl::OpenLoggingDirectory()
{
	QSettings ini(QSettings::IniFormat, QSettings::UserScope,
		QCoreApplication::organizationName(),
		QCoreApplication::applicationName());
	QString path = QFileInfo(ini.fileName()).absolutePath();

	QDesktopServices::openUrl(QUrl("file:///" + path));
}

FramePool* HostImpl::NewFramePool(unsigned int size)
{
	FramePool* pool = new FramePool(size);
	m_FramePoolList.append(pool);
	return pool;
}

void HostImpl::ReleaseFramePool(FramePool* pool)
{
	m_FramePoolListGC.append(pool);
	m_FramePoolList.removeAll(pool);

	/*
	// while (m_FramePool.Size() != BUFFER_COUNT)
	if (m_FramePool.Size() != BUFFER_COUNT)
	{
		WARNING_LOG("Waiting for frame pool to uninitialize (%s)... ", m_Path.toAscii());
		QThread::sleep(1);
	}*/
}


FramePool::FramePool( unsigned int size )
{
	for (unsigned int i=0; i<size; i++)
	{
		m_Pool.append(new FrameImpl(this));
	}
}

FramePool::~FramePool()
{
	for (unsigned int i=0; i<m_Pool.size(); i++)
	{
		delete m_Pool.at(i);
	}
}

void FramePool::Recycle( Frame* frame )
{
	QMutexLocker locker(&m_Mutex);

	m_Pool.append(frame);
}

FramePtr FramePool::Get()
{
	if (m_Pool.size())
	{
		QMutexLocker locker(&m_Mutex);

		Frame* frame = m_Pool.first();
		m_Pool.removeFirst();

		return FramePtr(frame, FrameImpl::Recyle);
	}

	return FramePtr(NULL);
}

int FramePool::Size()
{
	return m_Pool.size();
}
