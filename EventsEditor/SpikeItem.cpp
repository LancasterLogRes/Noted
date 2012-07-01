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
#include "SpikeItem.h"

using namespace std;
using namespace Lightbox;

static const float s_ySpike = 32.f;
static const float s_yChain = 42.f;

void SpikeChainItem::paint(QPainter* _p, const QStyleOptionGraphicsItem*, QWidget*)
{
	float xscale = view()->mapToScene(QPoint(1, 0)).x() - view()->mapToScene(QPoint(0, 0)).x();
	if (isSelected())
	{
		_p->fillRect(QRectF(core().x(), -view()->height(), core().width(), view()->height() * 3), QColor(0, 24, 255, 32));
		_p->fillRect(QRectF(core().x(), -view()->height(), 1, view()->height() * 3), QColor(0, 24, 255, 128));
	}

	if (xscale < 15)
	{
		if (m_se.strength > 0)
		{
			_p->setPen(cDark());
			_p->setBrush(cPastel());
			_p->drawRect(core());
			_p->fillRect(QRectF(core().topLeft(), QSizeF(1, core().height() + 4)), cDark());
			_p->setPen(Qt::black);
			_p->drawText(core(), Qt::AlignCenter, QString(toChar(m_se.character)));
			for (int j = 0; j < log2(m_se.strength) + 6; ++j)
				_p->fillRect(QRectF(core().bottomLeft() + QPointF(2, j * 2 + 2), QSizeF(core().width() - 1, 1)), cDark());
		}
		else
		{
			_p->setPen(cPastel());
			_p->setBrush(Qt::NoBrush);
			_p->drawRect(core());
			_p->fillRect(QRectF(core().topLeft(), QSizeF(1, core().height() + 4)), cDark());
			_p->setPen(Qt::black);
			_p->drawText(core(), Qt::AlignCenter, QString(toChar(m_se.character)));
			for (int j = 0; j < log2(-m_se.strength) + 6; ++j)
				_p->fillRect(QRectF(core().bottomLeft() + QPointF(0, j * 2 + 2), QSizeF(core().width(), 1)), cPastel());
		}
		for (int j = 0; j < log2(m_se.surprise) + 6; ++j)
		{
			_p->fillRect(QRectF(core().topRight() + QPointF((j + 1) * 2, 0), QSizeF(1, core().height() * 5 / 8)), cDark());
			_p->fillRect(QRectF(core().topRight() + QPointF((j + 1) * 2, core().height() * 6 / 8), QSizeF(1, core().height() * 2 / 8)), cDark());
		}
	}
	else
	{
		_p->fillRect(QRectF(core().topLeft(), QSizeF(1, core().height())), m_se.strength > 0 ? cDark() : cPastel());
		for (int j = 0; j < log2(fabs(m_se.strength)) + 6; ++j)
			_p->fillRect(QRectF(core().bottomLeft() + QPointF(0, j * 2 + 2), QSizeF(1, 1)), m_se.strength > 0 ? cDark() : cPastel());
	}
}


QRectF Chained::boundingRect() const
{
	return QRectF(m_begin, QSizeF(m_end.x() - m_begin.x(), m_end.y() - m_begin.y())).normalized().adjusted(0, -1, 0, 1);
}

void Chained::paint(QPainter* _p, const QStyleOptionGraphicsItem*, QWidget*)
{
	_p->setPen(QColor(0, 0, 0, 64));
	_p->drawLine(m_begin, m_end);
}


QRectF SpikeItem::core() const { return QRectF(0, 0, 7, 9); }

void SpikeItem::paint(QPainter* _p, const QStyleOptionGraphicsItem* _o, QWidget* _w)
{
	SpikeChainItem::paint(_p, _o, _w);
}

QPointF SpikeItem::evenUp(QPointF const& _n)
{
	return QPointF(_n.x(), s_ySpike);
}

QPointF ChainItem::evenUp(QPointF const& _n)
{
	return QPointF(_n.x(), s_yChain);
}

QRectF ChainItem::core() const { return QRectF(0, 0, 7, 5); }

void ChainItem::paint(QPainter* _p, const QStyleOptionGraphicsItem* _o, QWidget* _w)
{
	SpikeChainItem::paint(_p, _o, _w);
}
