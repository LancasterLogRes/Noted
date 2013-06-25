#pragma once

#include <Compute/Compute.h>
#include "NotedFace.h"

class NotedFeederImpl: public lb::ComputeImpl<lb::PCMInfo, float>
{
public:
	virtual lb::PCMInfo info()
	{
		return { NotedFace::audio()->rate(), NotedFace::audio()->hopSamples() };
	}
	virtual char const* name() const { return "NotedFeeder"; }
	virtual lb::SimpleKey hash() const { return 0; }
	virtual lb::foreign_vector<float> get()
	{
		if (NotedFace::audio()->wave()->isComplete())
			return NotedFace::audio()->wave()->peek(lb::ComputeRegistrar::get()->time(), NotedFace::audio()->hopSamples());
		return lb::foreign_vector<float>();
	}
};
using NotedFeeder = lb::ComputeBase<NotedFeederImpl>;
