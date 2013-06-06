#include "GraphManFace.h"
using namespace std;
using namespace lb;

GraphManFace::~GraphManFace()
{
	cnote << "~GraphManFace()";
}

void GraphManFace::registerGraph(QString const& _url, GraphMetadata const& _g)
{
	cnote << "+ GRAPH" << _url.toStdString();

	{
		QWriteLocker l(&x_graphs);
		beginResetModel();
		m_graphs.insert(_url, _g);
		m_graphs[_url].setUrl(_url.toStdString());
		endResetModel();
		emit addedGraph(_g);
	}
	emit graphsChanged();
}

void GraphManFace::unregisterGraph(QString const& _url)
{
	cnote << "- GRAPH" << _url.toStdString();

	{
		QWriteLocker l(&x_graphs);
		if (m_graphs.count(_url))
		{
			emit removingGraph(m_graphs[_url]);
			beginResetModel();
			m_graphs.remove(_url);
			endResetModel();
		}
	}
	emit graphsChanged();
}

void GraphManFace::unregisterGraphs(QString _ec)
{
	cnote << "- GRAPHS" << _ec.toStdString() << "/*";

	{
		QWriteLocker l(&x_graphs);
		beginResetModel();
		for (auto i = m_graphs.begin(); i != m_graphs.end();)
			if (i.key().section('/', 0, 0) == _ec)
			{
				emit removingGraph(i.key());
				i = m_graphs.erase(i);
			}
			else
				++i;
		endResetModel();
	}
	emit graphsChanged();
}
