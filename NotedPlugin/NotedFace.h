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
#include "QGLWidgetProxy.h"
#include "CausalAnalysis.h"
#include "AcausalAnalysis.h"

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

typedef uint32_t SimpleHash;

class IncomingAudio: public QObject
{
	Q_OBJECT

public:
	SimpleHash key() const { return m_key; }

	QString const& filename() const { return m_filename; }
	inline unsigned rate() const { return m_rate; }
	inline unsigned hopSamples() const { return m_hopSamples; }
	inline unsigned samples() const { return m_samples; }

	void setRate(unsigned _s) { m_rate = _s; rejig(); emit changed(); }
	void setHopSamples(unsigned _s) { m_hopSamples = _s; rejig(); emit changed(); }
	void setSamples(unsigned _s) { m_samples = _s; rejig(); emit changed(); }
	void setFilename(QString const& _fn) { m_filename = _fn; rejig(); emit changed(); }

	inline unsigned hops() const { return samples() ? samples() / hopSamples() : 0; }
	inline Lightbox::Time duration() const { return Lightbox::toBase(samples(), rate()); }
	inline Lightbox::Time hop() const { return Lightbox::toBase(hopSamples(), rate()); }
	inline unsigned index(Lightbox::Time _t) const { return (_t < 0) ? 0 : std::min<unsigned>(_t / hop(), samples() / hopSamples()); }

signals:
	void changed();

private:
	void rejig() { m_key = qHash(m_filename) ^ qHash(m_hopSamples) ^ qHash(m_rate); }

	QString m_filename;
	SimpleHash m_key = 0;
	unsigned m_rate = 1;
	unsigned m_hopSamples = 2;
	unsigned m_samples = 0;
};

class GraphMan: public QObject
{
	Q_OBJECT

public:
	GraphMan() {}

	// TODO: add a model

	// TODO: rename CompilerGraph -> GraphSpec, use this for *all* graphs (wave, spectrum, &c.).
	void registerGraph(QString _url, Lightbox::CompilerGraph const* _g) { { QReadLocker l(&x_graphs); m_graphs.insert(_url, _g); } emit graphAdded(_url); }
	void unregisterGraph(QString _url) { { QWriteLocker l(&x_graphs); m_graphs.remove(_url); } emit graphRemoved(_url); }

	QStringList graphs() const { QReadLocker l(&x_graphs); return m_graphs.keys(); }
	Lightbox::CompilerGraph const* lockGraph(QString const& _url) const { x_graphs.lockForRead(); if (m_graphs.contains(_url)) return m_graphs[_url]; x_graphs.unlock(); return nullptr; }
	void unlockGraph(Lightbox::CompilerGraph const* _graph) const { if (_graph) x_graphs.unlock(); }

signals:
	void graphAdded(QString _url);
	void graphRemoved(QString _url);

private:
	Q_PROPERTY(QStringList graphs READ graphs())

	mutable QReadWriteLock x_graphs;
	QHash<QString, Lightbox::CompilerGraph const*> m_graphs;
};

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
	virtual int causalCursorIndex() const = 0;	///< -1 when !isCausal()

	inline Lightbox::Time earliestVisible() const { return m_timelineOffset; }
	inline Lightbox::Time pixelDuration() const { return m_pixelDuration; }
	inline Lightbox::Time cursor() const { return m_fineCursor / hop() * hop(); }
	inline Lightbox::Time latestVisible() const { return earliestVisible() + visibleDuration(); }
	inline Lightbox::Time visibleDuration() const { return activeWidth() * pixelDuration(); }

	inline unsigned hopSamples() const { return m_incomingAudio->hopSamples(); }
	inline unsigned windowSizeSamples() const { return m_windowFunction.size(); }
	inline unsigned rate() const { return m_incomingAudio->rate(); }
	inline unsigned spectrumSize() const { return m_windowFunction.size() / 2 + 1; }
	inline std::vector<float> const& windowFunction() const { return m_windowFunction; }
	inline bool isZeroPhase() const { return m_zeroPhase; }
	inline bool isFloatFFT() const { return m_floatFFT; }
	inline unsigned samples() const { return m_incomingAudio->samples(); }

	inline Lightbox::Time hop() const { return Lightbox::toBase(hopSamples(), rate()); }
	inline Lightbox::Time windowSize() const { return Lightbox::toBase(windowSizeSamples(), rate()); }
	inline int widthOf(Lightbox::Time _t) const { return samples() ? (_t + pixelDuration() / 2) / pixelDuration() : 0; }
	inline Lightbox::Time durationOf(int _screenWidth) const { return _screenWidth * pixelDuration(); }
	inline int positionOf(Lightbox::Time _t) const { return widthOf(_t - earliestVisible()); }
	inline Lightbox::Time timeOf(int _x) const { return durationOf(_x) + earliestVisible(); }
	inline unsigned cursorIndex() const { return windowIndex(cursor()); }
	inline unsigned windowIndex(Lightbox::Time _t) const { return (_t < 0) ? 0 : std::min<unsigned>(_t / hop(), (samples() - windowSizeSamples()) / hopSamples()); }
	inline unsigned hops() const { return samples() ? samples() / hopSamples() : 0; }
	inline Lightbox::Time duration() const { return Lightbox::toBase(samples(), rate()); }

	virtual Lightbox::foreign_vector<float const> waveWindow(int _window) const = 0;
	// TODO: extra argument/double-size vector for min/max range of each sample in o_toFill.
	virtual bool waveBlock(Lightbox::Time _from, Lightbox::Time _duration, Lightbox::foreign_vector<float> o_toFill, bool _forceSamples = false) const = 0;
	virtual Lightbox::foreign_vector<float const> multiSpectrum(int _i, int _n) const = 0;
	virtual Lightbox::foreign_vector<float const> magSpectrum(int _i, int _n) const = 0;
	virtual Lightbox::foreign_vector<float const> phaseSpectrum(int _i, int _n) const = 0;
	virtual Lightbox::foreign_vector<float const> deltaPhaseSpectrum(int _i, int _n) const = 0;

	virtual QList<EventsStore*> eventsStores() const = 0;
	virtual std::vector<float> graphEvents(float _temperature) const = 0;
	virtual Lightbox::StreamEvent eventOf(Lightbox::EventType _et, float _temperature = std::numeric_limits<float>::infinity(), Lightbox::Time _t = Lightbox::UndefinedTime) const = 0;
	virtual Lightbox::StreamEvents initEventsOf(Lightbox::EventType _et, float _temperature = std::numeric_limits<float>::infinity()) const = 0;
	virtual Lightbox::EventCompiler newEventCompiler(QString const& _name) = 0;
	virtual Lightbox::EventCompiler findEventCompiler(QString const& _name) = 0;
	virtual QString getEventCompilerName(Lightbox::EventCompilerImpl* _ec) = 0;

	virtual void noteLastValidIs(AcausalAnalysisPtr const& _a = nullptr) = 0;
	virtual AcausalAnalysisPtr spectraAcAnalysis() const = 0;
	virtual CausalAnalysisPtr compileEventsAnalysis() const = 0;
	virtual CausalAnalysisPtr collateEventsAnalysis() const = 0;
	virtual AcausalAnalysisPtrs ripeAcausalAnalysis(AcausalAnalysisPtr const&) = 0;
	virtual CausalAnalysisPtrs ripeCausalAnalysis(CausalAnalysisPtr const&) = 0;

	virtual bool isPlaying() const = 0;
	virtual bool isCausal() const = 0;
	virtual bool isPassing() const = 0;
	inline bool isImmediate() const { return isCausal() || isPassing(); }
	inline bool isQuiet() const { return !isPlaying() && !isCausal() && !isPassing(); }

	virtual void setupPrerendered(Prerendered*) {}
	virtual void ensureRegistered(Prerendered*) {}
	virtual void ensureUnregistered(Prerendered*) {}
	virtual void addTimeline(Timeline* _p) = 0;
	virtual QWidget* addGLWidget(QGLWidgetProxy* _v, QWidget* _p = nullptr) = 0;
	virtual void addDockWidget(Qt::DockWidgetArea _a, QDockWidget* _d) = 0;
	virtual void info(QString const& _info, QString const& _color = "gray") = 0;
	virtual std::shared_ptr<NotedPlugin> getPlugin(QString const& _mangledName) = 0;

	inline void zoomTimeline(int _xFocus, double _factor) { auto pivot = timeOf(_xFocus); m_timelineOffset = pivot - (m_pixelDuration *= _factor) * _xFocus; emit durationChanged(); emit offsetChanged(); }

	static NotedFace* get() { assert(s_this); return s_this; }
	static IncomingAudio* audio() { return get()->m_incomingAudio; }
	static DataMan* data() { return get()->m_dataMan; }
	static GraphMan* graphs() { return get()->m_graphMan; }

public slots:
	virtual void setCursor(qint64 _c, bool _warp = false) = 0;
	inline void setTimelineOffset(qint64 _o) { if (m_timelineOffset != _o) { m_timelineOffset = _o; emit offsetChanged(); } }
	inline void setPixelDuration(qint64 _d) { if (m_pixelDuration != _d) { m_pixelDuration = _d; emit durationChanged(); } }

	virtual void updateWindowTitle() = 0;

	inline void noteEventCompilersChanged() { noteLastValidIs(spectraAcAnalysis()); }
	inline void notePluginDataChanged() { noteLastValidIs(collateEventsAnalysis()); }

signals:
	void offsetChanged();
	void durationChanged();
	void analysisFinished();
	void eventsChanged();
	void cursorChanged();

protected:
	virtual void timelineDead(Timeline* _tl) = 0;

	IncomingAudio* m_incomingAudio = new IncomingAudio;
	DataMan* m_dataMan = new DataMan;
	GraphMan* m_graphMan = new GraphMan;

	bool m_zeroPhase;
	bool m_floatFFT;
	std::vector<float> m_windowFunction;

	Lightbox::Time m_fineCursor;
	Lightbox::Time m_timelineOffset;
	Lightbox::Time m_pixelDuration;

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
	virtual Lightbox::Time earliestVisible() const { return 0; }
	virtual Lightbox::Time pixelDuration() const { return 1; }
	virtual Lightbox::Time cursor() const { return 0; }
	virtual int causalCursorIndex() const { return -1; }

	virtual void info(QString const&, QString const& = "gray") {}

	virtual Lightbox::foreign_vector<float const> waveWindow(int) const { return Lightbox::foreign_vector<float const>(); }
	virtual bool waveBlock(Lightbox::Time, Lightbox::Time, Lightbox::foreign_vector<float>, bool) const { return false; }

	virtual Lightbox::foreign_vector<float const> multiSpectrum(int, int) const { return Lightbox::foreign_vector<float const>(); }
	virtual Lightbox::foreign_vector<float const> magSpectrum(int, int) const { return Lightbox::foreign_vector<float const>(); }
	virtual Lightbox::foreign_vector<float const> deltaPhaseSpectrum(int, int) const { return Lightbox::foreign_vector<float const>(); }
	virtual Lightbox::foreign_vector<float const> phaseSpectrum(int, int) const { return Lightbox::foreign_vector<float const>(); }

	virtual std::vector<float> graphEvents(float) const { return std::vector<float>(); }
	virtual Lightbox::StreamEvent eventOf(Lightbox::EventType, float = std::numeric_limits<float>::infinity(), Lightbox::Time = Lightbox::UndefinedTime) const { return Lightbox::StreamEvent(); }
	virtual Lightbox::StreamEvents initEventsOf(Lightbox::EventType, float = std::numeric_limits<float>::infinity()) const { return Lightbox::StreamEvents(); }
	virtual QList<EventsStore*> eventsStores() const { return QList<EventsStore*>(); }
	virtual Lightbox::EventCompiler newEventCompiler(QString const&) { return Lightbox::EventCompiler(); }
	virtual Lightbox::EventCompiler findEventCompiler(QString const&) { return Lightbox::EventCompiler(); }
	virtual QString getEventCompilerName(Lightbox::EventCompilerImpl*) { return ""; }

	virtual void noteLastValidIs(AcausalAnalysisPtr const& = nullptr) {}
	virtual AcausalAnalysisPtr spectraAcAnalysis() const { return nullptr; }
	virtual CausalAnalysisPtr compileEventsAnalysis() const { return nullptr; }
	virtual CausalAnalysisPtr collateEventsAnalysis() const { return nullptr; }
	virtual AcausalAnalysisPtrs ripeAcausalAnalysis(AcausalAnalysisPtr const&) { return AcausalAnalysisPtrs(); }
	virtual CausalAnalysisPtrs ripeCausalAnalysis(CausalAnalysisPtr const&) { return CausalAnalysisPtrs(); }

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
