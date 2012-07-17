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
#include <Common/Common.h>
#include <EventsEditor/EventsEditor.h>
#include <EventsEditor/EventsEditScene.h>
#include <NotedPlugin/AuxLibraryFace.h>
#include <NotedPlugin/AcausalAnalysis.h>
#include <NotedPlugin/CausalAnalysis.h>

#include "DataView.h"
#include "EventsView.h"
#include "WorkerThread.h"
#include "ProcessEventCompiler.h"
#include "PropertiesEditor.h"
#include "Cursor.h"
#include "CompileEvents.h"
#include "CollateEvents.h"
#include "CompileEventsView.h"
#include "Noted.h"
#include "NotedGLWidget.h"
#include "ui_Noted.h"

using namespace std;
using namespace Lightbox;

class ResampleWaveAc: public AcausalAnalysis
{
public:
	ResampleWaveAc(): AcausalAnalysis("Resampling wave") {}

	virtual void init()
	{
	}
	virtual unsigned prepare(unsigned _from, unsigned _count, Lightbox::Time _hop)
	{
		(void)_from; (void)_count; (void)_hop;
		return 100;
	}
	virtual void analyze(unsigned _from, unsigned _count, Lightbox::Time _hop)
	{
		(void)_from; (void)_count; (void)_hop;
		dynamic_cast<Noted*>(noted())->updateParameters();
		dynamic_cast<Noted*>(noted())->resampleWave();
		if (dynamic_cast<Noted*>(noted())->m_pixelDuration == 1)
		{
			dynamic_cast<Noted*>(noted())->m_fineCursor = 0;
			dynamic_cast<Noted*>(noted())->normalizeView();
		}
	}
	virtual void fini()
	{
	}
};

// OPTIMIZE: allow reanalysis of spectra to be data-parallelized.
class SpectraAc: public AcausalAnalysis
{
public:
	SpectraAc(): AcausalAnalysis("Analyzing spectra") {}

	virtual void init()
	{
	}
	virtual unsigned prepare(unsigned _from, unsigned _count, Lightbox::Time _hop)
	{
		(void)_from; (void)_count; (void)_hop;
		return 100;
	}
	virtual void analyze(unsigned _from, unsigned _count, Lightbox::Time _hop)
	{
		(void)_from; (void)_count; (void)_hop;
		dynamic_cast<Noted*>(noted())->rejigSpectra();
	}
	virtual void fini()
	{
	}
};

class FinishUpAc: public AcausalAnalysis
{
public:
	FinishUpAc(): AcausalAnalysis("Finishing up") {}
	void fini()
	{
		dynamic_cast<Noted*>(noted())->d_initEvents = true;
		QMutexLocker l(&dynamic_cast<Noted*>(noted())->x_timelines);
		foreach (Timeline* t, dynamic_cast<Noted*>(noted())->m_timelines)
			if (PrerenderedTimeline* pt = dynamic_cast<PrerenderedTimeline*>(t))
				pt->sourceChanged();
		dynamic_cast<Noted*>(noted())->m_workFinished = true;
	}
};

Noted::Noted(QWidget* _p):
	NotedBase				(_p),
	ui						(new Ui::Noted),
	m_workerThread			(nullptr),
	m_fineCursorWas			(UndefinedTime),
	m_nextResample			(UndefinedTime),
	m_resampler				(nullptr),
	m_workFinished			(false),
	m_resampleWaveAcAnalysis(new ResampleWaveAc),
	m_spectraAcAnalysis		(new SpectraAc),
	m_finishUpAcAnalysis	(new FinishUpAc),
	m_compileEventsAnalysis	(new CompileEvents),
	m_collateEventsAnalysis	(new CollateEvents)
{
	g_debugPost = [&](std::string const& _s, int _id){ simpleDebugOut(_s, _id); info(_s.c_str(), _id); };

	ui->setupUi(this);
	setWindowIcon(QIcon(":/Noted.png"));

	updateAudioDevices();

	new Cursor(this, 0);
	new Cursor(this, 1);
	new Cursor(this, 2);
	ui->waveform->installEventFilter(this);
	ui->overview->installEventFilter(this);
	ui->dataDisplay->installEventFilter(this);

	m_timelines.insert(ui->waveform);
	m_timelines.insert(ui->spectra);

	m_workerThread = createWorkerThread([=](){return work();});
	m_playbackThread = createWorkerThread([=](){return serviceAudio();});

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

	connect(&m_libraryWatcher, SIGNAL(fileChanged(QString)), this, SLOT(onLibraryChange(QString)));
	connect(this, SIGNAL(analysisFinished()), SLOT(updateEventStuff()));

	foreach (CurrentView* v, findChildren<CurrentView*>())
		connect(this, SIGNAL(cursorChanged()), v, SLOT(check()));

	on_sampleRate_currentIndexChanged(0);

	startTimer(15);
	resumeWork();

	readSettings();

	setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
	setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
	setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
	setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
}

Noted::~Noted()
{
	g_debugPost = simpleDebugOut;
	foreach (Cursor* c, findChildren<Cursor*>())
		c->hide();

	suspendWork();

	while (m_libraries.size())
	{
		unload(*m_libraries.begin());
		m_libraries.erase(m_libraries.begin());
	}

	while (m_timelines.size())
		delete *m_timelines.begin();

	delete m_workerThread;
	for (m_playbackThread->quit(); !m_playbackThread->wait(1000); m_playbackThread->terminate()) {}
	delete m_playbackThread;

	delete ui;
}

void Noted::on_actAbout_activated()
{
	QMessageBox::about(this, "About Noted!", "<h1>Noted!</h1>Copyright &copyright;2011, 2012 Lancaster Logic Response Limited. This code is released under version 2 of the GNU General Public Licence.");
}

void Noted::updateAudioDevices()
{
	QString pd = ui->playDevice->currentText();
	ui->playDevice->clear();
	foreach (auto d, Audio::Playback::devices())
		ui->playDevice->addItem(QString::fromStdString(d.second), d.first);
	ui->playDevice->setCurrentIndex(ui->playDevice->findText(pd));
}

void Noted::addLibrary(QString const& _name)
{
	m_libraries.insert(_name, make_shared<Library>(_name));
	auto item = new QListWidgetItem("... " + _name.section('/', -1));
	item->setData(Qt::UserRole, _name);
	ui->loadedLibraries->addItem(item);
	m_dirtyLibraries.insert(_name);
	reloadDirties();
}

void Noted::load(LibraryPtr const& _dl)
{
	cnote << "Loading:" << _dl->name;
	m_libraryWatcher.addPath(_dl->name);
	if (QLibrary::isLibrary(_dl->name))
	{
		QString tempFile = QDir::tempPath() + "/Noted[" + _dl->name.section('/', -1) + "]" + QDateTime::currentDateTime().toString("yyyyMMdd-hh.mm.ss.zzz");
		_dl->l.setFileName(tempFile);
		_dl->l.setLoadHints(QLibrary::ResolveAllSymbolsHint);
		QFile::copy(_dl->name, tempFile);
		if (_dl->l.load())
		{
			typedef EventCompilerFactories&(*cf_t)();
			typedef NotedPlugin*(*pf_t)(NotedFace*);
			typedef char const*(*pnf_t)();
			if (cf_t cf = (cf_t)_dl->l.resolve("eventCompilerFactories"))
			{
				cnote << "LOAD" << _dl->name << " [ECF]";
				_dl->cf = cf();
				foreach (auto f, _dl->cf)
				{
					auto li = new QListWidgetItem(QString::fromStdString(f.first));
					li->setData(0, QString::fromStdString(f.first));
					ui->eventCompilersList->addItem(li);
					noteEventCompilersChanged();
				}
				cnote << _dl->cf.size() << " event compiler factories";
			}
			else if (pf_t np = (pf_t)_dl->l.resolve("newPlugin"))
			{
				cnote << "LOAD" << _dl->name << " [PLUGIN]";
				_dl->p = shared_ptr<NotedPlugin>(np(this));

				foreach (auto lib, m_libraries)
					if (!lib->l.isLoaded())
						load(lib);

				// load state.
				char const* name = ((pnf_t)_dl->l.resolve("pluginName"))();
				QSettings s("LancasterLogicResponse", "Noted");
				readBaseSettings(s);
				_dl->p->readSettings(s);
				Members<NotedPlugin> props(_dl->p->propertyMap(), _dl->p);
				props.deserialize(s.value(QString(name) + "/properties").toString().toStdString());
				if (props.size())
				{
					auto propsDock = new QDockWidget(QString("%1 Properties").arg(name), this);
					propsDock->setObjectName(_dl->name);
					auto pe = new PropertiesEditor(propsDock);
					pe->setProperties(props);
					propsDock->setWidget(pe);
					propsDock->setFeatures(propsDock->features()|QDockWidget::DockWidgetVerticalTitleBar);
					addDockWidget(Qt::RightDockWidgetArea, propsDock);
				}
			}
			else
			{
				foreach (auto lib, m_libraries)
					if (lib->p)
						if (auto f = shared_ptr<AuxLibraryFace>(lib->p->newAuxLibrary()))
						{
							// tentatively add it, so the library can use it transparently.
							lib->p->m_auxLibraries.append(f);
							if (f->load(_dl->l))
							{
								_dl->auxFace = f;
								_dl->aux = lib->p;
								cnote << "LOAD" << _dl->name << " [AUX:" << lib->name << "]";
								goto LOADED;
							}
							else
							{
								f.reset();
								lib->p->removeDeadAuxes();
							}
						}
				_dl->unload();
				LOADED:;
			}
		}
		else
		{
			cwarn << "ERROR on load: " << _dl->l.errorString();
		}
	}
	else if (QFile::exists(_dl->name))
	{
		_dl->cf[_dl->name.toStdString()] = [=](){ return new ProcessEventCompiler(_dl->name); };
		auto li = new QListWidgetItem(_dl->name);
		li->setData(0, _dl->name);
		ui->eventCompilersList->addItem(li);
	}
}

void Noted::Library::unload()
{
	if (bool** fed = (bool**)l.resolve("g_lightboxFinalized"))
	{
		bool isFinalized = false;
		*fed = &isFinalized;
		assert(l.unload());
		assert(isFinalized);
	}
	else
	{
		qWarning() << "Couldn't get the Lightbox 'finalized' symbol in " << name;
		assert(l.unload());
	}
	QFile::remove(l.fileName());
}

void Noted::unload(LibraryPtr const& _dl)
{
	if (_dl->l.isLoaded())
	{
		if (_dl->p)
		{
			// save state.
			QSettings s("LancasterLogicResponse", "Noted");
			writeBaseSettings(s);
			_dl->p->writeSettings(s);

			typedef char const*(*pnf_t)();
			char const* name = ((pnf_t)_dl->l.resolve("pluginName"))();
			Members<NotedPlugin> props(_dl->p->propertyMap(), _dl->p);
			s.setValue(QString(name) + "/properties", QString::fromStdString(props.serialized()));

			// kill the properties dock if there is one.
			delete findChild<QDockWidget*>(_dl->name);

			// unload dependents.
			foreach (auto l, m_libraries)
				if (l->auxFace && l->aux.lock() == _dl->p)
					unload(l);
			// if all dependents were successfully unloaded, then there should be nothing left in the auxLibrary list.
			assert(_dl->p->m_auxLibraries.isEmpty());
			// kill plugin.
			_dl->p.reset();
		}
		else if (_dl->cf.size())
		{
			clearEventsCache();
			foreach (auto f, _dl->cf)
			{
				// NOTE: A bit messy, this?
				foreach (EventsView* ev, eventsViews())
					if (ev->name() == QString::fromStdString(f.first))
						ev->save();
				delete ui->eventCompilersList->findItems(QString::fromStdString(f.first), 0).front();
			}
			_dl->cf.clear();
			foreach (auto w, findChildren<DataView*>())
				w->checkSpec();
		}
		else if (_dl->auxFace && _dl->aux.lock()) // check if we're a plugin's auxilliary
		{
			// remove ourselves from the plugin we're dependent on.
			_dl->auxFace->unload(_dl->l);
			_dl->auxFace.reset();
			_dl->aux.lock()->removeDeadAuxes();
			_dl->aux.reset();
		}
		cnote << "UNLOAD" << _dl->name;
		_dl->unload();
	}
	m_libraryWatcher.removePath(_dl->name);
}

void Noted::reloadLibrary(QString const& _name)
{
	shared_ptr<Library> dl = m_libraries[_name];
	assert(dl);
	assert(dl->name == _name);

	unload(dl);
	load(dl);
}

void Noted::onLibraryChange(QString const& _name)
{
	m_dirtyLibraries.insert(_name);
	reloadDirties();
}

void Noted::reloadDirties()
{
	if (!m_dirtyLibraries.empty())
	{
		bool doSuspend = m_workerThread && m_workerThread->isRunning();
		if (doSuspend)
			suspendWork();

		// OPTIMIZE: only bother saving for EVs whose EC is given by a dirty library.
		foreach (EventsView* ev, eventsViews())
			ev->save();

		foreach (QString const& name, m_dirtyLibraries)
		{
			for (int i = 0; i < ui->loadedLibraries->count(); ++i)
				if (ui->loadedLibraries->item(i)->data(Qt::UserRole).toString() == name)
				{
					reloadLibrary(name);
					goto OK;
				}
			// nothing found
			unload(m_libraries[name]);
			m_libraries.remove(name);
			OK:;
		}

		foreach (EventsView* ev, eventsViews())
			ev->restore();

		m_dirtyLibraries.clear();

		// OPTIMIZE: Something finer grained?
		noteEventCompilersChanged();

		if (doSuspend)
			resumeWork();
	}
}

QWidget* Noted::addGLWidget(QGLWidgetProxy* _v, QWidget* _p)
{
	return new NotedGLWidget(_v, _p);
}

bool Noted::eventFilter(QObject*, QEvent* _e)
{
	if (_e->type() == QEvent::Resize)
		viewSizesChanged();
	return false;
}

void Noted::moveEvent(QMoveEvent*)
{
	emit cursorChanged();
}

vector<float> Noted::graphEvents(float _nature) const
{
	// OPTIMIZE: memoize.
	foreach (EventsView* ev, eventsViews())
	{
		vector<float> ret = ev->graphEvents(_nature);
		if (ret.size())
			return ret;
	}
	return vector<float>();
}

StreamEvent Noted::eventOf(EventType _et, float _nature, Time _t) const
{
	if (_t == UndefinedTime)
		_t = cursor();
	StreamEvent ret(NoEvent);
	if (_t < duration() && _et != NoEvent)
	{
		bool careAboutNature = isFinite(_nature);
		auto evs = eventsViews();
//		foreach (EventsView* ev, evs)
//			ev->mutex()->lock();
		for (int i = windowIndex(_t); i >= 0; --i)
			foreach (EventsView* ev, evs)
				foreach (StreamEvent const& e, ev->events(i))
					if (e.type == _et && (e.nature == _nature || !careAboutNature))
					{
						ret = e;
						goto OK;
					}
		OK:;
//		foreach (EventsView* ev, evs)
//			ev->mutex()->unlock();
	}
	return ret;
}

StreamEvents Noted::initEventsOf(EventType _et, float _nature) const
{
	if (d_initEvents)
	{
		d_initEvents = false;
		foreach (EventsView* ev, eventsViews())
			catenate(m_initEvents, ev->initEvents());
	}
	StreamEvents ret;
	bool careAboutNature = isFinite(_nature);
	foreach (StreamEvent const& e, m_initEvents)
		if (e.type == _et && (e.nature == _nature || !careAboutNature))
			ret.push_back(e);
	return ret;
}

EventCompiler Noted::newEventCompiler(QString const& _name)
{
	foreach (auto dl, m_libraries)
		if (dl->cf.find(_name.toStdString()) != dl->cf.end())
			return EventCompiler::create(dl->cf[_name.toStdString()]());
	return EventCompiler();
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
	QString t = m_sourceFileName.isEmpty() ? QString("New Recording") : m_sourceFileName;
	foreach (auto l, m_libraries)
		if (l->p)
			t = l->p->titleAmendment(t);
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
	addLibrary(fn);
}

void Noted::on_killLibrary_clicked()
{
	if (ui->loadedLibraries->currentItem())
	{
		QString s = ui->loadedLibraries->currentItem()->data(Qt::UserRole).toString();
		delete ui->loadedLibraries->currentItem();
		m_dirtyLibraries.insert(s);
		reloadDirties();
	}
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
	if (settings.contains("libraries"))
		foreach (QString n, settings.value("libraries").toStringList())
			addLibrary(n);
#define DO(X, V, C) ui->X->V(settings.value(#X).C())
	if (settings.contains("sampleRate"))
	{
		DO(sampleRate, setCurrentIndex, toInt);
		DO(windowFunction, setCurrentIndex, toInt);
		DO(hopSlider, setValue, toInt);
		DO(windowSizeSlider, setValue, toInt);
		DO(zeroPhase, setChecked, toBool);
	}

	if (settings.contains("playRate"))
	{
		DO(playChunkSamples, setValue, toInt);
		DO(playChunks, setValue, toInt);
		DO(playRate, setCurrentIndex, toInt);
		DO(playDevice, setCurrentIndex, toInt);
		DO(force16Bit, setChecked, toBool);
	}
#undef DO
	if (settings.contains("eventsViews"))
		for (int i = 0; i < settings.value("eventsViews").toInt(); ++i)
		{
			EventsView* ev = new EventsView(ui->dataDisplay);
			ev->readSettings(settings, QString("eventsView%1").arg(i));
			addTimeline(ev);
		}

	setAudio(settings.value("fileName").toString());

	if (settings.contains("duration"))
	{
		m_pixelDuration = max<Time>(settings.value("pixelDuration").toLongLong(), 1);
		m_timelineOffset = settings.value("timelineOffset").toLongLong();
		m_fineCursor = settings.value("cursor").toLongLong();
	}

	foreach (QString s, settings.value("dataViews").toStringList())
		newDataView(s);

	m_hopSamples = ui->hop->value();
	if (settings.contains("eventEditors"))
		foreach (QString n, settings.value("eventEditors").toStringList())
		{
			EventsEditor* ev = new EventsEditor(ui->dataDisplay, n);
			addTimeline(ev);
			ev->load(settings);
		}

	readBaseSettings(settings);

	foreach (auto l, m_libraries)
		if (l->p)
			l->p->readSettings(settings);
}

void Noted::closeEvent(QCloseEvent* _event)
{
	writeSettings();
	QMainWindow::closeEvent(_event);
}

void Noted::writeSettings()
{
	QSettings settings("LancasterLogicResponse", "Noted");

	foreach (auto l, m_libraries)
		if (l->p)
			l->p->writeSettings(settings);

	QStringList dataViews;
	foreach (DataView* dv, findChildren<DataView*>())
		dataViews.push_back(dv->objectName());
	settings.setValue("dataViews", dataViews);

	writeBaseSettings(settings);

	int evc = 0;
	foreach (EventsView* ev, eventsViews())
	{
		ev->writeSettings(settings, QString("eventsView%1").arg(evc));
		++evc;
	}
	settings.setValue("eventsViews", evc);

	QStringList eds;
	QString s;
	foreach (EventsEditor* ed, findChildren<EventsEditor*>())
		if (!(s = ed->queryFilename()).isNull())
		{
			eds.append(s);
			ed->save(settings);
		}
	settings.setValue("eventEditors", eds);

	settings.setValue("libraries", QStringList(m_libraries.keys()));

#define DO(X, V) settings.setValue(#X, ui->X->V())
	DO(sampleRate, currentIndex);
	DO(windowFunction, currentIndex);
	DO(hopSlider, value);
	DO(windowSizeSlider, value);
	DO(zeroPhase, isChecked);
	DO(force16Bit, isChecked);
	DO(playChunkSamples, value);
	DO(playChunks, value);
	DO(playRate, currentIndex);
	DO(playDevice, currentIndex);
#undef DO

	settings.setValue("fileName", m_sourceFileName);
	settings.setValue("pixelDuration", (qlonglong)m_pixelDuration);
	settings.setValue("timelineOffset", (qlonglong)m_timelineOffset);
	settings.setValue("cursor", (qlonglong)m_fineCursor);

	settings.setValue("geometry", saveGeometry());
}

void Noted::on_addEventsView_clicked()
{
	if (ui->eventCompilersList->currentItem())
		addTimeline(new EventsView(ui->dataDisplay, newEventCompiler(ui->eventCompilersList->currentItem()->data(0).toString())));
}

void Noted::on_actNewEvents_activated()
{
	addTimeline(new EventsEditor(ui->dataDisplay));
}

void Noted::on_actNewEventsFrom_activated()
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

void Noted::on_actQuit_activated()
{
	QApplication::quit();
}

void Noted::on_actOpenEvents_activated()
{
	QString s = QFileDialog::getOpenFileName(this, "Open a Stream Events File", QDir::homePath(), "*.xml");
	if (!s.isNull())
	{
		EventsEditor* ev = new EventsEditor(ui->dataDisplay, s);
		addTimeline(ev);
	}
}

void Noted::on_sampleRate_currentIndexChanged(int)
{
	m_rate = ui->sampleRate->currentText().left(ui->sampleRate->currentText().size() - 3).toInt();
	ui->hopPeriod->setText(QString("%1ms %2%").arg(fromBase(toBase(ui->hop->value(), rate()), 100000) / 100.0).arg(((ui->windowSize->value() - ui->hop->value()) * 1000 / ui->windowSize->value()) / 10.0));
	ui->windowPeriod->setText(QString("%1ms %2Hz").arg(fromBase(toBase(ui->windowSize->value(), rate()), 100000) / 100.0).arg((rate() * 10 / ui->windowSize->value()) / 10.0));
	noteLastValidIs(nullptr);
}

void Noted::on_windowSizeSlider_valueChanged(int)
{
	ui->windowSize->setValue(1 << ui->windowSizeSlider->value());
	on_sampleRate_currentIndexChanged(0);
}

void Noted::on_hopSlider_valueChanged(int)
{
	ui->hop->setValue(1 << ui->hopSlider->value());
	on_sampleRate_currentIndexChanged(0);
}

class Sleeper: QThread { public: using QThread::usleep; };

void Noted::timelineDead(Timeline* _tl)
{
	QMutexLocker l(&x_timelines);
	m_timelines.remove(_tl);
	clearEventsCache();
}

bool Noted::work()
{
	if (m_toBeAnalyzed.size())
		rejigAudio();
	else
	{
		bool worked = false;
		{
			QMutexLocker l(&x_timelines);
			foreach (Timeline* t, m_timelines)
				if (PrerenderedTimeline* pt = dynamic_cast<PrerenderedTimeline*>(t))
					if (pt->rejigRender())
						worked = true;
		}
		if (!worked)
		{
			Sleeper::usleep(100000);
		}
	}
	return true;
}

void Noted::suspendWork()
{
	if (m_workerThread)
	{
		m_workerThread->quit();
		while (!m_workerThread->wait(1000))
			qWarning() << "Worker thread not responding :-(";
	}
}

void Noted::resumeWork()
{
	if (m_workerThread)
		m_workerThread->start(QThread::LowPriority);
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

void Noted::noteLastValidIs(AcausalAnalysisPtr const& _a)
{
	if (!m_toBeAnalyzed.count(_a))
	{
		suspendWork();
		m_workFinished = false;
		info(QString("WORK Last valid is now %1").arg(_a ? demangled(typeid(*_a).name()).c_str() : "(None)"));
		m_toBeAnalyzed.insert(_a);
		resumeWork();
	}
}

void Noted::on_actFollow_changed()
{
}

void Noted::on_actOpen_activated()
{
	QString s = QFileDialog::getOpenFileName(this, "Open an audio file", QDir::homePath(), "Microsoft Wave (*.wav *.WAV);;SGI/Apple (*.AIFF *.AIFC *.aiff *.aifc);;Sun/DEC/NeXT (*.AU *.SND *.au *.snd);;Paris Audio File (*.PAF *.paf);;Commodore Amiga (*.IFF *.SVX *.iff *.svx);;Sphere Nist (*.NIST *.nist);;IRCAM (*.SF *.sf);;Creative (*.VOC *.voc);;Soundforge (*.W64 *.w64);;GNU Octave 2.0 (*.MAT4 *.mat4);;GNU Octave 2.1 (*.MAT5 *.mat5);;Portable Voice Format (*.PVF *.pvf);;Fasttracker 2 (*.XI *.xi);;HMM Tool Kit (*.HTK *.htk);;Apple CAF (*.CAF *.caf);;Sound Designer II (*.SD2 *.sd2);;Free Lossless Audio Codec (*.FLAC *.flac);;Ogg Vorbis (*.OGG *.ogg)");
	if (!s.isNull())
		setAudio(s);
}

void Noted::setCursor(qint64 _c)
{
	if (m_fineCursor != _c)
	{
		Time oc = m_fineCursor;
		m_fineCursor = _c;
		if (oc / hop() * hop() != m_fineCursor / hop() * hop())
			m_cursorDirty = true;
	}
}

void Noted::on_actPlay_changed()
{
	if (ui->actPlay->isChecked())
	{
		ui->dockPlay->setEnabled(false);
		ui->actOpen->setEnabled(false);
		if (!m_playbackThread->isRunning())
			m_playbackThread->start(QThread::TimeCriticalPriority);
	}
	else
	{
		ui->dockPlay->setEnabled(true);
		ui->actOpen->setEnabled(true);
	}
}

// _dest[0] = _dest[1] = _source[0];
// _dest[2] = _dest[3] = _source[1];
// ...
// _dest[2*(n-1)] = _dest[2*(n-1)+1] = _source[n-1];
template <class _T>
void valfan2(_T* _dest, _T const* _source, unsigned _n)
{
	unsigned nLess4 = _n - 4;
	if (_dest + 2 * _n < _source || _source + _n < _dest)
	{
		// separate
		unsigned i = 0;
		for (; i < nLess4; i += 4)
		{
			_dest[2 * i] = _dest[2 * (i + 0) + 1] = _source[i];
			_dest[2 * (i + 1)] = _dest[2 * (i + 1) + 1] = _source[i + 1];
			_dest[2 * (i + 2)] = _dest[2 * (i + 2) + 1] = _source[i + 2];
			_dest[2 * (i + 3)] = _dest[2 * (i + 3) + 1] = _source[i + 3];
		}
		for (; i < _n; ++i)
			_dest[2 * i] = _dest[2 * i + 1] = _source[i];
	}
	else
	{
		// overlapping; assert we're the same - we can't handle any other situation.
		assert(_dest == _source);
		// do it backwards.
		int i = _n - 4;
		for (; i >= 0; i -= 4)
		{
			_dest[2 * (i + 3)] = _dest[2 * (i + 3) + 1] = _source[i + 3];
			_dest[2 * (i + 2)] = _dest[2 * (i + 2) + 1] = _source[i + 2];
			_dest[2 * (i + 1)] = _dest[2 * (i + 1) + 1] = _source[i + 1];
			_dest[2 * i] = _dest[2 * (i + 0) + 1] = _source[i];
		}
		for (; i >= 0; --i)
			_dest[2 * i] = _dest[2 * i + 1] = _source[i];
	}
}

bool Noted::serviceAudio()
{
	if (ui->actPlay->isChecked())
	{
		if (!m_playback)
		{
			try {
				int rate = -1;														// Default device rate.
				if (ui->playRate->currentIndex() == ui->playRate->count() - 1)		// Working rate.
					rate = m_rate;
				else if (ui->playRate->currentIndex() < ui->playRate->count() - 2)	// Specified rate.
					ui->playRate->currentText().section(' ', 0, 0).toInt();
				m_playback = shared_ptr<Audio::Playback>(new Audio::Playback(ui->playDevice->itemData(ui->playDevice->currentIndex()).toInt(), 2, rate, ui->playChunkSamples->value(), (ui->playChunks->value() == 2) ? -1 : ui->playChunks->value(), ui->force16Bit->isChecked()));
			} catch (...) {}
		}
		if (m_playback)
		{
			unsigned f = m_playback->frames();
			unsigned r = m_playback->rate();
			vector<float> output(f * m_playback->channels());
			if (m_fineCursor >= 0 && m_fineCursor < duration())
			{
				if (m_rate == m_playback->rate())
				{
					// no resampling necessary
					waveBlock(m_fineCursor, toBase(f, r), &output);
				}
				else
				{
					vector<float> source(f);
					double factor = double(m_playback->rate()) / m_rate;
					if (m_fineCursorWas != m_fineCursor || !m_resampler)
					{
						// restart resampling.
						if (m_resampler)
							resample_close(m_resampler);
						m_resampler = resample_open(1, factor, factor);
						m_nextResample = m_fineCursor;
					}
					m_fineCursorWas = m_fineCursor + toBase(m_playback->frames(), m_playback->rate());

					unsigned outPos = 0;
					int used = 0;
					// Try our luck without going to the expensive waveBlock call first.
					outPos += resample_process(m_resampler, factor, nullptr, 0, 0, &used, &(output[outPos]), f - outPos);
					while (outPos != f)
					{
						// At end of current (input) buffer - refill and reset position.
						waveBlock(m_nextResample, toBase(f, m_rate), &source);
						outPos += resample_process(m_resampler, factor, &(source[0]), f, 0, &used, &(output[outPos]), f - outPos);
						m_nextResample += toBase(used, m_rate);
					}
					for (float& f: output)
						f = clamp(f, -1.f, 1.f);
				}
				if (m_playback->isInterleaved())
					valfan2(&(output[0]), &(output[0]), f);
				else
					valcpy(&(output[f]), &(output[0]), f);
			}
			m_playback->write(output);
			setCursor(m_fineCursor + toBase(f, r));	// might be different to m_fineCursorWas...
			return true;
		}
	}
	else
	{
		if (m_playback)
			m_playback.reset();
		if (m_resampler)
		{
			resample_close(m_resampler);
			m_resampler = nullptr;
		}
	}
	return false;
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

void Noted::on_actZoomOut_activated()
{
	Time centre = m_timelineOffset + m_pixelDuration * activeWidth() / 2;
	setPixelDuration(m_pixelDuration *= 1.2);
	setTimelineOffset(centre - m_pixelDuration * activeWidth() / 2);
}

void Noted::on_actZoomIn_activated()
{
	Time centre = m_timelineOffset + m_pixelDuration * activeWidth() / 2;
	setPixelDuration(m_pixelDuration /= 1.2);
	setTimelineOffset(centre - m_pixelDuration * activeWidth() / 2);
}

void Noted::on_actPanBack_activated()
{
	setTimelineOffset(m_timelineOffset - m_pixelDuration * activeWidth() / 4);
}

void Noted::on_actPanForward_activated()
{
	setTimelineOffset(m_timelineOffset + m_pixelDuration * activeWidth() / 4);
}

void Noted::on_actPanic_activated()
{
	ui->actPlay->setChecked(false);
}

bool Noted::carryOn(int _progress)
{
	WorkerThread::setCurrentProgress(_progress);
	return !WorkerThread::quitting();
}

void Noted::setAudio(QString const& _filename)
{
	if (ui->actPlay->isChecked())
		ui->actPlay->setChecked(false);
	suspendWork();

	m_fineCursor = 0;
	m_timelineOffset = 0;
	m_pixelDuration = 1;

	emit analysisFinished();

	m_sourceFileName = _filename;
	if (!QFile(m_sourceFileName).open(QFile::ReadOnly))
		m_sourceFileName.clear();

	noteLastValidIs(nullptr);
	updateWindowTitle();
	resumeWork();
}

void Noted::newDataView(QString const& _n)
{
	QDockWidget* dw = new QDockWidget(_n, this);
	dw->setObjectName(_n + "Dock");
	DataView* dv = new DataView(dw, _n);
	dw->setAllowedAreas(Qt::AllDockWidgetAreas);
	addDockWidget(Qt::BottomDockWidgetArea, dw, Qt::Horizontal);
	dw->setFeatures(dw->features() | QDockWidget::DockWidgetVerticalTitleBar);
	dw->setWidget(dv);
	connect(dw, SIGNAL(visibilityChanged(bool)), this, SLOT(onDataViewDockClosed()));
	dw->show();
}

void Noted::updateGraphs(vector<shared_ptr<AuxGraphsSpec> > const& _specs)
{
	foreach (DataView* dv, findChildren<DataView*>())
		dv->setEnabled(false);

	foreach (shared_ptr<AuxGraphsSpec> const& s, _specs)
	{
		QString n = QString::fromStdString(s->name);
		if (DataView* dv = findChild<DataView*>(n))
		{
			dv->rejig();
			dv->setEnabled(true);
		}
		else
			newDataView(n);
	}
}

void Noted::onDataViewDockClosed()
{
	foreach (DataView* dv, findChildren<DataView*>())
		if (!dv->isEnabled() && dv->parentWidget()->isHidden())
			dv->parentWidget()->deleteLater();
}

void Noted::updateEventStuff()
{
	vector< shared_ptr<AuxGraphsSpec> > gspecs;
	foreach (auto e, initEventsOf(GraphSpecComment))
		gspecs.push_back(dynamic_pointer_cast<AuxGraphsSpec>(e.aux()));
	updateGraphs(gspecs);

	foreach (EventsView* ev, eventsViews())
		ev->updateEventTypes();
}

void Noted::timerEvent(QTimerEvent*)
{
	static int i = 0;
	if (++i % 10 == 0)
	{
		QProgressBar* pb = ui->statusBar->findChild<QProgressBar*>();
		if (m_workerThread->progress() < 100)
		{
			if (!pb)
			{
				pb = new QProgressBar;
				pb->setMaximum(100);
				pb->setObjectName("progress");
				pb->setMaximumWidth(128);
				ui->statusBar->addPermanentWidget(pb);
			}
			pb->setValue(m_workerThread->progress());
			statusBar()->showMessage(m_workerThread->description());
		}
		else if (pb)
			delete pb;

		if (m_workFinished)
		{
			m_workFinished = false;
			info("WORK All finished");
			emit analysisFinished();
			m_cursorDirty = true;
			if (pb)
				delete pb;
			m_workerThread->setProgress(100);
			statusBar()->showMessage("Ready");
		}

		if (m_playback)
			ui->statusBar->findChild<QLabel*>("alsa")->setText(QString("%1 %2# @ %3Hz, %4x%5 frames").arg(m_playback->deviceName().c_str()).arg(m_playback->channels()).arg(m_playback->rate()).arg(m_playback->periods()).arg(m_playback->frames()));
		else
			ui->statusBar->findChild<QLabel*>("alsa")->setText("No audio");
		{
			QMutexLocker l(&x_timelines);
			foreach (Timeline* t, m_timelines)
				if (PrerenderedTimeline* pt = dynamic_cast<PrerenderedTimeline*>(t))
					pt->updateIfNeeded();
		}
		{
			QMutexLocker l(&x_infos);
			if (m_infos.size())
			{
				bool lock = !ui->infoView->verticalScrollBar() || ui->infoView->verticalScrollBar()->value() == ui->infoView->verticalScrollBar()->maximum();
				m_info += m_infos;
				ui->infoView->setHtml(m_info);
				m_infos.clear();
				if (lock && ui->infoView->verticalScrollBar())
					ui->infoView->verticalScrollBar()->setValue(ui->infoView->verticalScrollBar()->maximum());
			}
		}
	}

	if (m_cursorDirty)
	{
		if (m_fineCursor >= duration())
		{
			// Played to end of audio
			setCursor(0);
			ui->actPlay->setChecked(false);
		}
		ui->statusBar->findChild<QLabel*>("cursor")->setText(textualTime(m_fineCursor, toBase(samples(), m_rate), 0, 0).c_str());
		m_cursorDirty = false;
		if (ui->actFollow->isChecked() && (m_fineCursor < earliestVisible() || m_fineCursor > earliestVisible() + visibleDuration() * 7 / 8))
			setTimelineOffset(m_fineCursor - visibleDuration() / 8);
		emit cursorChanged();
	}
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

bool eventVisible(QVariant const& _v, Lightbox::StreamEvent const& _e)
{
	return (QMetaType::Type(_v.type()) == QMetaType::Float && _e.nature == _v.toFloat() && _e.type >= Lightbox::Graph)
			|| (_v.type() == QVariant::Int && _v.toInt() == int(_e.type))
			|| (_v.type() == QVariant::Bool && (_e.type >= Lightbox::Graph) == (_v.toBool()))
			|| !_v.isValid();
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

bool Noted::cursorEvent(QEvent* _e, int _i)
{
	if (QMouseEvent* me = dynamic_cast<QMouseEvent*>(_e))
	{
		m_cursorDirty = true;
		QMouseEvent nme(me->type(), (ui->dataDisplay->mapFromGlobal(me->globalPos())), me->button(), me->buttons(), me->modifiers());
		return _i ? ui->overview->event(&nme) : ui->waveform->event(&nme);
	}
	else if (QWheelEvent* me = dynamic_cast<QWheelEvent*>(_e))
	{
		m_cursorDirty = true;
		QWheelEvent nme(ui->dataDisplay->mapFromGlobal(me->globalPos()), me->delta(), me->buttons(), me->modifiers(), me->orientation());
		return _i ? ui->overview->event(&nme) : ui->waveform->event(&nme);
	}
	return false;
}

// TODO: cursors of _id==0 should be per-Timeline, not handled by Noted at all.

pair<QRect, QRect> Noted::cursorGeoOffset(int _id) const
{
	QWidget* mthis = const_cast<QWidget*>((QWidget const*)this);
	QRect geo;		// where the cursor window will be placed on the screen.
	QRect canvas;	// the offset
	if (_id == 0)
	{
		// quite arbitrary limits placed on where the cursor may draw...
		int f = positionOf(cursor() - windowSize() + hop()) - 4;
		int t = positionOf(cursor() + hop()) + 4;

		int cf = clamp(f, 0, ui->dataDisplay->width() - 1);
		int ct = clamp(t, 0, ui->dataDisplay->width() - 1);
		geo = QRect(mapToGlobal(ui->dataDisplay->mapTo(mthis, QPoint(cf, 0))), QSize(ct - cf, ui->dataDisplay->height()));
		canvas = QRect(QPoint(-cf, 0), QSize(0, geo.height()));
	}
	else if (_id == 1)
	{
		int f = ui->overview->positionOf(earliestVisible());
		int t = ui->overview->positionOf(earliestVisible() + visibleDuration()) + 2;
		int cf = clamp(f, 0, ui->overview->width() - 1);
		int ct = clamp(t, 0, ui->overview->width() - 1);
		geo = QRect(mapToGlobal(ui->overview->mapTo(mthis, QPoint(cf, 0))), QSize(ct - cf, ui->overview->height()));
		canvas = QRect(QPoint(f - cf, 0), QSize(t - f, geo.height()));
	}
	else if (_id == 2)
	{
		int f = ui->overview->positionOf(cursor());
		int cf = clamp(f, 0, ui->overview->width() - 1);
		geo = QRect(mapToGlobal(ui->overview->mapTo(mthis, QPoint(cf, 0))), QSize(1, ui->overview->height()));
		canvas = QRect(QPoint(f - cf, 0), geo.size());
	}
	return make_pair(geo, canvas);
}

void Noted::info(QString const& _info, int _id)
{
	QString color = (_id == 255) ? "#700" : (_id == 254) ? "#007" : (_id == 253) ? "#440" : "#fff";
	QMutexLocker l(&x_infos);
	m_infos += "<div style=\"margin-top: 1px;\"><span style=\"background-color:" + color + ";\">&nbsp;</span> " + Qt::escape(_info) + "</div>";
}

void Noted::info(QString const& _info, char const* _c)
{
	QMutexLocker l(&x_infos);
	m_infos += QString("<div style=\"margin-top: 1px;\"><span style=\"background-color:%1;\">&nbsp;</span> %2</div>").arg(_c).arg(_info);
}

void Noted::paintCursor(QPainter& _p, int _id) const
{
	QRect r;
	QRect o;
	tie(r, o) = cursorGeoOffset(_id);
	_p.setCompositionMode(QPainter::CompositionMode_Source);
	_p.fillRect(QRect(0, 0, r.width(), r.height()), Qt::transparent);
	_p.setCompositionMode(QPainter::CompositionMode_SourceOver);
	_p.translate(o.topLeft());
	_p.setRenderHint(QPainter::Antialiasing, false);
	if (!r.width())
		return;
	if (_id == 0)
	{
		// already translated back to timelines' cooardinate system.
		// (cursor() - windowSize() + hop()) to hop() is guaranteed drawable.
		QMutexLocker l(&x_timelines);
		foreach (Timeline* t, m_timelines)
		{
			int y = t->widget()->y();
			if (y > -1)
			{
				int h = t->widget()->height();
				QColor cc = t->cursorColor();
				int x = positionOf(t->highlightFrom());
				int w = widthOf(t->highlightDuration());
				if (w > 2)
					_p.fillRect(x + 1, y, w - 1, h, QColor(255, 192, 127, 32));
				cc.setAlpha(128);
				_p.fillRect(x + w, y, 1, h, cc);
				cc.setAlpha(64);
				_p.fillRect(x, y, 1, h, cc);
			}
		}
	}
	else if (_id == 1)
	{
		int f = ui->overview->positionOf(earliestVisible());
		int t = ui->overview->positionOf(earliestVisible() + visibleDuration()) + 2;
		_p.fillRect(0, 0, t - f, o.height(), QColor(127, 192, 255, 128));
		_p.setPen(QColor(0, 0, 0, 128));
		_p.drawLine(0, 0, 0, o.height());
		_p.drawLine(t - f - 1, 0, t - f - 1, r.height());
	}
	else if (_id == 2)
	{
		_p.setPen(QColor(0, 0, 0, 128));
		_p.drawLine(0, 0, 0, o.height());
	}
}

void Noted::updateParameters()
{
	m_hopSamples = ui->hop->value();
	m_windowFunction = Lightbox::windowFunction(ui->windowSize->value(), WindowFunction(ui->windowFunction->currentIndex()));
	m_zeroPhase = ui->zeroPhase->isChecked();
}

void Noted::clearEventsCache()
{
	m_initEvents.clear();
	d_initEvents = true;
}

void Noted::rejigAudio()
{
	deque<AcausalAnalysisPtr> todo;	// will become a member.
	todo.push_back(nullptr);

	// OPTIMIZE: move into worker code; allow multiple workers.
	// OPTIMIZE: consider searching tree locally and completely, putting toBeAnalyzed things onto global todo, and skipping through otherwise.
	// ...keeping search local until RAAs needed to be done are found.
	while (todo.size())
	{
		AcausalAnalysisPtr aa = todo.front();
		todo.pop_front();
		if (m_toBeAnalyzed.count(aa) && aa)
		{
			WorkerThread::setCurrentDescription(demangled(typeid(*aa).name()).c_str());
			info(QString("WORKER Working on %1").arg(demangled(typeid(*aa).name()).c_str()));
			aa->go(this, 0, hops());
			info(QString("WORKER Finished %1").arg(demangled(typeid(*aa).name()).c_str()));
			if (WorkerThread::quitting())
				break;
		}
		else if (aa)
		{
			info(QString("WORKER Skipping job %1").arg(demangled(typeid(*aa).name()).c_str()));
		}
		AcausalAnalysisPtrs ripe = ripeAcausalAnalysis(aa);
		if (m_toBeAnalyzed.count(aa))
		{
			foreach (auto i, ripe)
				m_toBeAnalyzed.insert(i);
			m_toBeAnalyzed.erase(aa);
		}
		catenate(todo, ripe);
	}
}

AcausalAnalysisPtrs Noted::ripeAcausalAnalysis(AcausalAnalysisPtr const& _finished)
{
	AcausalAnalysisPtrs ret;

	if (_finished == nullptr)
		ret.push_back(m_resampleWaveAcAnalysis);
	else if (dynamic_cast<ResampleWaveAc*>(&*_finished))
		ret.push_back(m_spectraAcAnalysis);
	else if (dynamic_cast<SpectraAc*>(&*_finished))
		ret.push_back(m_compileEventsAnalysis);
	else if (dynamic_cast<CompileEvents*>(&*_finished) && eventsViews().size())
		foreach (EventsView* ev, eventsViews())
			ret.push_back(AcausalAnalysisPtr(new CompileEventsView(ev)));
	else if ((dynamic_cast<CompileEvents*>(&*_finished) && !eventsViews().size()) || (dynamic_cast<CompileEventsView*>(&*_finished) && ++m_eventsViewsDone == eventsViews().size()))
		ret.push_back(m_collateEventsAnalysis);
	else if (dynamic_cast<CollateEvents*>(&*_finished))
		ret.push_back(m_finishUpAcAnalysis);

	// Go through all other things that can give CAs; at this point, it's just the plugins
	AcausalAnalysisPtrs acc;
	foreach (LibraryPtr const& l, m_libraries)
		if (l->p && (acc = l->p->ripeAnalysis(_finished)).size())
			ret.insert(ret.end(), acc.begin(), acc.end());

	return ret;
}
