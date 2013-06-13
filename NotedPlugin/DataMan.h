#pragma once
#include <memory>
#include <QHash>
#include <QDir>
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

	// TODO: template <class _T> DataSetPtr<_T> get(DataKey _k);
	/// Either create a DataSet for the given key, or return the preexisting complete DataSet.
	/// @returns DataSet for the given key, possibly complete. Check DataSet::haveRaw() for whether
	/// it is complete before initialisation.
	DataSetPtr create(DataKey _k);

	/// Get a preexisting complete DataSet for a given key.
	/// @returns The complete DataSet for @a _k , or the null DataSetPtr if it doesn't (yet) exist.
	DataSetPtr get(DataKey _k);

	/// Deletes from disk unused datasets.
	/// @returns true if resultant cache is less than @a _maxBytes.
	/// @note If you call releaseDataSets first, there's no guarantee that you won't kill good data
	/// and need to call ComputeMan::invalidate() to regenerate it.
	bool pruneDataSets(uint64_t _maxBytes = 0);

	static uint64_t footprint();
	uint64_t inUse() const;

public slots:
	void releaseDataSets();

signals:
	void dataComplete(DataKey);
	void footprintChanged();
	void inUseChanged();
	void changed();

private:
	void noteDone(DataKey _k) { emit dataComplete(_k); }

	Q_PROPERTY(uint64_t footprint READ footprint NOTIFY footprintChanged)
	Q_PROPERTY(uint64_t inUse READ inUse NOTIFY inUseChanged)

	mutable QMutex x_data;
	mutable QHash<DataKey, DataSetPtr> m_data;		// not really mutable, just because QHash doesn't provide a const returner of const& items.
};
