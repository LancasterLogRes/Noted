#pragma once

#include <QQuickItem>
#include <QQuickPaintedItem>
#include <QSGGeometry>
#include "Noted.h"
#include "GraphView.h"

class TimelineItem: public QQuickItem
{
	Q_OBJECT

public:
	TimelineItem(QQuickItem* _p = nullptr);
	virtual ~TimelineItem();

	Q_INVOKABLE lb::Time localTime(lb::Time _t, lb::Time _p);

signals:
	void offsetChanged();
	void pitchChanged();

protected:
	Q_PROPERTY(lb::Time offset MEMBER m_offset NOTIFY offsetChanged)
	Q_PROPERTY(lb::Time pitch MEMBER m_pitch NOTIFY pitchChanged)

	lb::Time m_offset;
	lb::Time m_pitch;
};

class GraphItem: public TimelineItem
{
	Q_OBJECT

public:
	GraphItem();

	float yFrom() const { return m_yFrom; }
	float yDelta() const { return m_yDelta; }
	int yMode() const { return m_yMode; }
	void setYFrom(float _v) { m_yFrom = _v; yScaleChanged(); update(); }
	void setYDelta(float _v) { m_yDelta = _v; yScaleChanged(); update(); }
	void setYMode(int _m) { m_yMode = _m; yScaleChanged(); update(); }

signals:
	void urlChanged(QString _url);
	void yScaleChanged();
	void highlightChanged();

protected:
	Q_PROPERTY(QString url MEMBER m_url NOTIFY urlChanged)
	Q_PROPERTY(float yFrom READ yFrom WRITE setYFrom NOTIFY yScaleChanged)
	Q_PROPERTY(float yDelta READ yDelta WRITE setYDelta NOTIFY yScaleChanged)
	Q_PROPERTY(int yMode READ yMode WRITE setYMode NOTIFY yScaleChanged)
	Q_PROPERTY(bool highlight MEMBER m_highlight NOTIFY highlightChanged)

	virtual QSGNode* updatePaintNode(QSGNode* _old, UpdatePaintNodeData*);

	QString m_url;
	float m_yFrom = 0;
	float m_yDelta = 1;
	int m_yMode = 1;		///< 0 -> yFrom/yDelta, 1 /*-> auto (global)*/, 2 -> hint
	bool m_highlight = false;
};

class YScaleItem: public QQuickItem
{
	Q_OBJECT

public:
	YScaleItem(QQuickItem* _p = nullptr): QQuickItem(_p)
	{
		setFlag(ItemHasContents, true);
	}

signals:
	void changed();

protected:
	Q_PROPERTY(float yFrom MEMBER m_yFrom NOTIFY changed)
	Q_PROPERTY(float yDelta MEMBER m_yDelta NOTIFY changed)

	virtual QSGNode* updatePaintNode(QSGNode* _old, UpdatePaintNodeData*);

	float m_yFrom = 0;
	float m_yDelta = 1;
};

class YLabelsItem: public QQuickPaintedItem
{
	Q_OBJECT

public:

signals:
	void changed();

protected:
	Q_PROPERTY(float yFrom MEMBER m_yFrom NOTIFY changed)
	Q_PROPERTY(float yDelta MEMBER m_yDelta NOTIFY changed)

	virtual void paint(QPainter* _p);

	float m_yFrom = 0;
	float m_yDelta = 1;
};

class XLabelsItem: public QQuickPaintedItem
{
	Q_OBJECT

public:
	XLabelsItem(QQuickItem* _p = nullptr): QQuickPaintedItem(_p)
	{
		connect(this, SIGNAL(offsetChanged()), SLOT(update()));
		connect(this, SIGNAL(pitchChanged()), SLOT(update()));
	}

signals:
	void offsetChanged();
	void pitchChanged();

protected:
	Q_PROPERTY(lb::Time offset MEMBER m_offset NOTIFY offsetChanged)
	Q_PROPERTY(lb::Time pitch MEMBER m_pitch NOTIFY pitchChanged)

	virtual void paint(QPainter* _p);

	lb::Time m_offset = 0;
	lb::Time m_pitch = 1;
};

class TimelinesItem: public QQuickItem
{
	Q_OBJECT

public:
	TimelinesItem(QQuickItem* _p = nullptr);

	bool isAcceptingDrops() const { return m_accepting; }
	void setAcceptingDrops(bool _accepting);

signals:
	void offsetChanged();
	void pitchChanged();
	void widthChanged(int);

	void textDrop(QString text);
	void acceptingDropsChanged();

protected:
	Q_PROPERTY(lb::Time offset MEMBER m_offset NOTIFY offsetChanged)
	Q_PROPERTY(lb::Time pitch MEMBER m_pitch NOTIFY pitchChanged)
	Q_PROPERTY(bool acceptingDrops READ isAcceptingDrops WRITE setAcceptingDrops NOTIFY acceptingDropsChanged)

	virtual QSGNode* updatePaintNode(QSGNode* _old, UpdatePaintNodeData*);

	void dragEnterEvent(QDragEnterEvent* _event);
	void dragLeaveEvent(QDragLeaveEvent* _event);
	void dropEvent(QDropEvent* _event);

	lb::Time m_offset = 0;
	lb::Time m_pitch = 1;

	bool m_accepting = true;
};
