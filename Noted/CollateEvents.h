#pragma once

#include <NotedPlugin/CausalAnalysis.h>

class CollateEvents: public CausalAnalysis
{
public:
	CollateEvents(): CausalAnalysis("Collating events") {}
};

