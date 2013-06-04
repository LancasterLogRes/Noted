#pragma once

#include <NotedPlugin/ViewManFace.h>

class ViewMan: public ViewManFace
{
	Q_OBJECT

public:
	ViewMan(QObject* _p = nullptr): ViewManFace(_p) {}

	virtual lb::Time globalPitch() const;
	virtual lb::Time globalOffset() const;

public slots:
	virtual void normalize();
};

