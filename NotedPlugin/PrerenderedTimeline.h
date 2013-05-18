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
#include <QtOpenGL>
#include <QPainter>
#include <QDebug>
#include <QMouseEvent>
#include <QWidget>
#include <QMutex>

#include "Timeline.h"
#include "Prerendered.h"

class QGLWidget;
class QGLFramebufferObject;

class PrerenderedTimeline: public Prerendered, public Timeline
{
	Q_OBJECT

public:
	PrerenderedTimeline(QWidget* _p, bool _cursorSizeIsHop = true);
	~PrerenderedTimeline();

	using Prerendered::event;

protected:
	/// These two are frozen at the zoom configuration as it was prior to rendering; this is necessary as the real offset/visible duration may change during rendering (which is happening asynchronously).
	int renderingPositionOf(lb::Time _t) const;
	lb::Time renderingTimeOf(int _x) const;

	virtual bool needsRepaint() const;
	virtual bool needsRerender() const;

	virtual void paintGL(QSize);
	virtual void renderGL(QSize);

	virtual void mousePressEvent(QMouseEvent* _e);
	virtual void mouseReleaseEvent(QMouseEvent* _e);
	virtual void mouseMoveEvent(QMouseEvent* _e);
	virtual void wheelEvent(QWheelEvent* _e);

	lb::Time m_draggingTime;

	lb::Time m_renderedOffset;
	lb::Time m_renderedPixelDuration;
	unsigned m_paintedCursorIndex;
};
