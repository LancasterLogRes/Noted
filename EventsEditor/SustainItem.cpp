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

void SustainBarItem::paint(QPainter* _p, QStyleOptionGraphicsItem const*, QWidget*)
{
	QRectF br = boundingRect();
	_p->fillRect(br, QColor::fromHsvF(m_nature, .25f, 1.f * Lightbox::Color::hueCorrection(m_nature * 360)));
	_p->fillRect(QRectF(br.topLeft(), QSizeF(br.width(), 1)), QColor::fromHsvF(m_nature, .5f, .6f * Lightbox::Color::hueCorrection(m_nature * 360)));
}

QPointF SustainSuperItem::evenUp(QPointF const& _n)
{
	return QPointF(_n.x(), m_yPos);
}

void SustainBasicItem::paint(QPainter* _p, QStyleOptionGraphicsItem const*, QWidget*)
{
	if (isSelected())
	{
		_p->fillRect(QRectF(core().x(), -view()->height(), core().width(), view()->height() * 3), QColor(0, 24, 255, 32));
		_p->fillRect(QRectF(core().x(), -view()->height(), 1, view()->height() * 3), QColor(0, 24, 255, 128));
	}
	_p->setPen(cDark());
	_p->setBrush(cPastel());
	_p->drawPolygon(QPolygonF(QVector<QPointF>() << QPointF(core().right(), core().center().y()) << QPointF(core().center().x(), core().bottom()) << core().bottomLeft() << core().topLeft() << QPointF(core().center().x(), core().top())));
	_p->setPen(Qt::black);
	_p->drawText(core().adjusted(0, 0, -4, 0), Qt::AlignCenter, QString(toChar(m_se.character)));
}

void EndSustainBasicItem::paint(QPainter* _p, QStyleOptionGraphicsItem const*, QWidget*)
{
	if (isSelected())
	{
		_p->fillRect(QRectF(core().x(), -view()->height(), core().width(), view()->height() * 3), QColor(0, 24, 255, 32));
		_p->fillRect(QRectF(core().right(), -view()->height(), 1, view()->height() * 3), QColor(0, 24, 255, 128));
	}
	_p->setPen(QColor(128, 128, 128));
	_p->setBrush(QColor(240, 240, 240));
	_p->drawRect(QRectF(-6, 0, 6, 3));
	_p->drawRect(QRectF(-6, 9, 6, 3));
	_p->drawRect(QRectF(-1, 3, 1, 6));
}
