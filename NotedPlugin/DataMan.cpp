#include <QDateTime>
#include <Common/Global.h>
#include "NotedFace.h"
#include "DataMan.h"
using namespace std;
using namespace lb;

DataMan::DataMan()
{
}

DataSetPtr DataMan::dataSet(DataKeySet _ks)
{
	cdebug << "DataMan::dataSet(" << hex << _ks << ")";
	QMutexLocker l(&x_data);
	if (!m_data.contains(_ks))
	{
		m_data.insert(_ks, make_shared<DataSet>(_ks));
		x_data.unlock();
		emit inUseChanged();
		emit footprintChanged();
		emit changed();
		x_data.lock();
		cdebug << "Creating.";
	}
	return m_data[_ks];
}

DataSetPtr DataMan::readDataSet(DataKeySet _k) const
{
	QMutexLocker l(&x_data);
	auto ret = m_data.value(_k);
	return ret && ret->haveRaw() ? ret : nullptr;
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
	Cache::AvailableMap as = Cache::available();
	uint64_t ret = 0;
	for (auto i: as)
		ret += i.second;
	return ret;
}

uint64_t DataMan::inUse() const
{
	Cache::AvailableMap as = Cache::available();
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
	Cache::AvailableMap as = Cache::available();

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
				Cache::kill(i->first);
				amountToDelete -= i->second;
			}
	}
	emit footprintChanged();
	emit changed();

	return amountToDelete <= 0;
}
