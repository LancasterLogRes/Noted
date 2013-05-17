#include <Common/Global.h>
#include "NotedFace.h"
#include "DataMan.h"
using namespace std;
using namespace lb;

DataMan* DataMan::s_this = nullptr;

DataMan::DataMan()
{
	assert(!s_this);
	s_this = this;
}

DataSet* DataMan::dataSet(DataKey _k)
{
//	cdebug << "DataMan::dataSet(" << hex << _k << ")";
	QMutexLocker l(&x_data);
	if (!m_data.contains(_k))
	{
		m_data.insert(_k, make_shared<DataSet>(_k));
//		cdebug << "Creating.";
	}
	return m_data[_k].get();
}

void DataMan::removeDataSet(DataKey _k)
{
	QMutexLocker l(&x_data);
	m_data.remove(_k);
}

void DataMan::pruneDataSets(unsigned _maxMegabytes)
{
	Q_UNUSED(_maxMegabytes);
}

unsigned DataMan::rawRecordCount(DataKey _key) const
{
	QMutexLocker l(&x_data);
	if (m_data.contains(_key) && m_data[_key]->haveRaw())
		return m_data[_key]->rawRecords();
	return 0;
}

tuple<Time, unsigned, int> DataMan::bestFit(DataKey _key, Time _from, Time _duration, unsigned _idealRecords) const
{
	QMutexLocker l(&x_data);
	if (m_data.contains(_key) && m_data[_key]->haveRaw())
		return m_data[_key]->bestFit(_from, _duration, _idealRecords);
	return tuple<Time, unsigned, int>(0, 0, 0);
}

void DataMan::populateRaw(DataKey _key, lb::Time _from, float* _out, unsigned _size) const
{
	QMutexLocker l(&x_data);
	if (m_data.contains(_key) && m_data[_key]->haveRaw())
		m_data[_key]->populateRaw(_from, _out, _size);
}

void DataMan::populateDigest(DataKey _key, DigestFlag _digest, unsigned _level, lb::Time _from, float* _out, unsigned _size) const
{
	QMutexLocker l(&x_data);
	if (m_data.contains(_key) && m_data[_key]->haveRaw())
		m_data[_key]->populateDigest(_digest, _level, _from, _out, _size);
}

unsigned DataMan::recordLength(DataKey _key) const
{
	QMutexLocker l(&x_data);
	if (m_data.contains(_key) && m_data[_key]->haveRaw())
		return m_data[_key]->recordLength();
	return 0;
}

DigestFlags DataMan::availableDigests(DataKey _key) const
{
	QMutexLocker l(&x_data);
	if (m_data.contains(_key) && m_data[_key]->haveRaw())
		return m_data[_key]->availableDigests();
	return DigestFlags(0);
}
