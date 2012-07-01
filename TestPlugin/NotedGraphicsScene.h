#pragma once

#include <QGraphicsScene>

class LightshowPlugin;

class NotedGraphicsScene: public QGraphicsScene
{
public:
	NotedGraphicsScene(LightshowPlugin* _c);

	LightshowPlugin* c() const { return m_c; }

protected:
	void dragEnterEvent(QGraphicsSceneDragDropEvent* _e);
	void dragMoveEvent(QGraphicsSceneDragDropEvent* _e);
	void dropEvent(QGraphicsSceneDragDropEvent* _e);

	LightshowPlugin* m_c;
};

