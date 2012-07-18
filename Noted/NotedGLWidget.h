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

#pragma once

#include <iostream>
#include <boost/chrono.hpp>
#include <QGLWidget>
#include <QThread>
#include <QResizeEvent>

#include <NotedPlugin/QGLWidgetProxy.h>

class NotedGLWidget: public QGLWidget, public QThread
{
public:
	NotedGLWidget(QGLWidgetProxy* _v, QWidget* _p);
	~NotedGLWidget();

	virtual void run();

	virtual void initializeGL();
	virtual void resizeGL(int, int);
	virtual void paintGL();

	virtual void mousePressEvent(QMouseEvent* _e) { m_v->mousePressEvent(_e); }
	virtual void mouseReleaseEvent(QMouseEvent* _e) { m_v->mouseReleaseEvent(_e); }
	virtual void mouseMoveEvent(QMouseEvent* _e) { m_v->mouseMoveEvent(_e); }
	virtual void wheelEvent(QWheelEvent* _e) { m_v->wheelEvent(_e); }
	virtual void paintEvent(QPaintEvent*);
	virtual void hideEvent(QShowEvent*);
	virtual void closeEvent(QCloseEvent*);
	virtual void resizeEvent(QResizeEvent* _e);

private:
	void quit();

	QGLWidgetProxy* m_v;
	QSize m_newSize;
	bool m_quitting;
};
