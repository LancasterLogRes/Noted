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
#include "PeriodItem.h"

using namespace std;
using namespace lb;

static const float s_yPeriod = 0.f;

float PeriodBarItem::snapped(float _x) const
{
	float c = boundingRect().left();
	float p = m_period / 4;
	return round((_x - c) / p) * p + c;
}

void PeriodBarItem::paint(QPainter* _p, QStyleOptionGraphicsItem const*, QWidget*)
{
	_p->fillRect(boundingRect(), QColor(240, 240, 240));
	if (m_period)
	{
		int li = min<int>(1000, boundingRect().width() / m_period) + 2;
		QPointF brtl = boundingRect().topLeft();
		for (int i = 0; i < li; i += 2)
		{
			_p->fillRect(QRectF(i * m_period, 0, m_period, 10).translated(brtl), QColor(224, 224, 224));
			for (int j = 1; j < 4; j += 2)
			{
				_p->fillRect(QRectF(i * m_period + j * m_period / 4, 0, m_period / 4, 10).translated(brtl), QColor(216, 216, 216));
				_p->fillRect(QRectF((i + 1) * m_period + j * m_period / 4, 0, m_period / 4, 10).translated(brtl), QColor(248, 248, 248));
			}
		}
	}
}

QPointF PeriodItem::evenUp(QPointF const& _n)
{
	return QPointF(_n.x(), s_yPeriod);
}

void PeriodSetTweakItem::mousePressEvent(QGraphicsSceneMouseEvent* _e)
{
	if (_e->buttons() == Qt::MidButton)
		_e->accept();
}

void PeriodSetTweakItem::mouseMoveEvent(QGraphicsSceneMouseEvent* _e)
{
	if (_e->buttons() == Qt::MidButton)
	{
		QPointF rc = _e->pos() - core().center();
		static float lth = 0.f;
		int mag = sqrt(sqr(rc.x()) + sqr(rc.y())) / 32;
		float th = atan2(rc.x(), rc.y());
		int d = angularSubtraction(th, lth) * 12 / twoPi<float>() * (mag - 1);
		if (lth == 0.f || d)
			lth = th;
		m_se.constancy += toSeconds(d) * 1000 / 4;
		update();
		scene()->itemChanged(this);
		_e->accept();
	}
	else
		StreamEventItem::mouseMoveEvent(_e);
}

QRectF PeriodSetTweakItem::core() const { return QRectF(-160, 0, 160, 16); }

void PeriodSetTweakItem::paint(QPainter* _p, QStyleOptionGraphicsItem const*, QWidget*)
{
//	QRectF br = boundingRect();

	_p->setPen(cDark());
	_p->setBrush(cPastel());

	QString l = QString("%1 bpm").arg(round(toBpm(fromSeconds(m_se.constancy)) * 10) / 10);
	int tw = _p->fontMetrics().width(l) + 8;
	QRect tr(-tw, 0, tw, 12);
	if (isSelected())
	{
		_p->fillRect(QRectF(tr.x(), -view()->height(), tw, view()->height() * 3), QColor(0, 24, 255, 32));
		_p->fillRect(QRectF(tr.right(), -view()->height(), 1, view()->height() * 3), QColor(0, 24, 255, 128));
	}
	_p->drawRect(tr);
	_p->setPen(Qt::black);
	_p->drawText(tr, Qt::AlignCenter, l);
}

void PeriodResetItem::paint(QPainter* _p, QStyleOptionGraphicsItem const*, QWidget*)
{
	if (isSelected())
	{
		_p->fillRect(QRectF(core().x(), -view()->height(), core().width(), view()->height() * 3), QColor(0, 24, 255, 32));
		_p->fillRect(QRectF(core().x(), -view()->height(), 1, view()->height() * 3), QColor(0, 24, 255, 128));
	}
	_p->setPen(QColor(128, 0, 0, 255));
	_p->setBrush(QColor(255, 0, 0, 32));
	_p->drawRect(core());
	_p->setPen(Qt::black);
	_p->drawText(core(), Qt::AlignCenter, QString("X"));
}
