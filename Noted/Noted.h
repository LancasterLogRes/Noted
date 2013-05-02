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

#include <memory>
#include <set>
#include <vector>

#include <Common/FFT.h>
#include <Audio/Capture.h>
#include <Audio/Playback.h>
#include <EventCompiler/EventCompiler.h>
#include <NotedPlugin/NotedPlugin.h>
#include <NotedPlugin/Timeline.h>
#include <NotedPlugin/CausalAnalysis.h>
#include <NotedPlugin/AcausalAnalysis.h>
#include <NotedPlugin/Library.h>

#include <QLibrary>
#include <QFileSystemWatcher>
#include <QFile>
#include <QMap>
#include <QSet>
#include <QString>
#include <QMutex>
#include <QBuffer>
#include <QSlider>
#include <QGraphicsScene>

#include "NotedBase.h"

namespace Ui { class Noted; }

class QTemporaryFile;
class QTreeWidgetItem;
class QComboBox;
class WorkerThread;
class PrerenderedTimeline;
class EventsView;
class EventsEditor;
class CompileEvents;
class CollateEvents;

bool eventVisible(QVariant const& _v, Lightbox::StreamEvent const& _e);

struct RealLibrary: public Library
{
	RealLibrary(QString const& _f): Library(_f) {}
	QTreeWidgetItem* item;
	void unload();
	bool isEnabled() const;
};

typedef std::shared_ptr<RealLibrary> RealLibraryPtr;

class Noted: public NotedBase
{
	Q_OBJECT

	friend class CompileEvents;
	friend class CollateEvents;
	friend class ResampleWaveAc;
	friend class SpectraAc;
	friend class FinishUpAc;
	friend class Cursor;

public:
	explicit Noted(QWidget* parent = nullptr);
	~Noted();

	virtual int activeWidth() const;
	virtual QGLWidget* glMaster() const;
	virtual bool isPlaying() const { return !!m_playback; }
	virtual bool isCausal() const { return m_isCausal; }
	virtual bool isPassing() const { return m_isPassing; }
	virtual int causalCursorIndex() const { return m_causalCursorIndex; }

	virtual AcausalAnalysisPtr spectraAcAnalysis() const { return m_spectraAcAnalysis; }
	virtual CausalAnalysisPtr compileEventsAnalysis() const { return m_compileEventsAnalysis; }
	virtual CausalAnalysisPtr collateEventsAnalysis() const { return m_collateEventsAnalysis; }
	virtual AcausalAnalysisPtrs ripeAcausalAnalysis(AcausalAnalysisPtr const&);
	virtual CausalAnalysisPtrs ripeCausalAnalysis(CausalAnalysisPtr const&);
	virtual void noteLastValidIs(AcausalAnalysisPtr const& _a = nullptr);

	virtual QWidget* addGLWidget(QGLWidgetProxy* _v, QWidget* _p = nullptr);
	virtual void addTimeline(Timeline* _tl);
	virtual void addDockWidget(Qt::DockWidgetArea _a, QDockWidget* _d);

	virtual QList<EventsStore*> eventsStores() const;
	virtual std::vector<float> graphEvents(float _temperature) const;
	virtual Lightbox::StreamEvent eventOf(Lightbox::EventType _et, float _temperature, Lightbox::Time _t = Lightbox::UndefinedTime) const;
	virtual Lightbox::EventCompiler newEventCompiler(QString const& _name);
	virtual Lightbox::StreamEvents initEventsOf(Lightbox::EventType _et, float _temperature = std::numeric_limits<float>::infinity()) const;

	using QWidget::event;

	void updateGraphs(std::vector<std::shared_ptr<Lightbox::AuxGraphsSpec> > const& _specs);

	Lightbox::foreign_vector<float const> cursorWaveWindow() const;
	Lightbox::foreign_vector<float const> cursorMagSpectrum() const;
	Lightbox::foreign_vector<float const> cursorPhaseSpectrum() const;

public slots:
	void suspendWork();
	void resumeWork(bool _force = false);

	virtual void info(QString const& _info, QString const& _color = "gray");
	void info(QString const& _info, int _id);
	virtual void updateWindowTitle();

	virtual void addLibrary(QString const& _name, bool _isEnabled = true);
	virtual void reloadLibrary(QString const& _name);
	virtual void onLibraryChange(QString const& _name);
	virtual std::shared_ptr<NotedPlugin> getPlugin(QString const& _mangledName);

	virtual void setCursor(qint64 _c, bool _warp = false);

private slots:
	void on_actOpen_activated();
	void on_actQuit_activated();
	void on_actPlay_changed();
	void on_actPlayCausal_changed();
	void on_actPassthrough_changed();
	void on_actPanic_activated();
	void on_actFollow_changed();
	void on_actZoomIn_activated();
	void on_actZoomOut_activated();
	void on_actPanBack_activated();
	void on_actPanForward_activated();
	void on_actViewAll_activated() { normalizeView(); }
	void on_actRedoEvents_activated() { noteEventCompilersChanged(); }
	void on_actNewEvents_activated();
	void on_actNewEventsFrom_activated();
	void on_actOpenEvents_activated();
	void on_actAbout_activated();
	void on_clearInfo_clicked();

	void on_windowSizeSlider_valueChanged(int = 0);
	void on_hopSlider_valueChanged(int = 0);
	void on_sampleRate_currentIndexChanged(int = 0);
	void on_windowFunction_currentIndexChanged(int = 0) { noteLastValidIs(nullptr); }
	void on_zeroPhase_toggled(bool = false) { noteLastValidIs(nullptr); }
	void on_floatFFT_toggled(bool = false) { noteLastValidIs(nullptr); }
	void on_normalize_toggled(bool = false) { noteLastValidIs(nullptr); }
	void on_addEventsView_clicked();
	void on_addLibrary_clicked();
	void on_killLibrary_clicked();
	void on_refreshAudioDevices_clicked() { updateAudioDevices(); }
	void on_loadedLibraries_itemClicked(QTreeWidgetItem* _it, int);

	void on_actReadSettings_activated();
	void on_actWriteSettings_activated();

	void onDataViewDockClosed();
	void updateEventStuff();
	void updateAudioDevices();

signals:
	void viewSizesChanged();

private:
	void newDataView(QString const& _n);

	QList<EventsView*> eventsViews() const;

	void changeEvent(QEvent *e);
	void timerEvent(QTimerEvent*);
	void closeEvent(QCloseEvent*);
	void readSettings();
	void writeSettings();
	void readBaseSettings(QSettings&);
	void writeBaseSettings(QSettings&);
	virtual bool eventFilter(QObject*, QEvent*);

	void reloadDirties();

	virtual bool carryOn(int _progress);
	void updateParameters();
	bool serviceAudio();
	bool work();
	void rejigAudio();
	void setAudio(QString const& _filename);

	void initializeCausal(CausalAnalysisPtr const& _lastComplete);
	void finalizeCausal();
	void updateCausal(int _from, int _count);

	void normalizeView() { setTimelineOffset(duration() * -0.025); setPixelDuration(duration() / .95 / activeWidth()); }

	// Just for Timeline class.
	virtual void timelineDead(Timeline* _tl);

	virtual void ensureRegistered(Prerendered* _p);
	virtual void ensureUnregistered(Prerendered* _p);

	Ui::Noted* ui;

	QSet<Timeline*> m_timelines;
	mutable QMutex x_timelines;

	QSet<Prerendered*> m_prerendereds;
	mutable QMutex x_prerendereds;

	// Old working stuff...
	WorkerThread* m_workerThread;
	int m_suspends;

	bool m_cursorDirty;

	// Audio hardware i/o
	WorkerThread* m_audioThread;
	std::shared_ptr<Audio::Playback> m_playback;
	std::shared_ptr<Audio::Capture> m_capture;

	// Playback...
	Lightbox::Time m_fineCursorWas;
	Lightbox::Time m_nextResample;
	void* m_resampler;
	bool m_isCausal;
	bool m_isPassing;

	// Causal playback...
	int m_causalCursorIndex;

	// Passthrough...
	std::shared_ptr<Lightbox::FFTW> m_fftw;
	std::vector<float> m_currentWave;
	std::vector<float> m_currentMagSpectrum;
	std::vector<float> m_currentPhaseSpectrum;

	// Causal & passthrough...
	int m_lastIndex;
	unsigned m_sequenceIndex;
	CausalAnalysisPtrs m_causalQueue;

	// Extensions...
	void load(RealLibraryPtr const& _dl);
	void unload(RealLibraryPtr const& _dl);
	QMap<QString, RealLibraryPtr> m_libraries;
	QSet<QString> m_dirtyLibraries;
	QFileSystemWatcher m_libraryWatcher;

	// Events...
	void clearEventsCache();	// Call when eventsStores() or any of them have changed.
	mutable bool d_initEvents;
	mutable Lightbox::StreamEvents m_initEvents;

	// Analysis (to be working in general)...
	QMutex x_analysis;
	std::set<AcausalAnalysisPtr> m_toBeAnalyzed;	// TODO: Needs a lock.
	bool m_workFinished;
	AcausalAnalysisPtr m_resampleWaveAcAnalysis;
	AcausalAnalysisPtr m_spectraAcAnalysis;
	AcausalAnalysisPtr m_finishUpAcAnalysis;
	CausalAnalysisPtr m_compileEventsAnalysis;
	CausalAnalysisPtr m_collateEventsAnalysis;
	int m_eventsViewsDone;
	std::map<float, std::vector<float> > m_collatedGraphEvents;

	// Information output...
	QMutex x_infos;
	QString m_info;
	QString m_infos;

	// Master GL context...
	QGLWidget* m_glMaster;

	bool m_constructed;
};
