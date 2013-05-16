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
};
