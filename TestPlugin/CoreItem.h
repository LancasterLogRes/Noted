#pragma once

#include <QGraphicsItem>

class NotedFace;
class LightshowPlugin;

class CoreItem: public QGraphicsItem
{
public:
	CoreItem();

	virtual QPointF port() const = 0;

	NotedFace* c() const;
	LightshowPlugin* p() const;

	virtual void advance(int);
	virtual bool sceneEvent(QEvent* _e);
	static void reorder(QGraphicsItem* _g, QPointF const& _sep);
	virtual QPointF sep() = 0;

	QPointF m_targetPosition;
	bool m_dragging;
};
