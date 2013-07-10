#pragma once

#include <NotedPlugin/NotedFace.h>
#include <Compute/Compute.h>
#include "NotedFeeder.h"

class NotedComputeRegistrar: public lb::ComputeRegistrar
{
public:
	static NotedComputeRegistrar* get()
	{
		if (!dynamic_cast<NotedComputeRegistrar*>(s_this))
		{
			delete s_this;
			s_this = new NotedComputeRegistrar;
		}
		return dynamic_cast<NotedComputeRegistrar*>(s_this);
	}

protected:
	virtual lb::Compute<float, lb::PCMInfo> createFeeder() { return NotedFeeder(); }
	virtual lb::Compute<lb::StreamEvent, lb::EventStreamInfo> createEventFeeder() { return NotedEventFeeder(); }

	virtual void onInit();
	virtual void onEndTime(lb::Time _oldTime);
	virtual void onFini();
	virtual bool onStore(lb::GenericCompute const& _p);
	virtual void insertMemo(lb::SimpleKey _operation);

private:
	std::unordered_map<lb::SimpleKey, GenericDataSetPtr> m_stored;
};

