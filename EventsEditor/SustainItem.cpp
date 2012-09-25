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
#include <Common/Angular.h>
#include <NotedPlugin/NotedFace.h>
#include "EventsEditor.h"
#include "EventsEditScene.h"
#include "SustainItem.h"

using namespace std;
using namespace Lightbox;

SustainBarItem::SustainBarItem(QPointF const& _begin, QPointF const& _end, Lightbox::StreamEvent const& _bEv, Lightbox::StreamEvent const& _eEv):
	m_begin(_begin), m_end(_end), m_beginEvent(_bEv), m_endEvent(_eEv)
{
	setPos(m_begin);
}

QRectF SustainBarItem::boundingRect() const
{
	return QRectF(0, -15, m_end.x() - m_begin.x(), 16);
}

void SustainBarItem::paint(QPainter* _p, QStyleOptionGraphicsItem const*, QWidget*)
{
	if (m_beginEvent.type == Attack)
	{
		_p->setBrush(QColor::fromHsvF(toHue(m_endEvent.temperature), .0625f, 1.f * Lightbox::Color::hueCorrection(toHue(m_endEvent.temperature))));
		_p->setPen(Qt::NoPen);
		_p->drawPolygon(QPolygonF() << QPointF(0, 7 - 15 - 7 * m_beginEvent.strength) << QPointF(0, -6 + 7 * m_beginEvent.strength) << QPointF(m_end.x() - m_begin.x(), -6 + 7 * m_endEvent.strength) << QPointF(m_end.x() - m_begin.x(), 7 - 15 - 7 * m_endEvent.strength));
	}
	else
	{
		_p->fillRect(QRectF(0, 7 - 15 - 7 * m_beginEvent.strength, m_end.x() - m_begin.x(), 2 + 14 * m_beginEvent.strength), QColor::fromHsvF(toHue(m_beginEvent.temperature), .25f, 1.f * Lightbox::Color::hueCorrection(toHue(m_beginEvent.temperature))));
	}
}

QRectF AttackItem::core() const
{
	return QRectF(0, 0, 10, 16);
}

void AttackItem::paint(QPainter* _p, const QStyleOptionGraphicsItem*, QWidget*)
{
	handleSelected(_p);
	if (isMagnified())
	{
		auto cc = core().center();
		_p->drawPolygon(QPolygonF(QVector<QPointF>() <<
								  core().topLeft() <<
								  core().bottomLeft() <<
								  QPointF(core().right(), cc.y()) ));
		_p->setPen(Qt::black);
		_p->drawText(core(), Qt::AlignCenter, QString(toChar(m_se.character)));
	}
	_p->fillRect(QRectF(core().topLeft(), QSizeF(1, -16)), qLinearGradient(core().topLeft(), QPointF(core().left(), -16), cLight(), Qt::transparent));
}

QRectF SustainItem::core() const
{
	return QRectF(0, 0, 20, 16);
}

void SustainItem::paint(QPainter* _p, QStyleOptionGraphicsItem const*, QWidget*)
{
	handleSelected(_p);
	if (isMagnified())
	{
		auto cc = core().center();
		auto cw = core().width();
		_p->drawPolygon(QPolygonF(QVector<QPointF>() <<
								  core().topLeft() <<
								  QPointF(core().left() + cw / 4, cc.y()) <<
								  core().bottomLeft() <<
								  QPointF(core().right() - cw / 4, core().bottom()) <<
								  QPointF(core().right(), cc.y()) <<
								  QPointF(core().right() - cw / 4, core().top()) ));
		_p->setPen(Qt::black);
		_p->drawText(core(), Qt::AlignCenter, QString(toChar(m_se.character)));
		_p->setPen(cDark());
	}
	_p->drawLine(core().topLeft(), QPointF(core().left(), -16));
}

QRectF DecayItem::core() const
{
	return QRectF(0, 0, 20, 16);
}

void DecayItem::paint(QPainter* _p, QStyleOptionGraphicsItem const*, QWidget*)
{
	handleSelected(_p);
	if (isMagnified())
	{
		auto cc = core().center();
		auto cw = core().width();
		_p->drawPolygon(QPolygonF(QVector<QPointF>() <<
								  core().topLeft() <<
								  QPointF(core().left() + cw / 4, cc.y()) <<
								  core().bottomLeft() <<
								  QPointF(core().right(), cc.y())));
	}
	_p->drawLine(core().topLeft(), QPointF(core().left(), -16));
}

QRectF ReleaseItem::core() const
{
	return QRectF(-20, 0, 20, 16);
}

void ReleaseItem::paint(QPainter* _p, QStyleOptionGraphicsItem const*, QWidget*)
{
	handleSelected(_p);
	_p->setPen(cLight());
	_p->setBrush(cPastel());
	if (isMagnified())
	{
		auto cc = core().center();
		auto cw = core().width();
		_p->drawPolygon(QPolygonF(QVector<QPointF>() <<
								  core().topLeft() <<
								  QPointF(core().right(), cc.y()) <<
								  core().bottomLeft() + QPointF(1, 0) <<
								  core().bottomRight() + QPointF(1, 0) <<
								  core().topRight()));
	}
	_p->fillRect(QRectF(core().topRight(), QSizeF(1, -16)), qLinearGradient(core().topRight(), QPointF(core().right(), -16), cLight(), Qt::transparent));
}
