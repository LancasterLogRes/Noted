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
#include <NotedPlugin/NotedComputeRegistrar.h>
#include "ExamplePlugin.h"
using namespace std;
using namespace lb;

NOTED_PLUGIN(ExamplePlugin);

class GLView: public QGLWidgetProxy
{
	friend class ExamplePlugin;

public:
	GLView(ExamplePlugin* _p): m_p(_p) {}

	virtual bool init()
	{
		return ComputeRegistrar::get()->store(ms);
	}

	virtual void compute()
	{
		ms.get();
	}

	virtual void initializeGL()
	{
		glDisable(GL_DEPTH_TEST);
		glGenTextures (1, m_texture);
		for (int i = 0; i < 1; ++i)
		{
			glBindTexture (GL_TEXTURE_1D, m_texture[i]);
			glTexParameteri (GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri (GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
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
//		cbug(42) << __PRETTY_FUNCTION__;
		glEnable(GL_TEXTURE_1D);
		glLoadIdentity();
		{
			auto w = ms.get();
			glBindTexture(GL_TEXTURE_1D, m_texture[0]);
			float scale = m_p->scale;
			float bias = m_p->bias;
			glPixelTransferf(GL_RED_SCALE, scale);
			glPixelTransferf(GL_GREEN_SCALE, scale);
			glPixelTransferf(GL_BLUE_SCALE, scale);
			glPixelTransferf(GL_RED_BIAS, bias);
			glPixelTransferf(GL_GREEN_BIAS, bias);
			glPixelTransferf(GL_BLUE_BIAS, bias);
			glTexImage1D(GL_TEXTURE_1D, 0, 1, w.size(), 0, GL_LUMINANCE, GL_FLOAT, w.data());
		}

/*		if (auto p = NotedFace::get()->deltaPhaseSpectrum(NotedFace::audio()->cursorIndex(), 1))
		{
			auto s = NotedFace::get()->magSpectrum(NotedFace::audio()->cursorIndex(), 1);
			unsigned i = maxInRange(s.begin(), s.end()) - s.begin();
			glColor3ubv(Color(p[i], 1, 1).toRGBA8().data());
		}*/

		glBindTexture(GL_TEXTURE_1D, m_texture[0]);
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord1f(0.f);
		glVertex3f(-1.0f, -1.0f, 0.0f);
		glTexCoord1f(1.f);
		glVertex3f( 1.0f, -1.0f, 0.0f);
		glTexCoord1f(0.f);
		glVertex3f(-1.0f, 1.0f, 0.0f);
		glTexCoord1f(1.f);
		glVertex3f( 1.0f, 1.0f, 0.0f);
		glEnd();
		glDisable(GL_TEXTURE_1D);
	}

private:
//	MagnitudeComponent ms = MagnitudeComponent(WindowedFourier(Accumulate(NotedCursorFeeder())));
	Accumulate ms = Accumulate(ComputeRegistrar::feeder(), 8);

	mutable unsigned m_lastCursor = (unsigned)-1;
	mutable bool m_propertiesChanged = true;
	GLuint m_texture[2];
	ExamplePlugin* m_p;
};

class AnalyzeViz: public CausalAnalysis
{
public:
	AnalyzeViz(GLView* _p): CausalAnalysis("Precomputing visualization"), m_p(_p) {}

	bool init(bool _willRecord)
	{
		return !m_p->init();
	}
	void process(unsigned _i, lb::Time _t)
	{
		m_p->compute();
	}
	void fini(bool _completed, bool _didRecord)
	{
		m_p->update();
	}

private:
	GLView* m_p;
	bool m_alreadyComputed;
};

ExamplePlugin::ExamplePlugin()
{
	NotedComputeRegistrar::get();
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
