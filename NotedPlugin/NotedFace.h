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

class NotedFace: public QMainWindow
{
	Q_OBJECT

	friend class Timeline;

public:
	NotedFace(QWidget* _p);
	virtual ~NotedFace();

	virtual bool carryOn(int _progress) = 0;
	virtual int activeWidth() const = 0;
	virtual QGLWidget* glMaster() const = 0;

	inline lb::Time earliestVisible() const { return m_timelineOffset; }
	inline lb::Time pixelDuration() const { return m_pixelDuration; }
	inline lb::Time cursor() const { return m_fineCursor / hop() * hop(); }
	inline lb::Time latestVisible() const { return earliestVisible() + visibleDuration(); }
	inline lb::Time visibleDuration() const { return activeWidth() * pixelDuration(); }

	inline unsigned hopSamples() const { return m_audioMan->hopSamples(); }
	inline unsigned windowSizeSamples() const { return m_windowFunction.size(); }
	inline unsigned rate() const { return m_audioMan->rate(); }
	inline unsigned spectrumSize() const { return m_windowFunction.size() / 2 + 1; }
	inline std::vector<float> const& windowFunction() const { return m_windowFunction; }
	inline bool isZeroPhase() const { return m_zeroPhase; }
	inline bool isFloatFFT() const { return m_floatFFT; }
	inline unsigned samples() const { return m_audioMan->samples(); }

	inline lb::Time hop() const { return lb::toBase(hopSamples(), rate()); }
	inline lb::Time windowSize() const { return lb::toBase(windowSizeSamples(), rate()); }
	inline int widthOf(lb::Time _t) const { return samples() ? (_t + pixelDuration() / 2) / pixelDuration() : 0; }
	inline lb::Time durationOf(int _screenWidth) const { return _screenWidth * pixelDuration(); }
	inline int positionOf(lb::Time _t) const { return widthOf(_t - earliestVisible()); }
	inline lb::Time timeOf(int _x) const { return durationOf(_x) + earliestVisible(); }
	inline unsigned cursorIndex() const { return windowIndex(cursor()); }
	inline unsigned windowIndex(lb::Time _t) const { return (_t < 0) ? 0 : std::min<unsigned>(_t / hop(), (samples() - windowSizeSamples()) / hopSamples()); }
	inline unsigned hops() const { return samples() ? samples() / hopSamples() : 0; }
	inline lb::Time duration() const { return lb::toBase(samples(), rate()); }

	virtual lb::foreign_vector<float const> waveWindow(int _window) const = 0;
	// TODO: extra argument/double-size vector for min/max range of each sample in o_toFill.
	virtual bool waveBlock(lb::Time _from, lb::Time _duration, lb::foreign_vector<float> o_toFill, bool _forceSamples = false) const = 0;
	virtual lb::foreign_vector<float const> multiSpectrum(int _i, int _n) const = 0;
	virtual lb::foreign_vector<float const> magSpectrum(int _i, int _n) const = 0;
	virtual lb::foreign_vector<float const> phaseSpectrum(int _i, int _n) const = 0;
	virtual lb::foreign_vector<float const> deltaPhaseSpectrum(int _i, int _n) const = 0;

	virtual QList<EventsStore*> eventsStores() const = 0;
	virtual lb::EventCompiler newEventCompiler(QString const& _name) = 0;
	virtual lb::EventCompiler findEventCompiler(QString const& _name) = 0;
	virtual QString getEventCompilerName(lb::EventCompilerImpl* _ec) = 0;

	virtual bool isPlaying() const = 0;
	virtual bool isCausal() const = 0;
	virtual bool isPassing() const = 0;
	inline bool isImmediate() const { return isCausal() || isPassing(); }
	inline bool isQuiet() const { return !isPlaying() && !isCausal() && !isPassing(); }

	virtual void addTimeline(Timeline* _p) = 0;
	virtual QWidget* addGLWidget(QGLWidgetProxy* _v, QWidget* _p = nullptr) = 0;
	virtual void addDockWidget(Qt::DockWidgetArea _a, QDockWidget* _d) = 0;
	virtual void info(QString const& _info, QString const& _color = "gray") = 0;

	inline void zoomTimeline(int _xFocus, double _factor) { auto pivot = timeOf(_xFocus); m_timelineOffset = pivot - (m_pixelDuration *= _factor) * _xFocus; emit durationChanged(); emit offsetChanged(); }

	static NotedFace* get() { assert(s_this); return s_this; }
	static AudioManFace* audio() { return get()->m_audioMan; }
	static DataMan* data() { return get()->m_dataMan; }
	static GraphManFace* graphs() { return get()->m_graphMan; }
	static ComputeManFace* compute() { return get()->m_computeMan; }
	static LibraryManFace* libs() { return get()->m_libraryMan; }

public slots:
	virtual void setCursor(qint64 _c, bool _warp = false) = 0;
	inline void setTimelineOffset(qint64 _o) { if (m_timelineOffset != _o) { m_timelineOffset = _o; emit offsetChanged(); } }
	inline void setPixelDuration(qint64 _d) { if (m_pixelDuration != _d) { m_pixelDuration = _d; emit durationChanged(); } }

	virtual void updateWindowTitle() = 0;

signals:
	void offsetChanged();
	void durationChanged();
	void eventsChanged();
	void cursorChanged();

protected:
	virtual void timelineDead(Timeline* _tl) = 0;

	AudioManFace* m_audioMan = nullptr;
	DataMan* m_dataMan = nullptr;
	GraphManFace* m_graphMan = nullptr;
	ComputeManFace* m_computeMan = nullptr;
	LibraryManFace* m_libraryMan = nullptr;

	bool m_zeroPhase = false;
	bool m_floatFFT = true;
	std::vector<float> m_windowFunction;

	lb::Time m_fineCursor = 0;
	lb::Time m_timelineOffset = 0;
	lb::Time m_pixelDuration = 1;

	static NotedFace* s_this;
};

static const QVector<int16_t> DummyQVectorInt16;
static const std::vector<float> DummyVectorFloat;

class DummyNoted: public NotedFace
{
public:
	DummyNoted(QWidget* _p = nullptr): NotedFace(_p) {}
	virtual ~DummyNoted() {}

	virtual bool carryOn(int) { return false; }

	virtual int activeWidth() const { return 0; }
	virtual QGLWidget* glMaster() const { return nullptr; }
	virtual lb::Time earliestVisible() const { return 0; }
	virtual lb::Time pixelDuration() const { return 1; }
	virtual lb::Time cursor() const { return 0; }
	virtual int causalCursorIndex() const { return -1; }

	virtual void info(QString const&, QString const& = "gray") {}

	virtual lb::foreign_vector<float const> waveWindow(int) const { return lb::foreign_vector<float const>(); }
	virtual bool waveBlock(lb::Time, lb::Time, lb::foreign_vector<float>, bool) const { return false; }

	virtual lb::foreign_vector<float const> multiSpectrum(int, int) const { return lb::foreign_vector<float const>(); }
	virtual lb::foreign_vector<float const> magSpectrum(int, int) const { return lb::foreign_vector<float const>(); }
	virtual lb::foreign_vector<float const> deltaPhaseSpectrum(int, int) const { return lb::foreign_vector<float const>(); }
	virtual lb::foreign_vector<float const> phaseSpectrum(int, int) const { return lb::foreign_vector<float const>(); }

	virtual QList<EventsStore*> eventsStores() const { return QList<EventsStore*>(); }
	virtual lb::EventCompiler newEventCompiler(QString const&) { return lb::EventCompiler(); }
	virtual lb::EventCompiler findEventCompiler(QString const&) { return lb::EventCompiler(); }
	virtual QString getEventCompilerName(lb::EventCompilerImpl*) { return ""; }

	virtual bool isPlaying() const { return false; }
	virtual bool isPassing() const { return false; }
	virtual bool isCausal() const { return false; }

	virtual std::shared_ptr<NotedPlugin> getPlugin(QString const&) { return nullptr; }
	virtual void timelineDead(Timeline*) {}
	virtual void addTimeline(Timeline*) {}
	virtual QWidget* addGLWidget(QGLWidgetProxy*) { return nullptr; }
	virtual void addDockWidget(Qt::DockWidgetArea, QDockWidget*) {}

	virtual void setCursor(qint64, bool) {}
	virtual void setTimelineOffset(qint64) {}
	virtual void setPixelDuration(qint64) {}

	virtual void updateWindowTitle() {}
};
