#pragma once

#include <QReadWriteLock>
#include <QAbstractItemModel>
#include <QReadWriteLock>
#include <QSet>
#include <Common/GraphMetadata.h>
#include <EventCompiler/GraphSpec.h>
#include "Common.h"

class GraphManFace: public QAbstractItemModel
{
	Q_OBJECT

public:
	GraphManFace() {}
	virtual ~GraphManFace();

	void registerGraph(QString const& _url, lb::GraphMetadata const& _g);
	void unregisterGraph(QString const& _url);
	void unregisterGraphs(QString _ec);

	lb::GraphMetadata find(QString const& _url) const { QReadLocker l(&x_graphs); if (m_graphs.count(_url)) return m_graphs[_url]; return lb::NullGraphMetadata; }
	std::pair<lb::GraphMetadata, DataKey> graphAndKey(QString const& _url);

signals:
	void graphsChanged();
	void addedGraph(lb::GraphMetadata const&);
	void removedGraph(lb::GraphMetadata const&);

public slots:
	virtual void exportGraph(QString const& _url) = 0;
	virtual void exportGraph(QString const& _url, QString _filename) = 0;

protected:
	// TODO: replace lock with guarantee that GUI thread can't be running when graphs are going to change.
	mutable QReadWriteLock x_graphs;
	mutable QMap<QString, lb::GraphMetadata> m_graphs;
};

