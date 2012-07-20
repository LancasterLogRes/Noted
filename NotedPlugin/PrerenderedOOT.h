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

#include <utility>
#include <map>

#include <Common/Common.h>

#include <QThread>
#include <QGLFramebufferObject>
#include <QGLWidget>
#include <QMutex>
#include <QPainter>
#include <QDebug>
#include <QMouseEvent>
#include <QWidget>
#include "Prerendered.h"

class NotedFace;
class PrerenderedOOT;

class DisplayThread: public QThread
{
public:
	DisplayThread(PrerenderedOOT* _p): m_p(_p) {}

	virtual void run();

	using QThread::msleep;

private:
	PrerenderedOOT* m_p;
};

class PrerenderedOOT: public QGLWidget
{
	Q_OBJECT

	friend class DisplayThread;

public:
	PrerenderedOOT(QWidget* _p);
	~PrerenderedOOT();

	NotedFace* c() const;

public slots:
	void rerender();

protected:
	virtual bool needsRepaint() const;
	virtual void doRender(QGLFramebufferObject*) {}

	virtual void initializeGL();
	virtual void resizeGL(int _w, int _h);
	virtual void paintGL();

	virtual void run();

	virtual void paintEvent(QPaintEvent*);
	virtual void hideEvent(QShowEvent*);
	virtual void closeEvent(QCloseEvent*);
	virtual void resizeEvent(QResizeEvent* _e);

protected:
	QGLFramebufferObject* m_fbo;

private:
	void quit();

	DisplayThread m_display;
	bool m_quitting;

	mutable NotedFace* m_c;
	QSize m_newSize;
};
