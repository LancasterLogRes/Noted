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

#include <cstdlib>
#include <fstream>
#include <memory>
#include <deque>
#include <unordered_map>
#include <boost/functional/factory.hpp>
#include <functional>
#include <boost/variant.hpp>
#include <boost/foreach.hpp>
#include <libresample.h>
#include <QtGui>
#include <QtXml>
#include <QtOpenGL>
#include <QtQuick>
#include <Common/Common.h>
#include <EventsEditor/EventsEditor.h>
#include <EventsEditor/EventsEditScene.h>
#include <NotedPlugin/AuxLibraryFace.h>
#include <NotedPlugin/AcausalAnalysis.h>
#include <NotedPlugin/CausalAnalysis.h>

#include "GraphView.h"
#include "EventsView.h"
#include "WorkerThread.h"
#include "ProcessEventCompiler.h"
#include "PropertiesEditor.h"
#include "CompileEvents.h"
#include "CollateEvents.h"
#include "CompileEventsView.h"
#include "NotedGLWidget.h"
#include "TimelinesItem.h"
#include "Noted.h"
#include "ui_Noted.h"

using namespace std;
using namespace lb;

void ViewMan::normalize()
{
	if (Noted::audio()->duration() && activeWidth())
		setParameters(Noted::audio()->duration() * -0.025, Noted::audio()->duration() / .95 / activeWidth());
	else
		setParameters(0, FromMsecs<1>::value);
}

static QObject* getLibs(QQmlEngine*, QJSEngine*)
{
	return Noted::libs();
}
static QObject* getCompute(QQmlEngine*, QJSEngine*)
{
	return Noted::compute();
}
static QObject* getData(QQmlEngine*, QJSEngine*)
{
	return Noted::data();
}
static QObject* getGraphs(QQmlEngine*, QJSEngine*)
{
	return Noted::graphs();
}
static QObject* getAudio(QQmlEngine*, QJSEngine*)
{
	return Noted::audio();
}
static QObject* getView(QQmlEngine*, QJSEngine*)
{
	return Noted::view();
}

Noted::Noted(QWidget* _p):
	NotedBase					(_p),
	ui							(new Ui::Noted),
	x_timelines					(QMutex::Recursive),
	m_glMaster					(new QGLWidget)
{
	g_debugPost = [&](std::string const& _s, int _id){ simpleDebugOut(_s, _id); info(_s.c_str(), _id); };

	m_libraryMan = new LibraryMan;
	m_computeMan = new ComputeMan;
	m_dataMan = new DataMan;
	m_audioMan = new AudioMan;
	m_graphMan = new GraphManFace;
	m_viewMan = new ViewMan;

	ui->setupUi(this);
	ui->librariesView->setModel(m_libraryMan);
	setWindowIcon(QIcon(":/Noted.png"));

	qmlRegisterType<ChartItem>("com.llr", 1, 0, "Chart");
	qmlRegisterType<TimelinesItem>("com.llr", 1, 0, "Timelines");
	qmlRegisterType<XLabelsItem>("com.llr", 1, 0, "XLabels");
	qmlRegisterType<YLabelsItem>("com.llr", 1, 0, "YLabels");
	qmlRegisterType<YScaleItem>("com.llr", 1, 0, "YScale");

	qmlRegisterSingletonType<LibraryMan>("com.llr", 1, 0, "LibraryMan", getLibs);
	qmlRegisterSingletonType<ComputeMan>("com.llr", 1, 0, "ComputeMan", getCompute);
	qmlRegisterSingletonType<DataMan>("com.llr", 1, 0, "DataMan", getData);
	qmlRegisterSingletonType<GraphManFace>("com.llr", 1, 0, "GraphMan", getGraphs);
	qmlRegisterSingletonType<AudioMan>("com.llr", 1, 0, "AudioMan", getAudio);
	qmlRegisterSingletonType<ViewMan>("com.llr", 1, 0, "ViewMan", getView);

	m_view = new QQuickView(QUrl("qrc:/Noted.qml"));
	QWidget* w = QWidget::createWindowContainer(m_view);
	m_view->setResizeMode(QQuickView::SizeRootObjectToView);
	ui->fullDisplay->addWidget(w);
	m_view->create();

	m_timelinesItem = static_cast<TimelinesItem*>(m_view->rootObject());

	connect(audio(), SIGNAL(prepareForDataChange()), SLOT(onDataChanging()));
	connect(audio(), SIGNAL(dataLoaded()), SLOT(onDataLoaded()));
	connect(audio(), SIGNAL(stateChanged(bool,bool,bool)), SLOT(onPlaybackStatusChanged()));
	connect(audio(), SIGNAL(cursorChanged(lb::Time)), SLOT(onCursorChanged(lb::Time)));

	connect(compute(), SIGNAL(finished()), SLOT(onWorkFinished()));
	connect(compute(), SIGNAL(progressed(QString, int)), SLOT(onWorkProgressed(QString, int)));
	connect(compute(), SIGNAL(finished()), SLOT(updateEventStuff()));

	connect(libs(), SIGNAL(eventCompilerFactoryAvailable(QString, unsigned)), SLOT(onEventCompilerFactoryAvailable(QString, unsigned)));
	connect(libs(), SIGNAL(eventCompilerFactoryUnavailable(QString)), SLOT(onEventCompilerFactoryUnavailable(QString)));

	connect(m_timelinesItem, SIGNAL(widthChanged(int)), view(), SLOT(setWidth(int)));
/*	connect(view(), &ViewMan::parametersChanged, [=](lb::Time o, lb::Time d)
	{
		cnote << "View parameters changed to" << textualTime(o) << "x" << textualTime(d) << "(" << m_timelinesItem->property("offset").toDouble() << m_timelinesItem->property("pitch").toDouble() << ")";
	});*/

	for (auto i: findChildren<PrerenderedTimeline*>())
		i->hide();

	updateAudioDevices();

	ui->waveform->installEventFilter(this);
	ui->overview->installEventFilter(this);
	ui->dataDisplay->installEventFilter(this);

	m_timelines.insert(ui->waveform);
	m_timelines.insert(ui->spectra);

	{
		QLabel* l = new QLabel("No audio");
		l->setObjectName("alsa");
		l->setMinimumWidth(220);
		ui->statusBar->addPermanentWidget(l);
	}

	{
		QLabel* l = new QLabel;
		l->setObjectName("cursor");
		l->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
		l->setMinimumWidth(80);
		ui->statusBar->addPermanentWidget(l);
	}

	updateHopDisplay();

	readSettings();

	setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
	setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
	setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
	setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

	emit constructed();

	// After Noted's readSettings to avoid computing pointless default values.
	compute()->resumeWork();
}

Noted::~Noted()
{
	audio()->stop();

	g_debugPost = simpleDebugOut;

	qDebug() << "Disabling worker(s)...";
	compute()->suspendWork();
	qDebug() << "Disabled permenantly.";

	qDebug() << "Killing timelines...";
	while (m_timelines.size())
		delete *m_timelines.begin();
	qDebug() << "Killed.";

	delete m_libraryMan;

	for (auto i: findChildren<Prerendered*>())
		delete i;

	delete m_computeMan;
	delete m_graphMan;
	delete m_audioMan;
	delete m_dataMan;

	delete ui;
	delete m_glMaster;
}

void Noted::showEvent(QShowEvent*)
{
	// Needs initializing
	view()->setWidth(m_view->width());
}

void Noted::on_playDevice_currentIndexChanged(int _i)
{
	audio()->setPlayDevice(ui->playDevice->itemData(_i).toInt());
}

void Noted::on_force16Bit_toggled(bool _v)
{
	audio()->setPlayForce16Bit(_v);
}

void Noted::on_playRate_currentIndexChanged(int _i)
{
	int rate = -1;								// Default device rate.
	if (_i == ui->playRate->count() - 1)		// Working rate.
		rate = 0;
	else if (_i < ui->playRate->count() - 2)	// Specified rate.
		rate = ui->playRate->currentText().section(' ', 0, 0).toInt();
	audio()->setPlayRate(rate);
}

void Noted::on_playChunkSamples_valueChanged(int _i)
{
	audio()->setPlayChunkSamples(_i);
}

void Noted::on_playChunks_valueChanged(int _i)
{
	audio()->setPlayChunks((_i == 2) ? -1 : _i);
}

void Noted::on_captureDevice_currentIndexChanged(int _i)
{
	audio()->setCaptureDevice(ui->captureDevice->itemData(_i).toInt());
}

void Noted::on_captureChunks_valueChanged(int _i)
{
	audio()->setCaptureChunks((_i == 2) ? -1 : _i);
}

void Noted::onEventCompilerFactoryAvailable(QString _name, unsigned _version)
{
	auto li = new QListWidgetItem(_name + ":" + QString::number(_version));
	ui->eventCompilersList->addItem(li);
}

void Noted::onEventCompilerFactoryUnavailable(QString _name)
{
	delete ui->eventCompilersList->findItems(_name + ":", Qt::MatchStartsWith).front();
}

void Noted::onDataChanging()
{
	if (ui->actPlay->isChecked())
		ui->actPlay->setChecked(false);
	view()->setParameters(0, 1);
}

void Noted::onDataLoaded()
{
	if (view()->timelineOffset() == 0 && view()->pixelDuration() == 1)
		view()->normalize();
}

QGLWidget* Noted::glMaster() const
{
	return m_glMaster;
}

void Noted::on_actAbout_triggered()
{
	QMessageBox::about(this, "About Noted!", "<h1>Noted!</h1>Copyright (c)2011, 2012, 2013 Lancaster Logic Response Limited. This code is released under version 2 of the GNU General Public Licence.");
}

void Noted::onPlaybackStatusChanged()
{
	if (audio()->isPlaying())
		ui->statusBar->findChild<QLabel*>("alsa")->setText(QString("%1 %2# @ %3Hz, %4x%5 frames").arg(audio()->deviceName()).arg(audio()->deviceChannels()).arg(audio()->deviceRate()).arg(audio()->devicePeriods()).arg(audio()->deviceFrames()));
	else
		ui->statusBar->findChild<QLabel*>("alsa")->setText("No audio");
	if (!audio()->isCausal() && ui->actPlayCausal->isChecked())
		ui->actPlayCausal->setChecked(false);
	if (!audio()->isAcausal() && ui->actPlay->isChecked())
		ui->actPlay->setChecked(false);
	if (!audio()->isPassing() && ui->actPassthrough->isChecked())
		ui->actPassthrough->setChecked(false);
}

void Noted::updateAudioDevices()
{
	QString pd = ui->playDevice->currentText();
	ui->playDevice->clear();
	for (auto d: Audio::Playback::devices())
		ui->playDevice->addItem(QString::fromStdString(d.second), d.first);
	ui->playDevice->setCurrentIndex(ui->playDevice->findText(pd));

	QString cd = ui->captureDevice->currentText();
	ui->captureDevice->clear();
	for (auto d: Audio::Capture::devices())
		ui->captureDevice->addItem(QString::fromStdString(d.second), d.first);
	ui->captureDevice->setCurrentIndex(ui->captureDevice->findText(cd));
}

void Noted::addDockWidget(Qt::DockWidgetArea _a, QDockWidget* _d)
{
	if (_d->objectName().isEmpty())
		_d->setObjectName(_d->windowTitle());
	QMainWindow::addDockWidget(_a, _d);
}

QWidget* Noted::addGLWidget(QGLWidgetProxy* _v, QWidget* _p)
{
	return new NotedGLWidget(_v, _p);
}

EventCompiler Noted::findEventCompiler(QString const& _name)
{
	QMutexLocker l(&x_timelines);
	for (auto ev: eventsViews())
		if (ev->name() == _name)
			return ev->eventCompiler();
	return EventCompiler();
}

QString Noted::getEventCompilerName(EventCompilerImpl* _ec)
{
	QMutexLocker l(&x_timelines);
	for (auto ev: eventsViews())
		if (&ev->eventCompiler().asA<EventCompilerImpl>() == _ec)
			return ev->name();
	return QString();
}

void Noted::addTimeline(Timeline* _tl)
{
	ui->dataDisplay->addWidget(_tl->widget());
	_tl->widget()->installEventFilter(this);
	QMutexLocker l(&x_timelines);
	m_timelines.insert(_tl);
}

void Noted::updateWindowTitle()
{
	QString t = audio()->filename().isEmpty() ? QString("New Recording") : audio()->filename();
	foreach (auto l, libs()->libraries())
		if (l->plugin)
			t = l->plugin->titleAmendment(t);
	setWindowTitle(t + " - Noted!");
}

void Noted::on_addLibrary_clicked()
{
#if defined(Q_OS_LINUX)
	QString filter = "*.so";
#elif defined(Q_OS_MAC)
	QString filter = "*.dylib";
#elif defined(Q_OS_WIN)
	QString filter = "*.dll";
#endif
	QString fn = QFileDialog::getOpenFileName(this, "Add extension library", QDir::currentPath(), QString("Dynamic library (%1);;").arg(filter));
	if (fn.size())
		libs()->addLibrary(fn);
}

void Noted::on_killLibrary_clicked()
{
	if (ui->librariesView->currentIndex().isValid())
		libs()->removeLibrary(ui->librariesView->currentIndex());
}

void Noted::on_actReadSettings_triggered()
{
	QSettings s("LancasterLogicResponse", "Noted");
	for (auto d: findChildren<QDockWidget*>())
		cdebug << d->objectName();
	readBaseSettings(s);
}

void Noted::on_actWriteSettings_triggered()
{
	QSettings s("LancasterLogicResponse", "Noted");
	for (auto d: findChildren<QDockWidget*>())
		cdebug << d->objectName();
	writeBaseSettings(s);
}

void Noted::readBaseSettings(QSettings& _s)
{
	restoreState(_s.value("windowState").toByteArray());
	foreach (QSplitter* s, findChildren<QSplitter*>())
		s->restoreState(_s.value(s->objectName() + "State").toByteArray());
}

void Noted::writeBaseSettings(QSettings& _s)
{
	_s.setValue("windowState", saveState());
	foreach (QSplitter* s, findChildren<QSplitter*>())
		_s.setValue(s->objectName() + "State", s->saveState());
}

void Noted::readSettings()
{
	QSettings settings("LancasterLogicResponse", "Noted");
	restoreGeometry(settings.value("geometry").toByteArray());

#define DO(X, V, C) ui->X->V(settings.value(#X).C())
	if (settings.contains("sampleRate"))
	{
		DO(sampleRate, setCurrentIndex, toInt);
		DO(windowFunction, setCurrentIndex, toInt);
		DO(hopSlider, setValue, toInt);
		DO(windowSizeSlider, setValue, toInt);
		DO(zeroPhase, setChecked, toBool);
		DO(floatFFT, setChecked, toBool);
	}

	if (settings.contains("playRate"))
	{
		DO(playChunkSamples, setValue, toInt);
		DO(playChunks, setValue, toInt);
		DO(playRate, setCurrentIndex, toInt);
		DO(playDevice, setCurrentIndex, toInt);
		DO(force16Bit, setChecked, toBool);
		DO(captureDevice, setCurrentIndex, toInt);
		DO(captureChunks, setValue, toInt);
	}
#undef DO

	if (settings.contains("eventsViews"))
		for (int i = 0; i < settings.value("eventsViews").toInt(); ++i)
		{
			EventsView* ev = new EventsView(ui->dataDisplay);
			ev->readSettings(settings, QString("eventsView%1").arg(i));
			addTimeline(ev);
		}

	audio()->setFilename(settings.value("fileName").toString());

	if (settings.contains("pixelDuration"))
	{
		view()->setParameters(settings.value("timelineOffset").toLongLong(), max<Time>(settings.value("pixelDuration").toLongLong(), 1));
		audio()->setCursor(settings.value("cursor").toLongLong(), true);
	}

	m_audioMan->setHopSamples(ui->hop->value());
	if (settings.contains("eventEditors"))
		foreach (QString n, settings.value("eventEditors").toStringList())
		{
			EventsEditor* ev = new EventsEditor(ui->dataDisplay, n);
			addTimeline(ev);
			ev->load(settings);
		}

	libs()->readSettings(settings);

	readBaseSettings(settings);
}

void Noted::closeEvent(QCloseEvent* _event)
{
	writeSettings();
	QMainWindow::closeEvent(_event);
}

void Noted::writeSettings()
{
	QSettings settings("LancasterLogicResponse", "Noted");

	writeBaseSettings(settings);

	libs()->writeSettings(settings);

	int evc = 0;
	for (EventsView* ev: eventsViews())
	{
		ev->writeSettings(settings, QString("eventsView%1").arg(evc));
		++evc;
	}
	settings.setValue("eventsViews", evc);

	QStringList eds;
	QString s;
	for (EventsEditor* ed: findChildren<EventsEditor*>())
		if (ed->isIndependent() && !(s = ed->queryFilename()).isNull())
		{
			eds.append(s);
			ed->save(settings);
		}
	settings.setValue("eventEditors", eds);

#define DO(X, V) settings.setValue(#X, ui->X->V())
	DO(sampleRate, currentIndex);
	DO(windowFunction, currentIndex);
	DO(hopSlider, value);
	DO(windowSizeSlider, value);
	DO(zeroPhase, isChecked);
	DO(floatFFT, isChecked);
	DO(force16Bit, isChecked);
	DO(playChunkSamples, value);
	DO(playChunks, value);
	DO(playRate, currentIndex);
	DO(playDevice, currentIndex);
	DO(captureChunks, value);
	DO(captureDevice, currentIndex);
#undef DO

	settings.setValue("fileName", audio()->filename());
	settings.setValue("pixelDuration", (qlonglong)view()->pixelDuration());
	settings.setValue("timelineOffset", (qlonglong)view()->earliestVisible());
	settings.setValue("cursor", (qlonglong)audio()->cursor());

	settings.setValue("geometry", saveGeometry());
}

void Noted::on_addEventsView_clicked()
{
	if (ui->eventCompilersList->currentItem())
		addTimeline(new EventsView(ui->dataDisplay, libs()->newEventCompiler(ui->eventCompilersList->currentItem()->text().section(':', 0, 0))));
}

void Noted::on_actNewEvents_triggered()
{
	addTimeline(new EventsEditor(ui->dataDisplay));
}

void Noted::on_actNewEventsFrom_triggered()
{
	QStringList esns;
	auto ess = eventsStores();
	foreach (auto es, ess)
		esns.push_back(es->niceName());
	QString esn = QInputDialog::getItem(this, "Select Events View", "Select an events view to copy into the new editor.", esns, 0, false);
	if (esn.size())
	{
		EventsEditor* ev = new EventsEditor(ui->dataDisplay);
		addTimeline(ev);
		ev->scene()->copyFrom(ess[esns.indexOf(esn)]);
	}
}

void Noted::on_actQuit_triggered()
{
	QApplication::quit();
}

void Noted::on_actOpenEvents_triggered()
{
	QString s = QFileDialog::getOpenFileName(this, "Open a Stream Events File", QDir::homePath(), "*.xml");
	if (!s.isNull())
	{
		EventsEditor* ev = new EventsEditor(ui->dataDisplay, s);
		addTimeline(ev);
	}
}

void Noted::updateHopDisplay()
{
	ui->hopPeriod->setText(QString("%1ms %2%").arg(fromBase(toBase(ui->hop->value(), audio()->rate()), 100000) / 100.0).arg(((ui->windowSize->value() - ui->hop->value()) * 1000 / ui->windowSize->value()) / 10.0));
	ui->windowPeriod->setText(QString("%1ms %2Hz").arg(fromBase(toBase(ui->windowSize->value(), audio()->rate()), 100000) / 100.0).arg((audio()->rate() * 10 / ui->windowSize->value()) / 10.0));
}

void Noted::on_sampleRate_currentIndexChanged(int)
{
	audio()->setRate(ui->sampleRate->currentText().left(ui->sampleRate->currentText().size() - 3).toInt());
	updateHopDisplay();
}

void Noted::on_hopSlider_valueChanged(int)
{
	ui->hop->setValue(1 << ui->hopSlider->value());
	updateHopDisplay();
}

void Noted::on_hop_valueChanged(int _i)
{
	m_audioMan->setHopSamples(_i);
}

void Noted::on_windowSizeSlider_valueChanged(int)
{
	ui->windowSize->setValue(1 << ui->windowSizeSlider->value());
	updateHopDisplay();
}

void Noted::on_windowSize_valueChanged(int _i)
{
	if ((int)m_windowFunction.size() != _i)
	{
		compute()->suspendWork();
		m_windowFunction = lb::windowFunction(_i, WindowFunction(ui->windowFunction->currentIndex()));
		compute()->invalidate(compute()->spectraAcAnalysis());
		compute()->resumeWork();
	}
}

void Noted::on_windowFunction_currentIndexChanged(int _i)
{
	compute()->suspendWork();
	m_windowFunction = lb::windowFunction(ui->windowSize->value(), WindowFunction(_i));
	compute()->invalidate(compute()->spectraAcAnalysis());
	compute()->resumeWork();
}

void Noted::on_zeroPhase_toggled(bool _v)
{
	if (m_zeroPhase != _v)
	{
		compute()->suspendWork();
		m_zeroPhase = _v;
		compute()->invalidate(compute()->spectraAcAnalysis());
		compute()->resumeWork();
	}
}

void Noted::on_floatFFT_toggled(bool _v)
{
	if (m_floatFFT != _v)
	{
		compute()->suspendWork();
		m_floatFFT = _v;
		compute()->invalidate(compute()->spectraAcAnalysis());
		compute()->resumeWork();
	}
}

void Noted::timelineDead(Timeline* _tl)
{
	QMutexLocker l(&x_timelines);
	m_timelines.remove(_tl);
}

QList<EventsStore*> Noted::eventsStores() const
{
	QList<EventsStore*> ret;
	QMutexLocker l(&x_timelines);
	foreach (Timeline* i, m_timelines)
		if (EventsStore* es = dynamic_cast<EventsStore*>(i))
			ret.push_back(es);
	return ret;
}

void Noted::on_actFollow_changed()
{
}

void Noted::on_actOpen_triggered()
{
	QString s = QFileDialog::getOpenFileName(this, "Open an audio file", QDir::homePath(), "All Audio files (*.wav *.WAV *.aiff *.AIFF *.aif *.AIF *.aifc *.AIFC *.au *.AU *.snd *.SND *.nist *.NIST *.iff *.IFF *.svx *.SVX *.paf *.PAF *.w64 *.W64 *.voc *.VOC *.sf *.SF *.caf *.CAF *.htk *.HTK *.xi *.XI *.pvf *.PVF *.mat5 *.mat4 *.MAT5 *.MAT4 *.sd2 *.SD2 *.flac *.FLAC *.ogg *.OGG );;Microsoft Wave (*.wav *.WAV);;SGI/Apple (*.AIFF *.AIFC *.aiff *.aifc);;Sun/DEC/NeXT (*.AU *.SND *.au *.snd);;Paris Audio File (*.PAF *.paf);;Commodore Amiga (*.IFF *.SVX *.iff *.svx);;Sphere Nist (*.NIST *.nist);;IRCAM (*.SF *.sf);;Creative (*.VOC *.voc);;Soundforge (*.W64 *.w64);;GNU Octave 2.0 (*.MAT4 *.mat4);;GNU Octave 2.1 (*.MAT5 *.mat5);;Portable Voice Format (*.PVF *.pvf);;Fasttracker 2 (*.XI *.xi);;HMM Tool Kit (*.HTK *.htk);;Apple CAF (*.CAF *.caf);;Sound Designer II (*.SD2 *.sd2);;Free Lossless Audio Codec (*.FLAC *.flac);;Ogg Vorbis (*.OGG *.ogg)");
	if (!s.isNull())
		audio()->setFilename(s);
}

// start playing (acausally - with the pre generated data)
void Noted::on_actPlay_changed()
{
	if (ui->actPlay->isChecked())
	{
		ui->dockPlay->setEnabled(false);
		ui->actPlayCausal->setEnabled(false);
		ui->actPassthrough->setEnabled(false);
		ui->actOpen->setEnabled(false);
		audio()->play();
	}
	else if (ui->actPlay->isEnabled())
	{
		audio()->stop();
		ui->dockPlay->setEnabled(true);
		ui->actPlayCausal->setEnabled(true);
		ui->actPassthrough->setEnabled(true);
		ui->actOpen->setEnabled(true);
	}
}

void Noted::on_actPlayCausal_changed()
{
	if (ui->actPlayCausal->isChecked())
	{
		ui->dockPlay->setEnabled(false);
		ui->actPlay->setEnabled(false);
		ui->actPassthrough->setEnabled(false);
		ui->actOpen->setEnabled(false);
		audio()->play(true);
	}
	else if (ui->actPlayCausal->isEnabled())
	{
		audio()->stop();
		ui->dockPlay->setEnabled(true);
		ui->actPassthrough->setEnabled(true);
		ui->actPlay->setEnabled(true);
		ui->actOpen->setEnabled(true);
	}
}

void Noted::on_actPassthrough_changed()
{
	if (ui->actPassthrough->isChecked())
	{
		ui->dockPlay->setEnabled(false);
		ui->actPlay->setEnabled(false);
		ui->actPlayCausal->setEnabled(false);
		ui->dataDisplay->setEnabled(false);
		ui->actOpen->setEnabled(false);
		audio()->passthrough();
	}
	else if (ui->actPassthrough->isEnabled())
	{
		audio()->stop();
		ui->dockPlay->setEnabled(true);
		ui->actPlay->setEnabled(true);
		ui->actPlayCausal->setEnabled(true);
		ui->actOpen->setEnabled(true);
		ui->dataDisplay->setEnabled(true);
	}
}

QList<EventsView*> Noted::eventsViews() const
{
	QList<EventsView*> ret;
	QMutexLocker l(&x_timelines);
	foreach (Timeline* i, m_timelines)
		if (EventsView* ev = dynamic_cast<EventsView*>(i))
			ret.push_back(ev);
	return ret;
}

void Noted::on_actZoomOut_triggered()
{
	view()->zoomTimeline((audio()->cursor() > view()->earliestVisible() && audio()->cursor() < view()->latestVisible()) ? view()->positionOf(audio()->cursor()) : (view()->activeWidth() / 2), 1.2);
}

void Noted::on_actZoomIn_triggered()
{
	view()->zoomTimeline((audio()->cursor() > view()->earliestVisible() && audio()->cursor() < view()->latestVisible()) ? view()->positionOf(audio()->cursor()) : (view()->activeWidth() / 2), 1 / 1.2);
}

void Noted::on_actPanBack_triggered()
{
	view()->setTimelineOffset(view()->timelineOffset() - view()->visibleDuration() / 4);
}

void Noted::on_actPanForward_triggered()
{
	view()->setTimelineOffset(view()->timelineOffset() + view()->visibleDuration() / 4);
}

void Noted::on_actPanic_triggered()
{
	ui->actPlay->setChecked(false);
	ui->actPlayCausal->setChecked(false);
	ui->actPassthrough->setChecked(false);
}

void Noted::updateEventStuff()
{
	QMutexLocker l(&x_timelines);
	for (EventsView* ev: eventsViews())
		if (!ev->eventCompiler().isNull())
			for (auto g: ev->eventCompiler().asA<EventCompilerImpl>().graphMap())
				if (dynamic_cast<GraphSparseDense*>(g.second))
				{
					QString n = ev->name() + "/" + QString::fromStdString(g.second->name());
					if (!findChild<GraphView*>(n))
					{
//						cdebug << "Creating" << n.toStdString();
						QDockWidget* dw = new QDockWidget(n, this);
						dw->setObjectName(n + "-Dock");
						GraphView* dv = new GraphView(dw, n);
						dv->addGraph(g.second);
						dw->setAllowedAreas(Qt::AllDockWidgetAreas);
						QMainWindow::addDockWidget(Qt::BottomDockWidgetArea, dw, Qt::Horizontal);
						dw->setFeatures(dw->features() | QDockWidget::DockWidgetVerticalTitleBar);
						dw->setWidget(dv);
						dw->show();
					}
				}
}

void Noted::onWorkFinished()
{
	info("WORK All finished");
	m_cursorDirty = true;
	if (QProgressBar* pb = ui->statusBar->findChild<QProgressBar*>())
		delete pb;
	statusBar()->showMessage("Ready");

	QMutexLocker l(&x_timelines);
	foreach (Timeline* t, m_timelines)
		if (PrerenderedTimeline* pt = dynamic_cast<PrerenderedTimeline*>(t))
			pt->rerender();
}

void Noted::onWorkProgressed(QString _desc, int _progress)
{
	QProgressBar* pb = ui->statusBar->findChild<QProgressBar*>();
	if (_progress < 100)
	{
		if (!pb)
		{
			pb = new QProgressBar;
			pb->setMaximum(100);
			pb->setObjectName("progress");
			pb->setMaximumWidth(128);
			pb->setMaximumHeight(17);
			ui->statusBar->addPermanentWidget(pb);
		}
		pb->setValue(_progress);
		statusBar()->showMessage(_desc);
	}
	else if (pb)
		delete pb;
}

void Noted::changeEvent(QEvent *e)
{
	QMainWindow::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

void Noted::on_clearInfo_clicked()
{
	m_info.clear();
	ui->infoView->setHtml(m_info);
}

int Noted::activeWidth() const
{
	return max<int>(1, ui->dataDisplay->width());
}

void Noted::info(QString const& _info, int _id)
{
	QString color = (_id == 255) ? "#700" : (_id == 254) ? "#007" : (_id == 253) ? "#440" : "#fff";
	{
		QMutexLocker l(&x_infos);
		m_infos += "<div style=\"margin-top: 1px;\"><span style=\"background-color:" + color + ";\">&nbsp;</span> " + _info.toHtmlEscaped() + "</div>";
	}
	QMetaObject::invokeMethod(this, "processNewInfo");
}

void Noted::info(QString const& _info, QString const& _c)
{
	{
		QMutexLocker l(&x_infos);
		m_infos += QString("<div style=\"margin-top: 1px;\"><span style=\"background-color:%1;\">&nbsp;</span> %2</div>").arg(_c).arg(_info);
	}
	QMetaObject::invokeMethod(this, "processNewInfo");
}

void Noted::onCursorChanged(lb::Time _cursor)
{
	ui->statusBar->findChild<QLabel*>("cursor")->setText(textualTime(_cursor, toBase(audio()->samples(), audio()->rate()), 0, 0).c_str());
	if (ui->actFollow->isChecked() && (_cursor < view()->earliestVisible() || _cursor > view()->earliestVisible() + view()->visibleDuration() * 7 / 8))
		view()->setTimelineOffset(_cursor - view()->visibleDuration() / 8);
}

foreign_vector<float const> Noted::cursorMagSpectrum() const
{
	if (audio()->isCausal())
		return magSpectrum(compute()->causalCursorIndex(), 1);
	else if (audio()->isPassing())
		return foreign_vector<float const>((vector<float>*)&m_currentMagSpectrum);
	else
		return magSpectrum(audio()->cursorIndex(), 1); // NOTE: only approximate - no good for Analysers.
}

foreign_vector<float const> Noted::cursorPhaseSpectrum() const
{
	if (audio()->isCausal())
		return phaseSpectrum(compute()->causalCursorIndex(), 1);			// FIXME: will return phase normalized to [0, 1] rather than [0, pi].
	else if (audio()->isPassing())
		return foreign_vector<float const>((vector<float>*)&m_currentPhaseSpectrum);
	else
		return phaseSpectrum(audio()->cursorIndex(), 1); // NOTE: only approximate - no good for Analysers.
}

void Noted::processNewInfo()
{
	QMutexLocker l(&x_infos);
	if (m_infos.size())
	{
		m_info += m_infos;
		ui->infoView->setHtml(m_info);
		m_infos.clear();
		if (ui->lockLog->isChecked())
			ui->infoView->verticalScrollBar()->setValue(ui->infoView->verticalScrollBar()->maximum());
	}
}
