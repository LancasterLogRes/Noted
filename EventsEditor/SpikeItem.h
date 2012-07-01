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

class ChainItem;

class SpikeChainItem: public StreamEventItem
{
public:
	SpikeChainItem(Lightbox::StreamEvent const& _se): StreamEventItem(_se) {}

	virtual void paint(QPainter* _p, const QStyleOptionGraphicsItem* _o, QWidget* _w);
};

class Chained: public QGraphicsItem
{
public:
	Chained(QPointF const& _begin, QPointF const& _end): m_begin(_begin), m_end(_end) { setFlags(QGraphicsItem::ItemClipsToShape); }

	virtual QRectF boundingRect() const;
	virtual void paint(QPainter* _p, const QStyleOptionGraphicsItem* _o, QWidget* _w);

private:
	QPointF m_begin;
	QPointF m_end;
};

class SpikeItem: public SpikeChainItem
{
public:
	SpikeItem(Lightbox::StreamEvent const& _se): SpikeChainItem(_se) {}

	virtual QRectF core() const;
	virtual QPointF evenUp(QPointF const& _n);

	void paint(QPainter* _p, const QStyleOptionGraphicsItem* _o, QWidget* _w);
};

class ChainItem: public SpikeChainItem
{
public:
	ChainItem(Lightbox::StreamEvent const& _se): SpikeChainItem(_se) {}

	virtual QRectF core() const;
	virtual QPointF evenUp(QPointF const& _n);

	void paint(QPainter* _p, const QStyleOptionGraphicsItem* _o, QWidget* _w);
};
