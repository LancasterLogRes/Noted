#pragma once

#include <cassert>
#include <QObject>
#include "Common.h"

class ViewManFace: public QObject
{
	Q_OBJECT

public:
	ViewManFace(QObject* _p = nullptr): QObject(_p) {}

	inline int width() const { return m_width; }
	inline lb::Time offset() const { return m_offset; }
	inline lb::Time pitch() const { return m_pitch; }

	inline lb::Time earliestVisible() const { return m_offset; }
	inline lb::Time latestVisible() const { return earliestVisible() + visibleDuration(); }
	inline lb::Time visibleDuration() const { return width() * pitch(); }
	inline int widthOf(lb::Time _t) const { return (_t + pitch() / 2) / pitch(); }
	inline lb::Time durationOf(int _screenWidth) const { return _screenWidth * pitch(); }
	inline int positionOf(lb::Time _t) const { return widthOf(_t - earliestVisible()); }
	inline lb::Time timeOf(int _x) const { return durationOf(_x) + earliestVisible(); }

public slots:
	inline void zoomTimeline(int _xFocus, double _factor) { auto pivot = timeOf(_xFocus); setParameters(pivot - m_pitch * _factor * _xFocus, m_pitch * _factor); }
	virtual void normalize() = 0;

	// No more than an hour offset
	inline void setOffset(lb::Time _o) { assert(_o < lb::FromSeconds<60*60>::value); if (m_offset != _o) { m_offset = _o; emit offsetChanged(m_offset); emit parametersChanged(m_offset, m_pitch); } }
	inline void setPitch(lb::Time _d) { assert(_d > 0); if (m_pitch != _d) { m_pitch = _d; emit pitchChanged(m_pitch); emit parametersChanged(m_offset, m_pitch); } }
	inline void setParameters(lb::Time _o, lb::Time _d) { if (m_pitch != _d || m_offset != _o) { m_offset = _o; m_pitch = _d; emit offsetChanged(_o); emit pitchChanged(_d); emit parametersChanged(_o, _d); } }

	/// Private-ish API - only allowed to be called by the TimelinesItem.
	void setWidth(int _w) { m_width = _w; widthChanged(_w); }

signals:
	void widthChanged(int _w);
	void offsetChanged(lb::Time _offset);
	void pitchChanged(lb::Time _pitch);
	void parametersChanged(lb::Time _offset, lb::Time _pitch);

private:
	Q_PROPERTY(int width READ width NOTIFY widthChanged)
	Q_PROPERTY(lb::Time offset READ offset WRITE setOffset NOTIFY offsetChanged)
	Q_PROPERTY(lb::Time pitch READ pitch WRITE setPitch NOTIFY pitchChanged)

	lb::Time m_offset = 0;
	lb::Time m_pitch = lb::FromMsecs<1>::value;
	int m_width = 100;
};
