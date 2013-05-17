#pragma once
#include <memory>
#include <QHash>
#include <QMap>
#include <Common/Flags.h>
#include "Cache.h"

enum DigestFlag { MeanDigest = 0x01, MinMaxInOutDigest = 0x02, UnitCyclicMeanDigest = 0x04, PiCyclicMeanDigest = 0x08, TwoPiCyclicMeanDigest = 0x10};
typedef lb::Flags<DigestFlag> DigestFlags;
namespace lb { template <> struct is_flag<DigestFlag>: public std::true_type { typedef DigestFlags FlagsType; }; }

inline unsigned digestSize(DigestFlag _f) { switch (_f) { case MeanDigest: return 1; case MinMaxInOutDigest: return 4; default: return 0; } }

/** Contains dataset of a given key, a hash of the source data and any operations applied to it.
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
	explicit DataSet(DataKey _operationKey);

	void init(unsigned _recordLength, lb::Time _stride = 0, lb::Time _first = 0);	// _recordLength is in floats (0 for variable). _stride is the duration between sequential readings. will be related to hops for most things. (0 for variable). Don't call digest if either are zero.
	void init(unsigned _itemCount);

	void appendRecord(lb::Time _t, lb::foreign_vector<float> const& _vs);

	void digest(DigestFlag _type);
	void done();

	bool haveRaw() const { return m_raw.isGood() && (!m_toc.isMapped() || m_toc.isGood()); }
	bool haveDigest(DigestFlag _type) { return m_digest.contains(_type); }

	unsigned recordLength() const { return m_recordLength; }
	DigestFlags availableDigests() const { return m_availableDigests; }
	unsigned rawRecords() const;
	unsigned digestRecords() const { return (rawRecords() + m_digestBase - 1) / m_digestBase; }

	// Methods for extracting data from fixed stride & record length sets.
	std::tuple<lb::Time, unsigned, int> bestFit(lb::Time _from, lb::Time _duration, unsigned _idealRecords) const;
	void populateRaw(lb::Time _from, float* _out, unsigned _size) const;
	void populateDigest(DigestFlag _digest, unsigned _level, lb::Time _from, float* _out, unsigned _size) const;

private:
	typedef uint32_t TocRef;

	void setup(unsigned _itemCount);

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
