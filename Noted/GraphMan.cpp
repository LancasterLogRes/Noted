#include <Common/Global.h>
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

int GraphMan::rowCount(QModelIndex const& _parent) const
{
	return _parent.isValid() ? 0 : m_graphs.size();
}

int GraphMan::columnCount(QModelIndex const&) const
{
	return 1;
}

QModelIndex GraphMan::index(int _row, int _column, QModelIndex const& _parent) const
{
	int r = 0;
	if (!_parent.isValid())
		for (auto g: m_graphs)
			if (r++ == _row)
				return createIndex(_row, _column, (void*)g);
	return QModelIndex();
}

QModelIndex GraphMan::parent(QModelIndex const&) const
{
	return QModelIndex();
}

QVariant GraphMan::data(QModelIndex const& _index, int _role) const
{
	auto l = (GraphSpec const*)_index.internalPointer();
	if (_role == Qt::DisplayRole)
		return QString::fromStdString(l->name());
	return QVariant();
}

Qt::ItemFlags GraphMan::flags(QModelIndex const& _index) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

QVariant GraphMan::headerData(int _section, Qt::Orientation, int _role) const
{
	return _role == Qt::DisplayRole && _section == 0 ? "Name" : QVariant();
}

