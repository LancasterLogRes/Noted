#include "NotedFace.h"
#include "GraphManFace.h"
using namespace std;
using namespace lb;

GraphManFace::~GraphManFace()
{
	cnote << "~GraphManFace()";
}

pair<GraphMetadata, DataKey> GraphManFace::graphAndKey(QString const& _url)
{
//	cnote << "findGraph" << _url;
	if (GraphMetadata g = find(_url))
		return make_pair(g, DataKey(g.isRawSource() ? NotedFace::audio()->rawKey() : NotedFace::audio()->key(), g.operationKey()));
	return pair<GraphMetadata, DataKey>();
}

void GraphManFace::registerGraph(QString const& _url, GraphMetadata const& _g)
{
	cnote << "+ GRAPH" << _url.toStdString();

	GraphMetadata removed;

	{
		QWriteLocker l(&x_graphs);
		beginResetModel();
		if (m_graphs.count(_url))
			removed = m_graphs[_url];
		m_graphs.insert(_url, _g);
		m_graphs[_url].setUrl(_url.toStdString());
		endResetModel();
	}

	if (removed)
		emit removedGraph(removed);
	emit addedGraph(m_graphs[_url]);
	emit graphsChanged();
}

void GraphManFace::unregisterGraph(QString const& _url)
{
	cnote << "- GRAPH" << _url.toStdString();

	GraphMetadata removed;

	{
		QWriteLocker l(&x_graphs);
		if (m_graphs.count(_url))
		{
			removed = m_graphs[_url];
			beginResetModel();
			m_graphs.remove(_url);
			endResetModel();
		}
	}

	if (removed)
	{
		emit removedGraph(removed);
		emit graphsChanged();
	}
}

void GraphManFace::unregisterGraphs(QString _ec)
{
	cnote << "- GRAPHS" << _ec.toStdString() << "/*";

	QVector<GraphMetadata> removed;

	{
		QWriteLocker l(&x_graphs);
		beginResetModel();
		for (auto i = m_graphs.begin(); i != m_graphs.end();)
			if (i.key().section('/', 0, 0) == _ec)
			{
				removed.append(i.value());
				i = m_graphs.erase(i);
			}
			else
				++i;
		endResetModel();
	}

	if (removed.size())
	{
		for (auto i: removed)
			emit removedGraph(i);
		emit graphsChanged();
	}
}
