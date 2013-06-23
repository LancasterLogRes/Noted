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
	virtual lb::Compute<lb::PCMInfo, float> createFeeder()
	{
		return NotedFeeder();
	}

	virtual void onEndTime(lb::Time _oldTime)
	{
		for (auto i: m_stored)
		{
			assert(i.second->isAppendable());
			i.second->appendRecord(_oldTime, m_memos[i.first].second);
		}
	}

	virtual void onFini()
	{
		cnote << "ComputeRegistrar: Finishing all DSs";
		// Finished
		for (auto i: m_stored)
			i.second->done();
		m_stored.clear();
	}

	virtual bool onStore(lb::GenericCompute const& _p)
	{
		if (!_p)
			return true;
		auto ds = NotedFace::data()->create(DataKey(NotedFace::audio()->key(), _p.p()->hash()), _p.p()->elementSize(), _p.p()->elementTypeName());
		if (ds->isComplete())
			return true;

		cdebug << "ComputeRegistrar: Writing to DS" << _p.p()->hash();
		m_stored.insert(std::make_pair(_p.p()->hash(), ds));
		assert(ds->isAppendable());
		return false;
	}

	virtual void insertMemo(lb::SimpleKey _operation)
	{
		lb::foreign_vector<uint8_t> v;
		if (GenericDataSetPtr ds = NotedFace::data()->getGeneric(DataKey(NotedFace::audio()->key(), _operation)))
			if (ds->isComplete())
				v = ds->peekRecordBytes(m_time, nullptr);
		m_memos[_operation].second = v;
	}

private:
	std::unordered_map<lb::SimpleKey, GenericDataSetPtr> m_stored;
};

