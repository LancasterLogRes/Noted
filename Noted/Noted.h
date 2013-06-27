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
#include <NotedPlugin/NotedFace.h>
#include <NotedPlugin/NotedPlugin.h>
#include <NotedPlugin/Timeline.h>
#include <NotedPlugin/CausalAnalysis.h>
#include <NotedPlugin/AcausalAnalysis.h>
#include <NotedPlugin/Library.h>

#include <QQuickItem>
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
#include "GraphView.h"
#include "AudioMan.h"
#include "ComputeMan.h"
#include "LibraryMan.h"
#include "ViewMan.h"
#include "EventsMan.h"
#include "GraphMan.h"

namespace Ui { class Noted; }

class QSGGeometry;
class QTemporaryFile;
class QTreeWidgetItem;
class QComboBox;
class QQuickView;
class WorkerThread;
class PrerenderedTimeline;
class EventCompilerView;
class EventsEditor;
class CompileEvents;
class CollateEvents;
class TimelinesItem;

class Noted: public NotedFace
{
	Q_OBJECT

	friend class AudioMan; // TODO! Remove when there's no window call in AudioMan.

public:
	explicit Noted(QWidget* parent = nullptr);
	virtual ~Noted();

	static Noted* get() { return static_cast<Noted*>(NotedFace::get()); }
	static ComputeMan* compute() { return static_cast<ComputeMan*>(get()->m_computeMan); }
	static AudioMan* audio() { return static_cast<AudioMan*>(get()->m_audioMan); }
	static LibraryMan* libs() { return static_cast<LibraryMan*>(get()->m_libraryMan); }
	static ViewMan* view() { return static_cast<ViewMan*>(get()->m_viewMan); }
	static GraphMan* graphs() { return static_cast<GraphMan*>(get()->m_graphMan); }
	static TimelinesItem* timelines() { return get()->m_timelinesItem; }
	static QQuickView* quickView() { return get()->m_view; }

	virtual QGLWidget* glMaster() const;
	virtual void addLegacyTimeline(QWidget* _w);
	virtual QWidget* addGLView(QGLWidgetProxy* _v);
	virtual void addDockWidget(Qt::DockWidgetArea _a, QDockWidget* _d);

public slots:
	virtual void info(QString const& _info, QString const& _color = "gray");
	void info(QString const& _info, int _id);
	virtual void updateWindowTitle();

	void readBaseSettings(QSettings&);
	void writeBaseSettings(QSettings&);

private slots:
	void on_actOpen_triggered();
	void on_actQuit_triggered();
	void on_actPlay_changed();
	void on_actPlayCausal_changed();
	void on_actPassthrough_changed();
	void on_actPanic_triggered();
	void on_actFollow_changed();
	void on_actTrack_changed();
	void on_actZoomIn_triggered();
	void on_actZoomOut_triggered();
	void on_actPanBack_triggered();
	void on_actPanForward_triggered();
	void on_actViewAll_triggered() { view()->normalize(); }
	void on_actNewEvents_triggered();
	void on_actNewEventsFrom_triggered();
	void on_actOpenEvents_triggered();
	void on_actAbout_triggered();
	void on_actInvalidate_triggered();
	void on_actRecompute_triggered();
	void on_actRecomputeAnalyses_triggered();
	void on_actPrune_triggered();
	void on_clearInfo_clicked();

	void on_force16Bit_toggled(bool);
	void on_playDevice_currentIndexChanged(int);
	void on_playRate_currentIndexChanged(int);
	void on_playChunkSamples_valueChanged(int);
	void on_playChunks_valueChanged(int);

	void on_captureDevice_currentIndexChanged(int);
	void on_captureChunks_valueChanged(int);

	void on_hopSlider_valueChanged(int _i);
	void on_hop_valueChanged(int _i);
	void on_sampleRate_currentIndexChanged(int _i);

	void on_addEventsView_clicked();

	void on_addLibrary_clicked();
	void on_killLibrary_clicked();

	void on_refreshAudioDevices_clicked() { updateAudioDevices(); }

	void on_actReadSettings_triggered();
	void on_actWriteSettings_triggered();

	void onEventCompilerFactoryAvailable(QString _name, unsigned _version);
	void onEventCompilerFactoryUnavailable(QString _name);

	void onDataChanging();
	void onDataLoaded();

	void onCursorChanged(lb::Time _cursor);
	void onPlaybackStatusChanged();

	void onWorkProgressed(QString _desc, int _percent);
	void onWorkFinished();

	// TODO: Move to AudioDevices: public QAbstractItemModel.
	void updateAudioDevices();

	void processNewInfo();

	void updateHopDisplay();

private:
	void changeEvent(QEvent *e);
	void closeEvent(QCloseEvent*);
	void showEvent(QShowEvent*);
	void timerEvent(QTimerEvent*) { processNewInfo(); }

	void readSettings();
	void writeSettings();

	Ui::Noted* ui;
	QQuickView* m_view;
	TimelinesItem* m_timelinesItem;

	// Information output...
	QMutex x_infos;
	QString m_info;
	QString m_infos;

	// Master GL context...
	QGLWidget* m_glMaster;
};
