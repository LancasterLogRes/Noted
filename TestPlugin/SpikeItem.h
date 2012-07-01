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
