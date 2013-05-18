#pragma once

#include <NotedPlugin/EventsManFace.h>

class EventsMan: public EventsManFace
{
public:
	EventsMan(QObject* _p = nullptr);
	virtual ~EventsMan();
	
	virtual void registerTimeline(Timeline* _tl);
	virtual void unregisterTimeline(Timeline* _tl);

	virtual QList<EventsStore*> eventsStores() const;
	virtual QList<EventsView*> eventsViews() const;
	virtual lb::EventCompiler findEventCompiler(QString const& _name) const;
	virtual QString getEventCompilerName(lb::EventCompilerImpl* _ec) const;

private:
};
