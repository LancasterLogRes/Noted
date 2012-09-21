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

QRectF SustainBarItem::boundingRect() const
{
	return QRectF(m_begin + QPointF(0, 3), QSizeF(m_end.x() - m_begin.x(), 6 + (log2(m_strength) + 7) * 2));
}

void SustainBarItem::paint(QPainter* _p, QStyleOptionGraphicsItem const*, QWidget*)
{
/*	QRectF br = boundingRect();
	br.setHeight(32);
	_p->fillRect(br, QColor::fromHsvF(toHue(m_temperature), .25f, 1.f * Lightbox::Color::hueCorrection(toHue(m_temperature))));
	_p->fillRect(QRectF(br.topLeft(), QSizeF(br.width(), 1)), QColor::fromHsvF(toHue(m_temperature), .5f, .6f * Lightbox::Color::hueCorrection(toHue(m_temperature))));
	for (int j = 0; j < log2(m_strength) + 6; ++j)
		_p->fillRect(QRectF(br.bottomLeft() + QPointF(0, j * 2 + 2), QSizeF(br.width(), 1)), QColor::fromHsvF(toHue(m_temperature), .5f, .6f * Lightbox::Color::hueCorrection(toHue(m_temperature))));*/
}

QPointF SustainSuperItem::evenUp(QPointF const& _n)
{
	return QPointF(_n.x(), 15.f);
}

QRectF SustainItem::core() const
{
	return QRectF(0, 0, 20, 16);
}

void SustainItem::paint(QPainter* _p, QStyleOptionGraphicsItem const*, QWidget*)
{
	handleSelected(_p);
	_p->setPen(cDark());
	_p->setBrush(cPastel());
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
	_p->setPen(cDark());
	_p->setBrush(cPastel());
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
	_p->setBrush(Qt::NoBrush);
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
