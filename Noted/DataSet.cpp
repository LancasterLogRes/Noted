#include <Common/Global.h>
#include "DataSet.h"
#include "Global.h"
using namespace std;
using namespace Lightbox;

void DataSet::initHopper(unsigned _recordLength, unsigned _strideHops, Lightbox::Time _first)
{
	m_first = _first;
	m_stride = toBase(_strideHops * NotedFace::audio()->hopSamples(), NotedFace::audio()->rate());
	m_recordLength = _recordLength;
	setup(NotedFace::get()->audio()->hops() / _strideHops);
}

void DataSet::init(unsigned _recordLength, unsigned _stride, Lightbox::Time _first)
{
	m_first = _first;
	m_stride = toBase(_stride, NotedFace::audio()->rate());
	m_recordLength = _recordLength;
	setup(NotedFace::get()->audio()->samples() / _stride);
}

void DataSet::setup(unsigned _itemCount)
{
	bool haveRaw = m_raw.init(NotedFace::get()->audio()->key(), m_operationKey, 0, _itemCount * m_recordLength * sizeof(float));
	m_pos = haveRaw ? m_raw.bytes() : 0;
	m_digest.clear();
	m_rawData = m_raw.data<float>();
}

void DataSet::append(float _v)
{
	assert(m_rawData);
	assert(m_pos < m_rawData.size());
	m_rawData[m_pos] = _v;
	m_pos++;
}

void DataSet::append(Lightbox::foreign_vector<float> const& _vs)
{
	assert(m_rawData);
	assert(m_pos + _vs.size() <= m_rawData.size());
	valcpy(m_rawData.data() + m_pos, _vs.data(), _vs.size());
	m_pos += _vs.size();
}

void DataSet::done()
{
	m_rawData.reset();
	DataMan::get()->noteDone(m_operationKey);
}

void DataSet::digest(DigestFlag _t)
{
	m_availableDigests |= _t;
	m_digest[_t] = make_shared<MipmappedCache>();
	bool haveDigest = m_digest[_t]->init(NotedFace::get()->audio()->key(), m_operationKey, qHash(_t), digestSize(_t) * recordLength() * sizeof(float), digestRecords());
	if (haveDigest)
		return;

	foreign_vector<float> digestData = m_digest[_t]->data<float>(0);
	float* d = digestData.data();

	float* f = m_rawData.data();
	float* fe = m_rawData.data() + m_rawData.size();
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
		m_digest[_t]->generate([](Lightbox::foreign_vector<float> a, Lightbox::foreign_vector<float> b, Lightbox::foreign_vector<float> ret)
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
		m_digest[_t]->generate([](Lightbox::foreign_vector<float> a, Lightbox::foreign_vector<float> b, Lightbox::foreign_vector<float> ret)
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

void DataSet::populateRaw(Lightbox::Time _from, float* _out, unsigned _size) const
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

void DataSet::populateDigest(DigestFlag _digest, unsigned _level, Lightbox::Time _from, float* _out, unsigned _size) const
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
