#pragma once

#include <QQuickItem>
#include <QVector3D>
#include <QQuickPaintedItem>
#include <QSGGeometry>
#include "Noted.h"
#include "GraphView.h"

class TimeHelper: public QObject
{
	Q_OBJECT

public:
	Q_INVOKABLE lb::Time add(lb::Time _a, lb::Time _b) const { return _a + _b; }
	Q_INVOKABLE lb::Time sub(lb::Time _a, lb::Time _b) const { return _a - _b; }
	Q_INVOKABLE lb::Time mul(lb::Time _a, int _b) const { return _a * _b; }
	Q_INVOKABLE lb::Time mul(int _a, lb::Time _b) const { return _a * _b; }
	Q_INVOKABLE lb::Time div(lb::Time _a, int _b) const { return _a / _b; }
	Q_INVOKABLE lb::Time madd(lb::Time _a, int _m, lb::Time _b) const { return _a * _m + _b; }
	Q_INVOKABLE lb::Time madd(int _m, lb::Time _a, lb::Time _b) const { return _m * _a + _b; }
	Q_INVOKABLE lb::Time fromSeconds(double _s) const { return lb::fromSeconds(_s); }
	Q_INVOKABLE double toSeconds(lb::Time _t) const { return lb::toSeconds(_t); }
	Q_INVOKABLE QString toString(lb::Time _t) const { return QString::fromStdString(lb::textualTime(_t)); }
};

class TimelineItem: public QQuickItem
{
	Q_OBJECT

public:
	TimelineItem(QQuickItem* _p = nullptr);
	virtual ~TimelineItem();

	Q_INVOKABLE lb::Time localTime(lb::Time _t, lb::Time _p);
	Q_INVOKABLE lb::Time panOffset(lb::Time _t, int _px) { return _t + _px * m_pitch; }

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

	QVector3D yScaleHint() const;
	float yFromHint() const { return yScaleHint().x(); }
	float yDeltaHint() const { return yScaleHint().y(); }
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
	void yScaleHintChanged();

protected:
	Q_PROPERTY(QString url MEMBER m_url NOTIFY urlChanged)
	Q_PROPERTY(QVector3D yScaleHint READ yScaleHint NOTIFY yScaleHintChanged)
	Q_PROPERTY(float yFrom READ yFrom WRITE setYFrom NOTIFY yScaleChanged)
	Q_PROPERTY(float yDelta READ yDelta WRITE setYDelta NOTIFY yScaleChanged)
	Q_PROPERTY(int yMode READ yMode WRITE setYMode NOTIFY yScaleChanged)
	Q_PROPERTY(bool highlight MEMBER m_highlight NOTIFY highlightChanged)

	virtual QSGNode* updatePaintNode(QSGNode* _old, UpdatePaintNodeData*);

	QString m_url;
	float m_yFrom = 0;
	float m_yDelta = 1;
	int m_yMode = 0;		///< 0 -> yFrom/yDelta, 1 /*-> auto (global)*/, 2 -> hint
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
	YLabelsItem(QQuickItem* _p = nullptr): QQuickPaintedItem(_p)
	{
		connect(this, SIGNAL(changed()), SLOT(update()));
	}

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
