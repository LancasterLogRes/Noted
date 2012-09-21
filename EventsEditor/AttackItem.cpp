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
#include "AttackItem.h"

using namespace std;
using namespace Lightbox;

QPointF AttackItem::evenUp(QPointF const& _n)
{
	return QPointF(_n.x(), 15);
}

QRectF AttackItem::core() const
{
	return QRectF(0, 0, 10, 16);
}

void AttackItem::paint(QPainter* _p, const QStyleOptionGraphicsItem*, QWidget*)
{
	handleSelected(_p);
	_p->setPen(cDark());
	_p->setBrush(cPastel());
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

QRectF Chained::boundingRect() const
{
	return QRectF(m_begin, QSizeF(m_end.x() - m_begin.x(), m_end.y() - m_begin.y())).normalized().adjusted(0, -1, 0, 1);
}

void Chained::paint(QPainter* _p, const QStyleOptionGraphicsItem*, QWidget*)
{
}
