#pragma once

#include <QQuickItem>
#include <QVector3D>
#include <QQuickPaintedItem>
#include <QSGGeometryNode>
#include "Noted.h"
#include "GraphView.h"

class TimeHelper: public QObject
{
	Q_OBJECT

public:
	Q_INVOKABLE lb::Time add(lb::Time _a, lb::Time _b) const { return _a + _b; }
	Q_INVOKABLE lb::Time sub(lb::Time _a, lb::Time _b) const { return _a - _b; }
	Q_INVOKABLE lb::Time mul(lb::Time _a, double _b) const { return _a * _b; }
	Q_INVOKABLE lb::Time mul(lb::Time _a, float _b) const { return _a * _b; }
	Q_INVOKABLE lb::Time mul(lb::Time _a, int _b) const { return _a * _b; }
	Q_INVOKABLE lb::Time mul(int _a, lb::Time _b) const { return _a * _b; }
	Q_INVOKABLE lb::Time div(lb::Time _a, int _b) const { return _a / _b; }
	Q_INVOKABLE lb::Time div(lb::Time _a, float _b) const { return _a / _b; }
	Q_INVOKABLE lb::Time div(lb::Time _a, double _b) const { return _a / _b; }
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

	static QObject* constructTimeHelper(QQmlEngine*, QJSEngine*) { return new TimeHelper; }

signals:
	void offsetChanged();
	void pitchChanged();

protected:
	Q_PROPERTY(lb::Time offset MEMBER m_offset NOTIFY offsetChanged)
	Q_PROPERTY(lb::Time pitch MEMBER m_pitch NOTIFY pitchChanged)

	lb::Time m_offset;
	lb::Time m_pitch;
};
