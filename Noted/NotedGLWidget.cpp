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

#include <cassert>
#include <QtOpenGL>
#include <QDebug>
#include <QTimer>
#include <NotedPlugin/NotedComputeRegistrar.h>
#include "NotedGLWidget.h"

NotedGLWidget::NotedGLWidget(QGLWidgetProxy* _v, QWidget* _p):
	QGLWidget(QGLFormat(QGL::SampleBuffers), _p),
	m_v(_v)
{
	m_v->m_widget = this;
	m_newSize = size();
	setAutoBufferSwap(false);
}

NotedGLWidget::~NotedGLWidget()
{
	quit();
	delete m_v;
}

void NotedGLWidget::initializeGL()
{
	assert(false);
}

void NotedGLWidget::resizeGL(int, int)
{
	assert(false);
}

void NotedGLWidget::paintGL()
{
	assert(false);
}

void NotedGLWidget::paintEvent(QPaintEvent*)
{
	if (!isRunning())
	{
		m_quitting = false;
		doneCurrent();
		context()->moveToThread(this);
		start();
	}
	m_newSize = size();
}

void NotedGLWidget::quit()
{
	m_quitting = true;
	wait(1000);
	while (isRunning())
	{
		terminate();
		wait(1000);
	}
}

void NotedGLWidget::hideEvent(QShowEvent*)
{
	quit();
}

void NotedGLWidget::closeEvent(QCloseEvent*)
{
	quit();
}

void NotedGLWidget::resizeEvent(QResizeEvent* _e)
{
	m_newSize = _e->size();
}

void NotedGLWidget::run()
{
	makeCurrent();
	m_v->initializeGL();
	NotedComputeRegistrar::get()->init();
	while (!m_quitting)
	{
		bool resized = false;
		if (m_newSize.isValid())
		{
			m_v->resizeGL(m_newSize.width(), m_newSize.height());
			m_newSize = QSize();
			resized = true;
		}
		if (!size().isEmpty() && isVisible() && (resized || m_v->needsRepaint()))
		{
			NotedComputeRegistrar::get()->beginTime(NotedFace::audio()->hopCursor());
			m_v->paintGL();
			NotedComputeRegistrar::get()->endTime(NotedFace::audio()->hopCursor());
			swapBuffers();
		}
		else
			msleep(5);
	}
	NotedComputeRegistrar::get()->fini();
	doneCurrent();
}
