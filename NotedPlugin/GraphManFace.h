#pragma once

#include <QObject>
#include <QReadWriteLock>
#include <QHash>
#include <QStringList>
#include <EventCompiler/GraphSpec.h>

class GraphManFace: public QObject
{
	Q_OBJECT

public:
	GraphManFace() {}

	// TODO: add a model

	void registerGraph(QString _url, lb::GraphSpec const* _g) { { QReadLocker l(&x_graphs); m_graphs.insert(_url, _g); } emit graphAdded(_url); }
	void unregisterGraph(QString _url) { { QWriteLocker l(&x_graphs); m_graphs.remove(_url); } emit graphRemoved(_url); }

	QStringList graphs() const { QReadLocker l(&x_graphs); return m_graphs.keys(); }
	lb::GraphSpec const* lockGraph(QString const& _url) const { x_graphs.lockForRead(); if (m_graphs.contains(_url)) return m_graphs[_url]; x_graphs.unlock(); return nullptr; }
	void unlockGraph(lb::GraphSpec const* _graph) const { if (_graph) x_graphs.unlock(); }

signals:
	void graphAdded(QString _url);
	void graphRemoved(QString _url);

private:
	Q_PROPERTY(QStringList graphs READ graphs())

	mutable QReadWriteLock x_graphs;
	QHash<QString, lb::GraphSpec const*> m_graphs;
};

