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

	friend class GenericDataSet;

public:
	DataMan();

	/// Either create a DataSet for the given key, or return the preexisting complete DataSet.
	/// @returns DataSet for the given key, possibly complete. Check DataSet::haveRaw() for whether
	/// it is complete before initialisation.
	template <class _T = float> DataSetPtr<_T> create(DataKey _k)
	{
//		cdebug << "DataMan::dataSet(" << std::hex << _k << ")";
		QMutexLocker l(&x_data);
		if (m_data.contains(_k))
		{
			assert(m_data[_k]->isOfType<_T>());
		}
		else
		{
			m_data[_k] = std::make_shared<DataSet<_T>>(_k);
			x_data.unlock();
			emit inUseChanged();
			emit footprintChanged();
			emit changed();
			x_data.lock();
			cdebug << "Creating.";
		}
		return std::static_pointer_cast<DataSet<_T>>(m_data[_k]);
	}

	/// Get a preexisting complete DataSet for a given key.
	/// @returns The complete DataSet for @a _k , or the null DataSetFloatPtr if it doesn't (yet) exist.
	template <class _T = float> DataSetPtr<_T> get(DataKey _k)
	{
		QMutexLocker l(&x_data);
		if (m_data.contains(_k))
		{
			if (m_data[_k] && m_data[_k]->haveRaw())
				return dataset_cast<_T>(m_data[_k]);
			return nullptr;
		}
		auto ds = std::make_shared<DataSet<_T>>(_k);
		if (ds->haveRaw())
		{
			m_data[_k] = ds;
			x_data.unlock();
			emit inUseChanged();
			emit changed();
			x_data.lock();
			return ds;
		}
		return nullptr;
	}

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
	mutable QHash<DataKey, GenericDataSetPtr> m_data;		// not really mutable, just because QHash doesn't provide a const returner of const& items.
};
