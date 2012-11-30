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

#include <EventCompiler/StreamEvent.h>
#include <Common/RGBA8.h>
#include <QGraphicsItem>

class QWidget;
class EventsEditor;
class EventsEditScene;

inline QBrush qLinearGradient(QPointF _a, QPointF _b, QColor _ac, QColor _bc)
{
	QLinearGradient g(_a, _b);
	g.setColorAt(0, _ac);
	g.setColorAt(1, _bc);
	return QBrush(g);
}

class StreamEventItem: public QGraphicsItem
{
public:
	StreamEventItem(Lightbox::StreamEvent const& _se);

	QColor cDark() const { return m_se.temperature == -1 ? QColor::fromHsv(0, 0, 128) : QColor::fromHsvF(Lightbox::toHue(m_se.temperature), 0.5f, 0.6f * Lightbox::Color::hueCorrection(Lightbox::toHue(m_se.temperature))); }
	QColor color() const { return m_se.temperature == -1 ? QColor::fromHsv(0, 0, 160) : QColor::fromHsvF(Lightbox::toHue(m_se.temperature), 1.f, 1.f * Lightbox::Color::hueCorrection(Lightbox::toHue(m_se.temperature))); }
	QColor cLight() const { return m_se.temperature == -1 ? QColor::fromHsv(0, 0, 192) : QColor::fromHsvF(Lightbox::toHue(m_se.temperature), 0.5f, 1.0f * Lightbox::Color::hueCorrection(Lightbox::toHue(m_se.temperature))); }
	QColor cPastel() const { return m_se.temperature == -1 ? QColor::fromHsv(0, 0, 224) : QColor::fromHsvF(Lightbox::toHue(m_se.temperature), 0.25f, 1.0f * Lightbox::Color::hueCorrection(Lightbox::toHue(m_se.temperature))); }

	EventsEditor* view() const;
	QPointF distanceFrom(StreamEventItem* _i, QPointF const& _onThem = QPointF(0, 0), QPointF const& _onUs = QPointF(0, 0)) const;
	Lightbox::StreamEvent const& streamEvent() const { return m_se; }
	virtual QPointF evenUp(QPointF const& _n);
	virtual void setTime(int _hopIndex);
	virtual void setChannel(int _ch) { m_se.assign(_ch); }
	virtual QRectF core() const = 0;
	virtual QPainterPath shape() const { QPainterPath ret; ret.addRect(core()); return ret; }
	virtual QRectF boundingRect() const;
	virtual bool isCausal() const { return true; }
	void handleSelected(QPainter* _p);
	bool isMagnified() const { return magFactor() < 15; }
	float magFactor() const;

	static StreamEventItem* newItem(Lightbox::StreamEvent const& _se);
	EventsEditScene* scene() const;

protected:
	virtual void keyPressEvent(QKeyEvent* _e);
	virtual void wheelEvent(QGraphicsSceneWheelEvent* _e);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* _e);
	virtual bool sceneEvent(QEvent* _e);
	virtual QVariant itemChange(GraphicsItemChange _change, QVariant const& _v);

	Lightbox::StreamEvent m_se;
};

