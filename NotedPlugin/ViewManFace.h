#pragma once

#include <cassert>
#include <QObject>
#include "Common.h"

class ViewManFace: public QObject
{
	Q_OBJECT

public:
	ViewManFace(QObject* _p = nullptr): QObject(_p) {}

	inline int activeWidth() const { return m_width; }
	inline lb::Time timelineOffset() const { return m_timelineOffset; }
	inline lb::Time pixelDuration() const { return m_pixelDuration; }

	double offset() const { return lb::toSeconds(earliestVisible()); }
	double pitch() const { return lb::toSeconds(pixelDuration()); }

	inline lb::Time earliestVisible() const { return m_timelineOffset; }
	inline lb::Time latestVisible() const { return earliestVisible() + visibleDuration(); }
	inline lb::Time visibleDuration() const { return activeWidth() * pixelDuration(); }
	inline int widthOf(lb::Time _t) const { return (_t + pixelDuration() / 2) / pixelDuration(); }
	inline lb::Time durationOf(int _screenWidth) const { return _screenWidth * pixelDuration(); }
	inline int positionOf(lb::Time _t) const { return widthOf(_t - earliestVisible()); }
	inline lb::Time timeOf(int _x) const { return durationOf(_x) + earliestVisible(); }

public slots:
	inline void zoomTimeline(int _xFocus, double _factor) { auto pivot = timeOf(_xFocus); setParameters(pivot - m_pixelDuration * _factor * _xFocus, m_pixelDuration * _factor); }
	virtual void normalize() = 0;

	// No more than an hour offset
	inline void setTimelineOffset(lb::Time _o) { assert(_o < lb::FromSeconds<60*60>::value); if (m_timelineOffset != _o) { m_timelineOffset = _o; emit timelineOffsetChanged(m_timelineOffset); emit parametersChanged(m_timelineOffset, m_pixelDuration); } }
	inline void setPixelDuration(lb::Time _d) { assert(_d > 0); if (m_pixelDuration != _d) { m_pixelDuration = _d; emit pixelDurationChanged(m_pixelDuration); emit parametersChanged(m_timelineOffset, m_pixelDuration); } }
	inline void setParameters(lb::Time _o, lb::Time _d) { if (m_pixelDuration != _d || m_timelineOffset != _o) { m_timelineOffset = _o; m_pixelDuration = _d; emit timelineOffsetChanged(_o); emit pixelDurationChanged(_d); emit parametersChanged(_o, _d); } }

	void setOffset(double _s) { setTimelineOffset(lb::fromSeconds(_s)); }
	void setPitch(double _s) { setPixelDuration(lb::fromSeconds(_s)); }

	/// Private-ish API - only allowed to be called by the TimelinesItem.
	void setWidth(int _w) { m_width = _w; widthChanged(_w); }

signals:
	void widthChanged(int _w);
	void timelineOffsetChanged(lb::Time _timelineOffset);
	void pixelDurationChanged(lb::Time _pixelDuration);
	void parametersChanged(lb::Time _timelineOffset, lb::Time _pixelDuration);

private:
	Q_PROPERTY(int activeWidth READ activeWidth NOTIFY widthChanged)
	Q_PROPERTY(double offset READ offset WRITE setOffset NOTIFY timelineOffsetChanged)
	Q_PROPERTY(double pitch READ pitch WRITE setPitch NOTIFY pixelDurationChanged)
	Q_PROPERTY(lb::Time timelineOffset READ earliestVisible WRITE setTimelineOffset NOTIFY timelineOffsetChanged)
	Q_PROPERTY(lb::Time pixelDuration READ pixelDuration WRITE setPixelDuration NOTIFY pixelDurationChanged)

	lb::Time m_timelineOffset = 0;
	lb::Time m_pixelDuration = lb::FromMsecs<1>::value;
	int m_width = 100;
};
