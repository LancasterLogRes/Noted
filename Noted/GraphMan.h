#pragma once

#include <NotedPlugin/GraphManFace.h>

class GraphMan: public GraphManFace
{
public:
	GraphMan();
	virtual ~GraphMan();

protected:
	virtual int rowCount(QModelIndex const& _parent) const;
	virtual int columnCount(QModelIndex const& _parent) const;
	virtual QVariant data(QModelIndex const& _index, int _role) const;
	virtual Qt::ItemFlags flags(QModelIndex const& _index) const;
	virtual QVariant headerData(int _sectiom, Qt::Orientation _o, int _role) const;
	virtual QModelIndex index(int _row, int _column, QModelIndex const& _parent) const;
	virtual QModelIndex parent(QModelIndex const& _index) const;
	virtual QHash<int, QByteArray> roleNames() const;

private:
	mutable QList<QString> m_urls;
};
