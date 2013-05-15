#pragma once
#include <memory>
#include <QHash>
#include <QMutex>
#include <QObject>
#include <EventCompiler/EventCompiler.h>
#include "DataSet.h"

class DataMan: public QObject
{
	Q_OBJECT

	friend class DataSet;

public:
	DataMan();
	static DataMan* get() { assert(s_this); return s_this; }

	DataSet* dataSet(DataKey _k);

	// Shouldn't be called directly - make a higher-level function call that kills off graphs & what not, too.
	void removeDataSet(DataKey _k);

	// Deletes from disk unused datasets.
	void pruneDataSets(unsigned _maxMegabytes = 1024);

	// Will give most convenient data extraction that covers range [_from, _from+_duration) (i.e. first record will represent data no later than _from).
	// @Returns tuple of start time, (guaranteed > _from), number of records and digest level or -1 if raw.
	std::tuple<Lightbox::Time, unsigned, int> bestFit(DataKey _key, Lightbox::Time _from, Lightbox::Time _duration, unsigned _idealRecords) const;
	void populateRaw(DataKey _key, Lightbox::Time _from, float* _out, unsigned _size) const; // never a digest
	void populateDigest(DataKey _key, DigestFlag _digest, unsigned _level, Lightbox::Time _from, float* _out, unsigned _size) const;	// _size == records * digestSize(_digest) * recordLength(_key) for integer records.
	unsigned recordLength(DataKey _key) const;
	DigestFlags availableDigests(DataKey _key) const;
	unsigned rawRecordCount(DataKey _key) const;

signals:
	void dataComplete(quint32);

private:
	void noteDone(DataKey _k) { emit dataComplete(_k); }

	mutable QMutex x_data;
	mutable QHash<DataKey, std::shared_ptr<DataSet>> m_data;		// not really mutable, just because QHash doesn't provide a const returner of const& items.

	static DataMan* s_this;
};
