#pragma once

#include <QSet>
#include <QMap>
#include <QFileSystemWatcher>

#include <NotedPlugin/Library.h>
#include <NotedPlugin/LibraryManFace.h>

class QTreeWidgetItem;

struct RealLibrary: public Library
{
	RealLibrary(QString const& _f): Library(_f) {}
	QTreeWidgetItem* item;
	void unload();
	bool isEnabled() const;
};

typedef std::shared_ptr<RealLibrary> RealLibraryPtr;

class LibraryMan: public LibraryManFace
{
	Q_OBJECT

public:
	LibraryMan();
	virtual ~LibraryMan();

	virtual std::shared_ptr<NotedPlugin> getPlugin(QString const& _mangledName);

	QMap<QString, RealLibraryPtr> const& libraries() const { return m_libraries; }

public slots:
	void addLibrary(QString const& _name, bool _isEnabled = true);
	void reloadLibrary(QTreeWidgetItem* _it);
	void killLibrary(QTreeWidgetItem* _it);

private slots:
	void onLibraryChange(QString const& _name);

private:
	// Extensions...
	void load(RealLibraryPtr const& _dl);
	void unload(RealLibraryPtr const& _dl);

	void reloadDirties();

	QMap<QString, RealLibraryPtr> m_libraries;
	QSet<QString> m_dirtyLibraries;
	QFileSystemWatcher m_libraryWatcher;
};
