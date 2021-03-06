#include "NotedFace.h"
#include "EventsStore.h"

EventsStore::EventsStore()
{
}

EventsStore::~EventsStore()
{
}

lb::StreamEvents EventsStore::events(int _i) const
{
	return events(NotedFace::audio()->hop() * _i, NotedFace::audio()->hop() * (_i + 1));
}

lb::StreamEvents EventsStore::cursorEvents() const
{
	return events(NotedFace::audio()->cursorIndex());
}

unsigned EventsStore::eventCount() const
{
	return NotedFace::audio()->hops();
}
