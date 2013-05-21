#pragma once

#include <QReadWriteLock>
#include <QAbstractItemModel>
#include <QReadWriteLock>
#include <QSet>
#include <EventCompiler/GraphSpec.h>

class GraphManFace: public QAbstractItemModel
{
	Q_OBJECT

public:
	GraphManFace() {}

	void registerGraph(QString _url, lb::GraphSpec const* _g);
	void unregisterGraph(QString _url);
	void unregisterGraphs(QString _ec);

	lb::GraphSpec const* lockGraph(QString _url) const { x_graphs.lockForRead(); if (m_graphs.contains(_url)) return m_graphs.value(_url); return nullptr; }
	void unlockGraph() const { x_graphs.unlock(); }

signals:
	void graphsChanged();
	void addedGraph(QString _url);
	void removingGraph(QString _url);

protected:
	// TODO: replace lock with guarantee that GUI thread can't be running when graphs are going to change.
	mutable QReadWriteLock x_graphs;
	QMap<QString, lb::GraphSpec const*> m_graphs;
};

