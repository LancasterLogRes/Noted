#include <QFileDialog>
#include <Common/Global.h>
#include <NotedPlugin/DataMan.h>
#include <NotedPlugin/DataSet.h>
#include <NotedPlugin/NotedFace.h>
#include "Global.h"
#include "GraphMan.h"
using namespace std;
using namespace lb;

GraphMan::GraphMan()
{
}

GraphMan::~GraphMan()
{
}

QHash<int, QByteArray> GraphMan::roleNames() const
{
	return { { Qt::DisplayRole, "name" }, { Qt::UserRole, "url" }, { Qt::UserRole + 1, "range" } };
}

int GraphMan::rowCount(QModelIndex const& _parent) const
{
	if (m_urls.size() != m_graphs.size())
	{
		m_urls.clear();
		for (auto const& g: m_graphs)
			m_urls.append(QString::fromStdString(g.url()));
	}
	return _parent.isValid() ? 0 : m_graphs.size();
}

int GraphMan::columnCount(QModelIndex const&) const
{
	return 1;
}

QModelIndex GraphMan::index(int _row, int _column, QModelIndex const& _parent) const
{
	if (!_parent.isValid())
		return createIndex(_row, _column, nullptr);
	return QModelIndex();
}

QModelIndex GraphMan::parent(QModelIndex const&) const
{
	return QModelIndex();
}

QVariant GraphMan::data(QModelIndex const& _index, int _role) const
{
	if (_index.row() < m_urls.size())
	{
		auto const& g = m_graphs[m_urls[_index.row()]];
		if (_role == Qt::DisplayRole)
			return QString::fromStdString(g.title());
		if (_role == Qt::UserRole)
			return QString::fromStdString(g.url());
	}
	return QVariant();
}

Qt::ItemFlags GraphMan::flags(QModelIndex const&) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

QVariant GraphMan::headerData(int _section, Qt::Orientation, int _role) const
{
	return _role == Qt::DisplayRole && _section == 0 ? "Name" : QVariant();
}

void GraphMan::exportGraph(QString const& _url)
{
	QString f = QFileDialog::getSaveFileName(NotedFace::get(), "Export Graph Data", QDir::homePath(), "CSV format (*.csv *.txt)");
	if (f.size())
	{
		if (!f.contains("."))
			f.append(".csv");
		exportGraph(_url, f);
	}
}

void GraphMan::exportGraph(QString const& _url, QString _filename)
{
	pair<GraphMetadata, DataKey> m = graphAndKey(_url);
	if (DataSetPtr<float> ds = NotedFace::data()->get<float>(m.second))
	{
		ofstream out;
		out.open(_filename.toLocal8Bit(), ios::trunc);
		if (!out)
			return;
		out << "time";
		if (ds->isDynamic())
			out << "," << m.first.axis(GraphMetadata::XAxis).label << "," << m.first.title();
		else if (ds->isScalar())
			out << "," << m.first.title();
		else
			for (unsigned i = 0; i < ds->recordLength(); ++i)
				out << "," << m.first.axis(GraphMetadata::XAxis).label << m.first.axis(GraphMetadata::XAxis).transform.apply(i);
		out << endl;

		auto rs = ds->rawRecords();
		for (unsigned i = 0; i < rs; ++i)
		{
			if (ds->isDynamic())
			{
				unsigned i = 0;
				ds->forEachElement(i, [&](float f) {out << ds->timeOfRecord(i) << "," << m.first.axis(GraphMetadata::XAxis).transform.apply(i) << "," << m.first.axis().transform.apply(f) << endl; i++; });
			}
			else
			{
				out << toSeconds(ds->timeOfRecord(i));
				ds->forEachElement(i, [&](float f) {out << "," << m.first.axis().transform.apply(f);});
				out << endl;
			}
		}
	}
}
