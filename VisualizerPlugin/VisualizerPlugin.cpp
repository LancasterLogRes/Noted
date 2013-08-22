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

#include <array>
#include <vector>
#include <utility>
#include <iostream>
#include <QtGui>
#include <QtWidgets>
#ifdef Q_OS_MAC
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#include <Compute/All.h>
#include <Viz/All.h>
#include <NotedPlugin/ComputeAnalysis.h>
#include <NotedPlugin/NotedComputeRegistrar.h>
#include "VisualizerPlugin.h"
using namespace std;
using namespace lb;

NOTED_PLUGIN(VisualizerPlugin);

class VizGLWidgetProxy: public QGLWidgetProxy
{
	friend class VisualizerPlugin;

public:
	VizGLWidgetProxy(Viz const& _v);

	void stop() { m_suspended = true; }
	void start() { m_suspended = false; if (!m_wasSuspended) m_reset = true; }

	CausalAnalysisPtr analysis() const { return m_analysis; }

	virtual void initializeGL()
	{
		m_last = wallTime();
		m_viz.initializeGL();
	}

	virtual void resizeGL(int _w, int _h)
	{
		glViewport(0, 0, _w, _h);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-1, 1, 1, -1, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}

	virtual bool needsRepaint() const
	{
		if (m_lastCursor == NotedFace::audio()->cursorIndex() && !m_propertiesChanged)
			return false;

		m_lastCursor = NotedFace::audio()->cursorIndex();
		m_propertiesChanged = false;
		return true;
	}

	virtual void paintGL()
	{
		if (wallTime() - m_last < 16 * msecs)
			return;

		if (m_suspended)
		{
			if (!m_wasSuspended)
				NotedComputeRegistrar::get()->fini();
			m_wasSuspended = true;
			return;
		}
		if (m_wasSuspended)
		{
			NotedComputeRegistrar::get()->init();
			m_wasSuspended = false;
		}
		if (m_reset)
		{
			NotedComputeRegistrar::get()->fini();
			NotedComputeRegistrar::get()->init();
			m_reset = false;
		}

		NotedComputeRegistrar::get()->beginTime(NotedFace::audio()->hopCursor());

		m_viz.paintGL(wallTime() - m_last);

		NotedComputeRegistrar::get()->endTime(NotedFace::audio()->hopCursor());
		m_last = wallTime();
	}

	Viz const& viz() const { return m_viz; }

private:
	Time m_last;
	mutable unsigned m_lastCursor = (unsigned)-1;
	mutable bool m_propertiesChanged = true;

	bool m_suspended = true;
	bool m_wasSuspended = true;
	bool m_reset = false;

	Viz m_viz;
	CausalAnalysisPtr m_analysis;
};

class AnalyzeViz: public ComputeAnalysis
{
public:
	AnalyzeViz(VizGLWidgetProxy* _p): ComputeAnalysis(_p->viz().tasks(), "Precomputing visualization"), m_p(_p) {}

	bool init(bool _r)
	{
		m_p->stop();
		return ComputeAnalysis::init(_r);
	}
	void fini(bool _c, bool _r)
	{
		ComputeAnalysis::fini(_c, _r);
		m_p->start();
		m_p->update();
	}

private:
	VizGLWidgetProxy* m_p;
};

VizGLWidgetProxy::VizGLWidgetProxy(Viz const& _v): m_viz(_v), m_analysis(new AnalyzeViz(this)) {}

VisualizerPlugin::VisualizerPlugin()
{
	NotedFace::compute()->registerJobSource(this);

	m_availableDock = new QDockWidget("Available Visualizations", NotedFace::get());
	QListView* availableView = new QListView;
	m_availableDock->setWidget(availableView);
	m_availableDock->setFeatures(QDockWidget::DockWidgetVerticalTitleBar);
	availableView->setModel(&m_availableModel);
	availableView->setEditTriggers(QListView::NoEditTriggers);
	connect(availableView, &QListView::activated, [&](QModelIndex i)
	{
		if (Viz v = createViz(m_availableModel.data(i, Qt::DisplayRole).toString().toStdString()))
		{
			auto p = new VizGLWidgetProxy(v);
			NotedFace::get()->addGLView(p);
			m_active.push_back(p);
			NotedFace::compute()->invalidate(NotedFace::events()->collateEventsAnalysis());
		}
	});
	NotedFace::get()->addDockWidget(Qt::BottomDockWidgetArea, m_availableDock);
}

VisualizerPlugin::~VisualizerPlugin()
{
	NotedFace::compute()->unregisterJobSource(this);
	delete m_availableDock;
	for (auto i: m_active)
		delete i;
}

CausalAnalysisPtrs VisualizerPlugin::ripeAnalysis(AcausalAnalysisPtr const& _finished)
{
	if (_finished == NotedFace::events()->collateEventsAnalysis())
	{
		CausalAnalysisPtrs ret;
		for (auto w: m_active)
			ret.push_back(w->analysis());
		return ret;
	}
	return {};
}
/*
lb::MemberMap VisualizerPlugin::propertyMap() const
{
	MemberMap mm = m_glView->viz()->propertyMap();
	for (auto& m: mm)
		m.second.offset += (intptr_t)m_glView->viz() - (intptr_t)this;
	return mm;
}

void VisualizerPlugin::onPropertiesChanged()
{
	m_glView->viz()->onPropertiesChanged();
}
*/

Viz VisualizerPlugin::createViz(string const& _name)
{
	for (auto al: auxLibraries())
		if (auto dl = static_pointer_cast<AuxLibrary>(al.lock()))
			if (dl->m_vf.find(_name) != dl->m_vf.end())
				return Viz::create(dl->m_vf[_name]());
	return Viz();
}

void VisualizerPlugin::rerealize()
{
	auto active = m_active;
	m_active.clear();
	for (auto a: active)
	{
		if (Viz v = createViz(a->viz().name()))
		{
			auto p = new VizGLWidgetProxy(v);
			NotedFace::get()->addGLView(p);
			m_active.push_back(p);
			NotedFace::compute()->invalidate(p->analysis());
		}
		delete a->widget();	// deletes a along with it.
	}
}

bool AuxLibrary::load(LibraryPtr const& _p)
{
	QLibrary& l = _p->library;

	bool changed = false;

	typedef VizFactories&(*df_t)();
	if (df_t vfHook = (df_t)l.resolve(("vizFactories" + _p->nick).toLatin1().data()))
	{
		m_vf = vfHook();
		for (auto f: m_vf)
		{
			QString dn = QString::fromStdString(f.first);
			QStringList sl = m_p->m_availableModel.stringList();
			sl.push_back(dn);
			m_p->m_availableModel.setStringList(sl);
			changed = true;
		}
	}

	qDebug() << m_vf.size() << " viz factories.";

	if (changed)
		m_p->rerealize();
	return m_vf.size();
}

void AuxLibrary::unload(LibraryPtr const&)
{
	bool changed = false;
	for (auto i: m_vf)
	{
		QString vn = QString::fromStdString(i.first);

		QStringList sl = m_p->m_availableModel.stringList();
		sl.removeOne(vn);
		m_p->m_availableModel.setStringList(sl);

		changed = true;
	}
	m_vf.clear();

	if (changed)
		m_p->rerealize();
}
