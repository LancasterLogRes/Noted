#include <QDateTime>
#include <Common/Global.h>
#include "NotedFace.h"
#include "DataMan.h"
using namespace std;
using namespace lb;

DataMan::DataMan()
{
}

GenericDataSetPtr DataMan::create(DataKey _k, size_t _elementSize, char const* _elementTypeName)
{
	QMutexLocker l(&x_data);
	if (m_data.contains(_k))
	{
		cdebug << "DataMan::dataSet(" << std::hex << _k << "):" << m_data[_k]->elementTypeName() << "vs." << _elementTypeName;
		assert(m_data[_k]->isOfType(_elementSize, _elementTypeName));
	}
	else
	{
		m_data[_k] = std::make_shared<GenericDataSet>(_k, _elementSize, _elementTypeName);
		x_data.unlock();
		emit inUseChanged();
		emit footprintChanged();
		emit changed();
		x_data.lock();
		cdebug << "Creating.";
	}
	return m_data[_k];
}

void DataMan::releaseDataSets()
{
	{
		QMutexLocker l(&x_data);
		m_data.clear();
	}
	emit inUseChanged();
	emit changed();
}

uint64_t DataMan::footprint()
{
	ProtoCache::AvailableMap as = ProtoCache::available();
	uint64_t ret = 0;
	for (auto i: as)
		ret += i.second;
	return ret;
}

uint64_t DataMan::inUse() const
{
	ProtoCache::AvailableMap as = ProtoCache::available();
	uint64_t ret = 0;
	{
		QMutexLocker l(&x_data);
		for (auto i: as)
			if (m_data.contains(i.first))
				ret += i.second;
	}
	return ret;
}

bool DataMan::pruneDataSets(uint64_t _maxBytes)
{
	ProtoCache::AvailableMap as = ProtoCache::available();

	int64_t amountToDelete = -(int64_t)_maxBytes;
	for (auto i: as)
		amountToDelete += i.second;
	if (amountToDelete <= 0)
		return true;

	{
		QMutexLocker l(&x_data);
		for (auto i = as.begin(); i != as.end() && amountToDelete > 0; ++i)
			if (!m_data.contains(i->first))
			{
				ProtoCache::kill(i->first);
				amountToDelete -= i->second;
			}
	}
	emit footprintChanged();
	emit changed();

	return amountToDelete <= 0;
}
