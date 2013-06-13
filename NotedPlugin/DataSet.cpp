#include <QHash>
#include <QThread>
#include <Common/Global.h>
#include "NotedFace.h"
#include "DataSet.h"
using namespace std;
using namespace lb;

DataSetDataStore::DataSetDataStore(GraphSpec const* _gs): m_operationKey(operationKey(_gs))
{
}

DataSetDataStore::~DataSetDataStore()
{
}

DataKey DataSetDataStore::operationKey(lb::GraphSpec const* _gs)
{
	return qHash(QString::fromStdString(_gs->name())) + 69 * qHash(QString::fromStdString(typeid(*_gs->ec()).name()));	// TODO: fold in parameters and version.
}

// Variable record length if 0. _dense if all hops are stored, otherwise will store sparsely.
void DataSetDataStore::init(unsigned _recordLength, bool _dense)
{
	m_s = NotedFace::data()->dataSet(DataKeySet(NotedFace::audio()->key(), m_operationKey));
	if (!m_s)
		cwarn << "Something else already opened the DataSet for writing?! :-(";

	if (!m_s->haveRaw())
		m_s->init(_recordLength, _dense ? NotedFace::audio()->hop(): 0, 0);
}

void DataSetDataStore::shiftBuffer(unsigned _index, foreign_vector<float> const& _record)
{
	if (m_s && !m_s->haveRaw())
		m_s->appendRecord(NotedFace::audio()->hop() * _index, _record);
}

void DataSetDataStore::fini(DigestTypes _digests)
{
	if (m_s)
	{
		for (DigestType t: _digests)
			m_s->ensureHaveDigest(t);
		m_s->done();
	}
	m_s = nullptr;
}

DataSet::DataSet(DataKeySet _key):
	m_sourceKey(_key.source),
	m_operationKey(_key.operation)
{
//	cdebug << (void*)this << (QThread::currentThreadId()) << "DataSet::DataSet(" << _operationKey << ")";
	if (!m_raw.open(m_sourceKey, m_operationKey, 0))
	{
		m_raw.reset();
		return;
	}

	m_first = m_raw.metadata().first;
	m_stride = m_raw.metadata().stride;
	m_recordLength = m_raw.metadata().recordLength;
	m_digestBase = m_raw.metadata().digestBase;
	m_elementSize = m_raw.metadata().elementSize;
	m_elementTypeName = std::string(m_raw.metadata().elementTypeName, sizeof(m_raw.mutableMetadata().elementTypeName));

	if (!m_recordLength)
	{
		if (!m_toc.open(m_sourceKey, m_operationKey, 1))
		{
			m_raw.reset();
			m_toc.reset();
			return;
		}
	}

	for (auto d: digestTypes())
	{
		MipmappedCache<>* c = new MipmappedCache<>;
		bool haveDigest = c->open(m_sourceKey, m_operationKey, 2 + (int)d, digestSize(d) * m_recordLength * m_elementSize, digestRecords());
		if (haveDigest)
			m_digest[d] = shared_ptr<MipmappedCache<> >(c);
	}
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
	m_raw.open(m_sourceKey, m_operationKey, 0);
	if (m_recordLength)
		m_toc.reset();
	else
		m_toc.open(m_sourceKey, m_operationKey, 1);
	m_recordCount = 0;
	m_pos = 0;
	m_digest.clear();
}

void DataSet::appendRecords(foreign_vector<float> const& _vs)
{
	assert(_vs.size());
	assert(m_recordLength && m_stride);
	assert(_vs.size() % m_recordLength == 0);

	if (!m_raw.isGood())
		m_raw.append(_vs);
	m_pos += _vs.size();
	m_recordCount += _vs.size() / m_recordLength;
//	cdebug << "[" << hex << m_operationKey << "] <<" << _vs.size() << "(pos:" << m_pos << " rc:" << m_recordCount << " rl:" << m_raw.file().size() << "ts" << m_toc.file().size() << ")";
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
//	cdebug << "[" << hex << m_operationKey << "] <<" << _vs.size() << "(pos:" << m_pos << " rc:" << m_recordCount << " rl:" << m_raw.file().size() << "ts" << m_toc.file().size() << ")";
}

void DataSet::finishedRaw()
{
	m_raw.ensureMapped();
	if (!m_raw.isGood())
	{
		m_raw.mutableMetadata().first = m_first;
		m_raw.mutableMetadata().stride = m_stride;
		m_raw.mutableMetadata().recordLength = m_recordLength;
		m_raw.mutableMetadata().digestBase = m_digestBase;
		m_raw.mutableMetadata().elementSize = m_elementSize;
		strncpy(m_raw.mutableMetadata().elementTypeName, m_elementTypeName.c_str(), sizeof(m_raw.mutableMetadata().elementTypeName));
		m_raw.setGood();
		if (m_toc.isOpen())
			m_toc.setGood();
	}
}

void DataSet::done()
{
	cdebug << "DONE [" << hex << m_operationKey << "] (pos:" << m_pos << " rc:" << m_recordCount << " rl:" << m_raw.file().size() << "ts" << m_toc.file().size() << ")";
	finishedRaw();
	NotedFace::data()->noteDone(DataKeySet(m_sourceKey, m_operationKey));
}

void DataSet::ensureHaveDigest(DigestType _t)
{
	assert(m_stride);
	assert(m_recordLength);
	m_raw.ensureMapped();
	m_digest[_t] = make_shared<MipmappedCache<>>();
	bool haveDigest = m_digest[_t]->open(m_sourceKey, m_operationKey, 2 + (int)_t, digestSize(_t) * recordLength() * sizeof(float), digestRecords());
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
		for (unsigned i = 0; f < fe; f += m_recordLength)
		{
			for (unsigned r = 0; r < m_recordLength; ++r)
				if (!i)
					d[r] = f[r];
				else
					d[r] += f[r];
			i++;
			if (i == m_digestBase)
			{
				for (unsigned r = 0; r < m_recordLength; ++r)
					d[r] /= m_digestBase;
				d += m_recordLength;
				i = 0;
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
	case MeanRmsDigest:
		assert(m_recordLength == 1);
		for (unsigned i = m_digestBase, t = 0; f < fe; ++f, ++t)
		{
			if (i == m_digestBase)
			{
				i = 0;
				d[0] = *f;
				d[1] = sqr(*f);
			}
			else
			{
				d[0] += *f;
				d[1] += sqr(*f);
			}
			i++;
			if (i == m_digestBase)
			{
				d[0] /= m_digestBase;
				d[1] = sqrt(d[1] / m_digestBase);
				d += 2;
			}
		}
		m_digest[_t]->generate([](lb::foreign_vector<float> a, lb::foreign_vector<float> b, lb::foreign_vector<float> ret)
		{
			unsigned i = 0;
			for (; i + 3 < a.size(); i += 4)
			{
				ret[i] = (a[i] + b[i]) / 2;
				ret[i+1] = sqrt((sqr(a[i+1]) + sqr(b[i+1])) / 2);
				ret[i+2] = (a[i+2] + b[i+2]) / 2;
				ret[i+3] = sqrt((sqr(a[i+3]) + sqr(b[i+3])) / 2);
			}
			for (; i + 1 < a.size(); i += 2)
			{
				ret[i] = (a[i] + b[i]) / 2;
				ret[i+1] = sqrt((sqr(a[i+1]) + sqr(b[i+1])) / 2);
			}
		}, float(0));
		break;
	case MinMaxInOutDigest:
		assert(m_recordLength == 1);
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

tuple<Time, unsigned, int, Time> DataSet::bestFit(Time _from, Time _duration, unsigned _idealRecords) const
{
	int64_t recordBegin = (_from - m_first) / m_stride;
	int64_t recordEnd = (_from + _duration - m_first + m_stride - 1) / m_stride;
	int64_t recordRes = _duration / m_stride;

	int level = -1;

	while (true)
	{
		int nextLevel = level + 1;
		int64_t nextBegin = nextLevel ? recordBegin / 2 : (recordBegin / (int64_t)m_digestBase);
		int64_t nextEnd = nextLevel ? (recordEnd + 1) / 2 : ((recordEnd + (int64_t)m_digestBase - 1) / (int64_t)m_digestBase);
		int64_t nextRes = nextLevel ? recordRes / 2 : (recordRes / (int64_t)m_digestBase);

		if (nextRes < _idealRecords)
			// Passed the place... use the last one cunningly left in recordBegin/recordEnd/level.
			return tuple<Time, unsigned, int, Time>(m_first + m_stride * (level > -1 ? m_digestBase << level : 1) * recordBegin, recordEnd - recordBegin, level, (recordEnd - recordBegin) * m_stride * (level > -1 ? m_digestBase << level : 1));
		recordBegin = nextBegin;
		recordEnd = nextEnd;
		recordRes = nextRes;
		level = nextLevel;
	}
}

void DataSet::populateRaw(lb::Time _from, foreign_vector<float> const& _out, XOf _transform) const
{
	assert(this);
	if (!haveRaw())
	{
		memset(_out.data(), 0, sizeof(float) * _out.size());
		return;
	}

	int recordBegin = (_from - m_first) / m_stride;
	int rLen = recordLength();
	int records = _out.size() / rLen;
	assert((int)_out.size() == records * rLen);

	foreign_vector<float const> d = m_raw.data<float>();
	int recordsAvailable = d.size() / rLen;

	// Beginning part - anything before our records begin should be zeroed.
	int beforeStart = clamp(-recordBegin, 0, records);
	memset(_out.data(), 0, sizeof(float) * rLen * beforeStart);

	// End part - anything after our records end should be zeroed.
	int overEnd = clamp(recordBegin + records - recordsAvailable, 0, records);
	memset(_out.data() + (records - overEnd) * rLen, 0, sizeof(float) * rLen * overEnd);

	int valid = records - beforeStart - overEnd;
	assert(valid <= records);
	if (_transform.isIdentity())
		valcpy(_out.data() + beforeStart * rLen, d.data() + (recordBegin + beforeStart) * rLen, rLen * valid);
	else
	{
		float const* i = d.data() + (recordBegin + beforeStart) * rLen;
		for (float* o = _out.data() + beforeStart * rLen, *oe = _out.data() + (beforeStart + valid) * rLen; o != oe; ++o, ++i)
			*o = _transform.apply(*i);
	}
}

void DataSet::populateDigest(DigestType _digest, unsigned _level, lb::Time _from, foreign_vector<float> const& _out, lb::XOf _transform) const
{
	assert(this);
	if (!m_digest.contains(_digest))
	{
		memset(_out.data(), 0, _out.size() * sizeof(float));
		return;
	}

/*	for (unsigned i = 0; i < _size; ++i)
		_out[i] = 0;*/
	int recordBegin = (_from - m_first) / m_stride / (m_digestBase << _level);
	int drLen = digestSize(_digest) * recordLength();
	int records = _out.size() / drLen;
	assert((int)_out.size() == records * drLen);

	foreign_vector<float> d = m_digest[_digest]->data<float>(_level);
	int recordsAvailable = d.size() / drLen;

	// Beginning part - anything before our records begin should be zeroed.
	int beforeStart = clamp(-recordBegin, 0, records);
	memset(_out.data(), 0, sizeof(float) * drLen * beforeStart);

	// End part - anything after our records end should be zeroed.
	int overEnd = clamp(recordBegin + records - recordsAvailable, 0, records);
	memset(_out.data() + (records - overEnd) * drLen, 0, sizeof(float) * drLen * overEnd);

	int valid = records - beforeStart - overEnd;
	assert(valid <= records);

	if (_transform.isIdentity())
	{
/*		for (unsigned i = 0; i < drLen * valid; ++i)
		{
			(_out + beforeStart * drLen)[i] = 0;
			float f = (d.data() + (recordBegin + beforeStart) * drLen)[i];
			(_out + beforeStart * drLen)[i] = f;
		}*/
		valcpy(_out.data() + beforeStart * drLen, d.data() + (recordBegin + beforeStart) * drLen, drLen * valid);
	}
	else
	{
		float const* i = d.data() + (recordBegin + beforeStart) * drLen;
		for (float* o = _out.data() + beforeStart * drLen, *oe = _out.data() + (beforeStart + valid) * drLen; o != oe; ++o, ++i)
			*o = _transform.apply(*i);
	}

//	cnote << "popDig: " << recordBegin << drLen << records << "(" << recordsAvailable << ") -> [" << beforeStart << "]" << valid << "[" << overEnd << "]";
}
