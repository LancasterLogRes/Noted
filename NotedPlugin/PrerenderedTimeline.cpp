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

#include <utility>
#include <QApplication>
#include <QPainter>
#include <QGLFramebufferObject>
#include <Common/Common.h>

#include "NotedFace.h"
#include "PrerenderedTimeline.h"

using namespace lb;
using namespace std;

PrerenderedTimeline::PrerenderedTimeline(QWidget* _p, bool /*_cursorSizeIsHop*/): Prerendered(_p), m_draggingTime(lb::UndefinedTime), m_renderedOffset(0), m_renderedPixelDuration(0)
{
	connect(NotedFace::compute(), SIGNAL(finished()), SLOT(rerender()));
}

PrerenderedTimeline::~PrerenderedTimeline()
{
}

void PrerenderedTimeline::mousePressEvent(QMouseEvent* _e)
{
	if (_e->button() == Qt::LeftButton)
		c()->setCursor(c()->timeOf(_e->x()), true);
	else if (_e->button() == Qt::MiddleButton)
		m_draggingTime = c()->timeOf(_e->x());
}

void PrerenderedTimeline::mouseReleaseEvent(QMouseEvent* _e)
{
	if (_e->button() == Qt::MiddleButton)
		m_draggingTime = lb::UndefinedTime;
}

void PrerenderedTimeline::mouseMoveEvent(QMouseEvent* _e)
{
	if (m_draggingTime != lb::UndefinedTime && _e->buttons() & Qt::MiddleButton)
		c()->setTimelineOffset(m_draggingTime - _e->x() * c()->pixelDuration());
	else if (_e->buttons() & Qt::LeftButton)
		c()->setCursor(c()->timeOf(_e->x()), true);
}

void PrerenderedTimeline::wheelEvent(QWheelEvent* _e)
{
	c()->zoomTimeline(_e->x(), exp(-_e->delta() / (QApplication::keyboardModifiers() & Qt::ControlModifier ? 2400.0 : QApplication::keyboardModifiers() & Qt::ShiftModifier ? 24 : 240.0)));
}

int PrerenderedTimeline::renderingPositionOf(lb::Time _t) const
{
	return (_t - m_renderedOffset + m_renderedPixelDuration / 2) / m_renderedPixelDuration;
}

lb::Time PrerenderedTimeline::renderingTimeOf(int _x) const
{
	return int64_t(_x) * m_renderedPixelDuration + m_renderedOffset;
}

bool PrerenderedTimeline::needsRerender() const
{
	if (Prerendered::needsRerender() || m_renderedOffset != c()->earliestVisible() || m_renderedPixelDuration != c()->pixelDuration())
		return true;
	return false;
}

bool PrerenderedTimeline::needsRepaint() const
{
	return Prerendered::needsRepaint() || m_paintedCursorIndex != c()->cursorIndex();
}

void PrerenderedTimeline::paintGL(QSize _s)
{
	Prerendered::paintGL(_s);

	QRect r = rect();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_paintedCursorIndex = c()->cursorIndex();
	int lastCursorL = c()->positionOf(highlightFrom()) + 1;
	int lastCursorR = lastCursorL + c()->widthOf(highlightDuration());
	glColor4f(0.f, .3f, 1.f, 0.2f);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBegin(GL_TRIANGLE_STRIP);
	glVertex2i(lastCursorL, r.top());
	glVertex2i(lastCursorR, r.top());
	glVertex2i(lastCursorL, r.top() + r.height());
	glVertex2i(lastCursorR, r.top() + r.height());
	glEnd();

	glBegin(GL_LINES);
	QColor cc = cursorColor();
	glColor4f(cc.red(), cc.green(), cc.blue(), 0.5f);
	glVertex2i(lastCursorL, r.top());
	glVertex2i(lastCursorL, r.top() + r.height());
	glColor4f(cc.red(), cc.green(), cc.blue(), 1.0f);
	glVertex2i(lastCursorR, r.top());
	glVertex2i(lastCursorR, r.top() + r.height());
	glEnd();
}

void PrerenderedTimeline::renderGL(QSize)
{
	m_renderedOffset = c()->earliestVisible();
	m_renderedPixelDuration = c()->pixelDuration();
}
