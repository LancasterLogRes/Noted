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

#include <Common/Common.h>
#include <EventCompiler/EventCompiler.h>

#include "QGLWidgetProxy.h"
#include "CausalAnalysis.h"
#include "AcausalAnalysis.h"

LIGHTBOX_TEXTUAL_ENUM(AudioStage, Wave, Spectrum);

class Timeline;
class EventsStore;
class CausalAnalysis;

class NotedFace: public QMainWindow
{
	Q_OBJECT

	friend class Timeline;

public:
	NotedFace(QWidget* _p);
	virtual ~NotedFace();

	virtual bool carryOn(QString const& _msg, int _progress) = 0;

	virtual int activeWidth() const = 0;
	virtual Lightbox::Time timelineOffset() const = 0;
	virtual Lightbox::Time timelineDuration() const = 0;
	virtual Lightbox::Time cursor() const = 0;

	virtual unsigned hopSamples() const { return m_hopSamples; }
	virtual unsigned windowSizeSamples() const { return m_windowFunction.size(); }
	virtual unsigned rate() const { return m_rate; }
	virtual unsigned spectrumSize() const { return m_windowFunction.size() / 2 + 1; }
	virtual std::vector<float> const& windowFunction() const { return m_windowFunction; }
	virtual bool isZeroPhase() const { return m_zeroPhase; }
	virtual unsigned samples() const { return m_samples; }

	inline Lightbox::Time hop() const { return Lightbox::toBase(hopSamples(), rate()); }
	inline Lightbox::Time windowSize() const { return Lightbox::toBase(windowSizeSamples(), rate()); }
	inline int screenWidth(Lightbox::Time _t) const { return samples() ? (_t * activeWidth() + timelineDuration() / 2) / timelineDuration() : 0; }
	inline Lightbox::Time durationOf(int _screenWidth) const { return _screenWidth * timelineDuration() / activeWidth(); }
	inline int xOf(Lightbox::Time _t) const { return screenWidth(_t - timelineOffset()); }
	inline int cursorX() const { return xOf(cursor()); }
	inline int windowSizeW() const { return screenWidth(windowSize()); }
	inline int hopW() const { return screenWidth(hop()); }
	inline Lightbox::Time timeOf(int _x) const { return durationOf(_x) + timelineOffset(); }
	inline unsigned cursorIndex() const { return windowIndex(cursor()); }
	inline unsigned windowIndex(Lightbox::Time _t) const { return (_t < 0) ? 0 : std::min<unsigned>(_t / hop(), (samples() - windowSizeSamples()) / hopSamples()); }
	inline unsigned hops() const { return samples() ? samples() / hopSamples() : 0; }
	inline Lightbox::Time duration() const { return Lightbox::toBase(samples(), rate()); }

	virtual Lightbox::foreign_vector<float> waveWindow(int _window) const = 0;
	// TODO: extra argument/double-size vector for min/max range of each sample in o_toFill.
	virtual bool waveBlock(Lightbox::Time _from, Lightbox::Time _duration, Lightbox::foreign_vector<float> o_toFill, bool _forceBest = false) const = 0;
	virtual Lightbox::foreign_vector<float> magSpectrum(int _i, int _n, bool _force = false) const = 0;
	virtual Lightbox::foreign_vector<float> phaseSpectrum(int _i, int _n, bool _force = false) const = 0;
	virtual Lightbox::foreign_vector<float> deltaPhaseSpectrum(int _i, int _n, bool _force = false) const = 0;

	virtual QList<EventsStore*> eventsStores() const = 0;
	virtual std::vector<float> graphEvents(float _nature) const = 0;
	virtual Lightbox::StreamEvent eventOf(Lightbox::EventType _et, float _nature = std::numeric_limits<float>::infinity(), Lightbox::Time _t = Lightbox::UndefinedTime) const = 0;
	virtual Lightbox::StreamEvents initEventsOf(Lightbox::EventType _et, float _nature = std::numeric_limits<float>::infinity()) const = 0;
	virtual Lightbox::EventCompiler newEventCompiler(QString const& _name) = 0;

	virtual void noteLastValidIs(AcausalAnalysisPtr const& _a) = 0;
	virtual CausalAnalysisPtr compileEventsAnalysis() const = 0;
	virtual CausalAnalysisPtr collateEventsAnalysis() const = 0;
	virtual AcausalAnalysisPtrs ripeAcausalAnalysis(AcausalAnalysisPtr const&) = 0;

	inline void noteEventCompilersChanged() { noteLastValidIs(nullptr); }
	inline void notePluginDataChanged() { noteLastValidIs(collateEventsAnalysis()); }

	virtual bool isPlaying() const = 0;

	virtual void addTimeline(Timeline* _p) = 0;
	virtual QWidget* addGLWidget(QGLWidgetProxy* _v, QWidget* _p = nullptr) = 0;
	virtual void info(QString const& _info) = 0;

public slots:
	virtual void setCursor(qint64 _c) = 0;
	virtual void setOffset(qint64 _o) = 0;
	virtual void setDuration(qint64 _d) = 0;

	virtual void updateWindowTitle() = 0;

signals:
	void cursorChanged();
	void offsetChanged();
	void durationChanged();
	void analysisFinished();
	void eventsChanged();
	void analysisFinished();

protected:
	virtual void timelineDead(Timeline* _tl) = 0;

	unsigned m_rate;
	unsigned m_hopSamples;
	unsigned m_samples;
	bool m_zeroPhase;
	bool m_normalize;
	std::vector<float> m_windowFunction;
};

static const QVector<int16_t> DummyQVectorInt16;
static const std::vector<float> DummyVectorFloat;

class DummyNoted: public NotedFace
{
public:
	DummyNoted(QWidget* _p = nullptr): NotedFace(_p) {}
	virtual ~DummyNoted() {}

	virtual bool carryOn(QString const&, int) { return false; }

	virtual int activeWidth() const { return 0; }
	virtual Lightbox::Time timelineOffset() const { return 0; }
	virtual Lightbox::Time timelineDuration() const { return 0; }
	virtual Lightbox::Time cursor() const { return 0; }

	virtual void info(QString const&) {}

	virtual Lightbox::foreign_vector<float> waveWindow(int) const { return Lightbox::foreign_vector<float>(); }
	virtual bool waveBlock(Lightbox::Time, Lightbox::Time, Lightbox::foreign_vector<float>, bool = false) const { return false; }

	virtual Lightbox::foreign_vector<float> magSpectrum(int, int, bool = false) const { return Lightbox::foreign_vector<float>(); }
	virtual Lightbox::foreign_vector<float> deltaPhaseSpectrum(int, int, bool = false) const { return Lightbox::foreign_vector<float>(); }
	virtual Lightbox::foreign_vector<float> phaseSpectrum(int, int, bool = false) const { return Lightbox::foreign_vector<float>(); }

	virtual std::vector<float> graphEvents(float) const { return std::vector<float>(); }
	virtual Lightbox::StreamEvent eventOf(Lightbox::EventType, float = std::numeric_limits<float>::infinity(), Lightbox::Time = Lightbox::UndefinedTime) const { return Lightbox::StreamEvent(); }
	virtual Lightbox::StreamEvents initEventsOf(Lightbox::EventType, float = std::numeric_limits<float>::infinity()) const { return Lightbox::StreamEvents(); }
	virtual QList<EventsStore*> eventsStores() const { return QList<EventsStore*>(); }
	virtual Lightbox::EventCompiler newEventCompiler(QString const&) { return Lightbox::EventCompiler(); }

	virtual void noteLastValidIs(AcausalAnalysisPtr const&) {}
	virtual CausalAnalysisPtr compileEventsAnalysis() const { return nullptr; }
	virtual CausalAnalysisPtr collateEventsAnalysis() const { return nullptr; }
	virtual AcausalAnalysisPtrs ripeAcausalAnalysis(AcausalAnalysisPtr const&) { return AcausalAnalysisPtrs(); }

	virtual bool isPlaying() const { return false; }

	virtual void timelineDead(Timeline*) {}
	virtual void addTimeline(Timeline*) {}
	virtual QWidget* addGLWidget(QGLWidgetProxy*) { return nullptr; }

	virtual void setCursor(qint64) {}
	virtual void setOffset(qint64) {}
	virtual void setDuration(qint64) {}

	virtual void updateWindowTitle() {}
};
