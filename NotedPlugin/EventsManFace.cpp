#include <Common/Global.h>
#include "NotedFace.h"
#include "EventsManFace.h"
using namespace std;
using namespace lb;

void EventsManFace::registerStore(EventsStore* _es)
{
	NotedFace::compute()->suspendWork();
	m_stores.insert(_es);
	NotedFace::compute()->resumeWork();
	emit storesChanged();
}

void EventsManFace::unregisterStore(EventsStore* _es)
{
	NotedFace::compute()->suspendWork();
	m_stores.remove(_es);
	NotedFace::compute()->resumeWork();
	emit storesChanged();
}

void EventsManFace::noteEventCompilersChanged()
{
	NotedFace::compute()->invalidate(compileEventsAnalysis());
}

void EventsManFace::notePluginDataChanged()
{
	NotedFace::compute()->invalidate(collateEventsAnalysis());
}
