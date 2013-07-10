#pragma once
#include <memory>
#include <boost/function_types/parameter_types.hpp>
#include <QHash>
#include <QMap>
#include <Common/Flags.h>
#include <EventCompiler/GraphSpec.h>
#include "Cache.h"

enum DigestType
{
	NullDigest = 0,
	MeanDigest = 1,
	MeanRmsDigest = 2,
	MinMaxInOutDigest = 3,
	UnitCyclicMeanDigest = 4,
	PiCyclicMeanDigest = 5,
	TwoPiCyclicMeanDigest = 6
};
typedef std::set<DigestType> DigestTypes;

inline DigestTypes digestTypes() { return { MeanDigest, MeanRmsDigest, MinMaxInOutDigest }; }
inline unsigned digestSize(DigestType _f) { switch (_f) { case MeanDigest: return 1; case MeanRmsDigest: return 2; case MinMaxInOutDigest: return 4; default: return 0; } }

typedef uint32_t TocRef;
static const TocRef InvalidTocRef = (unsigned)-1;

/** Implementation of non-volatile map<lb::Time, Record>, where typedef vector<ElementType> Record;
 * API optimized for appending records, especially where the Record::size() is fixed (isFixed()) and the difference
 * between subsequent keys (i.e. lb::Times) is constant (isMonotonic()). Also features a digest function for
 * such cases to summarise the data in a recursive mapmap-like fashion.
 *
 * Contains dataset of a given key, a hash of the source data and any operations applied to it.
 * Data is composed of records, each at a particular point in time.
 * Each record is composed of a (possibly varying) number of ElementTypes (recordLength).
 * Each record is separated from the previous records by some (possibly varying) time step (stride).
 * Data may be digested using one or more of several techniques, but only when neither recordLength nor stride vary.
 * e.g.
@code
void foo(DataSet* ds)
{
  ds->init(...)
  if (!ds->haveRaw()) { ... ds->append() ... }
  ds->ensureHaveDigest(...);
  ds->done();
}
@endcode
 *
 */
class GenericDataSet
{
public:
	explicit GenericDataSet(DataKey _key, size_t _elementSize = 0, char const* _elementTypeName = nullptr);

	void setDense(unsigned _recordLength, lb::Time _stride, lb::Time _first = 0);

	void appendRecord(lb::Time _t, lb::foreign_vector<uint8_t> const& _vs);
	void appendDenseRecords(lb::foreign_vector<uint8_t> const& _vs);

	void ensureHaveDigest(DigestType _type);
	void done();

	lb::SimpleKey operationKey() const { return m_operationKey; }

	bool isAppendable() const { return m_raw.isOpen() && !isComplete(); }
	bool isValid() const { return m_elementTypeName.size() && m_elementSize; }
	bool isComplete() const { return haveRaw(); }
	bool haveRaw() const { return m_raw.isGood() && (!m_recordPositions.isMapped() || m_recordPositions.isGood()) && (!m_recordTimes.isMapped() || m_recordTimes.isGood()); }
	bool haveDigest(DigestType _type) { return m_digest.contains(_type); }
	template <class _T> bool isOfType() const { return sizeof(_T) == m_elementSize && typeid(_T).name() == m_elementTypeName; }
	bool isOfType(size_t _elementSize, char const* _elementTypeName) const { return _elementSize == m_elementSize && _elementTypeName == m_elementTypeName; }
	std::string const& elementTypeName() const { return m_elementTypeName; }

	bool isScalar() const { return m_recordLength == 1; }
	bool isFixed() const { return m_recordLength; }
	bool isDynamic() const { return !m_recordLength; }
	bool isMonotonic() const { return m_stride; }
	unsigned recordLength() const { return m_recordLength; }
	lb::Time first() const { return m_first ? m_first : timeOfRecord(0); }
	lb::Time stride() const { return m_stride; }
	lb::Time last() const { return timeOfRecord(rawRecords() - 1); }
	TocRef elements() const { return m_raw.bytes() / m_elementSize; }
	unsigned rawRecords() const;
	unsigned digestRecords() const { return (rawRecords() + m_digestBase - 1) / m_digestBase; }
	unsigned lodFactor(int _lod) const { return _lod == -1 ? 1 : (m_digestBase << _lod); }
	void finishedRaw();

	// Methods for extracting data when isMonotonic() && isFixed()
	std::tuple<lb::Time, unsigned, int, lb::Time> bestFit(lb::Time _from, lb::Time _duration, unsigned _idealRecords) const;
	void populateSeries(lb::Time _from, lb::foreign_vector<uint8_t> const& _out) const;
	void populateDigest(DigestType _digest, unsigned _level, lb::Time _from, lb::foreign_vector<uint8_t> const& _out) const;

	// General extraction method
	template <class _T> std::vector<_T> getInterval(lb::Time _from, lb::Time _before) const
	{
		if (!rawRecords())
			return {};

		TocRef f = elementIndex(_from);
		TocRef t = lb::defaultTo(elementIndex(_before), elements(), InvalidTocRef);
		std::vector<_T> ret(t - f);
		valcpy(ret.data(), m_raw.data<_T>().data() + f, ret.size());
		return ret;
	}

	unsigned recordIndex(lb::Time _t, bool _earlier = false) const;
	unsigned recordIndexLater(lb::Time _earliest) const { return recordIndex(_earliest, false); }
	unsigned recordIndexEarlier(lb::Time _latest) const { return recordIndex(_latest, true); }

	TocRef elementIndex(lb::Time _from) const { return elementIndexOfRecord(recordIndex(_from)); }
	TocRef elementIndexEarlier(lb::Time _latest) const { return elementIndexOfRecord(recordIndexEarlier(_latest)); }

	TocRef elementIndexOfRecord(unsigned _e) const;
	lb::Time timeOfRecord(unsigned _e) const;
	unsigned lengthOfRecord(unsigned _e) const { return lb::defaultTo(elementIndexOfRecord(_e + 1), elements(), InvalidTocRef) - elementIndexOfRecord(_e); }

	template <class _T> std::vector<_T> getRecord(lb::Time _latest, lb::Time* o_recordTime = nullptr) const
	{
		if (!isOfType<_T>())
			return std::vector<_T>();
		auto i = recordIndexEarlier(_latest);
		if (o_recordTime)
			*o_recordTime = timeOfRecord(i);
		std::vector<_T> ret(lengthOfRecord(i));
		populateRecord<_T>(i, &ret);
		return ret;
	}

	template <class _T> lb::foreign_vector<_T> peek(lb::Time _latest, unsigned _elements, lb::Time* o_recordTime = nullptr) const
	{
		if (!isOfType<_T>())
			return lb::foreign_vector<_T>();
		auto i = recordIndexEarlier(_latest);
		if (o_recordTime)
			*o_recordTime = timeOfRecord(i);
		auto ei = elementIndexOfRecord(i);
		if (ei == InvalidTocRef)
			return lb::foreign_vector<_T>();
		return m_raw.data<_T>().cropped(ei, _elements);
	}

	template <class _T> lb::foreign_vector<_T> peek(lb::Time _exact) const
	{
		if (!isOfType<_T>())
			return lb::foreign_vector<_T>();
		auto i = recordIndexEarlier(_exact);
		if (timeOfRecord(i) != _exact)
			return lb::foreign_vector<_T>();
		auto ei = elementIndexOfRecord(i);
		if (ei == InvalidTocRef)
			return lb::foreign_vector<_T>();
		return m_raw.data<_T>().cropped(ei, lengthOfRecord(i));
	}

	lb::foreign_vector<uint8_t> peekRecordBytes(lb::Time _latest, lb::Time* o_recordTime = nullptr) const
	{
		auto i = recordIndexEarlier(_latest);
		if (o_recordTime)
			*o_recordTime = timeOfRecord(i);
		auto ei = elementIndexOfRecord(i);
		if (ei == InvalidTocRef)
			return lb::foreign_vector<uint8_t>();
		return m_raw.data<uint8_t>().cropped(ei * m_elementSize, lengthOfRecord(i) * m_elementSize);
	}

	template <class _T> void populateRecord(unsigned _i, lb::foreign_vector<_T> const& _data) const
	{
		if (_i >= rawRecords())
			return;

		if (lengthOfRecord(_i) != _data.size())
			return;

		if (!isOfType<_T>())
			return;

		lb::valcpy(_data.data(), m_raw.data<_T>().data() + elementIndexOfRecord(_i), _data.size());
	}

	template <class _T, class _F> void forEachElement(unsigned _i, _F const& _f) const
	{
		if (_i >= rawRecords())
			return;

		if (!isOfType<_T>())
			return;

		auto d = m_raw.data<_T>().data() + elementIndexOfRecord(_i);
		unsigned l = lengthOfRecord(_i);
		for (unsigned i = 0; i < l; ++i, ++d)
			_f(*d);
	}

private:
	void init();
	void populateElements(TocRef _from, lb::foreign_vector<uint8_t> const& _out) const;

	struct Metadata
	{
		char elementTypeName[256];
		lb::Time first;
		lb::Time stride;
		uint32_t recordLength;
		uint32_t digestBase;
		uint32_t elementSize;
	};

	lb::SimpleKey m_sourceKey = 0;
	lb::SimpleKey m_operationKey = 0;
	Cache<Metadata> m_raw;
	Cache<> m_recordTimes;
	Cache<> m_recordPositions;

	QMap<DigestType, std::shared_ptr<MipmappedCache<>>> m_digest;
	unsigned m_digestBase = 8;
	size_t m_elementSize = 4;
	std::string m_elementTypeName;

	unsigned m_recordLength = 0;
	lb::Time m_stride = 0;
	lb::Time m_first = 0;

	unsigned m_recordCount = 0;
	TocRef m_pos = 0;
};

using GenericDataSetPtr = std::shared_ptr<GenericDataSet>;

template <class _T>
class DataSet: public GenericDataSet
{
public:
	explicit DataSet(DataKey _key): GenericDataSet(_key, sizeof(_T), typeid(_T).name()) {}

	void appendRecord(lb::Time _t, lb::foreign_vector<_T> const& _vs) { GenericDataSet::appendRecord(_t, (lb::foreign_vector<uint8_t>)_vs); }
	void appendDenseRecords(lb::foreign_vector<_T> const& _vs) { GenericDataSet::appendDenseRecords((lb::foreign_vector<uint8_t>)_vs); }

	void populateSeries(lb::Time _from, lb::foreign_vector<_T> const& _out) const { GenericDataSet::populateSeries(_from, (lb::foreign_vector<uint8_t>)_out); }
	void populateDigest(DigestType _digest, unsigned _level, lb::Time _from, lb::foreign_vector<_T> const& _out) const { GenericDataSet::populateDigest(_digest, _level, _from, (lb::foreign_vector<uint8_t>)_out); }

	std::vector<_T> getInterval(lb::Time _from, lb::Time _before) const { return GenericDataSet::getInterval<_T>(_from, _before); }
	std::vector<_T> getRecord(lb::Time _latest, lb::Time* o_recordTime = nullptr) const { return GenericDataSet::getRecord<_T>(_latest, o_recordTime); }
	lb::foreign_vector<_T> peek(lb::Time _latest, unsigned _elements, lb::Time* o_recordTime = nullptr) const { return GenericDataSet::peek<_T>(_latest, _elements, o_recordTime); }
	lb::foreign_vector<_T> peek(lb::Time _exact) const { return GenericDataSet::peek<_T>(_exact); }

	template <class _F> void forEachElement(unsigned _i, _F const& _f) const { GenericDataSet::forEachElement<_T, _F>(_i, _f); }
};

template <class _T> using DataSetPtr = std::shared_ptr<DataSet<_T> >;

using DataSetFloatPtr = DataSetPtr<float>;

template <class _T> DataSetPtr<_T> dataset_cast(GenericDataSetPtr const& _p)
{
	if (_p && _p->isOfType<_T>())
		return std::static_pointer_cast<DataSet<_T>>(_p);
	return nullptr;
}

class DataSetDataStore: public lb::DataStore
{
public:
	DataSetDataStore(lb::GraphSpec const* _gs, lb::SimpleKey _ecOpKey);
	virtual ~DataSetDataStore();

	bool isActive() const { return !!m_s; }
	void fini(DigestTypes _digests);

	static lb::SimpleKey operationKey(lb::GraphSpec const* _gs, lb::SimpleKey _ecOpKey) { return qHash(qMakePair(QString::fromStdString(_gs->name()), _ecOpKey)); }
	lb::SimpleKey operationKey() const { return m_operationKey; }

protected:
	// Variable record length if 0. _dense if all hops are stored, otherwise will store sparsely.
	virtual void init();
	virtual void shiftBuffer(unsigned _index, lb::foreign_vector<float> const& _record);

private:
	lb::SimpleKey m_operationKey = 0;
	DataSetFloatPtr m_s;
};

