#include <QtGui>

#include <Common/Common.h>
#include <DMX/Device.h>
#include <Faceman/Output.h>

#include "ChannelItem.h"

using namespace std;
using namespace Lightbox;

ChannelItem::ChannelItem(Lightbox::Output& _o): m_o(_o)
{
	setFlags(QGraphicsItem::ItemIsSelectable);
}

void ChannelItem::paint(QPainter* _p, QStyleOptionGraphicsItem const*, QWidget*)
{
	QRectF r = boundingRect();
	_p->setPen(Qt::black);
	_p->setBrush(Qt::white);
	_p->drawRect(r);
	if (!m_o.isNull())
		_p->drawText(r, Qt::AlignCenter, QString("%1-%2").arg(m_o.asA<DmxOutputImpl>().base()).arg(m_o.asA<DmxOutputImpl>().base() + m_o.asA<DmxOutputImpl>().channels() - 1));
}

static float lth = 0.f;

bool ChannelItem::sceneEvent(QEvent* _e)
{
	if (auto e = dynamic_cast<QGraphicsSceneMouseEvent*>(_e))
		if (e->buttons() == Qt::LeftButton)
		{
//				int d = e->screenPos().x() - e->lastScreenPos().x();
			QPointF rc = e->scenePos() - mapToScene(boundingRect().center());
			int mag = sqrt(sqr(rc.x()) + sqr(rc.y())) / 32;
			float th = atan2(rc.x(), rc.y());
			int d = angularSubtraction(th, lth) * 12 / TwoPi * (mag - 1);
			if (lth == 0.f || d)
				lth = th;
			if (d)
				m_o.setBaseChannel(max<int>(1, min<int>(512 - m_o.channels(), m_o.baseChannel() + d)));
			e->accept();
			return true;
		}
	return false;
}
