#include <QtGui>
#include <Common/Common.h>
#include <NotedPlugin/NotedFace.h>

#include "LightshowPlugin.h"
#include "CoreItem.h"

using namespace std;
using namespace Lightbox;

CoreItem::CoreItem()
{
	setFlags(QGraphicsItem::ItemIsMovable|QGraphicsItem::ItemIsSelectable);
}

LightshowPlugin* CoreItem::p() const
{
	LightshowPlugin* ret = dynamic_cast<LightshowPlugin*>(scene()->parent());
	assert(ret);
	return ret;
}

NotedFace* CoreItem::c() const
{
	return p()->noted();
}

void CoreItem::advance(int)
{
	if (!m_dragging)
	{
		if ((m_targetPosition - pos()).manhattanLength() > 2)
			setPos(lerp<200>(pos().x(), m_targetPosition.x()), lerp<500>(pos().y(), m_targetPosition.y()));
		else
			setPos(m_targetPosition);
	}
}

bool CoreItem::sceneEvent(QEvent* _e)
{
	if (auto e = dynamic_cast<QGraphicsSceneMouseEvent*>(_e))
	{
		if (e->buttons() == Qt::LeftButton && _e->type() == QEvent::GraphicsSceneMousePress)
			m_dragging = true;
		else if (_e->type() == QEvent::GraphicsSceneMouseRelease)
			m_dragging = false;
		else if (e->buttons() == Qt::LeftButton && _e->type() == QEvent::GraphicsSceneMouseMove)
		{
			setPos(QPointF(pos().x(), pos().y() + e->pos().y() - e->lastPos().y()));
			reorder(parentItem(), sep());
		}
		else
			return false;
		e->accept();
		return true;
	}
	return false;
}

void CoreItem::reorder(QGraphicsItem* _g, QPointF const& _sep)
{
	vector<CoreItem*> cw;
	foreach (QGraphicsItem* it, _g->childItems())
		if (CoreItem* c = dynamic_cast<CoreItem*>(it))
			cw.push_back(c);
	sort(cw.begin(), cw.end(), [](CoreItem* _a, CoreItem* _b) { return _a->pos().y() < _b->pos().y(); });
	QPointF s(0, 0);
	foreach (CoreItem* c, cw)
	{
		c->m_targetPosition = s;
		s += _sep;
	}
}
