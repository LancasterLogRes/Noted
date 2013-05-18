#include <Common/Global.h>
#include <NotedPlugin/Timeline.h>
#include "Global.h"
#include "EventsView.h"
#include "EventsMan.h"
using namespace std;
using namespace lb;

EventsMan::EventsMan(QObject* _p): EventsManFace(_p)
{
}

EventsMan::~EventsMan()
{
	qDebug() << "Killing timelines...";
	while (m_timelines.size())
		delete *m_timelines.begin();
	qDebug() << "Killed.";
}

void EventsMan::registerTimeline(Timeline* _tl)
{
	QMutexLocker l(&x_timelines);
	m_timelines.insert(_tl);
}

void EventsMan::unregisterTimeline(Timeline* _tl)
{
	QMutexLocker l(&x_timelines);
	m_timelines.remove(_tl);
}

QList<EventsStore*> EventsMan::eventsStores() const
{
	QList<EventsStore*> ret;
	QMutexLocker l(&x_timelines);
	for (Timeline* i: m_timelines)
		if (EventsStore* es = dynamic_cast<EventsStore*>(i))
			ret.push_back(es);
	return ret;
}

QList<EventsView*> EventsMan::eventsViews() const
{
	QList<EventsView*> ret;
	QMutexLocker l(&x_timelines);
	for (Timeline* i: m_timelines)
		if (EventsView* ev = dynamic_cast<EventsView*>(i))
			ret.push_back(ev);
	return ret;
}

EventCompiler EventsMan::findEventCompiler(QString const& _name) const
{
	QMutexLocker l(&x_timelines);
	for (auto ev: eventsViews())
		if (ev->name() == _name)
			return ev->eventCompiler();
	return EventCompiler();
}

QString EventsMan::getEventCompilerName(EventCompilerImpl* _ec) const
{
	QMutexLocker l(&x_timelines);
	for (auto ev: eventsViews())
		if (&ev->eventCompiler().asA<EventCompilerImpl>() == _ec)
			return ev->name();
	return QString();
}
