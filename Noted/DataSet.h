#pragma once
#include <memory>
#include <QHash>
#include <QMap>
#include <Common/Flags.h>
#include "Cache.h"

enum DigestFlag { MeanDigest = 0x01, MinMaxInOutDigest = 0x02, UnitCyclicMeanDigest = 0x04, PiCyclicMeanDigest = 0x08, TwoPiCyclicMeanDigest = 0x10};
typedef Lightbox::Flags<DigestFlag> DigestFlags;
namespace Lightbox { template <> struct is_flag<DigestFlag>: public std::true_type { typedef DigestFlags FlagsType; }; }

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
	DataSet() {}
	explicit DataSet(DataKey _operationKey): m_operationKey(_operationKey) {}

	void init(unsigned _recordLength, unsigned _stride, Lightbox::Time _first = 0);	// _recordLength is in floats. _stride is in basic audio samples and is the distance between sequential readings. 0 for variable (NB no digest will be made). will be related to hops for most things.
	void initRandom(unsigned _recordLength);	// _recordLength is in floats. no digest will be made
	void initHopper(unsigned _recordLength, unsigned _strideHops = 1, Lightbox::Time _first = 0);	// _recordLength is in floats. _stride is in hops and is the distance between sequential readings. will be related to hops for most things.
	void init(unsigned _itemCount);

	void append(float _v);
	void append(Lightbox::foreign_vector<float> const& _vs);

	void digest(DigestFlag _type);
	void done();

	bool haveRaw() const { return m_pos == m_raw.bytes(); }
	bool haveDigest(DigestFlag _type) { return m_digest.contains(_type); }

	unsigned recordLength() const { return m_recordLength; }
	DigestFlags availableDigests() const { return m_availableDigests; }
	unsigned rawRecords() const { return m_raw.bytes() / sizeof(float) / m_recordLength; }
	unsigned digestRecords() const { return (rawRecords() + m_digestBase - 1) / m_digestBase; }

	std::tuple<Lightbox::Time, unsigned, int> bestFit(Lightbox::Time _from, Lightbox::Time _duration, unsigned _idealRecords) const;
	void populateRaw(Lightbox::Time _from, float* _out, unsigned _size) const;
	void populateDigest(DigestFlag _digest, unsigned _level, Lightbox::Time _from, float* _out, unsigned _size) const;

private:
	void setup(unsigned _itemCount);

	DataKey m_operationKey = 0;
	Cache m_raw;
	QMap<DigestFlag, std::shared_ptr<MipmappedCache>> m_digest;
	DigestFlags m_availableDigests;
	unsigned m_digestBase = 8;

	bool m_isReady = false;
	unsigned m_recordLength = 0;
	Lightbox::Time m_stride = 0;
	Lightbox::Time m_first = 0;

	unsigned m_pos = 0;
	Lightbox::foreign_vector<float> m_rawData;
};
