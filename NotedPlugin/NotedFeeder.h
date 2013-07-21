#pragma once

#include <EventCompiler/StreamEvent.h>
#include <Compute/Compute.h>
#include "NotedFace.h"

class NotedFeederImpl: public lb::ComputeImpl<float, lb::PCMInfo>
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

class NotedMultiFeederImpl: public lb::ComputeImpl<float, lb::MultiPCMInfo>
{
public:
	virtual lb::MultiPCMInfo info()
	{
		return { NotedFace::audio()->rate(), NotedFace::audio()->hopSamples(), 1 };
	}
	virtual char const* name() const { return "NotedMultiFeeder"; }
	virtual lb::SimpleKey hash() const { return 2; }
	virtual lb::foreign_vector<float> get()
	{
		if (NotedFace::audio()->wave()->isComplete())
			return NotedFace::audio()->wave()->peek(lb::ComputeRegistrar::get()->time(), NotedFace::audio()->hopSamples());
		return lb::foreign_vector<float>();
	}
};
using NotedMultiFeeder = lb::ComputeBase<NotedMultiFeederImpl>;

class NotedEventFeederImpl: public lb::ComputeImpl<lb::StreamEvent, lb::EventStreamInfo>
{
public:
	virtual lb::EventStreamInfo info()
	{
		return {};
	}
	virtual char const* name() const { return "NotedEventFeeder"; }
	virtual lb::SimpleKey hash() const { return NotedFace::events()->hash(); }
	virtual void compute(std::vector<Element>& _v)
	{
		unsigned i = NotedFace::audio()->index(lb::ComputeRegistrar::get()->time());
		_v = NotedFace::events()->inWindow(i, true);
	}
};
using NotedEventFeeder = lb::ComputeBase<NotedEventFeederImpl>;

