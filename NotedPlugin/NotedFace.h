/* BEGIN COPYRIGHT
 *
 * This file is part of Noted.
 *
 * Copyright ©2011, 2012, Lancaster Logic Response Limited.
 *
 * Noted is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Noted is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Noted.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <cassert>
#include <memory>
#include <vector>
#include <functional>

#include <QMultiMap>
#include <QHash>
#include <QFile>
#include <QMainWindow>
#include <QRect>
#include <QMutex>
#include <QReadWriteLock>
#include <QLibrary>

#include <Common/Common.h>
#include <EventCompiler/EventCompiler.h>

#include "DataMan.h"
#include "AudioManFace.h"
#include "ComputeManFace.h"
#include "GraphManFace.h"
#include "LibraryManFace.h"
#include "ViewManFace.h"
#include "EventsManFace.h"
#include "QGLWidgetProxy.h"

inline std::ostream& operator<<(std::ostream& _out, QString const& _s) { return _out << _s.toLocal8Bit().data(); }

class Timeline;
class EventsStore;
class CausalAnalysis;
class QGLWidget;
class NotedPlugin;
class AuxLibraryFace;
class Prerendered;
class DataMan;
class GraphManFace;
class AudioManFace;
class ComputeManFace;
class ViewManFace;
class EventCompilerView;

class NotedFace: public QMainWindow
{
	Q_OBJECT

	friend class Timeline;

public:
	NotedFace(QWidget* _p);
	virtual ~NotedFace();

	virtual QGLWidget* glMaster() const = 0;

	virtual void addLegacyTimeline(QWidget* _p) = 0;
	virtual QWidget* addGLWidget(QGLWidgetProxy* _v, QWidget* _p = nullptr) = 0;
	virtual void addDockWidget(Qt::DockWidgetArea _a, QDockWidget* _d) = 0;
	virtual void info(QString const& _info, QString const& _color = "gray") = 0;

	static NotedFace* get() { assert(s_this); return s_this; }
	static AudioManFace* audio() { return get()->m_audioMan; }
	static DataMan* data() { return get()->m_dataMan; }
	static GraphManFace* graphs() { return get()->m_graphMan; }
	static ComputeManFace* compute() { return get()->m_computeMan; }
	static LibraryManFace* libs() { return get()->m_libraryMan; }
	static ViewManFace* view() { return get()->m_viewMan; }
	static EventsManFace* events() { return get()->m_eventsMan; }

public slots:
	virtual void updateWindowTitle() = 0;

signals:
	void constructed();

protected:
	AudioManFace* m_audioMan = nullptr;
	DataMan* m_dataMan = nullptr;
	GraphManFace* m_graphMan = nullptr;
	ComputeManFace* m_computeMan = nullptr;
	LibraryManFace* m_libraryMan = nullptr;
	ViewManFace* m_viewMan = nullptr;
	EventsManFace* m_eventsMan = nullptr;

	static NotedFace* s_this;
};

static const QVector<int16_t> DummyQVectorInt16;
static const std::vector<float> DummyVectorFloat;

class DummyNoted: public NotedFace
{
public:
	DummyNoted(QWidget* _p = nullptr): NotedFace(_p) {}
	virtual ~DummyNoted() {}

	virtual QGLWidget* glMaster() const { return nullptr; }

	virtual void addLegacyTimeline(QWidget*) {}
	virtual QWidget* addGLWidget(QGLWidgetProxy*) { return nullptr; }
	virtual void addDockWidget(Qt::DockWidgetArea, QDockWidget*) {}
	virtual void info(QString const&, QString const& = "gray") {}

	virtual void updateWindowTitle() {}
};
