#pragma once

#include <QQuickItem>
#include <QVector3D>
#include <QQuickPaintedItem>
#include <QSGGeometryNode>
#include "Noted.h"
#include "GraphView.h"
#include "TimelineItem.h"

class YScaleItem: public QQuickItem
{
	Q_OBJECT

public:
	YScaleItem(QQuickItem* _p = nullptr): QQuickItem(_p)
	{
		setFlag(ItemHasContents, true);
		connect(this, SIGNAL(changed()), SLOT(update()));
	}

signals:
	void changed();

protected:
	Q_PROPERTY(QVector3D yScale MEMBER m_yScale NOTIFY changed)

	virtual QSGNode* updatePaintNode(QSGNode* _old, UpdatePaintNodeData*);

	QVector3D m_yScale = QVector3D(0, 1, 0);
};

class XScaleItem: public QQuickItem
{
	Q_OBJECT

public:
	XScaleItem(QQuickItem* _p = nullptr): QQuickItem(_p)
	{
		setFlag(ItemHasContents, true);
		connect(this, SIGNAL(changed()), SLOT(update()));
	}

signals:
	void changed();

protected:
	Q_PROPERTY(QVector3D xScale MEMBER m_xScale NOTIFY changed)

	virtual QSGNode* updatePaintNode(QSGNode* _old, UpdatePaintNodeData*);

	QVector3D m_xScale = QVector3D(0, 1, 0);
};

class YLabelsItem: public QQuickPaintedItem
{
	Q_OBJECT

public:
	YLabelsItem(QQuickItem* _p = nullptr): QQuickPaintedItem(_p)
	{
		connect(this, SIGNAL(changed()), SLOT(update()));
	}

signals:
	void changed();

protected:
	Q_PROPERTY(QVector3D yScale MEMBER m_yScale NOTIFY changed)
	Q_PROPERTY(int overflow MEMBER m_overflow NOTIFY changed)

	virtual void paint(QPainter* _p);

	QVector3D m_yScale = QVector3D(0, 1, 0);

	int m_overflow = 0;
};

class XLabelsItem: public QQuickPaintedItem
{
	Q_OBJECT

public:
	XLabelsItem(QQuickItem* _p = nullptr): QQuickPaintedItem(_p)
	{
		connect(this, SIGNAL(changed()), SLOT(update()));
	}

signals:
	void changed();

protected:
	Q_PROPERTY(QVector3D xScale MEMBER m_xScale NOTIFY changed)
	Q_PROPERTY(int overflow MEMBER m_overflow NOTIFY changed)

	virtual void paint(QPainter* _p);

	QVector3D m_xScale = QVector3D(0, 1, 0);

	int m_overflow = 0;
};

class TimeLabelsItem: public QQuickPaintedItem
{
	Q_OBJECT

public:
	TimeLabelsItem(QQuickItem* _p = nullptr): QQuickPaintedItem(_p)
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

class CursorItem: public TimelineItem
{
	Q_OBJECT

public:
	CursorItem(QQuickItem* _p = nullptr): TimelineItem(_p)
	{
		connect(this, SIGNAL(cursorChanged()), SLOT(update()));
		connect(this, SIGNAL(cursorWidthChanged()), SLOT(update()));
	}

signals:
	void cursorChanged();
	void cursorWidthChanged();

protected:
	Q_PROPERTY(lb::Time cursor MEMBER m_cursor NOTIFY cursorChanged)
	Q_PROPERTY(lb::Time cursorWidth MEMBER m_cursorWidth NOTIFY cursorWidthChanged)

	virtual QSGNode* updatePaintNode(QSGNode* _old, UpdatePaintNodeData*);

	lb::Time m_cursor = 0;
	lb::Time m_cursorWidth = lb::FromSeconds<1>::value;
};

class IntervalItem: public TimelineItem
{
	Q_OBJECT

public:
	IntervalItem(QQuickItem* _p = nullptr): TimelineItem(_p)
	{
		connect(this, SIGNAL(changed()), SLOT(update()));
	}

signals:
	void changed();

protected:
	Q_PROPERTY(lb::Time begin MEMBER m_begin NOTIFY changed)
	Q_PROPERTY(lb::Time duration MEMBER m_duration NOTIFY changed)

	virtual QSGNode* updatePaintNode(QSGNode* _old, UpdatePaintNodeData*);

	lb::Time m_begin = 0;
	lb::Time m_duration = lb::FromSeconds<1>::value;
};

class TimelinesItem: public TimelineItem
{
	Q_OBJECT

public:
	TimelinesItem(QQuickItem* _p = nullptr);

	bool isAcceptingDrops() const { return m_accepting; }
	void setAcceptingDrops(bool _accepting);

signals:
	void widthChanged(int);

	void textDrop(QString _text);
	void acceptingDropsChanged();

protected:
	Q_PROPERTY(bool acceptingDrops READ isAcceptingDrops WRITE setAcceptingDrops NOTIFY acceptingDropsChanged)

	virtual QSGNode* updatePaintNode(QSGNode* _old, UpdatePaintNodeData*);

	void dragEnterEvent(QDragEnterEvent* _event);
	void dragLeaveEvent(QDragLeaveEvent* _event);
	void dropEvent(QDropEvent* _event);

	bool m_accepting = true;
};
