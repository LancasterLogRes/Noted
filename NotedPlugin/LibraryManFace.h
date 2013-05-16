#pragma once

#include <memory>
#include <QAbstractItemModel>

class NotedPlugin;

class LibraryManFace: public QAbstractItemModel
{
	Q_OBJECT

public:
	virtual ~LibraryManFace() {}

	virtual std::shared_ptr<NotedPlugin> getPlugin(QString const& _mangledName) = 0;
	virtual bool providesEventCompiler(QString const& _library, QString const& _ec) = 0;

signals:
	void prepareLibraryUnload(QString _library);
	void doneLibraryLoad(QString _library);
	void eventCompilerFactoryAvailable(QString _name);
	void eventCompilerFactoryUnavailable(QString _name);
};
