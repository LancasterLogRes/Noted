#pragma once

#include <QReadWriteLock>
#include <QAbstractItemModel>
#include <QReadWriteLock>
#include <QSet>
#include <EventCompiler/GraphSpec.h>
#include "Common.h"

/**
 * @brief Data concerning, but not expressing, a graph.
 * url is an identifier for this graph within the context of an analysis; it is valid
 * between subsequent executions. It is stored within this class as a convenience; this class
 * does not administer the value; that is left to GraphMan, which happens whenever an instance
 * of this class is registered with GraphMan.
 *
 * For event compiler graphs, it will be composed of some reference to the event compiler's
 * instance (e.g. its name) together with the operation at hand.
 *
 * key is the operation key for this graph's data. Multiple GraphMetadata instances may have
 * the same key, meaning that they refer to the same underlying graph data. For event
 * compiler graphs it is a hash of the event compiler's class name, its version and any
 * parameters it has. System graphs may have reserved keys (e.g. wave data, 0), or may hash
 * dependent parameters.
 */
class GraphMetadata
{
	friend class GraphManFace;

public:
	enum { ValueAxis = 0, XAxis = 1 };

	struct Axis
	{
		std::string label;
		lb::XOf transform;
		lb::Range range;
	};

	typedef std::vector<Axis> Axes;

	GraphMetadata() {}
	GraphMetadata(DataKey _operationKey, Axes const& _axes = { { "", lb::XOf(), lb::AutoRange } }, std::string const& _title = "Anonymous", bool _rawSource = false): m_rawSource(_rawSource), m_operationKey(_operationKey), m_title(_title), m_axes(_axes) {}

	bool isRawSource() const { return m_rawSource; }
	DataKey operationKey() const { return m_operationKey; }
	std::string const& url() const { return m_url; }
	std::string const& title() const { return m_title; }

	void setTitle(std::string const& _title) { m_title = _title; }
	void setRawSource(bool _rawSource = true) { m_rawSource = _rawSource; }
	void setOperationKey(DataKey _k) { m_operationKey = _k; }

	Axis const& axis(unsigned _i = ValueAxis) const { return m_axes[_i]; }
	Axes const& axes() const { return m_axes; }
	void setAxes(Axes const& _as) { m_axes = _as; }

protected:
	void setUrl(std::string const& _url) const { m_url = _url; }

	bool m_rawSource = false;
	DataKey m_operationKey = (unsigned)-1;
	std::string m_title = "Anonymous";

	Axes m_axes = { { "", lb::XOf(), lb::AutoRange } };

	mutable std::string m_url = "unregistered";
};

class GraphManFace: public QAbstractItemModel
{
	Q_OBJECT

public:
	GraphManFace() {}
	virtual ~GraphManFace();

	void registerGraph(QString const& _url, GraphMetadata const* _g);
	void unregisterGraph(GraphMetadata const* _g);

	void registerGraph(QString _url, lb::GraphSpec const* _g);
	void unregisterGraph(QString _url);
	void unregisterGraphs(QString _ec);

	lb::GraphSpec const* lockGraph(QString _url) const { x_graphs.lockForRead(); if (m_graphs.value(_url, nullptr)) return m_graphs.value(_url); unlockGraph(); return nullptr; }
	void unlockGraph() const { x_graphs.unlock(); }
	GraphMetadata const* lock(QString const& _url) const { x_graphs.lockForRead(); if (m_graphs2.value(_url, nullptr)) return m_graphs2.value(_url); unlock(); return nullptr; }
	void unlock() const { x_graphs.unlock(); }

signals:
	void graphsChanged();
	void addedGraph(QString _url);
	void removingGraph(QString _url);
	void addedGraph(GraphMetadata const*);
	void removingGraph(GraphMetadata const*);

protected:
	// TODO: replace lock with guarantee that GUI thread can't be running when graphs are going to change.
	mutable QReadWriteLock x_graphs;
	QMap<QString, lb::GraphSpec const*> m_graphs;

	QMap<QString, GraphMetadata const*> m_graphs2;
};

