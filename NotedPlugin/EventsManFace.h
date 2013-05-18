#pragma once

#include <QObject>
#include <QSet>
#include <QMutex>
#include <EventCompiler/EventCompiler.h>

class EventsStore;
class EventsView;
class Timeline;

class EventsManFace: public QObject
{
public:
	EventsManFace(QObject* _p): QObject(_p), x_timelines(QMutex::Recursive) {}

	virtual void registerTimeline(Timeline* _tl) = 0;
	virtual void unregisterTimeline(Timeline* _tl) = 0;

	virtual QList<EventsStore*> eventsStores() const = 0;
	virtual QList<EventsView*> eventsViews() const = 0;
	virtual lb::EventCompiler findEventCompiler(QString const& _name) const = 0;
	virtual QString getEventCompilerName(lb::EventCompilerImpl* _ec) const = 0;

protected:
	QSet<Timeline*> m_timelines;
	mutable QMutex x_timelines;
};

