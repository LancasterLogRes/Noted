#pragma once

#include <Common/StreamEvent.h>
#include <Common/Color.h>
#include <QGraphicsItem>

class QWidget;
class EventsEditor;
class EventsEditScene;

class StreamEventItem: public QGraphicsItem
{
public:
	StreamEventItem(Lightbox::StreamEvent const& _se);

	QColor color() const { return QColor::fromHsvF(m_se.nature, 1.f, 1.f * Lightbox::Color::hueCorrection(m_se.nature * 360)); }
	QColor cDark() const { return QColor::fromHsvF(m_se.nature, 0.5f, 0.6f * Lightbox::Color::hueCorrection(m_se.nature * 360)); }
	QColor cLight() const { return QColor::fromHsvF(m_se.nature, 0.5f, 1.0f * Lightbox::Color::hueCorrection(m_se.nature * 360)); }
	QColor cPastel() const { return QColor::fromHsvF(m_se.nature, 0.25f, 1.0f * Lightbox::Color::hueCorrection(m_se.nature * 360)); }

	EventsEditor* view() const;
	QPointF distanceFrom(StreamEventItem* _i, QPointF const& _onThem = QPointF(0, 0), QPointF const& _onUs = QPointF(0, 0)) const;
	Lightbox::StreamEvent const& streamEvent() const { return m_se; }
	virtual QPointF evenUp(QPointF const& _n) = 0;
	virtual void setTime(int _hopIndex);
	virtual QRectF core() const = 0;
	virtual QPainterPath shape() const { QPainterPath ret; ret.addRect(core()); return ret; }
	virtual QRectF boundingRect() const;

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

