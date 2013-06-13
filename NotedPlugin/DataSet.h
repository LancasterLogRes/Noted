#pragma once
#include <memory>
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

/** Implementation of non-volatile multimap<lb::Time, Record>, where typedef vector<ElementType> Record; typedef float ElementType;
 * TODO: ElementType should be a template parameter.
 * API optimized for appending records, especially where the Record::size() is fixed (isFixed()) and the difference
 * between subsequent keys (i.e. lb::Times) is constant (isMonotonic()). Also features a digest function for
 * such cases to summarise the data in a recursive mapmap-like fashion.
 *
 * Contains dataset of a given key, a hash of the source data and any operations applied to it.
 * Data is composed of records, each at a particular point in time.
 * Each record is composed of a (possibly varying) number of floats (recordLength).
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
class DataSet
{
public:
	explicit DataSet(DataKey _key);

	void init(unsigned _recordLength, lb::Time _stride = 0, lb::Time _first = 0);	// _recordLength is in floats (0 for variable). _stride is the duration between sequential readings. will be related to hops for most things. (0 for variable). Don't call digest if either are zero.
	void init(unsigned _itemCount);

	void appendRecord(lb::Time _t, lb::foreign_vector<float> const& _vs);
	void appendRecords(lb::foreign_vector<float> const& _vs);

	void ensureHaveDigest(DigestType _type);
	void done();

	bool haveRaw() const { return m_raw.isGood() && (!m_toc.isMapped() || m_toc.isGood()); }
	bool haveDigest(DigestType _type) { return m_digest.contains(_type); }

	bool isScalar() const { return m_recordLength == 1; }
	bool isFixed() const { return m_recordLength; }
	bool isDynamic() const { return !m_recordLength; }
	bool isMonotonic() const { return m_stride; }
	unsigned recordLength() const { return m_recordLength; }
	lb::Time first() const { return m_first; }
	lb::Time stride() const { return m_stride; }
	unsigned rawRecords() const;
	unsigned digestRecords() const { return (rawRecords() + m_digestBase - 1) / m_digestBase; }
	unsigned lodFactor(int _lod) const { return _lod == -1 ? 1 : (m_digestBase << _lod); }
	void finishedRaw();

	// Methods for extracting data when isMonotonic() && isFixed()
	std::tuple<lb::Time, unsigned, int, lb::Time> bestFit(lb::Time _from, lb::Time _duration, unsigned _idealRecords) const;
	void populateRaw(lb::Time _from, lb::foreign_vector<float> const& _out, lb::XOf _transform = lb::XOf()) const;
	void populateDigest(DigestType _digest, unsigned _level, lb::Time _from, lb::foreign_vector<float> const& _out, lb::XOf _transform = lb::XOf()) const;

	// Methods for extracting data when isFixed()
//	void populateRaw(lb::Time _latest, lb::foreign_vector<float> _dest) const;

	// Methods for extracting data when !isFixed()
//	std::vector<float> readRaw(lb::Time _latest) const;

private:
	typedef uint32_t TocRef;

	void setup(unsigned _itemCount);

	struct Metadata
	{
		char elementTypeName[256];
		lb::Time first;
		lb::Time stride;
		uint32_t recordLength;
		uint32_t digestBase;
		uint32_t elementSize;
	};

	SimpleKey m_sourceKey = 0;
	SimpleKey m_operationKey = 0;
	Cache<Metadata> m_raw;
	Cache<> m_toc;

	QMap<DigestType, std::shared_ptr<MipmappedCache<>>> m_digest;
	DigestTypes m_availableDigests;
	unsigned m_digestBase = 8;
	size_t m_elementSize = 4;
	std::string m_elementTypeName;

	bool m_isReady = false;
	unsigned m_recordLength = 0;
	lb::Time m_stride = 0;
	lb::Time m_first = 0;
	unsigned m_recordCount = 0;
	unsigned m_pos = 0;
};

typedef std::shared_ptr<DataSet> DataSetPtr;

class DataSetDataStore: public lb::DataStore
{
public:
	DataSetDataStore(lb::GraphSpec const* _gs);
	virtual ~DataSetDataStore();

	bool isActive() const { return !!m_s; }
	void fini(DigestTypes _digests);

	static SimpleKey operationKey(lb::GraphSpec const* _gs);
	SimpleKey operationKey() const { return m_operationKey; }

protected:
	// Variable record length if 0. _dense if all hops are stored, otherwise will store sparsely.
	virtual void init(unsigned _recordLength, bool _dense);
	virtual void shiftBuffer(unsigned _index, lb::foreign_vector<float> const& _record);

private:
	SimpleKey m_operationKey = 0;
	DataSetPtr m_s;
};

