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
	TimelineItem(QQuickItem* _p = nullptr): QQuickItem(_p)
	{
		setClip(true);
		setFlag(ItemHasContents, true);
		connect(this, SIGNAL(offsetChanged()), SLOT(update()));
		connect(this, SIGNAL(pitchChanged()), SLOT(update()));
	}

signals:
	void offsetChanged();
	void pitchChanged();

protected:
	Q_PROPERTY(double offset MEMBER m_offset NOTIFY offsetChanged)
	Q_PROPERTY(double pitch MEMBER m_pitch NOTIFY pitchChanged)

	double m_offset;
	double m_pitch;
};

class GraphItem: public TimelineItem
{
	Q_OBJECT

public:
	GraphItem();

	GraphViewPlot spec() const { return m_spec; }
	QString ecName() const { return QString::fromStdString(m_spec.ec); }
	QString graphName() const { return QString::fromStdString(m_spec.graph); }
	void setEcName(QString const& _s) { m_spec.ec = _s.toStdString(); graphChanged(); update(); }
	void setGraphName(QString const& _s) { m_spec.graph = _s.toStdString(); graphChanged(); update(); }

	Lightbox::EventCompiler eventCompiler() const;

signals:
	void graphChanged();

private slots:
	void noteDataComplete(quint32);

protected:
	Q_PROPERTY(QString ec READ ecName WRITE setEcName NOTIFY graphChanged)
	Q_PROPERTY(QString graph READ graphName WRITE setGraphName NOTIFY graphChanged)

	GraphViewPlot m_spec;
};

class ChartItem: public GraphItem
{
	Q_OBJECT

public:
	float yFrom() const { return m_yFrom; }
	float yDelta() const { return m_yDelta; }
	int yMode() const { return m_yMode; }
	void setYFrom(float _v) { m_yFrom = _v; yScaleChanged(); update(); }
	void setYDelta(float _v) { m_yDelta = _v; yScaleChanged(); update(); }
	void setYMode(int _m) { m_yMode = _m; yScaleChanged(); update(); }

signals:
	void yScaleChanged();

protected:
	Q_PROPERTY(float yFrom READ yFrom WRITE setYFrom NOTIFY yScaleChanged)
	Q_PROPERTY(float yDelta READ yDelta WRITE setYDelta NOTIFY yScaleChanged)
	Q_PROPERTY(int yMode READ yMode WRITE setYMode NOTIFY yScaleChanged)

	virtual QSGNode* updatePaintNode(QSGNode* _old, UpdatePaintNodeData*);

	float m_yFrom = 0;
	float m_yDelta = 1;
	int m_yMode = 0;		///< 0 -> yFrom/yDelta, 1 -> auto (global), 2 -> hint

	QSGGeometry* m_geo = nullptr;
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
	Q_PROPERTY(double offset MEMBER m_offset NOTIFY offsetChanged)
	Q_PROPERTY(double pitch MEMBER m_pitch NOTIFY pitchChanged)

	virtual void paint(QPainter* _p);

	double m_offset = 0;
	double m_pitch = 1;
};

class TimelinesItem: public QQuickItem
{
	Q_OBJECT

public:
	TimelinesItem(QQuickItem* _p = nullptr): QQuickItem(_p)
	{
		setFlag(ItemHasContents, true);
		connect(this, SIGNAL(offsetChanged()), SLOT(update()));
		connect(this, SIGNAL(pitchChanged()), SLOT(update()));
	}

	double offset() const { return Lightbox::toSeconds(Noted::get()->earliestVisible()); }
	double pitch() const { return Lightbox::toSeconds(Noted::get()->pixelDuration()); }
	void setOffset(double _s) const { Noted::get()->setTimelineOffset(Lightbox::fromSeconds(_s)); }
	void setPitch(double _s) const { Noted::get()->setPixelDuration(Lightbox::fromSeconds(_s)); }

signals:
	void offsetChanged();
	void pitchChanged();

private:
	Q_PROPERTY(double offset READ offset WRITE setOffset NOTIFY offsetChanged)
	Q_PROPERTY(double pitch READ pitch WRITE setPitch NOTIFY pitchChanged)

	virtual QSGNode* updatePaintNode(QSGNode* _old, UpdatePaintNodeData*);
};
