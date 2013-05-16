#pragma once

#include <memory>
#include <QObject>

class NotedPlugin;

class LibraryManFace: public QObject
{
	Q_OBJECT

public:
	virtual ~LibraryManFace() {}

	virtual std::shared_ptr<NotedPlugin> getPlugin(QString const& _mangledName) = 0;
	virtual bool providesEventCompiler(QString const& _library, QString const& _ec) = 0;

signals:
	void prepareLibraryUnload(QString _library);
	void doneLibraryLoad(QString _library);
};
