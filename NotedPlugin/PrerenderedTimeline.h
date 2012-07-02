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
#include <QPainter>
#include <QDebug>
#include <QMouseEvent>
#include <QWidget>
#include <QMutex>

#include "Timeline.h"
#include "Prerendered.h"

class PrerenderedTimeline: public Prerendered, public Timeline
{
	Q_OBJECT

public:
	PrerenderedTimeline(QWidget* _p, bool _cursorSizeIsHop = true);
	~PrerenderedTimeline();

	virtual QWidget* widget() { return this; }

	/// Called from the worker thread.
	bool rejigRender();

	/// Called from the GUI thread.
	void updateIfNeeded() { if (m_needsUpdate) update(); m_needsUpdate = false; }

    using Prerendered::event;

public slots:
	void sourceChanged();

protected:
	int xOf(Lightbox::Time _t) const;
	Lightbox::Time timeOf(int _x) const;

	virtual void doRender(QImage& _img) { doRender(_img, 0, width()); }
	virtual void doRender(QImage& _img, int _dx, int _dw) = 0;
	virtual QImage renderOverlay() { return QImage(); }

	virtual void paintEvent(QPaintEvent*);
	virtual void mousePressEvent(QMouseEvent* _e);
	virtual void mouseReleaseEvent(QMouseEvent* _e);
	virtual void mouseMoveEvent(QMouseEvent* _e);
	virtual void wheelEvent(QWheelEvent* _e);

	mutable QMutex m_lock;
	Lightbox::Time m_draggingTime;
	bool m_cursorSizeIsHop;
	Lightbox::Time m_renderedOffset;
	Lightbox::Time m_renderedDuration;
	Lightbox::Time m_renderingOffset;
	Lightbox::Time m_renderingDuration;
	bool m_needsUpdate;
	bool m_sourceChanged;
	Lightbox::Time m_lastEnd;
	Lightbox::Time m_lastWarped;

	QImage m_overlay;
};
