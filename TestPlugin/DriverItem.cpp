#include <QtGui>

#include <Common/Common.h>
#include <NotedPlugin/NotedFace.h>

#include "LightshowPlugin.h"
#include "DriverItem.h"

using namespace std;
using namespace Lightbox;

DriverItem::DriverItem(Driver const& _d): m_d(_d) {}

void DriverItem::restore()
{
	m_d = p()->newDriver(m_savedName);
}

QString DriverItem::name() const
{
	return QString::fromStdString(m_d.name());
}

void DriverItem::save()
{
	m_savedName = name();
	m_d = Driver();
}

void DriverItem::paint(QPainter* _p, QStyleOptionGraphicsItem const*, QWidget*)
{
	_p->setPen(QColor::fromHsvF(0, 0, 0.5, 1));
	_p->setBrush(QColor::fromHsvF(0, 0, 0.5, 0.3));
	QRectF nr = boundingRect();
	_p->drawRect(nr);
	if (!m_d.isNull())
	{
		_p->drawText(nr.adjusted(2, 1, -2, -1), QString::fromStdString(m_d.name()));
		_p->drawText(nr.adjusted(2, 17, -2, -1), QString("%1 properties").arg(m_d.properties().size()));
	}
}

QPointF DriverItem::port() const
{
	return mapToScene(QPointF(boundingRect().right(), boundingRect().height() / 2));
}

QPointF DriverItem::sep()
{
	return QPointF(0, 36);
}

void DriverItem::reorder(QGraphicsItem* _g)
{
	CoreItem::reorder(_g, QPointF(0, 36));
}

QRectF DriverItem::boundingRect() const
{
	return QRectF(0, 0, 200, 32);
}
