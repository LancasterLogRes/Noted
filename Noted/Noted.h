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

#include <Audio/Capture.h>
#include <Audio/Playback.h>
#include <EventCompiler/EventCompiler.h>
#include <NotedPlugin/NotedPlugin.h>
#include <NotedPlugin/Timeline.h>
#include <NotedPlugin/CausalAnalysis.h>
#include <NotedPlugin/AcausalAnalysis.h>

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

class WorkerThread;
class PrerenderedTimeline;
class EventsView;
class EventsEditor;
class CompileEvents;
class CollateEvents;
class Cursor;
class QComboBox;

bool eventVisible(QVariant const& _v, Lightbox::StreamEvent const& _e);

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
	virtual bool isPlaying() const { return !!m_playback; }
	virtual void info(QString const& _info, char const* _color = "gray");
	void info(QString const& _info, int _id);

	virtual AcausalAnalysisPtr spectraAcAnalysis() const { return m_spectraAcAnalysis; }
	virtual CausalAnalysisPtr compileEventsAnalysis() const { return m_compileEventsAnalysis; }
	virtual CausalAnalysisPtr collateEventsAnalysis() const { return m_collateEventsAnalysis; }
	virtual AcausalAnalysisPtrs ripeAcausalAnalysis(AcausalAnalysisPtr const&);
	virtual void noteLastValidIs(AcausalAnalysisPtr const& _a = nullptr);

	virtual QWidget* addGLWidget(QGLWidgetProxy* _v, QWidget* _p = nullptr);
	virtual void addTimeline(Timeline* _tl);

	virtual QList<EventsStore*> eventsStores() const;
	virtual std::vector<float> graphEvents(float _nature) const;
	virtual Lightbox::StreamEvent eventOf(Lightbox::EventType _et, float _nature, Lightbox::Time _t = Lightbox::UndefinedTime) const;
	virtual Lightbox::EventCompiler newEventCompiler(QString const& _name);
	virtual Lightbox::StreamEvents initEventsOf(Lightbox::EventType _et, float _nature = std::numeric_limits<float>::infinity()) const;

	void suspendWork();
	void resumeWork();

	using QWidget::event;

	void updateGraphs(std::vector<std::shared_ptr<Lightbox::AuxGraphsSpec> > const& _specs);

public slots:
	virtual void updateWindowTitle();

	virtual void addLibrary(QString const& _name);
	virtual void reloadLibrary(QString const& _name);
	virtual void onLibraryChange(QString const& _name);

	virtual void setCursor(qint64 _c);
	virtual void setTimelineOffset(qint64 _o) { if (m_timelineOffset != _o) { m_timelineOffset = _o; emit offsetChanged(); } }
	virtual void setPixelDuration(qint64 _d) { if (m_pixelDuration != _d) { m_pixelDuration = _d; emit durationChanged(); } }

private slots:
	void on_actOpen_activated();
	void on_actQuit_activated();
	void on_actPlay_changed();
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
	void on_normalize_toggled(bool = false) { noteLastValidIs(nullptr); }
	void on_addEventsView_clicked();
	void on_addLibrary_clicked();
	void on_killLibrary_clicked();
	void on_refreshAudioDevices_clicked() { updateAudioDevices(); }

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
	void moveEvent(QMoveEvent*);
	void readSettings();
	void writeSettings();
	void readBaseSettings(QSettings&);
	void writeBaseSettings(QSettings&);
	virtual bool eventFilter(QObject*, QEvent*);

	void reloadDirties();

	std::pair<QRect, QRect> cursorGeoOffset(int _id) const;
	bool cursorEvent(QEvent* _e, int _id);
	QRect cursorGeo(int _id) const { return cursorGeoOffset(_id).first; }
	void paintCursor(QPainter& _p, int _id) const;

	virtual bool carryOn(int _progress);
	void updateParameters();
	bool serviceAudio();
	bool work();
	void rejigAudio();
	void setAudio(QString const& _filename);

	void normalizeView() { setTimelineOffset(duration() * -0.025); setPixelDuration(duration() / .95 / activeWidth()); }

	// Just for Timeline class.
	virtual void timelineDead(Timeline* _tl);

	Ui::Noted* ui;

	QSet<Timeline*> m_timelines;
	mutable QMutex x_timelines;

	// Old working stuff...
	WorkerThread* m_workerThread;

	bool m_cursorDirty;

	// Playback...
	WorkerThread* m_playbackThread;
	std::shared_ptr<Audio::Playback> m_playback;
	Lightbox::Time m_fineCursorWas;
	Lightbox::Time m_nextResample;
	void* m_resampler;

	// Extensions...
	struct Library
	{
		Library(QString const& _name): name(_name) {}

		QString name;
		QLibrary l;

		// One of (p, cf, auxFace) is valid.
		std::shared_ptr<NotedPlugin> p;
		Lightbox::EventCompilerFactories cf;
		std::shared_ptr<AuxLibraryFace> auxFace;
		std::weak_ptr<NotedPlugin> aux;

		void unload();
	};
	typedef std::shared_ptr<Library> LibraryPtr;
	void load(LibraryPtr const& _dl);
	void unload(LibraryPtr const& _dl);
	QMap<QString, LibraryPtr> m_libraries;
	QSet<QString> m_dirtyLibraries;
	QFileSystemWatcher m_libraryWatcher;

	// Events...
	void clearEventsCache();	// Call when eventsStores() or any of them have changed.
	mutable bool d_initEvents;
	mutable Lightbox::StreamEvents m_initEvents;

	// Analysis (to be working in general)...
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
};
