#pragma once

#include "StreamEventItem.h"

class SustainBarItem: public QGraphicsItem
{
public:
	SustainBarItem(QPointF const& _begin, QPointF const& _end, float _nature): m_begin(_begin), m_end(_end), m_nature(_nature) {}

	virtual QPainterPath shape() const { QPainterPath ret; ret.addRect(boundingRect()); return ret; }
	virtual QRectF boundingRect() const { return QRectF(m_begin + QPointF(0, 3), QSizeF(m_end.x() - m_begin.x(), 6)); }
	virtual void paint(QPainter* _p, const QStyleOptionGraphicsItem* _o, QWidget* _w);

	void setBegin(QPointF const& _p) { prepareGeometryChange(); m_begin = _p; }
	void setEnd(QPointF const& _p) { prepareGeometryChange(); m_end = _p; }

private:
	QPointF m_begin;
	QPointF m_end;
	float m_nature;
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
