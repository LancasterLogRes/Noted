#include <utility>
#include "NotedComputeRegistrar.h"
using namespace std;
using namespace lb;

void NotedComputeRegistrar::onEndTime(lb::Time _oldTime)
{
	for (auto i: m_stored)
		if (i.second && i.second->isAppendable())
			i.second->appendRecord(_oldTime, m_memos[i.first].second);
}

void NotedComputeRegistrar::onInit()
{
	// Should already be empty, but best to be safe...
	m_stored.clear();
}

void NotedComputeRegistrar::onFini()
{
	cnote << "ComputeRegistrar: Finishing all DSs";
	// Finished
	for (auto i: m_stored)
		if (i.second)
			i.second->done();
	m_stored.clear();
}

bool NotedComputeRegistrar::onStore(lb::GenericCompute const& _p, bool _precompute)
{
	if (!_p)
		return true;
	auto h = _p.hash();
	auto ds = NotedFace::data()->create(DataKey(NotedFace::audio()->key(), h), _p.p()->elementSize(), _p.p()->elementTypeName());
	m_stored[h] = ds;
	if (_p.isVolatile() && _precompute)
		ds->init();
	if (!ds->isComplete())
		cnote << "ComputeRegistrar: Writing" << _p.name() << "to DS" << std::hex << h;
	assert(ds->isComplete() || ds->isAppendable());
	return ds->isComplete();
}

void NotedComputeRegistrar::insertMemo(lb::SimpleKey _operation)
{
	lb::foreign_vector<uint8_t> v;
	auto it = m_stored.find(_operation);
	if (it == m_stored.end())
		it = m_stored.insert(make_pair(_operation, NotedFace::data()->getGeneric(DataKey(NotedFace::audio()->key(), _operation)))).first;
	if (it != m_stored.end() && it->second && it->second->isComplete())
		v = it->second->peekRecordBytes(m_time, nullptr);
	m_memos[_operation].second = v;
}

