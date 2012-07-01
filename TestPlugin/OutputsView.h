#pragma once

#include <iostream>
#include <cmath>

#include <Common/EventCompiler.h>
#include <Common/StreamEvent.h>

#include <QComboBox>
#include <QDebug>
#include <QFrame>
#include <QPaintEvent>
#include <QPainter>

#include <NotedPlugin/Timeline.h>

class LightshowPlugin;

class OutputsView: public Timeline
{
	Q_OBJECT

public:
	OutputsView(QWidget* _parent, LightshowPlugin* _p);
	~OutputsView();

private:
	virtual void doRender(QImage& _img, int _dx, int _dw);
	virtual QImage renderOverlay();

	LightshowPlugin* m_p;
};
