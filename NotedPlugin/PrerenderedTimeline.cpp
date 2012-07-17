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
#include <Common/Common.h>
using namespace Lightbox;
using namespace std;

#include "NotedFace.h"
#include "PrerenderedTimeline.h"

PrerenderedTimeline::PrerenderedTimeline(QWidget* _p, bool _cursorSizeIsHop): Prerendered(_p), m_draggingTime(Lightbox::UndefinedTime), m_cursorSizeIsHop(_cursorSizeIsHop), m_renderedOffset(0), m_renderedPixelDuration(0)
{
	connect(c(), SIGNAL(offsetChanged()), this, SLOT(update()));
	connect(c(), SIGNAL(durationChanged()), this, SLOT(update()));
	connect(c(), SIGNAL(analysisFinished()), this, SLOT(sourceChanged()));
	connect(c(), SIGNAL(analysisFinished()), this, SLOT(update()));
	initTimeline(c());
}

PrerenderedTimeline::~PrerenderedTimeline()
{
}

void PrerenderedTimeline::mousePressEvent(QMouseEvent* _e)
{
	if (_e->button() == Qt::LeftButton)
		c()->setCursor(c()->timeOf(_e->x()));
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
		c()->setCursor(c()->timeOf(_e->x()));
}

void PrerenderedTimeline::wheelEvent(QWheelEvent* _e)
{
	// Want to keep timeOf(_e->x()) constant.
	Lightbox::Time t = c()->timeOf(_e->x());
#ifdef Q_OS_MAC
#define CORRECT_SIGN -
#else
#define CORRECT_SIGN
#endif
	c()->setPixelDuration(c()->pixelDuration() * exp(CORRECT_SIGN _e->delta() / (QApplication::keyboardModifiers() & Qt::ControlModifier ? 2400.0 : QApplication::keyboardModifiers() & Qt::ShiftModifier ? 24 : 240.0)));
#undef CORRECT_SIGN
	c()->setTimelineOffset(t - _e->x() * pixelDuration());
}

int PrerenderedTimeline::renderingPositionOf(Lightbox::Time _t) const
{
	return (_t - m_renderingOffset + m_renderingPixelDuration / 2) / m_renderingPixelDuration;
}

Lightbox::Time PrerenderedTimeline::renderingTimeOf(int _x) const
{
	return int64_t(_x) * m_renderingPixelDuration + m_renderingOffset;
}

void PrerenderedTimeline::resizeGL(int _w, int _h)
{
	Prerendered::resizeGL(_w, _h);
}

void PrerenderedTimeline::sourceChanged()
{
	m_sourceChanged = true;
	m_needsUpdate = true;
}

void PrerenderedTimeline::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT);
	QRect r(0, 0, 0, height());
	{
		QMutexLocker l(&m_lock);
		if (m_needsUpdate)
		{
			// Update the GL texture from the rendered image.
			QImage glRendered = convertToGLFormat(m_rendered);
			glBindTexture(GL_TEXTURE_2D, m_texture[0]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glRendered.size().width(), glRendered.size().height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, glRendered.constBits());
		}
		Time o = c()->earliestVisible();
		Time d = c()->pixelDuration();
		Time relativeOffset = m_renderedOffset - o;
		r.setX((relativeOffset + sign(relativeOffset) * d / 2) / d);
		r.setWidth((m_renderedPixelDuration * m_rendered.width() + d / 2) / d);
		if (r.x() == 0 && m_renderedOffset != o)
			qDebug() << "Stange -> zero drawing offset, but have ro=" << m_renderedOffset << " and need o=" << o;
	}
	glBindTexture(GL_TEXTURE_2D, m_texture[0]);
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

bool PrerenderedTimeline::rejigRender()
{
	m_renderingOffset = c()->earliestVisible();
	m_renderingPixelDuration = c()->pixelDuration();
	int w = width();
	int h = height();
	//qDebug() << size() << m_rendered.size();
	if (w && h && (m_renderedOffset != m_renderingOffset || m_renderedPixelDuration != m_renderingPixelDuration || height() != m_rendered.height() || width() > m_rendered.width() || m_sourceChanged) && c()->samples())
	{
		QImage img(width(), height(), QImage::Format_RGB32);
		img.fill(qRgb(255, 255, 255));

		{
			QPainter p(&img);
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

		if (m_renderedPixelDuration == m_renderingPixelDuration && height() == m_rendered.height() && !m_sourceChanged)
		{
			if (m_renderingOffset < m_renderedOffset)
			{
				assert(width() == m_rendered.width());	// currently can't handle changes in width when the new offset is earlier.
				// |//////|----------|
				//    rW       kW
				Time rerenderPeriod = m_renderedOffset - m_renderingOffset;
				int rerenderWidth = min<int>(w, (rerenderPeriod + m_renderedPixelDuration / 2) / m_renderedPixelDuration);
				int keepWidth = w - rerenderWidth;
				QPainter(&img).drawImage(rerenderWidth, 0, m_rendered, 0, 0, keepWidth, -1);
				doRender(img, 0, rerenderWidth);
			}
			else
			{
				// NOTE: Here it may be that rendered & rendering are not equal widths - in which case rW + kW != w.
				// |----------|//////|
				//      kW       rW
				Time rerenderPeriod = m_renderingOffset + width() * m_renderingPixelDuration - (m_renderedOffset + m_rendered.width() * m_renderedPixelDuration);
				int rerenderWidth = min<int>(w, (rerenderPeriod + m_renderedPixelDuration / 2) / m_renderedPixelDuration);
				int keepWidth = w - rerenderWidth;
				QPainter(&img).drawImage(0, 0, m_rendered, (m_renderingOffset - m_renderedOffset + m_renderedPixelDuration / 2) / m_renderedPixelDuration, 0, keepWidth, -1);
				doRender(img, keepWidth, rerenderWidth);
			}
		}
		else
		{
			m_sourceChanged = false;
			doRender(img);
		}

		QMutexLocker l(&m_lock);
		m_rendered = img;
		m_renderedOffset = m_renderingOffset;
		m_renderedPixelDuration = m_renderingPixelDuration;
		m_needsUpdate = true;
		return true;
	}
	return false;
}

void PrerenderedTimeline::updateIfNeeded()
{
	if (m_needsUpdate)
		updateGL();
}
