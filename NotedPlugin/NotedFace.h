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
#include "QGLWidgetProxy.h"

LIGHTBOX_TEXTUAL_ENUM(AudioStage, Wave, Spectrum);

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

class NotedFace: public QMainWindow
{
	Q_OBJECT

	friend class Timeline;

public:
	NotedFace(QWidget* _p);
	virtual ~NotedFace();

	virtual QGLWidget* glMaster() const = 0;

	inline unsigned windowSizeSamples() const { return m_windowFunction.size(); }
	inline unsigned spectrumSize() const { return m_windowFunction.size() / 2 + 1; }
	inline std::vector<float> const& windowFunction() const { return m_windowFunction; }
	inline bool isZeroPhase() const { return m_zeroPhase; }
	inline bool isFloatFFT() const { return m_floatFFT; }
	inline lb::Time windowSize() const { return lb::toBase(windowSizeSamples(), audio()->rate()); }

	virtual lb::foreign_vector<float const> multiSpectrum(int _i, int _n) const = 0;
	virtual lb::foreign_vector<float const> magSpectrum(int _i, int _n) const = 0;
	virtual lb::foreign_vector<float const> phaseSpectrum(int _i, int _n) const = 0;
	virtual lb::foreign_vector<float const> deltaPhaseSpectrum(int _i, int _n) const = 0;

	virtual QList<EventsStore*> eventsStores() const = 0;
	virtual lb::EventCompiler findEventCompiler(QString const& _name) = 0;
	virtual QString getEventCompilerName(lb::EventCompilerImpl* _ec) = 0;

	virtual void addTimeline(Timeline* _p) = 0;
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

public slots:
	virtual void updateWindowTitle() = 0;

signals:
	void constructed();

	void eventsChanged();

protected:
	virtual void timelineDead(Timeline* _tl) = 0;

	AudioManFace* m_audioMan = nullptr;
	DataMan* m_dataMan = nullptr;
	GraphManFace* m_graphMan = nullptr;
	ComputeManFace* m_computeMan = nullptr;
	LibraryManFace* m_libraryMan = nullptr;
	ViewManFace* m_viewMan = nullptr;

	bool m_zeroPhase = false;
	bool m_floatFFT = true;
	std::vector<float> m_windowFunction;

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

	virtual void info(QString const&, QString const& = "gray") {}

	virtual lb::foreign_vector<float const> multiSpectrum(int, int) const { return lb::foreign_vector<float const>(); }
	virtual lb::foreign_vector<float const> magSpectrum(int, int) const { return lb::foreign_vector<float const>(); }
	virtual lb::foreign_vector<float const> deltaPhaseSpectrum(int, int) const { return lb::foreign_vector<float const>(); }
	virtual lb::foreign_vector<float const> phaseSpectrum(int, int) const { return lb::foreign_vector<float const>(); }

	virtual QList<EventsStore*> eventsStores() const { return QList<EventsStore*>(); }
	virtual lb::EventCompiler findEventCompiler(QString const&) { return lb::EventCompiler(); }
	virtual QString getEventCompilerName(lb::EventCompilerImpl*) { return ""; }

	virtual void timelineDead(Timeline*) {}
	virtual void addTimeline(Timeline*) {}
	virtual QWidget* addGLWidget(QGLWidgetProxy*) { return nullptr; }
	virtual void addDockWidget(Qt::DockWidgetArea, QDockWidget*) {}

	virtual void updateWindowTitle() {}
};
