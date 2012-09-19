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

#ifdef Q_OS_MAC
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include "ExamplePlugin.h"

using namespace std;
using namespace Lightbox;

NOTED_PLUGIN(ExamplePlugin);

class GLView: public QGLWidgetProxy
{
	friend class ExamplePlugin;

public:
	GLView(ExamplePlugin* _p): m_p(_p), m_c(_p->noted()) {}

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
		glEnable(GL_TEXTURE_1D);
	}
	GLuint m_texture[2];

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
		if (m_lastCursor == m_c->cursorIndex() && !m_propertiesChanged)
			return false;
		m_lastCursor = m_c->cursorIndex();
		m_propertiesChanged = false;
		return true;
	}

	virtual void paintGL()
	{
//		cbug(42) << __PRETTY_FUNCTION__;
		glLoadIdentity();
		{
			auto w = m_c->waveWindow(m_c->cursorIndex());
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

		if (auto p = m_c->deltaPhaseSpectrum(m_c->cursorIndex(), 1))
		{
			auto s = m_c->magSpectrum(m_c->cursorIndex(), 1);
			unsigned i = maxInRange(s.begin(), s.end()) - s.begin();
			glColor3ubv(Color(p[i], 1, 1).toRGBA().data());
		}

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
	}

private:
	mutable unsigned m_lastCursor;
	mutable bool m_propertiesChanged;
	ExamplePlugin* m_p;
	NotedFace* m_c;
};

ExamplePlugin::ExamplePlugin(NotedFace* _c): NotedPlugin(_c), scale(2.f), bias(0.5f)
{
	m_glView = new GLView(this);
	m_vizDock = new QDockWidget("Viz", _c);
	m_vizDock->setWidget(_c->addGLWidget(m_glView, m_vizDock));
	m_vizDock->setFeatures(m_vizDock->features()|QDockWidget::DockWidgetVerticalTitleBar);
	_c->addDockWidget(Qt::RightDockWidgetArea, m_vizDock);
}

ExamplePlugin::~ExamplePlugin()
{
	delete m_vizDock;
}

void ExamplePlugin::onPropertiesChanged()
{
	m_glView->m_propertiesChanged = true;
}
