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
#include <NotedPlugin/ComputeAnalysis.h>
#include <NotedPlugin/NotedComputeRegistrar.h>
#include "ExamplePlugin.h"
using namespace std;
using namespace lb;

NOTED_PLUGIN(ExamplePlugin);

template <class _EventsStores>
inline lb::StreamEvents mergedAt(_EventsStores const& _s, int _i)
{
	StreamEvents ses;
	foreach (EventsStore* es, _s)
		merge(ses, es->events(_i));
	return ses;
}

template <class _EventsStores>
inline lb::StreamEvents mergedAtCursor(_EventsStores const& _s, bool _force, bool _usePre, int _trackPos)
{
	StreamEvents ses;
	foreach (EventsStore* es, _s)
		if (_force)
			merge(ses, es->cursorEvents());
		else if (_usePre && es->isPredetermined())
			merge(ses, es->events(_trackPos));
		else if (!_usePre && !es->isPredetermined())
			merge(ses, es->cursorEvents());
	return ses;
}

class VizImpl
{
public:
	typedef VizImpl LIGHTBOX_PROPERTIES_BaseClass;

	virtual lb::MemberMap propertyMap() const { return lb::NullMemberMap; }
	virtual void onPropertiesChanged() {}

	virtual std::vector<ComputeTask> tasks() { return {}; }
	virtual void initializeGL() {}
	virtual void resizeGL(int, int) {}
	virtual void updateState(StreamEvents const&) {}
	virtual void paintGL() {}
};

class VizGLWidgetProxy: public QGLWidgetProxy
{
	friend class ExamplePlugin;

public:
	VizGLWidgetProxy(VizImpl* _v): m_viz(_v) {}

	void stop() { m_suspended = true; }
	void start() { m_suspended = false; }

	virtual void initializeGL()
	{
		m_last = wallTime();
		m_viz->initializeGL();
	}

	virtual void resizeGL(int _w, int _h)
	{
		m_viz->resizeGL(_w, _h);
	}

	virtual bool needsRepaint() const
	{
		if (m_lastCursor == NotedFace::audio()->cursorIndex() && !m_propertiesChanged)
			return false;

		if (NotedFace::audio()->cursorIndex() - m_lastCursor < 10)
			for (; m_lastCursor < NotedFace::audio()->cursorIndex(); m_lastCursor++)
				m_viz->updateState({});
				// TODO: merge events from Noted.
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
			NotedComputeRegistrar::get()->init();
		m_wasSuspended = false;

		NotedComputeRegistrar::get()->beginTime(NotedFace::audio()->hopCursor());

		m_viz->paintGL();

		NotedComputeRegistrar::get()->endTime(NotedFace::audio()->hopCursor());
		m_last = wallTime();
	}

	VizImpl* viz() const { return m_viz; }

private:
	Time m_last;
	mutable unsigned m_lastCursor = (unsigned)-1;
	mutable bool m_propertiesChanged = true;

	bool m_suspended = true;
	bool m_wasSuspended = true;

	VizImpl* m_viz;
};

class AnalyzeViz: public ComputeAnalysis
{
public:
	AnalyzeViz(VizGLWidgetProxy* _p): ComputeAnalysis(_p->viz()->tasks(), "Precomputing visualization"), m_p(_p) {}

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

class SplitBarsViz: public VizImpl
{
public:
	virtual vector<ComputeTask> tasks() { return { { tracks, [=](){ return tracks.info().axes(); }, {} }, { peaks, [=](){ return peaks.info().axes(); }, {} } }; }

	virtual void initializeGL()
	{
		glDisable(GL_DEPTH_TEST);
		glGenTextures(1, m_texture);
		for (int i = 0; i < 1; ++i)
		{
			glBindTexture(GL_TEXTURE_1D, m_texture[i]);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		float f[2] = { 0, 1 };
		glTexImage1D(GL_TEXTURE_1D, 0, 1, 2, 0, GL_LUMINANCE, GL_FLOAT, &f[0]);
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

	virtual void updateState(StreamEvents const& _ses)
	{
		(void)_ses;
	}

	virtual void paintGL()
	{
		auto peaks = this->tracks.get().toVector();
		sort(peaks.begin(), peaks.end(), [](FreqPeak a, FreqPeak b){return a.band < b.band;});
		float t = 0;
		for (auto p: peaks)
			t += p.mag;

		glLoadIdentity();
		glEnable(GL_TEXTURE_1D);
		glBindTexture(GL_TEXTURE_1D, m_texture[0]);
		glBegin(GL_QUADS);
		float bands = tracks.info().bands;
		float y = -1.f;
		if (t)
			for (auto p: peaks)
			{
				float f = p.band * .2;
				float ph = p.phase * sqrt(float(p.band) * .01);

				float yf = y;
				y += 2 * p.mag / t;

				glColor3ubv(Color(p.band / bands * .8 + .07, 1, 1).toRGBA8().data());

				glTexCoord1f(ph);
				glVertex3f(-1.0f, yf, 0.0f);
				glTexCoord1f(f + ph);
				glVertex3f( 1.0f, yf, 0.0f);
				glTexCoord1f(f + ph);
				glVertex3f( 1.0f, y, 0.0f);
				glTexCoord1f(ph);
				glVertex3f(-1.0f, y, 0.0f);
			}
		glEnd();
		glDisable(GL_TEXTURE_1D);
	}

private:
	PeakGather peaks = PeakGather(ExtractMagnitude(WindowedFourier(AccumulateWave(ComputeRegistrar::feeder()))), 16);
	PeakTrack tracks = PeakTrack(peaks, 16, 1.f);

	GLuint m_texture[1];
};

class AttractorViz: public VizImpl
{
public:
	AttractorViz()
	{
		for (int i = 0; i < m_initialIterations; i++)
			iterate();
	}

	virtual vector<ComputeTask> tasks() { return {}; }

	virtual void initializeGL()
	{
		glDisable(GL_DEPTH_TEST);
		glColor4f(1.0f, 1.0f, 1.0f, 0.02f);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_POINT_SMOOTH);
		glPointSize(1.0f);
	}

	virtual void resizeGL(int _w, int _h)
	{
		glViewport(0, 0, _w, _h);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-1, 1, 1, -1, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glScalef(.5f, .5f, 1.f);
	}

	virtual void updateState(StreamEvents const& _ses)
	{
		(void)_ses;
	}

	virtual void paintGL()
	{
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBegin(GL_POINTS);
		for (int i = 0; i < m_iterations; i++)
		{
			iterate();
			glVertex2f(x, y);
		}
		glEnd();
	}

private:
	float x = 0.1;
	float y = 0.1;
	float a = -0.966918;
	float b = 2.879879;
	float c = 0.765145;
	float d = 0.744728;

	void iterate()
	{
		float xnew = sin(y*b) + c*sin(x*b);
		float ynew = sin(x*a) + d*sin(y*a);
		x = xnew;
		y = ynew;
	}


	int	m_initialIterations = 100;
	int m_iterations = 100000;
};

class Coeff
{
	friend class Traj;
public:
	void randomize()
	{
		for (unsigned i = 0; i < 12; ++i)
			m_a[i] = rand() * 2.f / RAND_MAX - 1.f;
	}

	fVector2 applied(fVector2 _p) const
	{
		return fVector2(m_a[0] + _p.x() * (m_a[1] + m_a[2] * _p.x() + m_a[3] * _p.y()) + _p.y() * (m_a[4] + m_a[5] * _p.y()),
						m_a[6] + _p.x() * (m_a[7] + m_a[8] * _p.x() + m_a[9] * _p.y()) + _p.y() * (m_a[10] + m_a[11] * _p.y()));
	}

	float niceness() const
	{
		s_lp.resize(0);
		s_lp.reserve(30);
		float tol = 1.f / 1024;
		fVector2 p(0.01, 0.01);
		for (int i = 0; i < 100 && isFinite(p.x()) && isFinite(p.y()); i++)
			p = applied(p);
		for (int i = 0; i < 100 && isFinite(p.x()) && isFinite(p.y()) && tol < 1024; i++)
		{
			p = applied(p);
			for (auto& sp: s_lp)
				if (p.approximates(sp, tol))
					goto OK;
			s_lp.push_back(p);
			if (s_lp.size() > 4)
				tol *= 2;
			OK:;
		}
		return isFinite(p.x()) && isFinite(p.y()) ? tol <= 1 ? tol : 1 / tol : 0;
	}

protected:
	float m_a[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	static vector<fVector2> s_lp;
};

vector<fVector2> Coeff::s_lp;

class Traj: public Coeff
{
public:
	void randomize()
	{
		float t = 0;
		for (unsigned i = 0; i < 12; ++i)
		{
			m_a[i] = rand() * 2.f / RAND_MAX - 1.f;
			t += sqr(m_a[i]);
		}
		t = 1.f / sqrt(t);
		for (unsigned i = 0; i < 12; ++i)
			m_a[i] *= t;
	}
	Coeff applied(Coeff _c, float _q) const
	{
		Coeff ret;
		for (unsigned i = 0; i < 12; ++i)
			ret.m_a[i] = _c.m_a[i] + m_a[i] * _q;
		return ret;
	}
};

class QuadAttractorViz: public VizImpl
{
public:
	QuadAttractorViz()
	{
		m_current.randomize();
		m_currentDir.randomize();
	}

	virtual vector<ComputeTask> tasks() { return {}; }

	virtual void initializeGL()
	{
		glDisable(GL_DEPTH_TEST);
		glColor4f(1.0f, 1.0f, 1.0f, .1f);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_POINT_SMOOTH);
		glPointSize(1.0f);
	}

	virtual void resizeGL(int _w, int _h)
	{
		glViewport(0, 0, _w, _h);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-1, 1, 1, -1, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glScalef(.5f, .5f, 1.f);
	}

	virtual void updateState(StreamEvents const& _ses)
	{
		(void)_ses;

		Time dt = wallTime() - m_lt;
		if (dt > FromMsecs<33>::value || dt < 0)
		{
			float n = m_current.niceness();
			for (; n < 0.25; n = m_current.niceness())
				m_current.randomize();
			Traj rt;
			rt.randomize();

			Coeff c1 = m_currentDir.applied(m_current, 0.001);
			Coeff c2 = rt.applied(m_current, 0.001);

			if (c1.niceness() < c2.niceness())
				m_currentDir = rt;

			m_lastNiceness = n;
			m_lt = wallTime();
		}

		m_current = m_currentDir.applied(m_current, m_lastNiceness == .25f ? .01f : m_lastNiceness == .5f ? .001f : .00001f);
	}

	virtual void paintGL()
	{
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBegin(GL_POINTS);
		fVector2 pos(0.01, 0.01);
		for (int i = 0; i < m_iterations; i++)
		{
			pos = m_current.applied(pos);
			if (i > m_initialIterations)
				glVertex2f(pos.x(), pos.y());
		}
		glEnd();
	}

private:
	Coeff m_current;
	Traj m_currentDir;
	Time m_lt;
	float m_lastNiceness;

	int	m_initialIterations = 100;
	int m_iterations = 1000000;
};


class WaveBarsViz: public VizImpl
{
public:
	virtual vector<ComputeTask> tasks() { return { mag, deltaPhase }; }

	virtual void initializeGL()
	{
		glDisable(GL_DEPTH_TEST);
		glGenTextures(1, m_texture);
		for (int i = 0; i < 1; ++i)
		{
			glBindTexture(GL_TEXTURE_1D, m_texture[i]);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
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

	virtual void updateState(StreamEvents const& _ses)
	{
		(void)_ses;
	}

	virtual void paintGL()
	{
		glEnable(GL_TEXTURE_1D);
		glLoadIdentity();
		auto w = ComputeRegistrar::feeder().get();
		glBindTexture(GL_TEXTURE_1D, m_texture[0]);
		glPixelTransferf(GL_RED_SCALE, scale);
		glPixelTransferf(GL_GREEN_SCALE, scale);
		glPixelTransferf(GL_BLUE_SCALE, scale);
		glPixelTransferf(GL_RED_BIAS, bias);
		glPixelTransferf(GL_GREEN_BIAS, bias);
		glPixelTransferf(GL_BLUE_BIAS, bias);
		glTexImage1D(GL_TEXTURE_1D, 0, 1, w.size(), 0, GL_LUMINANCE, GL_FLOAT, w.data());

		auto p = deltaPhase.get();
		auto s = mag.get();
		unsigned i = maxInRange(s.begin(), s.end()) - s.begin();
		float x = p[i];
		x = min(x, fPi - x);
		glColor3ubv(Color(x / fHalfPi, 1, 1).toRGBA8().data());

		glBegin(GL_QUADS);
		glTexCoord1f(0.f);
		glVertex3f(-1.0f, -1.0f, 0.0f);
		glTexCoord1f(1.f);
		glVertex3f( 1.0f, -1.0f, 0.0f);
		glTexCoord1f(1.f);
		glVertex3f( 1.0f, 1.0f, 0.0f);
		glTexCoord1f(0.f);
		glVertex3f(-1.0f, 1.0f, 0.0f);
		glEnd();
		glDisable(GL_TEXTURE_1D);
	}

	float scale = 2.f;
	float bias = 0.5f;

	LIGHTBOX_PROPERTIES(scale, bias);

private:
	ExtractMagnitude mag = ExtractMagnitude(WindowedFourier(AccumulateWave(ComputeRegistrar::feeder())));
	CycleDelta deltaPhase = CycleDelta(ExtractPhase(WindowedFourier(AccumulateWave(ComputeRegistrar::feeder()))));

	GLuint m_texture[1];
};

ExamplePlugin::ExamplePlugin()
{
	m_glView = new VizGLWidgetProxy(new QuadAttractorViz);
	m_vizDock = NotedFace::get()->addGLView(m_glView);
	m_analysis = CausalAnalysisPtr(new AnalyzeViz(m_glView));
	NotedFace::compute()->registerJobSource(this);
}

ExamplePlugin::~ExamplePlugin()
{
	NotedFace::compute()->unregisterJobSource(this);
	delete m_vizDock;
}

CausalAnalysisPtrs ExamplePlugin::ripeAnalysis(AcausalAnalysisPtr const& _finished)
{
	if (_finished == NotedFace::audio()->resampleWaveAcAnalysis())
		return { m_analysis };
	return {};
}

lb::MemberMap ExamplePlugin::propertyMap() const
{
	MemberMap mm = m_glView->viz()->propertyMap();
	for (auto& m: mm)
		m.second.offset += (intptr_t)m_glView->viz() - (intptr_t)this;
	return mm;
}

void ExamplePlugin::onPropertiesChanged()
{
	m_glView->viz()->onPropertiesChanged();
}
