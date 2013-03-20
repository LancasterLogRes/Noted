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

#include <QPainter>
#include <QDebug>
#include <QApplication>
#include <QEvent>
#include <QGraphicsSceneWheelEvent>
#include <QKeyEvent>
#include <Common/Angular.h>
#include <NotedPlugin/NotedFace.h>
#include "EventsEditor.h"
#include "EventsEditScene.h"
#include "AttackItem.h"
#include "SustainItem.h"
#include "PeriodItem.h"
#include "SyncPointItem.h"
#include "StreamEventItem.h"

using namespace std;
using namespace Lightbox;

static const qreal pw = 1;
static const qreal hpw = pw / 2;

static const float s_yPeriod = 64.f;

StreamEventItem::StreamEventItem(StreamEvent const& _se):
	QGraphicsItem(),
	m_se(_se)
{
	setFlags(ItemIgnoresTransformations|ItemIsSelectable|ItemIsFocusable|ItemIsMovable|ItemSendsGeometryChanges);
}

float levelled(float _x, int _levels)
{
	return min<float>(_levels - 1, floor(_x * _levels)) / _levels;
}

QPointF StreamEventItem::evenUp(QPointF const& _n)
{
	return QPointF(_n.x(), 15.f);
}

QPointF StreamEventItem::distanceFrom(StreamEventItem* _it, QPointF const& _onThem, QPointF const& _onUs) const
{
	auto v = view()->viewportTransform();
	return _it->deviceTransform(v).map(_onThem) - deviceTransform(v).map(_onUs);
}

void StreamEventItem::mouseMoveEvent(QGraphicsSceneMouseEvent* _e)
{
	if (_e->buttons() == Qt::MidButton)
	{
		QPointF rc = _e->pos() - core().center();
//		static float lth = 0.f;
		int mag = sqrt(sqr(rc.x()) + sqr(rc.y())) / 32;
		float th = atan2(rc.x(), rc.y());
		if (mag > 0)
			m_se.temperature = levelled(fracPart((th + twoPi<float>()) / twoPi<float>()), 6 * sqr(mag));
		update();
		scene()->itemChanged(this);
		_e->accept();
	}
	else
		QGraphicsItem::mouseMoveEvent(_e);
}

QVariant StreamEventItem::itemChange(GraphicsItemChange _change, QVariant const& _value)
{
	if (_change == ItemPositionChange)
	{
		QPointF v = QPointF(0, (m_se.channel == -1) ? 0 : (m_se.channel * 32 + 16)) + evenUp(_value.toPointF());

		if (!(QApplication::keyboardModifiers() & Qt::ControlModifier) && scene())
		{
			foreach (auto it, scene()->items(QPointF(v.x(), s_yPeriod)))
				if (auto pbi = dynamic_cast<PeriodBarItem*>(it))
					v.setX(pbi->snapped(v.x()));
		}
		setZValue(v.x());
		if (scene())
		{
			scene()->itemChanged(this);	// OPTIMIZE: call once for batch moves.
			scene()->c()->setCursor(fromSeconds(x() / 1000), true);
		}
		return v;
	}
	else if (_change == ItemSelectedHasChanged)
		prepareGeometryChange();
	return QGraphicsItem::itemChange(_change, _value);
}

bool StreamEventItem::sceneEvent(QEvent* _e)
{
//	if (QGraphicsSceneMouseEvent* e = dynamic_cast<QGraphicsSceneMouseEvent*>(_e))
	return QGraphicsItem::sceneEvent(_e);
}

void StreamEventItem::keyPressEvent(QKeyEvent* _e)
{
	Character c = toCharacter(_e->text()[0].toUpper().toLatin1());
	if (_e->text() == "_")
		c = Dull;
	if (_e->text()[0].toUpper().toLatin1() == toChar(c) || _e->text() == "_")
	{
		m_se.character = c;
		scene()->itemChanged(this);
		update();
	}
}

float StreamEventItem::magFactor() const
{
	return view()->mapToScene(QPoint(1, 0)).x() - view()->mapToScene(QPoint(0, 0)).x();
}

void StreamEventItem::wheelEvent(QGraphicsSceneWheelEvent* _e)
{
	bool isSur = (_e->modifiers() & Qt::ShiftModifier);
	float& v = isSur ? m_se.surprise : m_se.strength;

	float const static c_lowest = 1.f / 32;

	if (isSur && v == 0 && _e->delta() > 0)
		v = c_lowest;
	else if (isSur && v <= c_lowest && _e->delta() < 0)
		v = 0;
	else if (!isSur && ((v > 0 && v <= c_lowest && _e->delta() < 0) || (v < 0 && v >= -c_lowest && _e->delta() > 0)))
		v = -v;
	else if ((_e->delta() > 0) == (v > 0))
		v *= 2;
	else
		v /= 2;
	if (isSur)
		v = clamp(v, 0.f, 1.f);
	else
		v = clamp(v, -1.f, 1.f);
	update();
	scene()->setDirty(true);
}

EventsEditScene* StreamEventItem::scene() const
{
	return dynamic_cast<EventsEditScene*>(const_cast<StreamEventItem*>(this)->QGraphicsItem::scene());
}

void StreamEventItem::handleSelected(QPainter* _p)
{
	_p->setRenderHint(QPainter::Antialiasing);
	if (isSelected())
	{
		_p->fillRect(QRectF(core().x(), -view()->height(), core().width(), view()->height() * 3), QColor(0, 24, 255, 32));
		_p->fillRect(QRectF(core().x(), -view()->height(), 1, view()->height() * 3), QColor(0, 24, 255, 128));
	}
	_p->setPen(cDark());
	_p->setBrush(cPastel());
}

StreamEventItem* StreamEventItem::newItem(Lightbox::StreamEvent const& _se)
{
	StreamEventItem* it = nullptr;
#define DO(X) \
	if (_se.type == X) \
		it = new X ## Item(_se);
#include "DoEventTypes.h"
#undef DO
	return it;
}

EventsEditor* StreamEventItem::view() const
{
	return scene()->view();
}

void StreamEventItem::setTime(int _hopIndex)
{
	setPos(_hopIndex, pos().y());
	setZValue(_hopIndex);
	scene()->itemChanged(this);
}

QRectF StreamEventItem::boundingRect() const
{
//	if (isSelected())
		return QRectF(core().x() - hpw, -view()->height(), core().width() + hpw, view()->height() * 3);
//	else
//		return core().adjusted(-hpw, -hpw, hpw + 12, hpw + 12);
}
