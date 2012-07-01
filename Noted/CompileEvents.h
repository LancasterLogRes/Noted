#pragma once

#include "Noted.h"

class CompileEvents: public CausalAnalysis
{
public:
	CompileEvents(): CausalAnalysis("Compiling all events") {}

	virtual void init(bool _willRecord)
	{
		if (_willRecord)
			dynamic_cast<Noted*>(noted())->m_initEvents.clear();
	}
	virtual void fini(bool)
	{
		dynamic_cast<Noted*>(noted())->m_eventsViewsDone = 0;
	}
};
