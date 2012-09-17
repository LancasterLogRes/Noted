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

using namespace Lightbox;
using namespace std;

PrerenderedTimeline::PrerenderedTimeline(QWidget* _p, bool _cursorSizeIsHop): Prerendered(_p), m_draggingTime(Lightbox::UndefinedTime), m_cursorSizeIsHop(_cursorSizeIsHop), m_renderedOffset(0), m_renderedPixelDuration(0), m_renderingContext(nullptr), m_renderingFrame(nullptr)
{
	m_renderingContext = new QGLWidget(0, this);
	connect(c(), SIGNAL(analysisFinished()), SLOT(sourceChanged()));
	initTimeline(c());
}

PrerenderedTimeline::~PrerenderedTimeline()
{
	delete m_renderingContext;
	delete m_renderingFrame;
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
		m_draggingTime = Lightbox::UndefinedTime;
}

void PrerenderedTimeline::mouseMoveEvent(QMouseEvent* _e)
{
	if (m_draggingTime != Lightbox::UndefinedTime && _e->buttons() & Qt::MiddleButton)
		c()->setTimelineOffset(m_draggingTime - _e->x() * c()->pixelDuration());
	else if (_e->buttons() & Qt::LeftButton)
		c()->setCursor(c()->timeOf(_e->x()), true);
}

void PrerenderedTimeline::wheelEvent(QWheelEvent* _e)
{
#ifdef Q_OS_MAC
#define CORRECT_SIGN -
#else
#define CORRECT_SIGN
#endif
	c()->zoomTimeline(_e->x(), exp(CORRECT_SIGN _e->delta() / (QApplication::keyboardModifiers() & Qt::ControlModifier ? 2400.0 : QApplication::keyboardModifiers() & Qt::ShiftModifier ? 24 : 240.0)));
#undef CORRECT_SIGN
}

int PrerenderedTimeline::renderingPositionOf(Lightbox::Time _t) const
{
	return (_t - m_renderingOffset + m_renderingPixelDuration / 2) / m_renderingPixelDuration;
}

Lightbox::Time PrerenderedTimeline::renderingTimeOf(int _x) const
{
	return int64_t(_x) * m_renderingPixelDuration + m_renderingOffset;
}

bool PrerenderedTimeline::needsRepaint() const
{
	int cursorL = c()->positionOf(highlightFrom()) + 1;
	int cursorR = cursorL + c()->widthOf(highlightDuration());
	if (Prerendered::needsRepaint() && (m_needsUpdate || /*m_lastOffset != c()->earliestVisible() || m_lastPixelDuration != c()->pixelDuration() || */(m_lastCursorR != cursorR && !((cursorL > size().width() && m_lastCursorL > size().width()) || cursorR < 0 && m_lastCursorR < 0))))
		return true;
	return false;
}

void PrerenderedTimeline::resizeGL(int _w, int _h)
{
	Prerendered::resizeGL(_w, _h);
}

void PrerenderedTimeline::sourceChanged()
{
	m_sourceChanged = true;
}

void PrerenderedTimeline::paintGL()
{
//	cbug(42) << __PRETTY_FUNCTION__;
	glClear(GL_COLOR_BUFFER_BIT);
	QRect r(0, 0, 0, height());
	{
		QMutexLocker l(&m_lock);
		if (m_fbo)
		{
			Time o = c()->earliestVisible();
			Time d = c()->pixelDuration();
			Time relativeOffset = m_renderedOffset - o;
			r.setX((relativeOffset + sign(relativeOffset) * d / 2) / d);
			r.setWidth((m_renderedPixelDuration * m_fbo->width() + d / 2) / d);
			if (r.x() == 0 && m_renderedOffset != o)
				qDebug() << "Strange -> zero drawing offset, but have ro=" << m_renderedOffset << " and need o=" << o;
			glBindTexture(GL_TEXTURE_2D, m_fbo->texture());
		}
		else
			glBindTexture(GL_TEXTURE_2D, 0);
		glColor4f(1.f, 1.f, 1.f, 1.f);
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2i(0, 0);
		glVertex2i(r.x(), r.top());
		glTexCoord2i(1, 0);
		glVertex2i(r.x() + r.width(), r.top());
		glTexCoord2i(0, 1);
		glVertex2i(r.x(), r.top() + r.height());
		glTexCoord2i(1, 1);
		glVertex2i(r.x() + r.width(), r.top() + r.height());
		glEnd();
		m_needsUpdate = false;
	}

	m_lastCursorL = c()->positionOf(highlightFrom()) + 1;
	m_lastCursorR = m_lastCursorL + c()->widthOf(highlightDuration());
	glColor4f(0.f, .3f, 1.f, 0.2f);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBegin(GL_TRIANGLE_STRIP);
	glVertex2i(m_lastCursorL, r.top());
	glVertex2i(m_lastCursorR, r.top());
	glVertex2i(m_lastCursorL, r.top() + r.height());
	glVertex2i(m_lastCursorR, r.top() + r.height());
	glEnd();

	glBegin(GL_LINES);
	QColor cc = cursorColor();
	glColor4f(cc.red(), cc.green(), cc.blue(), 0.5f);
	glVertex2i(m_lastCursorL, r.top());
	glVertex2i(m_lastCursorL, r.top() + r.height());
	glColor4f(cc.red(), cc.green(), cc.blue(), 1.0f);
	glVertex2i(m_lastCursorR, r.top());
	glVertex2i(m_lastCursorR, r.top() + r.height());
	glEnd();
}

bool PrerenderedTimeline::rejigRender()
{
	m_renderingOffset = c()->earliestVisible();
	m_renderingPixelDuration = c()->pixelDuration();
	int w = width();
	int h = height();
	if (w && h && (m_renderedOffset != m_renderingOffset || m_renderedPixelDuration != m_renderingPixelDuration || !m_fbo || height() != m_fbo->height() || width() > m_fbo->width() || m_sourceChanged) && c()->samples())
	{
		m_renderingContext->makeCurrent();
		if (!m_renderingFrame || m_renderingFrame->size() != QSize(w, h))
		{
			delete m_renderingFrame;
			m_renderingFrame = new QGLFramebufferObject(w, h);
			resizeGL(w, h);
		}
		m_renderingFrame->bind();

		glClearColor(1, 1, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		{
			QPainter p(m_renderingFrame);
			GraphParameters<double> nor(make_pair(m_renderingOffset, m_renderingOffset + m_renderingPixelDuration * width()), width() / 80, toBase(1, 1000000));
			for (Time t = nor.from; t < nor.to; t += nor.incr)
			{
				int x = renderingPositionOf(t);
				if (nor.isMajor(t))
				{
					p.setPen(QColor(160, 160, 160));
					p.drawText(QRect(x - 40, 0, 80, 12), Qt::AlignHCenter | Qt::AlignBottom, ::Lightbox::textualTime(t, nor.delta, nor.major).c_str());
					p.setPen(QColor(192, 192, 192));
					p.drawLine(x, 14, x, height());
				}
				else
				{
					p.setPen(QColor(224, 224, 224));
					p.drawLine(x, 0, x, height());
				}
			}
		}

		if (m_fbo && m_renderedPixelDuration == m_renderingPixelDuration && height() == m_fbo->height() && !m_sourceChanged)
		{
			if (m_renderingOffset < m_renderedOffset)
			{
				assert(width() == m_fbo->width());	// currently can't handle changes in width when the new offset is earlier.
				// |//////|----------|
				//    rW       kW
				Time rerenderPeriod = m_renderedOffset - m_renderingOffset;
				int rerenderWidth = min<int>(w, (rerenderPeriod + m_renderedPixelDuration / 2) / m_renderedPixelDuration);
				glPushMatrix();
				glScalef(1.f, -1.f, 1.f);
				glTranslatef(0, -h, 0);
				m_renderingContext->drawTexture(QPointF(rerenderWidth, 0), m_fbo->texture());
				glPopMatrix();
				doRender(m_renderingFrame, 0, rerenderWidth);
			}
			else
			{
				// NOTE: Here it may be that rendered & rendering are not equal widths - in which case rW + kW != w.
				// |----------|//////|
				//      kW       rW
				Time rerenderPeriod = m_renderingOffset + width() * m_renderingPixelDuration - (m_renderedOffset + m_fbo->width() * m_renderedPixelDuration);
				int rerenderWidth = min<int>(w, (rerenderPeriod + m_renderedPixelDuration / 2) / m_renderedPixelDuration);
				int keepWidth = w - rerenderWidth;
				glPushMatrix();
				glScalef(1.f, -1.f, 1.f);
				glTranslatef(0, -h, 0);
				m_renderingContext->drawTexture(QPointF(-m_fbo->width() + keepWidth, 0), m_fbo->texture());
				glPopMatrix();
				doRender(m_renderingFrame, keepWidth, rerenderWidth);
			}
		}
		else
		{
			m_sourceChanged = false;
			doRender(m_renderingFrame, 0, w);
		}
		m_renderingFrame->release();
		m_renderingContext->doneCurrent();

		QMutexLocker l(&m_lock);
		swap(m_renderingFrame, m_fbo);
		m_renderedOffset = m_renderingOffset;
		m_renderedPixelDuration = m_renderingPixelDuration;
		m_needsUpdate = true;
		return true;
	}
	return false;
}
