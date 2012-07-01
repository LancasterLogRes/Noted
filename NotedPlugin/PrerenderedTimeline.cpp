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

PrerenderedTimeline::PrerenderedTimeline(QWidget* _p, bool _cursorSizeIsHop): Prerendered(_p), m_draggingTime(Lightbox::UndefinedTime), m_cursorSizeIsHop(_cursorSizeIsHop), m_renderedOffset(0), m_renderedDuration(0)
{
	connect(c(), SIGNAL(offsetChanged()), this, SLOT(update()));
	connect(c(), SIGNAL(durationChanged()), this, SLOT(update()));
	connect(c(), SIGNAL(audioChanged()), this, SLOT(sourceChanged()));
	connect(c(), SIGNAL(audioChanged()), this, SLOT(update()));
	initTimeline(c());
}

PrerenderedTimeline::~PrerenderedTimeline()
{
}

void PrerenderedTimeline::mousePressEvent(QMouseEvent* _e)
{
	if (_e->button() == Qt::LeftButton)
		c()->setCursor(timeOfReal(_e->x()));
	else if (_e->button() == Qt::MiddleButton)
		m_draggingTime = timeOfReal(_e->x());
}

void PrerenderedTimeline::mouseReleaseEvent(QMouseEvent* _e)
{
	if (_e->button() == Qt::MiddleButton)
		m_draggingTime = Lightbox::UndefinedTime;
}

void PrerenderedTimeline::mouseMoveEvent(QMouseEvent* _e)
{
	if (m_draggingTime != Lightbox::UndefinedTime && _e->buttons() & Qt::MiddleButton)
		c()->setOffset(m_draggingTime - _e->x() * (timelineDuration() / c()->activeWidth()));
	else if (_e->buttons() & Qt::LeftButton)
		c()->setCursor(timeOfReal(_e->x()));
}

void PrerenderedTimeline::wheelEvent(QWheelEvent* _e)
{
	// Want to keep timeOf(_e->x()) constant.
	Lightbox::Time t = timeOfReal(_e->x());
	c()->setDuration(timelineDuration() * exp(_e->delta() / (QApplication::keyboardModifiers() & Qt::ControlModifier ? 2400.0 : QApplication::keyboardModifiers() & Qt::ShiftModifier ? 24 : 240.0)));
	c()->setOffset(t - _e->x() * (timelineDuration() / c()->activeWidth()));
}

void PrerenderedTimeline::sourceChanged()
{
	m_sourceChanged = true;
	m_needsUpdate = true;
}

void PrerenderedTimeline::paintEvent(QPaintEvent*)
{
	QPainter p(this);
	{
		QMutexLocker l(&m_lock);
		if (m_rendered.width())
		{
			Time d = timelineDuration();
			Time o = timelineOffset();
			int at = ((m_renderedOffset - o) * width() + sign(m_renderedOffset - o) * d / 2) / d;
			int w = m_renderedDuration * width() / d;
			if (at == 0 && m_renderedOffset != o)
				qDebug() << "Stange -> zero drawing offset, but have ro=" << m_renderedOffset << " and need o=" << o;
			p.drawImage(QRect(at, 0, w, height()), m_rendered);
			if (!m_overlay.isNull())
				p.drawImage(0, 0, m_overlay);
		}
	}
}

bool PrerenderedTimeline::rejigRender()
{
	m_renderingOffset  = timelineOffset();
	m_renderingDuration = timelineDuration();
	int w = width();
	int h = height();
	//qDebug() << size() << m_rendered.size();
	if (w && h && (m_renderedOffset != m_renderingOffset || m_renderedDuration != m_renderingDuration || size() != m_rendered.size() || m_sourceChanged) && c()->samples())
	{
		{
			QMutexLocker l(&m_lock);
			m_overlay = renderOverlay();
		}
		QImage img(width(), height(), QImage::Format_ARGB32_Premultiplied);
		img.fill(qRgb(255, 255, 255));

		{
			QPainter p(&img);
			GraphParameters<double> nor(make_pair(m_renderingOffset, m_renderingOffset + m_renderingDuration), width() / 80, toBase(1, 1000000));
			for (Time t = nor.from; t < nor.to; t += nor.incr)
			{
				int x = xOf(t);
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

		if (m_renderedDuration == m_renderingDuration && size() == m_rendered.size() && !m_sourceChanged)
		{
			if (m_renderingOffset < m_renderedOffset)
			{
				// |//////|----------|
				//    rW       kW
				Time rerenderPeriod = m_renderedOffset - m_renderingOffset;
				int rerenderWidth = min<int>(w, (rerenderPeriod * w + m_renderedDuration / 2) / m_renderedDuration);
				int keepWidth = w - rerenderWidth;
				QPainter(&img).drawImage(rerenderWidth, 0, m_rendered, 0, 0, keepWidth, -1);
				doRender(img, 0, rerenderWidth);
			}
			else
			{
				// |----------|//////|
				//      kW       rW
				Time rerenderPeriod = m_renderingOffset - m_renderedOffset;
				int rerenderWidth = min<int>(w, (rerenderPeriod * w + m_renderedDuration / 2) / m_renderedDuration);
				int keepWidth = w - rerenderWidth;
				QPainter(&img).drawImage(0, 0, m_rendered, rerenderWidth, 0, keepWidth, -1);
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
		m_renderedDuration = m_renderingDuration;

		m_needsUpdate = true;
		return true;
	}
	return false;
}

int PrerenderedTimeline::xOf(Lightbox::Time _t) const
{
	return (_t - m_renderingOffset) * c()->activeWidth() / m_renderingDuration;
}

Lightbox::Time PrerenderedTimeline::timeOf(int _x) const
{
	return int64_t(_x) * m_renderingDuration / c()->activeWidth() + m_renderingOffset;
}
