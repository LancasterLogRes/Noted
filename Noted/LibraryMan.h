#pragma once

#include <QSettings>
#include <QStringList>
#include <QSet>
#include <QMap>
#include <QFileSystemWatcher>

#include <NotedPlugin/Library.h>
#include <NotedPlugin/LibraryManFace.h>

class QTreeWidgetItem;

struct RealLibrary: public Library
{
	RealLibrary(QString const& _f, bool _enabled): Library(_f), enabled(_enabled) {}
	void unload();
	bool enabled;
	QStringList required;
	QString parent;
	QString error;
};

typedef std::shared_ptr<RealLibrary> RealLibraryPtr;

class LibraryMan: public LibraryManFace
{
	Q_OBJECT

	friend class RealLibrary;

public:
	LibraryMan();
	virtual ~LibraryMan();

	virtual std::shared_ptr<NotedPlugin> getPlugin(QString const& _mangledName);

	virtual bool providesEventCompiler(QString const& _library, QString const& _ec);
	virtual lb::EventCompiler newEventCompiler(QString const& _name);
	virtual unsigned eventCompilerVersion(QString const& _name);

	QMap<QString, RealLibraryPtr> const& libraries() const { return m_libraries; }

	void readSettings(QSettings& _s);
	void writeSettings(QSettings& _s);

	void unloadAll();

public slots:
	void addLibrary(QString const& _name, bool _isEnabled = true);
	void removeLibrary(QString const& _name);
	void removeLibrary(QModelIndex _index);
	void reloadLibrary(QString const& _name);

	void reloadLibrary(QTreeWidgetItem*) {}
	void killLibrary(QTreeWidgetItem*) {}

private slots:
	void onLibraryChange(QString const& _name);

protected:
	virtual int rowCount(QModelIndex const& _parent) const;
	virtual int columnCount(QModelIndex const& _parent) const;
	virtual QVariant data(QModelIndex const& _index, int _role) const;
	virtual Qt::ItemFlags flags(QModelIndex const& _index) const;
	virtual QVariant headerData(int _sectiom, Qt::Orientation _o, int _role) const;
	virtual QModelIndex index(int _row, int _column, QModelIndex const& _parent) const;
	virtual QModelIndex parent(QModelIndex const& _index) const;
	virtual bool setData(QModelIndex const& _index, QVariant const& _value, int _role);

private:
	// Extensions...
	void load(RealLibraryPtr const& _dl);
	void unload(RealLibraryPtr const& _dl);
	void setEnabled(RealLibraryPtr const& _dl, bool _e);

	QMap<QString, RealLibraryPtr> m_libraries;
	QSet<QString> m_dirtyLibraries;
	QFileSystemWatcher m_libraryWatcher;
	bool m_notedIsConstructed = false;
};
