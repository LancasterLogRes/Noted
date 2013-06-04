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

DataSetPtr DataMan::dataSet(DataKeys _ks)
{
	cdebug << "DataMan::dataSet(" << hex << _ks << ")";
	QMutexLocker l(&x_data);
	if (!m_data.contains(_ks))
	{
		m_data.insert(_ks, make_shared<DataSet>(_ks));
		cdebug << "Creating.";
	}
	return m_data[_ks];
}

void DataMan::removeDataSet(DataKeys _k)
{
	QMutexLocker l(&x_data);
	m_data.remove(_k);
}

void DataMan::clearData()
{
	QMutexLocker l(&x_data);
	m_data.clear();
}

void DataMan::pruneDataSets(unsigned _maxMegabytes)
{
	Q_UNUSED(_maxMegabytes);
}

unsigned DataMan::rawRecordCount(DataKeys _key) const
{
	QMutexLocker l(&x_data);
	if (m_data.contains(_key) && m_data[_key]->haveRaw())
		return m_data[_key]->rawRecords();
	return 0;
}

tuple<Time, unsigned, int, Time> DataMan::bestFit(DataKeys _key, Time _from, Time _duration, unsigned _idealRecords) const
{
	QMutexLocker l(&x_data);
	if (m_data.contains(_key) && m_data[_key]->haveRaw())
		return m_data[_key]->bestFit(_from, _duration, _idealRecords);
	return tuple<Time, unsigned, int, Time>(0, 0, 0, 0);
}

void DataMan::populateRaw(DataKeys _key, lb::Time _from, float* _out, unsigned _size) const
{
	QMutexLocker l(&x_data);
	if (m_data.contains(_key) && m_data[_key]->haveRaw())
		m_data[_key]->populateRaw(_from, _out, _size);
}

void DataMan::populateDigest(DataKeys _key, DigestFlag _digest, unsigned _level, lb::Time _from, float* _out, unsigned _size) const
{
	QMutexLocker l(&x_data);
	if (m_data.contains(_key) && m_data[_key]->haveRaw())
		m_data[_key]->populateDigest(_digest, _level, _from, _out, _size);
}

unsigned DataMan::recordLength(DataKeys _key) const
{
	QMutexLocker l(&x_data);
	if (m_data.contains(_key) && m_data[_key]->haveRaw())
		return m_data[_key]->recordLength();
	return 0;
}

DigestFlags DataMan::availableDigests(DataKeys _key) const
{
	QMutexLocker l(&x_data);
	if (m_data.contains(_key) && m_data[_key]->haveRaw())
		return m_data[_key]->availableDigests();
	return DigestFlags(0);
}
