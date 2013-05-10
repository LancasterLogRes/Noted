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

#define USE_RENDER_THREAD 1

Prerendered::Prerendered(QWidget* _p): QGLWidget(_p), m_renderThread(nullptr), m_c(nullptr)
{
}

Prerendered::~Prerendered()
{
#if USE_RENDER_THREAD
	quit();
#endif
	delete m_renderThread;
}

void Prerendered::paintEvent(QPaintEvent* _e)
{
#if USE_RENDER_THREAD
	(void)_e;
	if (!m_renderThread)
	{
		setAutoBufferSwap(false);
		m_renderThread = createWorkerThread([=](){serviceRender(); return true;}, [=](){context()->makeCurrent(); initializeGL();}, [=](){context()->doneCurrent(); });
		context()->moveToThread(m_renderThread);
	}
	if (!m_renderThread->isRunning())
	{
		m_resize = size();
		m_size = QSize();
		m_needsRepaint = true;
		m_needsRerender = true;
		m_renderThread->start();
	}
#else
	QGLWidget::paintEvent(_e);
#endif
}

void Prerendered::quit()
{
	if (m_renderThread)
	{
		m_renderThread->quit();
		m_renderThread->wait();
	}
}

void Prerendered::hideEvent(QHideEvent* _e)
{
#if USE_RENDER_THREAD
	(void)_e;
	quit();
#else
	QGLWidget::hideEvent(_e);
#endif
}

void Prerendered::closeEvent(QCloseEvent* _e)
{
#if USE_RENDER_THREAD
	(void)_e;
	quit();
#else
	QGLWidget::closeEvent(_e);
#endif
}

void Prerendered::resizeEvent(QResizeEvent* _e)
{
#if USE_RENDER_THREAD
	m_resize = _e->size();
#else
	QGLWidget::resizeEvent(_e);
#endif
}

NotedFace* Prerendered::c() const
{
	if (!m_c)
		m_c = dynamic_cast<NotedFace*>(window());
	if (!m_c)
		m_c = dynamic_cast<NotedFace*>(window()->parentWidget()->window());
	return m_c;
}

bool Prerendered::serviceRender()
{
	bool ret = false;
	bool resized = false;
	if (m_resize.isValid())
	{
		m_size = QSize();
		swap(m_size, m_resize);
		resizeGL(m_size.width(), m_size.height());
		resized = true;
	}
	if (resized || shouldRepaint())
	{
		m_needsRepaint = false;
		paintGL(m_size);
		swapBuffers();
		ret = true;
	}

	if (!ret)
		usleep(10000);
	return true;
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

void Prerendered::paintGL(QSize _s)
{
	glLoadIdentity();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_needsRerender = false;
	renderGL(_s);
}
