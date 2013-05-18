#include <QThread>
#include <Common/Global.h>
#include "NotedFace.h"
#include "DataSet.h"
using namespace std;
using namespace lb;

DataSet::DataSet(DataKey _operationKey):
	m_operationKey(_operationKey)
{
//	cdebug << (void*)this << (QThread::currentThreadId()) << "DataSet::DataSet(" << _operationKey << ")";
}

void DataSet::init(unsigned _recordLength, Time _stride, Time _first)
{
//	cdebug << (void*)this << (QThread::currentThreadId()) << "DataSet::init()";
	m_first = _first;
	m_stride = _stride;
	m_recordLength = _recordLength;
	setup(_stride ? (NotedFace::audio()->duration() - _first) / _stride : 0);
}

unsigned DataSet::rawRecords() const
{
	return m_raw.bytes() / sizeof(float) / m_recordLength;
}

void DataSet::setup(unsigned _itemCount)
{
	(void)_itemCount;
	m_raw.init(NotedFace::get()->audio()->key(), m_operationKey, 0);
	if (m_recordLength)
		m_toc.reset();
	else
		m_toc.init(NotedFace::get()->audio()->key(), m_operationKey, 1);
	m_recordCount = 0;
	m_pos = 0;
	m_digest.clear();
}

void DataSet::appendRecord(Time _t, foreign_vector<float> const& _vs)
{
	assert(_vs.size());

	if (m_stride && m_recordLength)
		assert(m_recordCount == (_t - m_first) / m_stride);
	if (m_stride && !m_recordLength)
		assert(m_recordCount == (_t - m_first) / m_stride);

	if (!m_toc.isGood() && !m_recordLength)
	{
		if (!m_stride)
			m_toc.append(_t);
		m_toc.append((TocRef)m_pos);
	}
	if (!m_raw.isGood())
	{
		if (!m_stride && m_recordLength)
			m_raw.append(_t);
		m_raw.append(_vs);
	}
	m_pos += _vs.size();
	m_recordCount++;
}

void DataSet::done()
{
	m_raw.setGood();
	if (m_toc.isOpen())
		m_toc.setGood();
	DataMan::get()->noteDone(m_operationKey);
}

void DataSet::digest(DigestFlag _t)
{
	assert(m_stride);
	assert(m_recordLength);
	m_raw.setGood();
	if (m_toc.isOpen())
		m_toc.setGood();
	m_availableDigests |= _t;
	m_digest[_t] = make_shared<MipmappedCache>();
	bool haveDigest = m_digest[_t]->init(NotedFace::audio()->key(), m_operationKey, qHash(2 + _t), digestSize(_t) * recordLength() * sizeof(float), digestRecords());
	if (haveDigest)
		return;

	foreign_vector<float> digestData = m_digest[_t]->data<float>(0);
	float* d = digestData.data();

	auto rawData = m_raw.data<float>();
	float* f = rawData.data();
	float* fe = rawData.data() + rawData.size();
	switch (_t)
	{
	case MeanDigest:
		for (unsigned i = m_digestBase; f < fe; ++f)
		{
			if (i == m_digestBase)
			{
				i = 0;
				*d = *f;
			}
			else
				*d += *f;
			i++;
			if (i == m_digestBase)
			{
				*d /= m_digestBase;
				d++;
			}
		}
		m_digest[_t]->generate([](lb::foreign_vector<float> a, lb::foreign_vector<float> b, lb::foreign_vector<float> ret)
		{
			unsigned i = 0;
			for (; i + 3 < a.size(); i += 4)
			{
				ret[i] = (a[i] + b[i]) / 2;
				ret[i+1] = (a[i+1] + b[i+1]) / 2;
				ret[i+2] = (a[i+2] + b[i+2]) / 2;
				ret[i+3] = (a[i+3] + b[i+3]) / 2;
			}
			for (; i < a.size(); ++i)
				ret[i] = (a[i] + b[i]) / 2;
		}, float(0));
		break;
	case MinMaxInOutDigest:
		for (unsigned i = m_digestBase; f < fe; ++f)
		{
			if (i == m_digestBase)
			{
				i = 0;
				d[0] = d[1] = d[2] = *f;
			}
			else
			{
				d[0] = min(d[0], *f);
				d[1] = max(d[1], *f);
			}
			i++;
			if (i == m_digestBase)
			{
				d[3] = *f;
				d += 4;
			}
		}
		// assumes a & b are sequential.
		m_digest[_t]->generate([](lb::foreign_vector<float> a, lb::foreign_vector<float> b, lb::foreign_vector<float> ret)
		{
			for (unsigned i = 0; i < a.size(); i += 4)
			{
				ret[i] = min(a[i], b[i]);
				ret[i+1] = max(a[i+1], b[i+1]);
				ret[i+2] = a[i + 2];
				ret[i+3] = b[i + 3];
			}
		}, float(0));
		break;
	default:
		break;
	}
}

tuple<Time, unsigned, int> DataSet::bestFit(Time _from, Time _duration, unsigned _idealRecords) const
{
	unsigned recordBegin = (_from - m_first) / m_stride;
	unsigned recordEnd = (_from + _duration - m_first + m_stride - 1) / m_stride;
	unsigned recordRes = _duration / m_stride;

	int level = -1;

	while (true)
	{
		int nextLevel = level + 1;
		unsigned nextBegin = nextLevel ? recordBegin / 2 : recordBegin / m_digestBase;
		unsigned nextEnd = nextLevel ? (recordEnd + 1) / 2 : (recordBegin + m_digestBase - 1) / m_digestBase;
		unsigned nextRes = nextLevel ? recordRes / 2 : recordBegin / m_digestBase;

//		if (nextEnd - nextBegin < _idealRecords || recordEnd - recordBegin == _idealRecords)
		if (recordRes < _idealRecords)
			// Passed the place... use the last one cunningly left in recordBegin/recordEnd/level.
			return tuple<Time, unsigned, int>(m_first + m_stride * (level > -1 ? m_digestBase << level : 1) * recordBegin, recordEnd - recordBegin, level);
		recordBegin = nextBegin;
		recordEnd = nextEnd;
		recordRes = nextRes;
		level = nextLevel;
	}
}

void DataSet::populateRaw(lb::Time _from, float* _out, unsigned _size) const
{
	int recordBegin = (_from - m_first) / m_stride;
	int rLen = recordLength();
	int records = _size / rLen;
	assert((int)_size == records * rLen);

	foreign_vector<float const> d = m_raw.data<float>();
	int recordsAvailable = d.size() / rLen;

	// Beginning part - anything before our records begin should be zeroed.
	int beforeStart = clamp(-recordBegin, 0, records);
	memset(_out, 0, sizeof(float) * rLen * beforeStart);

	// End part - anything after our records end should be zeroed.
	int overEnd = clamp(recordBegin + records - recordsAvailable, 0, records);
	memset(_out + (records - overEnd) * rLen, 0, sizeof(float) * rLen * overEnd);

	int valid = records - beforeStart - overEnd;
	assert(valid <= records);
	valcpy(_out + beforeStart * rLen, d.data() + (recordBegin + beforeStart) * rLen, rLen * valid);
}

void DataSet::populateDigest(DigestFlag _digest, unsigned _level, lb::Time _from, float* _out, unsigned _size) const
{
	assert(m_availableDigests & _digest);
	assert(m_digest.contains(_digest));
	int recordBegin = (_from - m_first) / m_stride / (m_digestBase << _level);
	int drLen = digestSize(_digest) * recordLength();
	int records = _size / drLen;
	assert((int)_size == records * drLen);

	foreign_vector<float> d = m_digest[_digest]->data<float>(_level);
	int recordsAvailable = d.size() / drLen;

	// Beginning part - anything before our records begin should be zeroed.
	int beforeStart = clamp(-recordBegin, 0, records);
	memset(_out, 0, sizeof(float) * drLen * beforeStart);

	// End part - anything after our records end should be zeroed.
	int overEnd = clamp(recordBegin + records - recordsAvailable, 0, records);
	memset(_out + (records - overEnd) * drLen, 0, sizeof(float) * drLen * overEnd);

	int valid = records - beforeStart - overEnd;
	assert(valid <= records);
	valcpy(_out + beforeStart * drLen, d.data() + (recordBegin + beforeStart) * drLen, drLen * valid);
}
