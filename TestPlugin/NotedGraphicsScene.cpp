#include <QtGui>

#include <Common/Common.h>
#include <NotedPlugin/NotedFace.h>

#include "DriverItem.h"
#include "OutputItem.h"
#include "DrivenItem.h"
#include "LightshowPlugin.h"
#include "NotedGraphicsScene.h"

using namespace Lightbox;

NotedGraphicsScene::NotedGraphicsScene(LightshowPlugin* _c): QGraphicsScene(_c), m_c(_c)
{
}

void NotedGraphicsScene::dragEnterEvent(QGraphicsSceneDragDropEvent* _e)
{
	qDebug() << _e->source() << m_c->m_fixture->driversList << m_c->m_fixture->outputsList;
	if (_e->source() == m_c->m_fixture->driversList || _e->source() == m_c->m_fixture->outputsList)
		_e->accept();
	else
		return QGraphicsScene::dragEnterEvent(_e);
}

void NotedGraphicsScene::dragMoveEvent(QGraphicsSceneDragDropEvent* _e)
{
	if (_e->source() == m_c->m_fixture->driversList || _e->source() == m_c->m_fixture->outputsList)
		_e->accept();
	else
		return QGraphicsScene::dragMoveEvent(_e);
}

void NotedGraphicsScene::dropEvent(QGraphicsSceneDragDropEvent* _e)
{
	if (_e->source() == m_c->m_fixture->driversList)
	{
		qDebug() << "Dropping" << m_c->m_fixture->driversList->currentItem()->data(0).toString();
		DriverItem* dw = new DriverItem(m_c->newDriver(m_c->m_fixture->driversList->currentItem()->data(0).toString()));
		m_c->m_scene.addItem(dw);
		m_c->m_drivers->addToGroup(dw);
		DriverItem::reorder(m_c->m_drivers);
		if (auto ow = dynamic_cast<OutputItem*>(itemAt(_e->scenePos())))
		{
			foreach (auto it, items())
				if (DrivenItem* dnw = dynamic_cast<DrivenItem*>(it))
					if (dnw->ow() == ow)
						delete dnw;
			auto dnw = new DrivenItem(dw, ow);
			addItem(dnw);
		}
	}
	else if (_e->source() == m_c->m_fixture->outputsList)
	{
		OutputItem* ow = new OutputItem(&m_c->m_scene, m_c->newOutput(m_c->m_fixture->outputsList->currentItem()->data(0).toString(), 1, Physical()));
		m_c->m_outputs->addToGroup(ow);
		OutputItem::reorder(m_c->m_outputs);
		if (auto dw = dynamic_cast<DriverItem*>(itemAt(_e->scenePos())))
		{
			auto dnw = new DrivenItem(dw, ow);
			addItem(dnw);
		}
	}
	else
	{
		return QGraphicsScene::dropEvent(_e);
	}
	m_c->noteDriversOrOutputsChanged();
	_e->accept();
}

