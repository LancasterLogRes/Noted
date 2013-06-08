#pragma once
#include <memory>
#include <QHash>
#include <QMap>
#include <Common/Flags.h>
#include <EventCompiler/GraphSpec.h>
#include "Cache.h"

enum DigestFlag { MeanDigest = 0x01, MeanRmsDigest = 0x20, MinMaxInOutDigest = 0x02, UnitCyclicMeanDigest = 0x04, PiCyclicMeanDigest = 0x08, TwoPiCyclicMeanDigest = 0x10};
typedef lb::Flags<DigestFlag> DigestFlags;
namespace lb { template <> struct is_flag<DigestFlag>: public std::true_type { typedef DigestFlags FlagsType; }; }

inline unsigned digestSize(DigestFlag _f) { switch (_f) { case MeanDigest: return 1; case MeanRmsDigest: return 2; case MinMaxInOutDigest: return 4; default: return 0; } }

LIGHTBOX_STRUCT(2, DataKeys, DataKey, source, DataKey, operation);
inline uint qHash(DataKeys _k) { return _k.source ^ DataKey(_k.operation << 16) ^ DataKey(_k.operation >> 16); }


/** Implementation of non-volatile map<lb::Time, Record>, where typedef vector<float> Record;
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
  ds->digest(...);
  ds->done();
}
@endcode
 *
 */
class DataSet
{
public:
	explicit DataSet(DataKeys _key);

	void init(unsigned _recordLength, lb::Time _stride = 0, lb::Time _first = 0);	// _recordLength is in floats (0 for variable). _stride is the duration between sequential readings. will be related to hops for most things. (0 for variable). Don't call digest if either are zero.
	void init(unsigned _itemCount);

	void appendRecord(lb::Time _t, lb::foreign_vector<float> const& _vs);
	void appendRecords(lb::foreign_vector<float> const& _vs);

	void digest(DigestFlag _type);
	void done();

	bool haveRaw() const { return m_raw.isGood() && (!m_toc.isMapped() || m_toc.isGood()); }
	bool haveDigest(DigestFlag _type) { return m_digest.contains(_type); }

	bool isScalar() const { return m_recordLength == 1; }
	bool isFixed() const { return m_recordLength; }
	bool isDynamic() const { return !m_recordLength; }
	bool isMonotonic() const { return m_stride; }
	unsigned recordLength() const { return m_recordLength; }
	lb::Time first() const { return m_first; }
	lb::Time stride() const { return m_stride; }
	DigestFlags availableDigests() const { return m_availableDigests; }
	unsigned rawRecords() const;
	unsigned digestRecords() const { return (rawRecords() + m_digestBase - 1) / m_digestBase; }

	// Methods for extracting data when isMonotonic() && isFixed()
	std::tuple<lb::Time, unsigned, int, lb::Time> bestFit(lb::Time _from, lb::Time _duration, unsigned _idealRecords) const;
	void populateRaw(lb::Time _from, float* _out, unsigned _size, lb::XOf _transform = lb::XOf()) const;
	void populateDigest(DigestFlag _digest, unsigned _level, lb::Time _from, float* _out, unsigned _size, lb::XOf _transform = lb::XOf()) const;

	// Methods for extracting data when isFixed()
	void populateRaw(lb::Time _latest, lb::foreign_vector<float> _dest) const;

	// Methods for extracting data when !isFixed()
	std::vector<float> readRaw(lb::Time _latest) const;

private:
	typedef uint32_t TocRef;

	void setup(unsigned _itemCount);

	DataKey m_sourceKey = 0;
	DataKey m_operationKey = 0;
	Cache m_toc;
	Cache m_raw;

	QMap<DigestFlag, std::shared_ptr<MipmappedCache>> m_digest;
	DigestFlags m_availableDigests;
	unsigned m_digestBase = 8;

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
	void fini(DigestFlags _digests);

	static DataKey operationKey(lb::GraphSpec const* _gs);
	DataKey operationKey() const { return m_operationKey; }

protected:
	// Variable record length if 0. _dense if all hops are stored, otherwise will store sparsely.
	virtual void init(unsigned _recordLength, bool _dense);
	virtual void shiftBuffer(unsigned _index, lb::foreign_vector<float> const& _record);

private:
	DataKey m_operationKey = 0;
	DataSetPtr m_s;
};

