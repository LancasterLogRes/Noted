#pragma once

#include "StreamEventItem.h"

class PeriodBarItem: public QGraphicsItem
{
public:
	PeriodBarItem(QPointF const& _begin, QPointF const& _end, float _period): m_begin(_begin), m_end(_end), m_period(_period) { setFlag(QGraphicsItem::ItemClipsToShape); }

	float snapped(float _x) const;

	virtual QPainterPath shape() const { QPainterPath ret; ret.addRect(boundingRect()); return ret; }
	QRectF boundingRect() const { return QRectF(m_begin, QSizeF(m_end.x() - m_begin.x(), 10)); }
	void paint(QPainter* _p, const QStyleOptionGraphicsItem* _o, QWidget* _w);

	void setBegin(QPointF const& _p) { prepareGeometryChange(); m_begin = _p; }
	void setEnd(QPointF const& _p) { prepareGeometryChange(); m_end = _p; }

private:
	QPointF m_begin;
	QPointF m_end;
	float m_period;
};

class PeriodItem: public StreamEventItem
{
public:
	PeriodItem(Lightbox::StreamEvent const& _se): StreamEventItem(_se) {}
	virtual QPointF evenUp(QPointF const& _n);
};

class PeriodSetTweakItem: public PeriodItem
{
public:
	PeriodSetTweakItem(Lightbox::StreamEvent const& _se): PeriodItem(_se) {}
	virtual QRectF core() const;
	virtual void paint(QPainter* _p, const QStyleOptionGraphicsItem* _o, QWidget* _w);
	virtual void mousePressEvent(QGraphicsSceneMouseEvent* _e);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* _e);
};

class PeriodSetItem: public PeriodSetTweakItem
{
public:
	PeriodSetItem(Lightbox::StreamEvent const& _se): PeriodSetTweakItem(_se) {}
};

class PeriodTweakItem: public PeriodSetTweakItem
{
public:
	PeriodTweakItem(Lightbox::StreamEvent const& _se): PeriodSetTweakItem(_se) {}
};

class PeriodResetItem: public PeriodItem
{
public:
	PeriodResetItem(Lightbox::StreamEvent const& _se): PeriodItem(_se) {}
	virtual QRectF core() const { return QRectF(0, 0, 12, 12); }
	virtual void paint(QPainter* _p, const QStyleOptionGraphicsItem* _o, QWidget* _w);
};
