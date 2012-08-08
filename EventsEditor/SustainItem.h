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

#include "StreamEventItem.h"

class SustainBarItem: public QGraphicsItem
{
public:
	SustainBarItem(QPointF const& _begin, QPointF const& _end, float _temperature): m_begin(_begin), m_end(_end), m_temperature(_temperature) {}

	virtual QPainterPath shape() const { QPainterPath ret; ret.addRect(boundingRect()); return ret; }
	virtual QRectF boundingRect() const { return QRectF(m_begin + QPointF(0, 3), QSizeF(m_end.x() - m_begin.x(), 6)); }
	virtual void paint(QPainter* _p, const QStyleOptionGraphicsItem* _o, QWidget* _w);

	void setBegin(QPointF const& _p) { prepareGeometryChange(); m_begin = _p; }
	void setEnd(QPointF const& _p) { prepareGeometryChange(); m_end = _p; }

private:
	QPointF m_begin;
	QPointF m_end;
	float m_temperature;
	float m_subNature;
};

class SustainSuperItem: public StreamEventItem
{
public:
	SustainSuperItem(Lightbox::StreamEvent const& _se): StreamEventItem(_se), m_yPos(2) {}
	virtual QPointF evenUp(QPointF const& _n);

protected:
	float m_yPos;
};

class SustainBasicItem: public SustainSuperItem
{
public:
	SustainBasicItem(Lightbox::StreamEvent const& _se): SustainSuperItem(_se) {}
	virtual QRectF core() const { return QRectF(0, 0, 12, 12); }
	virtual void paint(QPainter* _p, const QStyleOptionGraphicsItem* _o, QWidget* _w);
};

class EndSustainBasicItem: public SustainSuperItem
{
public:
	EndSustainBasicItem(Lightbox::StreamEvent const& _se): SustainSuperItem(_se) {}
	virtual QRectF core() const { return QRectF(-6, 0, 6, 12); }
	virtual void paint(QPainter* _p, const QStyleOptionGraphicsItem* _o, QWidget* _w);
};

class SustainItem: public SustainBasicItem
{
public:
	SustainItem(Lightbox::StreamEvent const& _se): SustainBasicItem(_se) { m_yPos = 14.f; }
};

class EndSustainItem: public EndSustainBasicItem
{
public:
	EndSustainItem(Lightbox::StreamEvent const& _se): EndSustainBasicItem(_se) { m_yPos = 14.f; }
};

class BackSustainItem: public SustainBasicItem
{
public:
	BackSustainItem(Lightbox::StreamEvent const& _se): SustainBasicItem(_se) { m_yPos = 1.f; }
};

class EndBackSustainItem: public EndSustainBasicItem
{
public:
	EndBackSustainItem(Lightbox::StreamEvent const& _se): EndSustainBasicItem(_se) { m_yPos = 1.f; }
};
