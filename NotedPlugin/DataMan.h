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

	DataSetPtr dataSet(DataKeys _k);

	// Shouldn't be called directly - make a higher-level function call that kills off graphs & what not, too.
	void removeDataSet(DataKeys _k);

	// Deletes from disk unused datasets.
	void pruneDataSets(unsigned _maxMegabytes = 1024);

	DataSetPtr readDataSet(DataKeys _k) const { QMutexLocker l(&x_data); auto ret = m_data.value(_k); return ret && ret->haveRaw() ? ret : nullptr; }

	// Will give most convenient data extraction that covers range [_from, _from+_duration) (i.e. first record will represent data no later than _from).
	// @Returns tuple of start time, (guaranteed > _from), number of records and digest level or -1 if raw.
	std::tuple<lb::Time, unsigned, int, lb::Time> bestFit(DataKeys _operation, lb::Time _from, lb::Time _duration, unsigned _idealRecords) const;
	void populateRaw(DataKeys _operation, lb::Time _from, float* _out, unsigned _size) const; // never a digest
	void populateDigest(DataKeys _operation, DigestFlag _digest, unsigned _level, lb::Time _from, float* _out, unsigned _size) const;	// _size == records * digestSize(_digest) * recordLength(_key) for integer records.
	unsigned recordLength(DataKeys _operation) const;
	DigestFlags availableDigests(DataKeys _operation) const;
	unsigned rawRecordCount(DataKeys _operation) const;

public slots:
	void clearData();

signals:
	void dataComplete(DataKeys);

private:
	void noteDone(DataKeys _k) { emit dataComplete(_k); }

	mutable QMutex x_data;
	mutable QHash<DataKeys, DataSetPtr> m_data;		// not really mutable, just because QHash doesn't provide a const returner of const& items.

	static DataMan* s_this;
};
