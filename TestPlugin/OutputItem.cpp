#include <QtGui>

#include <Common/Common.h>
#include <NotedPlugin/NotedFace.h>

#include "LightshowPlugin.h"
#include "ChannelItem.h"
#include "OutputItem.h"

using namespace std;
using namespace Lightbox;

OutputItem::OutputItem(QGraphicsScene* _s, Output const& _o): m_o(_o)
{
	_s->addItem(this);
	ChannelItem* ci = new ChannelItem(m_o);
	_s->addItem(ci);
	ci->setParentItem(this);
	ci->setRect(0, boundingRect().height() / 2, 48, boundingRect().height() / 2);
	p()->setOutput(m_o, this);
}

bool OutputItem::haveColor() const
{
	return m_o.colorSet() != Lightbox::WhiteOnly;
}

bool OutputItem::haveDirection() const
{
	return !m_o.directionLimits().isEmpty();
}

bool OutputItem::haveGobo() const
{
	return m_o.maxGobo();
}

void OutputItem::restore()
{
	m_o = p()->newOutput(m_savedName, m_savedCh, m_savedPh);
	p()->setOutput(m_o, this);
}

QString OutputItem::name() const
{
	return QString::fromStdString(m_o.name());
}

void OutputItem::save()
{
	m_savedName = name();
	m_savedCh = m_o.baseChannel();
	m_savedPh = m_o.physical();
	m_o = Output();
}

void OutputItem::paint(QPainter* _p, QStyleOptionGraphicsItem const*, QWidget*)
{
	//			_p->drawRoundedRect(QRectF(r.right - ew, r.y(), ew, r.height()), 3, 3);
	_p->setPen(QColor::fromHsvF(0, 0, 0.5));
	_p->setBrush(QColor::fromHsvF(0, 0, 0.85));
	QRectF nr = boundingRect().adjusted(0, 0, 0, 0);
	_p->drawRect(nr);
	if (!m_o.isNull())
	{
		_p->drawText(nr.adjusted(2, 1, -2, -1), Qt::AlignLeft|Qt::AlignTop, QString::fromStdString(m_o.nickName()));
		OutputProfiles ops = cursorProfiles();
		if (ops.size())
		{
			OutputProfile const& op = ops[0];	// TODO: display all channels.
/*			if (m_o.colorSet() & SeparateRed && m_o.colorSet() & SeparateGreen && m_o.colorSet() & SeparateBlue)
			{
				_p->fillRect(nr.right() - 16, nr.y(), 16, nr.height() / 3, QColor(op.color.r(), 0, 0));
				_p->fillRect(nr.right() - 16, nr.y() + nr.height() / 3, 16, nr.height() / 3, QColor(0, op.color.g(), 0));
				_p->fillRect(nr.right() - 16, nr.y() + 2 * nr.height() / 3, 16, nr.height() / 3, QColor(0, 0, op.color.b()));
			}
			else*/
				_p->fillRect(nr.right() - 16, nr.top(), 16, nr.bottom(), QColor(op.color.r(), op.color.g(), op.color.b()));
			if (haveDirection())
			{
				_p->setPen(Qt::black);
				_p->setBrush(Qt::NoBrush);
				_p->drawPie(QRectF(nr.right() - 16 - nr.height(), 0, nr.height(), nr.height()), m_o.directionLimits().from.theta * 16 * 180 / Pi, (m_o.directionLimits().to.theta - m_o.directionLimits().from.theta) * 16 * 180 / Pi);
				_p->drawPie(QRectF(nr.right() - 16 - 2 * nr.height(), 0, nr.height(), nr.height()), m_o.directionLimits().from.phi * 16 * 180 / Pi, (m_o.directionLimits().to.phi - m_o.directionLimits().from.phi) * 16 * 180 / Pi);
				_p->setBrush(Qt::black);
				float spread = 12.f / 180 * Pi;
				_p->drawPie(QRectF(nr.right() - 16 - nr.height(), 0, nr.height(), nr.height()), (op.direction.theta - spread / 2) * 16 * 180 / Pi, (spread) * 16 * 180 / Pi);
				_p->drawPie(QRectF(nr.right() - 16 - 2 * nr.height(), 0, nr.height(), nr.height()), (op.direction.phi - spread / 2) * 16 * 180 / Pi, (spread) * 16 * 180 / Pi);
			}
		}
	}
}

void OutputItem::initProfiles(unsigned _s)
{
	QMutexLocker l(&x_op);
	m_op.clear();
	m_op.reserve(_s);
}

void OutputItem::appendProfile()
{
	QMutexLocker l(&x_op);
	m_op.append(m_o.profiles());
}

QPointF OutputItem::port() const
{
	return mapToScene(QPointF(0, boundingRect().height() / 2));
}

QPointF OutputItem::sep()
{
	return QPointF(0, 36);
}

void OutputItem::reorder(QGraphicsItem* _g)
{
	CoreItem::reorder(_g, QPointF(0, 36));
}

QRectF OutputItem::boundingRect() const
{
	return QRectF(0, 0, 200, 32);
}

OutputProfile OutputItem::outputProfile(unsigned _i, unsigned _li) const
{
	QMutexLocker l(&x_op);
	return ((int)_i < m_op.size()) ? m_op[_i][_li] : NullOutputProfile;
}

OutputProfiles OutputItem::outputProfiles(unsigned _i) const
{
	QMutexLocker l(&x_op);
	return ((int)_i < m_op.size()) ? m_op[_i] : NullOutputProfiles;
}

OutputProfiles OutputItem::cursorProfiles() const
{
	if (c()->isRecording())
		return m_o.profiles();
	return outputProfiles(c()->cursorIndex());
}

void OutputItem::shiftProfiles(unsigned _n)
{
	QMutexLocker l(&x_op);
	m_op.erase(m_op.begin(), next(m_op.begin(), _n));
}
