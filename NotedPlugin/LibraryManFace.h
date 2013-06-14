#pragma once

#include <memory>
#include <QAbstractItemModel>
#include <EventCompiler/EventCompiler.h>

class NotedPlugin;

static const unsigned InvalidVersion = (unsigned)-1;

class LibraryManFace: public QAbstractItemModel
{
	Q_OBJECT

public:
	virtual ~LibraryManFace() {}

	virtual std::shared_ptr<NotedPlugin> getPlugin(QString const& _mangledName) = 0;
	virtual bool providesEventCompiler(QString const& _library, QString const& _ec) = 0;

	virtual lb::EventCompiler newEventCompiler(QString const& _name) = 0;
	virtual unsigned eventCompilerVersion(QString const& _name) = 0;

signals:
	void prepareLibraryUnload(QString _library);
	void doneLibraryLoad(QString _library);
	void eventCompilerFactoryAvailable(QString _name, unsigned _version);
	void eventCompilerFactoryUnavailable(QString _name);
};
