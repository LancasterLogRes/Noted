#pragma once
#include <memory>
#include <QHash>
#include <QDir>
#include <QMutex>
#include <QObject>
#include <EventCompiler/EventCompiler.h>
#include "DataSet.h"

/// If you call releaseDataSets() then you'll currently need to manually call ComputeMan::invalidate() to
/// get the cache back up to speed (it forgets the metadata, so need reinitialisation, therefore a simple read
/// call can't open the file usefully).
///
/// Eventually, I'll fix that, so you'll be ok unless you subsequently call pruneDataSets(0), which just deletes
/// the lot. If you do that, you'll definitely need to recompute.
class DataMan: public QObject
{
	Q_OBJECT

	friend class DataSet;

public:
	DataMan();

	DataSetPtr dataSet(DataKeySet _k);

	/// Deletes from disk unused datasets.
	/// @returns true if resultant cache is less than @a _maxBytes.
	bool pruneDataSets(uint64_t _maxBytes = 0);

	static uint64_t footprint();
	uint64_t inUse() const;

	// TODO: template <class _T> DataSetPtr<_T> get(DataKeySet _k);
	DataSetPtr readDataSet(DataKeySet _k) const;

public slots:
	/// Don't call unless you're planning to recompute everything - it'll kill all your data!
	void releaseDataSets();

signals:
	void dataComplete(DataKeySet);
	void footprintChanged();
	void inUseChanged();
	void changed();

private:
	void noteDone(DataKeySet _k) { emit dataComplete(_k); }

	Q_PROPERTY(uint64_t footprint READ footprint NOTIFY footprintChanged)
	Q_PROPERTY(uint64_t inUse READ inUse NOTIFY inUseChanged)

	mutable QMutex x_data;
	mutable QHash<DataKeySet, DataSetPtr> m_data;		// not really mutable, just because QHash doesn't provide a const returner of const& items.
};
