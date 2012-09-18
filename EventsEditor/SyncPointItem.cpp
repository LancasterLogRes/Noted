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
#include "SyncPointItem.h"

using namespace std;
using namespace Lightbox;

void SyncPointItem::paint(QPainter* _p, const QStyleOptionGraphicsItem*, QWidget*)
{
//	float xscale = view()->mapToScene(QPoint(1, 0)).x() - view()->mapToScene(QPoint(0, 0)).x();
	if (isSelected())
	{
		_p->fillRect(QRectF(core().x(), -view()->height(), core().width(), view()->height() * 3), QColor(0, 24, 255, 32));
		_p->fillRect(QRectF(core().x(), -view()->height(), 1, view()->height() * 3), QColor(0, 24, 255, 128));
	}

	_p->setPen(Qt::black);
	_p->setBrush(Qt::white);
	_p->drawEllipse(core().center(), core().height() / 2, core().height() / 2);
	_p->drawText(core(), Qt::AlignCenter, QString::number(m_order));
}


QRectF SyncPointItem::core() const
{
	return QRectF(0, 0, 12, 12);
}

QPointF SyncPointItem::evenUp(QPointF const& _n)
{
	return QPointF(_n.x(), 1.f);
}
