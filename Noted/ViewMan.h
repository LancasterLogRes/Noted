#pragma once

#include <NotedPlugin/ViewManFace.h>

class ViewMan: public ViewManFace
{
	Q_OBJECT

public:
	ViewMan(QObject* _p = nullptr): ViewManFace(_p) {}

public slots:
	virtual void normalize();
};

