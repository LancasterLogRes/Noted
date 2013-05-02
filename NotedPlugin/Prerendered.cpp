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

#include <QPainter>
#include <QGLFramebufferObject>
#ifdef Q_OS_MAC
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#include <Common/Common.h>
#include <boost/thread.hpp>

#include "NotedFace.h"
#include "Prerendered.h"

using namespace std;
using namespace Lightbox;

void DisplayThread::run()
{
	m_p->run();
}

Prerendered::Prerendered(QWidget* _p): QGLWidget(_p), m_fbo(nullptr), m_display(this), m_c(nullptr)
{
	setAutoBufferSwap(false);
}

Prerendered::~Prerendered()
{
	if (m_display.isRunning())
		cwarn << "BAD!!! Prerendered's destructor was called without a called to quit() first! Call quit() in the destructor of the final class.";
	quit();
	delete m_fbo;
}

void Prerendered::paintEvent(QPaintEvent*)
{
	doneCurrent();
	c()->ensureRegistered(this);
/*	if (!m_display.isRunning())
	{
		m_quitting = false;
		doneCurrent();
		m_display.start();
	}*/
	m_newSize = size();
}

bool Prerendered::needsRepaint() const
{
	return !size().isEmpty() && isVisible();
}

void Prerendered::quit()
{
	c()->ensureUnregistered(this);
/*	m_quitting = true;
	m_display.wait(1000);
	while (m_display.isRunning())
	{
		m_display.terminate();
		m_display.wait(1000);
	}*/
	makeCurrent();
}

void Prerendered::hideEvent(QHideEvent*)
{
	quit();
}

void Prerendered::closeEvent(QCloseEvent*)
{
	quit();
}

void Prerendered::resizeEvent(QResizeEvent* _e)
{
	m_newSize = _e->size();
}

void Prerendered::run()
{
	makeCurrent();
	initializeGL();
	while (!m_quitting)
		if (!check())
			m_display.msleep(5);
	doneCurrent();
}

NotedFace* Prerendered::c() const
{
	if (!m_c)
		m_c = dynamic_cast<NotedFace*>(window());
	if (!m_c)
		m_c = dynamic_cast<NotedFace*>(window()->parentWidget()->window());
	return m_c;
}

void Prerendered::rerender()
{
}

bool Prerendered::check()
{
	makeCurrent();
	bool resized = false;
	if (m_newSize.isValid())
	{
		initializeGL();
		resizeGL(m_newSize.width(), m_newSize.height());
		m_newSize = QSize();
		resized = true;
	}
	if (resized || needsRepaint())
	{
		paintGL();
		swapBuffers();
		return true;
	}
	return false;
}

void Prerendered::initializeGL()
{
	glClearColor(1, 1, 1, 1);
	glClearDepth(-1.f);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void Prerendered::resizeGL(int _w, int _h)
{
	glViewport(0, 0, _w, _h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, _w, 0, _h, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void Prerendered::paintGL()
{
//	cbug(42) << __PRETTY_FUNCTION__;
	if ((true || !m_fbo || m_fbo->size() != size()) && c()->samples())
	{
		if (!m_fbo || m_fbo->size() != size())
		{
			delete m_fbo;
			auto s = size();
			if (s.isNull() || s.width() < 1 || s.height() < 1)
				s = QSize(1, 1);
			m_fbo = new QGLFramebufferObject(s);
		}
		m_fbo->bind();
		doRender(m_fbo);
		m_fbo->release();
		initializeGL();
	}
	glColor4f(1.f, 1.f, 1.f, 1.f);
	glBindTexture(GL_TEXTURE_2D, m_fbo ? m_fbo->texture() : 0);
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2i(0, 0);
	glVertex2i(0, 0);
	glTexCoord2i(1, 0);
	glVertex2i(width(), 0);
	glTexCoord2i(0, 1);
	glVertex2i(0, height());
	glTexCoord2i(1, 1);
	glVertex2i(width(), height());
	glEnd();
}
