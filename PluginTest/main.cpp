/* BEGIN COPYRIGHT
 *
 * This file is part of Noted.
 *
 * Copyright ©2011, 2012, Lancaster Logic Response Limited.
 *
 * Noted is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Noted is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Noted.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtGui>
#include <QtCore>
#include <QtOpenGL>
#include <fstream>
#include <boost/thread.hpp>
#include <boost/array.hpp>
#include <boost/system/system_error.hpp>
#include <cassert>
#include <Common/Common.h>
#include <NotedPlugin/NotedPlugin.h>
#include <NotedPlugin/NotedFace.h>

using namespace std;
using namespace Lightbox;

class NotedGLWidget: public QGLWidget
{
public:
	NotedGLWidget(QGLWidgetProxy* _v, QWidget* _p): QGLWidget(_p), m_v(_v) {}
    virtual ~NotedGLWidget() { delete m_v; }

	virtual void initializeGL() { m_v->initializeGL(); }
	virtual void resizeGL(int _w, int _h) { m_v->resizeGL(_w, _h); }
	virtual void paintGL() { m_v->paintGL(); }

	virtual void mousePressEvent(QMouseEvent* _e) { m_v->mousePressEvent(_e); }
	virtual void mouseReleaseEvent(QMouseEvent* _e) { m_v->mouseReleaseEvent(_e); }
	virtual void mouseMoveEvent(QMouseEvent* _e) { m_v->mouseMoveEvent(_e); }
	virtual void wheelEvent(QWheelEvent* _e) { m_v->wheelEvent(_e); }

private:
	QGLWidgetProxy* m_v;
};

class App: public DummyNoted
{
	Q_OBJECT

public:
	App(char const* _file)
	{
		QVBoxLayout* l = new QVBoxLayout;
		QFrame* fr = new QFrame(this);
		new QTextBrowser(fr);
		l->addWidget(fr);
		pb = new QPushButton(this);
		l->addWidget(pb);
		name = _file;
		pb->setText(_file);
		pb->setCheckable(true);
		connect(pb, SIGNAL(clicked()), SLOT(onClicked()));
		setLayout(l);
	}
	~App()
	{
		pb->setChecked(false);
	}

	QWidget* addGLWidget(QGLWidgetProxy* _v, QWidget* _p = nullptr)
	{
		return new NotedGLWidget(_v, _p);
	}

public slots:
	void onClicked()
	{
		if (pb->isChecked())
		{
			QString tempFile = QDir::tempPath() + "/PluginTest-" + QDateTime::currentDateTime().toString("yyyy-MM-dd.hh-mm-ss.zzz");
			l.setFileName(tempFile);
			l.setLoadHints(QLibrary::ResolveAllSymbolsHint);
			QFile::copy(name, tempFile);
			if (l.load())
			{
				typedef NotedPlugin*(*pf_t)(NotedFace*);
				if (pf_t np = (pf_t)l.resolve("newPlugin"))
				{
					qDebug() << "LOAD" << name << " [PLUGIN]";
					p = shared_ptr<NotedPlugin>(np(this));
				}
			}
			else
				pb->setChecked(false);
		}
		else
		{
			qDebug() << "UNLOAD" << name << " [PLUGIN]";
			p.reset();
			bool isFinilized = false;
			bool** fed = (bool**)l.resolve("g_lightboxFinilized");
			assert(fed);
			*fed = &isFinilized;
			assert(l.unload());
			assert(isFinilized);
			QFile::remove(l.fileName());
		}
	}

private:
	QString name;
	QLibrary l;
	std::shared_ptr<NotedPlugin> p;
	QPushButton* pb;
};

#include "main.moc"

int main(int argc, char** argv)
{
	QApplication a(argc, argv);
	App app(argc > 1 ? argv[1] : "./libTestPlugin.so");
	app.show();
	return a.exec();
}

