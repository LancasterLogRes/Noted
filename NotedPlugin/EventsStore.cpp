#include "NotedFace.h"
#include "EventsStore.h"

EventsStore::EventsStore()
{
	NotedFace::events()->registerStore(this);
}

EventsStore::~EventsStore()
{
	NotedFace::events()->unregisterStore(this);
}

