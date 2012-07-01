#pragma once

#include <Common/Time.h>
#include <EventCompiler/EventCompiler.h>
#include <NotedPlugin/CausalAnalysis.h>

class EventsView;

class CompileEventsView: public CausalAnalysis
{
public:
	CompileEventsView(EventsView* _ev);
	virtual ~CompileEventsView() {}

	virtual void init(bool _willRecord);
	virtual void process(unsigned _i, Lightbox::Time);
	virtual void record();

private:
	EventsView* m_ev;
	Lightbox::EventCompiler m_ec;
};

