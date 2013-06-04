#include "GraphManFace.h"
using namespace std;
using namespace lb;

GraphManFace::~GraphManFace()
{
	cnote << "~GraphManFace()";
}

void GraphManFace::registerGraph(QString _url, lb::GraphSpec const* _g)
{
	cnote << "+ GRAPH" << _url.toStdString();
	{
		QWriteLocker l(&x_graphs);
		beginResetModel();
		m_graphs.insert(_url, _g);
		endResetModel();
		emit addedGraph(_url);
	}
	emit graphsChanged();
}

void GraphManFace::unregisterGraph(QString _url)
{
	cnote << "- GRAPH" << _url.toStdString();
	{
		QWriteLocker l(&x_graphs);
		emit removingGraph(_url);
		beginResetModel();
		m_graphs.remove(_url);
		endResetModel();
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

void GraphManFace::registerGraph(QString const& _url, GraphMetadata const* _g)
{
	cnote << "+ GRAPH2" << _url.toStdString();

	_g->setUrl(_url.toStdString());
	{
		QWriteLocker l(&x_graphs);
		beginResetModel();
		m_graphs2.insert(_url, _g);
		endResetModel();
		emit addedGraph(_g);
	}
	emit graphsChanged();
}

void GraphManFace::unregisterGraph(GraphMetadata const* _g)
{
	cnote << "- GRAPH" << _g->url();

	{
		QWriteLocker l(&x_graphs);
		emit removingGraph(_g);
		beginResetModel();
		m_graphs.remove(QString::fromStdString(_g->url()));
		endResetModel();
	}
	emit graphsChanged();
}
