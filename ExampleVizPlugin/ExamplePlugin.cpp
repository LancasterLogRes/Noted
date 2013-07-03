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

float lext(float _a, float _b)
{
	return _b / (_a + _b);
}

float llerp(float _x, float _a, float _b)
{
	return pow(_a, 1.f - _x) * pow(_b, _x);
}

class PeakGatherImpl: public ComputeImplBase<SpectrumInfo, Peak<> >
{
public:
	PeakGatherImpl(Compute<float, SpectrumInfo> const& _input, unsigned _count): ComputeImplBase(input, count), input(_input), count(_count) {}
	virtual ~PeakGatherImpl() {}

	virtual char const* name() const { return "PeakGather"; }
	virtual SpectrumInfo info() { return input.info(); }
	virtual void init()
	{
	}

	virtual void compute(std::vector<Peak<>>& _v)
	{
		_v.resize(count);
		auto m = input.get();
		unsigned peakCount = topPeaks(m, foreign_vector<Peak<>>(&_v));
		_v.resize(peakCount);
	}

	Compute<float, SpectrumInfo> input;
	unsigned count;
};

using PeakGather = ComputeBase<PeakGatherImpl>;

struct PeakState: public FreqPeak
{
	float speed(float _gravity) const { return _gravity / mag; }
	void move(float _b, float _gravity)
	{
		band += sign(_b - band) * min(speed(_gravity), fabs(_b - band));
		assert(band >= 0);
	}
	void attract(Peak<> _p, float _gravity)
	{
		float db = _p.band - band;
		band += sign(db) * min(speed(_gravity) * _p.mag, fabs(db));
		assert(band > 0);
		mag += 1.f / (fabs(db) + 1) * (_p.mag - mag);
	}
	float fit(Peak<> _p) const { return fabs(_p.band - band) * fabs(_p.mag - mag); }
	bool operator<(PeakState _s) const { return band < _s.band; }
};

class PeakTrackImpl: public ComputeImplBase<SpectrumInfo, FreqPeak>
{
public:
	PeakTrackImpl(Compute<Peak<>, SpectrumInfo> const& _input, unsigned _count, float _gravity): ComputeImplBase(input, count, gravity), input(_input), count(_count), gravity(_gravity) {}
	virtual ~PeakTrackImpl() {}

	virtual char const* name() const { return "PeakTrackv2"; }
	virtual SpectrumInfo info() { return input.info(); }
	virtual void init()
	{
		m_peaks.resize(count);
		for (auto& p: m_peaks)
			p.band = rand() * input.info().bands / RAND_MAX;
		sort(m_peaks.begin(), m_peaks.end());
	}

	virtual void compute(std::vector<FreqPeak>& _v)
	{
		_v.resize(count);

		foreign_vector<Peak<>> peaks = input.get();

		for (int pi = peaks.size() - 1; pi >= 0; --pi)
		{
			int bestSi = -1;
			PeakState bestS;
			float best;

			Peak<> p = peaks[pi];
			for (unsigned si = 0; si < m_peaks.size(); ++si)
			{
				PeakState s = m_peaks[si];
				// Attract s to peaks[pi]
				s.attract(p, gravity);
				float f = s.fit(p);
				if (bestSi == -1 || f < best)
				{
					bestSi = si;
					bestS = s;
					best = f;
				}
			}
			m_peaks[bestSi] = bestS;
		}

		for (auto it = m_peaks.begin(); it != m_peaks.end(); it++)
		{
			/*if (it == m_peaks.begin())
			{
				auto np = *next(it);
				it->move(llerp(lext(it->mag, np.mag), 0.f, np.band), gravity);
			}
			else if (it == prev(m_peaks.end()))
			{
				auto pp = *prev(it);
				it->move(llerp(lext(pp.mag, it->mag), pp.band, (float)m.size()), gravity);
			}
			else
			{
				auto pp = *prev(it);
				auto np = *next(it);
				it->move(llerp(1.f - lext(pp.mag, np.mag), pp.band, np.band), gravity);
			}*/

			it->phase += 0.1;
			it->mag *= 0.95;
		}
		valcpy(_v.data(), (FreqPeak*)m_peaks.data(), count);
	}

	Compute<Peak<>, SpectrumInfo> input;
	unsigned count;
	float gravity;

private:
	vector<PeakState> m_peaks;
};

using PeakTrack = ComputeBase<PeakTrackImpl>;

class GLView: public QGLWidgetProxy
{
	friend class ExamplePlugin;

public:
	GLView(ExamplePlugin* _p): m_p(_p) {}

	virtual vector<ComputeTask> tasks() { return { { tracks, [=](){ return tracks.info().axes(); }, {} }, { peaks, [=](){ return peaks.info().axes(); }, {} } }; }

	virtual void initializeGL()
	{
		m_last = wallTime();
		glDisable(GL_DEPTH_TEST);
		glGenTextures (1, m_texture);
		for (int i = 0; i < 1; ++i)
		{
			glBindTexture (GL_TEXTURE_1D, m_texture[i]);
			glTexParameteri (GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri (GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
		glTexParameteri (GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri (GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		float f[2] = { 0, 1 };
		glTexImage1D(GL_TEXTURE_1D, 0, 1, 2, 0, GL_LUMINANCE, GL_FLOAT, &f[0]);
/*
		// compute some initial iterations to settle into the orbit of the attractor
		for (int i = 0; i < initialIterations; i++) {

			// compute a new point using the strange attractor equations
			float xnew = sin(y*b) + c*sin(x*b);
			float ynew = sin(x*a) + d*sin(y*a);

			// save the new point
			x = xnew;
			y = ynew;
		}
		// set the foreground (pen) color
		glColor4f(1.0f, 1.0f, 1.0f, 0.02f);

		// enable blending
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// enable point smoothing
		glEnable(GL_POINT_SMOOTH);
		glPointSize(1.0f);*/
}

	virtual void resizeGL(int _w, int _h)
	{
/*		glViewport(0, 0, _w, _h);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-1, 1, 1, -1, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();*/
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
			NotedComputeRegistrar::get()->init();
		m_wasSuspended = false;

		NotedComputeRegistrar::get()->beginTime(NotedFace::audio()->hopCursor());

		auto peaks = this->tracks.get().toVector();
		sort(peaks.begin(), peaks.end(), [](FreqPeak a, FreqPeak b){return a.band < b.band;});
		float t = 0;
		for (auto p: peaks)
			t += p.mag;

		glLoadIdentity();
		glEnable(GL_TEXTURE_1D);
		glBindTexture(GL_TEXTURE_1D, m_texture[0]);
		glBegin(GL_QUADS);
		float y = -1.f;
		if (t && false)
			for (auto p: peaks)
			{
				float f = p.band * .2;
				float ph = p.phase * sqrt(float(p.band) * .01);

				float yf = y;
				y += 2 * p.mag / t;

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
/*
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		// set up the projection matrix (the camera)
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		// set up the modelview matrix (the objects)
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glScalef(.5f, .5f, 1.f);

		glBegin(GL_POINTS);

			// go through the equations many times, drawing a point for each iteration
			for (int i = 0; i < iterations; i++) {

				// compute a new point using the strange attractor equations
				float xnew = sin(y*b) + c*sin(x*b);
				float ynew = sin(x*a) + d*sin(y*a);

				// save the new point
				x = xnew;
				y = ynew;

				// draw the new point
				glVertex2f(x, y);
			}

		glEnd();
		b += 0.001;
		c -= 0.001;*/
		NotedComputeRegistrar::get()->endTime(NotedFace::audio()->hopCursor());
		m_last = wallTime();
	}

	void stop() { m_suspended = true; }
	void start() { m_suspended = false; }

private:
	PeakGather peaks = PeakGather(ExtractMagnitude(WindowedFourier(AccumulateWave(ComputeRegistrar::feeder()))), 16);
	PeakTrack tracks = PeakTrack(peaks, 16, 1.f);

	mutable unsigned m_lastCursor = (unsigned)-1;
	mutable bool m_propertiesChanged = true;
	GLuint m_texture[2];
	ExamplePlugin* m_p;
	Time m_last;

	bool m_suspended = true;
	bool m_wasSuspended = true;


	float	x = 0.1, y = 0.1,		// starting point
		a = -0.966918,			// coefficients for "The King's Dream"
		b = 2.879879,
		c = 0.765145,
		d = 0.744728;
	int	initialIterations = 100,	// initial number of iterations
						// to allow the attractor to settle
		iterations = 100000;		// number of times to iterate through
						// the functions and draw a point
};

class AnalyzeViz: public ComputeAnalysis
{
public:
	AnalyzeViz(GLView* _p): ComputeAnalysis(_p->tasks(), "Precomputing visualization"), m_p(_p) {}

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
	GLView* m_p;
};

ExamplePlugin::ExamplePlugin()
{
	m_glView = new GLView(this);
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

void ExamplePlugin::onPropertiesChanged()
{
	m_glView->m_propertiesChanged = true;
}
