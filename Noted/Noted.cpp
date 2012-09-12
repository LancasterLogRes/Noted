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
	NotedBase					(_p),
	ui							(new Ui::Noted),
	m_workerThread				(nullptr),
	m_suspends					(0),
	m_fineCursorWas				(UndefinedTime),
	m_nextResample				(UndefinedTime),
	m_resampler					(nullptr),
	m_isCausal					(false),
	m_workFinished				(false),
	m_resampleWaveAcAnalysis	(new ResampleWaveAc),
	m_spectraAcAnalysis			(new SpectraAc),
	m_finishUpAcAnalysis		(new FinishUpAc),
	m_compileEventsAnalysis		(new CompileEvents),
	m_collateEventsAnalysis		(new CollateEvents),
	m_glMaster					(new QGLWidget),
	m_constructed				(false)
{
	g_debugPost = [&](std::string const& _s, int _id){ simpleDebugOut(_s, _id); info(_s.c_str(), _id); };

	ui->setupUi(this);
	ui->loadedLibraries->clear();
	setWindowIcon(QIcon(":/Noted.png"));

	updateAudioDevices();

	ui->waveform->installEventFilter(this);
	ui->overview->installEventFilter(this);
	ui->dataDisplay->installEventFilter(this);

	m_timelines.insert(ui->waveform);
	m_timelines.insert(ui->spectra);

	m_workerThread = createWorkerThread([=](){return work();});
	m_audioThread = createWorkerThread([=](){return serviceAudio();});

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

	on_sampleRate_currentIndexChanged(0);

	startTimer(5);
	resumeWork();

	readSettings();

	setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
	setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
	setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
	setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

	m_constructed = true;
}

Noted::~Noted()
{
	qDebug() << "Disabling playback...";
	ui->actPlay->setChecked(false);
	for (m_audioThread->quit(); !m_audioThread->wait(1000); m_audioThread->terminate()) {}
	delete m_audioThread;
	m_audioThread = nullptr;
	qDebug() << "Disabled permenantly.";

	g_debugPost = simpleDebugOut;

	qDebug() << "Disabling worker(s)...";
	suspendWork(true);
	delete m_workerThread;
	m_workerThread = nullptr;
	qDebug() << "Disabled permenantly.";

	qDebug() << "Unloading libraries...";
	while (m_libraries.size())
	{
		unload(*m_libraries.begin());
		m_libraries.erase(m_libraries.begin());
	}
	qDebug() << "Unloaded all libraries.";

	qDebug() << "Killing timelines...";
	while (m_timelines.size())
		delete *m_timelines.begin();
	qDebug() << "Killed.";

	delete ui;
	delete m_glMaster;
}

QGLWidget* Noted::glMaster() const
{
	return m_glMaster;
}

void Noted::on_actAbout_activated()
{
	QMessageBox::about(this, "About Noted!", "<h1>Noted!</h1>Copyright (c)2011, 2012 Lancaster Logic Response Limited. This code is released under version 2 of the GNU General Public Licence.");
}

void Noted::updateAudioDevices()
{
	QString pd = ui->playDevice->currentText();
	ui->playDevice->clear();
	foreach (auto d, Audio::Playback::devices())
		ui->playDevice->addItem(QString::fromStdString(d.second), d.first);
	ui->playDevice->setCurrentIndex(ui->playDevice->findText(pd));

	QString cd = ui->captureDevice->currentText();
	ui->captureDevice->clear();
	foreach (auto d, Audio::Capture::devices())
		ui->captureDevice->addItem(QString::fromStdString(d.second), d.first);
	ui->captureDevice->setCurrentIndex(ui->captureDevice->findText(cd));
}

QString defaultNick(QString const& _filename)
{
	QString ret = _filename.section('/', -1);
	QRegExp re("(lib)?(.*)\\.[a-zA-Z]*");
	if (re.exactMatch(ret))
		ret = re.cap(2);
	return ret;
}

void Noted::addLibrary(QString const& _name, bool _isEnabled)
{
	cnote << "Adding library" << _name.toLocal8Bit().data() << ".";
	if (m_libraries.contains(_name))
		cwarn << "Ignoring duplicate library" << _name.toLocal8Bit().data() << ".";
	else
	{
		cnote << "Not a duplicate - loading...";
		auto lp = make_shared<RealLibrary>(_name);
		m_libraries.insert(_name, lp);
		lp->item = new QTreeWidgetItem(ui->loadedLibraries, QStringList() << defaultNick(_name) << "Unknown" << _name);
		lp->item->setFlags(lp->item->flags() | Qt::ItemIsUserCheckable);
		lp->item->setCheckState(0, _isEnabled ? Qt::Checked : Qt::Unchecked);
		if (_isEnabled)
		{
			m_dirtyLibraries.insert(_name);
			reloadDirties();
		}
	}
}

void Noted::on_loadedLibraries_itemClicked(QTreeWidgetItem* _it, int)
{
	for (auto l: m_libraries)
		if (l->item == _it)
		{
			m_dirtyLibraries.insert(l->filename);
			break;
		}
	reloadDirties();
}

void Noted::addDockWidget(Qt::DockWidgetArea _a, QDockWidget* _d)
{
	if (_d->objectName().isEmpty())
		_d->setObjectName(_d->windowTitle());
	QMainWindow::addDockWidget(_a, _d);
}

shared_ptr<NotedPlugin> Noted::getPlugin(QString const& _mangledName)
{
	for (auto l: m_libraries)
		if (l->p && typeid(*l->p).name() == _mangledName)
			return l->p;
	return nullptr;
}

void Noted::load(RealLibraryPtr const& _dl)
{
	cnote << "Loading:" << _dl->filename;
	m_libraryWatcher.addPath(_dl->filename);
	if (QLibrary::isLibrary(_dl->filename))
	{
		_dl->nick = defaultNick(_dl->filename);
		QString tempFile = QDir::tempPath() + "/Noted[" + _dl->nick + "]" + QDateTime::currentDateTime().toString("yyyyMMdd-hh.mm.ss.zzz");
		_dl->l.setFileName(tempFile);
		_dl->l.setLoadHints(QLibrary::ResolveAllSymbolsHint);
		QFile::copy(_dl->filename, tempFile);
		if (_dl->l.load())
		{
			typedef EventCompilerFactories&(*cf_t)();
			typedef NotedPlugin*(*pf_t)(NotedFace*);
			typedef char const*(*pnf_t)();
			if (cf_t cf = (cf_t)_dl->l.resolve("eventCompilerFactories"))
			{
				_dl->item->setText(1, "Event Compilers");
				cnote << "LOAD" << _dl->nick << " [ECF]";
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
				_dl->item->setText(1, "Plugin");
				cnote << "LOAD" << _dl->nick << " [PLUGIN]";

				_dl->p = shared_ptr<NotedPlugin>(np(this));

				if (_dl->p->m_required.empty())
				{
					foreach (auto lib, m_libraries)
						if (!lib->l.isLoaded() && lib->isEnabled())
							load(lib);

					QSettings s("LancasterLogicResponse", "Noted");
					Members<NotedPlugin> props(_dl->p->propertyMap(), _dl->p, [=](std::string const&){_dl->p->onPropertiesChanged();});
					props.deserialize(s.value(_dl->nick + "/properties").toString().toStdString());
					PropertiesEditor* pe = nullptr;
					if (props.size())
					{
						QDockWidget* propsDock = new QDockWidget(QString("%1 Properties").arg(_dl->nick), this);
						propsDock->setObjectName(_dl->nick + "/properties");
						pe = new PropertiesEditor(propsDock);
						propsDock->setWidget(pe);
						propsDock->setFeatures(propsDock->features()|QDockWidget::DockWidgetVerticalTitleBar);
						addDockWidget(Qt::RightDockWidgetArea, propsDock);
						if (s.contains(_dl->nick + "/propertiesGeometry"))
							propsDock->restoreGeometry(s.value(_dl->nick + "/propertiesGeometry").toByteArray());
					}
					if (m_constructed)
					{
						readBaseSettings(s);
						_dl->p->readSettings(s);
					}
					if (pe)
						pe->setProperties(props);
				}
				else
				{
					_dl->item->setText(1, "Plugin: Requires " + _dl->p->m_required.join(" "));
					_dl->p.reset();
					_dl->unload();
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
							if (f->load(_dl))
							{
								_dl->auxFace = f;
								_dl->aux = lib->p;
								cnote << "LOAD" << _dl->nick << " [AUX:" << lib->nick << "]";
								_dl->item->setText(1, "Aux: " + lib->nick);
								goto LOADED;
							}
							else
							{
								f.reset();
								lib->p->removeDeadAuxes();
							}
						}
				cnote << "Useless library - unloading" << _dl->nick;
				_dl->unload();
				_dl->item->setText(1, "Aux: ?");
				LOADED:;
			}
		}
		else
		{
			cwarn << "ERROR on load: " << _dl->l.errorString();
		}
		_dl->item->setText(0, _dl->nick);
	}
	else if (QFile::exists(_dl->filename))
	{
		_dl->cf[_dl->filename.toStdString()] = [=](){ return new ProcessEventCompiler(_dl->filename); };
		auto li = new QListWidgetItem(_dl->filename);
		li->setData(0, _dl->filename);
		ui->eventCompilersList->addItem(li);
	}
}

bool RealLibrary::isEnabled() const
{
	return item->checkState(0) == Qt::Checked;
}

void RealLibrary::unload()
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
		qWarning() << "Couldn't get the Lightbox 'finalized' symbol in " << filename;
		assert(l.unload());
	}
	QFile::remove(l.fileName());
}

void Noted::unload(RealLibraryPtr const& _dl)
{
	if (_dl->l.isLoaded())
	{
		if (_dl->p)
		{
			// save state.
			{
				QSettings s("LancasterLogicResponse", "Noted");
				writeBaseSettings(s);
				_dl->p->writeSettings(s);

				Members<NotedPlugin> props(_dl->p->propertyMap(), _dl->p);
				s.setValue(_dl->nick + "/properties", QString::fromStdString(props.serialized()));

				// kill the properties dock if there is one.
				if (auto pw = findChild<QDockWidget*>(_dl->nick + "/properties"))
				{
					s.setValue(_dl->nick + "/propertiesGeometry", pw->saveGeometry());
					delete pw;
				}
			}

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
			_dl->item->setText(1, "Aux: ?");
			_dl->auxFace->unload(_dl);
			_dl->auxFace.reset();
			_dl->aux.lock()->removeDeadAuxes();
			_dl->aux.reset();
		}
		cnote << "UNLOAD" << _dl->filename;
		_dl->unload();
	}
	m_libraryWatcher.removePath(_dl->filename);
}

void Noted::reloadLibrary(QString const& _name)
{
	RealLibraryPtr dl = m_libraries[_name];
	assert(dl);
	assert(dl->filename == _name);

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
		suspendWork();

		// OPTIMIZE: only bother saving for EVs whose EC is given by a dirty library.
		foreach (EventsView* ev, eventsViews())
			ev->save();

		foreach (QString const& name, m_dirtyLibraries)
		{
			bool load = false;
			bool kill = true;
			for (auto i: ui->loadedLibraries->findItems(name, Qt::MatchExactly, 2))
			{
				kill = false;
				load = (i->checkState(0) == Qt::Checked);
				break;
			}

			if (load)
				reloadLibrary(name);
			else
			{
				unload(m_libraries[name]);
				if (kill)
					m_libraries.remove(name);
			}
		}

		foreach (EventsView* ev, eventsViews())
			ev->restore();

		m_dirtyLibraries.clear();

		// OPTIMIZE: Something finer grained?
		noteEventCompilersChanged();

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

vector<float> Noted::graphEvents(float _temperature) const
{
	// OPTIMIZE: memoize.
	foreach (EventsView* ev, eventsViews())
	{
		vector<float> ret = ev->graphEvents(_temperature);
		if (ret.size())
			return ret;
	}
	return vector<float>();
}

StreamEvent Noted::eventOf(EventType _et, float _temperature, Time _t) const
{
	if (_t == UndefinedTime)
		_t = cursor();
	StreamEvent ret(NoEvent);
	if (_t < duration() && _et != NoEvent)
	{
		bool careAboutNature = isFinite(_temperature);
		auto evs = eventsViews();
//		foreach (EventsView* ev, evs)
//			ev->mutex()->lock();
		for (int i = windowIndex(_t); i >= 0; --i)
			foreach (EventsView* ev, evs)
				foreach (StreamEvent const& e, ev->events(i))
					if (e.type == _et && (e.temperature == _temperature || !careAboutNature))
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

StreamEvents Noted::initEventsOf(EventType _et, float _temperature) const
{
	if (d_initEvents)
	{
		d_initEvents = false;
		foreach (EventsView* ev, eventsViews())
			catenate(m_initEvents, ev->initEvents());
	}
	StreamEvents ret;
	bool careAboutNature = isFinite(_temperature);
	foreach (StreamEvent const& e, m_initEvents)
		if (e.type == _et && (e.temperature == _temperature || !careAboutNature))
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
		QString s = ui->loadedLibraries->currentItem()->text(2);
		delete ui->loadedLibraries->currentItem();
		m_dirtyLibraries.insert(s);
		reloadDirties();
	}
}

void Noted::on_actReadSettings_activated()
{
	QSettings s("LancasterLogicResponse", "Noted");
	for (auto d: findChildren<QDockWidget*>())
		cdebug << d->objectName();
	readBaseSettings(s);
}

void Noted::on_actWriteSettings_activated()
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
	if (settings.contains("libraryCount"))
		for (int i = 0; i < settings.value("libraryCount").toInt(); ++i)
			addLibrary(settings.value(QString("library%1").arg(i)).toString(), settings.value(QString("library%1.enabled").arg(i)).toBool());
	else if (settings.contains("libraries"))
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

/*	foreach (auto l, m_libraries)
		if (l->p)
			l->p->writeSettings(settings);*/

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

	int lc = 0;
	for (auto l: m_libraries)
	{
		settings.setValue(QString("library%1").arg(lc), l->filename);
		settings.setValue(QString("library%1.enabled").arg(lc), l->isEnabled());
		++lc;
	}
	settings.setValue("libraryCount", lc);

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
	DO(captureChunks, value);
	DO(captureDevice, currentIndex);
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

void Noted::suspendWork(bool _force)
{
	cnote << "WORK Suspending..." << m_suspends;
	if (m_workerThread && m_workerThread->isRunning())
	{
		m_workerThread->quit();
		while (!m_workerThread->wait(1000))
			cwarn << "Worker thread not responding :-(";
		m_suspends = 0;
		cnote << "WORK Suspended";
	}
	else
	{
		++m_suspends;
		cnote << "WORK Additional suspended" << m_suspends;
	}
}

void Noted::resumeWork(bool _force)
{
	if (!_force && m_suspends)
	{
		m_suspends--;
		cnote << "WORK One fewer suspend" << m_suspends;
	}
	else
	{
		if (m_workerThread && !m_workerThread->isRunning())
		{
			m_workerThread->start(QThread::LowPriority);
			cnote << "WORK Resumed" << m_suspends;
		}
		m_suspends = 0;
	}
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
		cnote << "WORK Last valid is now " << (_a ? _a->name().toLatin1().data() : "(None)");
		m_toBeAnalyzed.insert(_a);
		resumeWork();
	}
}

void Noted::on_actFollow_changed()
{
}

void Noted::on_actOpen_activated()
{
	QString s = QFileDialog::getOpenFileName(this, "Open an audio file", QDir::homePath(), "All Audio files (*.wav *.WAV *.aiff *.AIFF *.aifc *.AIFC *.au *.AU *.snd *.SND *.nist *.NIST *.iff *.IFF *.svx *.SVX *.paf *.PAF *.w64 *.W64 *.voc *.VOC *.sf *.SF *.caf *.CAF *.htk *.HTK *.xi *.XI *.pvf *.PVF *.mat5 *.mat4 *.MAT5 *.MAT4 *.sd2 *.SD2 *.flac *.FLAC *.ogg *.OGG );;Microsoft Wave (*.wav *.WAV);;SGI/Apple (*.AIFF *.AIFC *.aiff *.aifc);;Sun/DEC/NeXT (*.AU *.SND *.au *.snd);;Paris Audio File (*.PAF *.paf);;Commodore Amiga (*.IFF *.SVX *.iff *.svx);;Sphere Nist (*.NIST *.nist);;IRCAM (*.SF *.sf);;Creative (*.VOC *.voc);;Soundforge (*.W64 *.w64);;GNU Octave 2.0 (*.MAT4 *.mat4);;GNU Octave 2.1 (*.MAT5 *.mat5);;Portable Voice Format (*.PVF *.pvf);;Fasttracker 2 (*.XI *.xi);;HMM Tool Kit (*.HTK *.htk);;Apple CAF (*.CAF *.caf);;Sound Designer II (*.SD2 *.sd2);;Free Lossless Audio Codec (*.FLAC *.flac);;Ogg Vorbis (*.OGG *.ogg)");
	if (!s.isNull())
		setAudio(s);
}

void Noted::setCursor(qint64 _c, bool _warp)
{
	if (m_fineCursor != _c)
	{
		Time oc = m_fineCursor;
		m_fineCursor = _c;
		if (oc / hop() * hop() != m_fineCursor / hop() * hop())
			m_cursorDirty = true;
		if (_warp && isCausal())
			m_lastIndex = cursorIndex();
	}
}

// start playing (acausally - with the pre generated data)
void Noted::on_actPlay_changed()
{
	if (ui->actPlay->isChecked())
	{
		m_isCausal = false;
		m_causalCursorIndex = -1;
		ui->dockPlay->setEnabled(false);
		ui->actPlayCausal->setEnabled(false);
		ui->actOpen->setEnabled(false);
		if (!m_audioThread->isRunning())
			m_audioThread->start(QThread::TimeCriticalPriority);
	}
	else
	{
		ui->dockPlay->setEnabled(true);
		ui->actPlayCausal->setEnabled(true);
		ui->actOpen->setEnabled(true);
	}
}

void Noted::on_actPlayCausal_changed()
{
	if (ui->actPlayCausal->isChecked())
	{
		suspendWork();
		initializeCausal(nullptr);
		m_isCausal = true;
		m_causalCursorIndex = 0;
		ui->dockPlay->setEnabled(false);
		ui->actPlay->setEnabled(false);
		ui->actOpen->setEnabled(false);
		if (!m_audioThread->isRunning())
			m_audioThread->start(QThread::TimeCriticalPriority);
		m_lastIndex = cursorIndex();
	}
	else
	{
		finalizeCausal();
		m_isCausal = false;
		m_causalCursorIndex = -1;
		ui->dockPlay->setEnabled(true);
		ui->actPlay->setEnabled(true);
		ui->actOpen->setEnabled(true);
		resumeWork();
	}
}

void Noted::on_actPassthrough_changed()
{
	if (ui->actPassthrough->isChecked())
	{
		suspendWork();
		initializeCausal(nullptr);
		m_isCausal = false;
		m_causalCursorIndex = -1;
		ui->dockPlay->setEnabled(false);
		ui->actPlay->setEnabled(false);
		ui->actPlayCausal->setEnabled(false);
		ui->actOpen->setEnabled(false);
		if (!m_audioThread->isRunning())
			m_audioThread->start(QThread::TimeCriticalPriority);
		m_lastIndex = cursorIndex();
	}
	else
	{
		finalizeCausal();
		ui->dockPlay->setEnabled(true);
		ui->actPlay->setEnabled(true);
		ui->actPlayCausal->setEnabled(true);
		ui->actOpen->setEnabled(true);
		resumeWork();
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
	if (ui->actPlay->isChecked() || ui->actPlayCausal->isChecked())
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
		}
	}
	else if (m_playback)
	{
		if (m_playback)
			m_playback.reset();
		if (m_resampler)
		{
			resample_close(m_resampler);
			m_resampler = nullptr;
		}
		return false;
	}

	if (ui->actPassthrough->isChecked())
	{
		if (!m_capture)
		{
			try {
				m_capture = shared_ptr<Audio::Capture>(new Audio::Capture(ui->captureDevice->itemData(ui->captureDevice->currentIndex()).toInt(), 1, m_rate, hopSamples(), (ui->captureChunks->value() == 2) ? -1 : ui->captureChunks->value()));
			} catch (...) {}
			m_fftw = shared_ptr<FFTW>(new FFTW(windowSizeSamples()));
			m_currentWave = vector<float>(windowSizeSamples(), 0);
			m_currentMagSpectrum = vector<float>(spectrumSize(), 0);
			m_currentPhaseSpectrum = vector<float>(spectrumSize(), 0);
		}
		if (m_capture)
		{
			// pull out another chunk, rotate m_currentWave hopSamples
			memmove(m_currentWave.data(), m_currentWave.data() + hopSamples(), (windowSizeSamples() - hopSamples()) * sizeof(float));
			m_capture->read(foreign_vector<float>(m_currentWave.data() + windowSizeSamples() - hopSamples(), hopSamples()));

			float* b = m_fftw->in();
			foreign_vector<float> win(&m_currentWave);
			assert(win.data());
			float* d = win.data();
			float* w = m_windowFunction.data();
			unsigned off = m_zeroPhase ? m_windowFunction.size() / 2 : 0;
			for (unsigned j = 0; j < m_windowFunction.size(); ++d, ++j, ++w)
				b[(j + off) % m_windowFunction.size()] = *d * *w;
			m_fftw->process();

			m_currentMagSpectrum = m_fftw->mag();
			m_currentPhaseSpectrum = m_fftw->phase();
			/*
			float const* phase = fftw.phase().data();
			float intpart;
			for (int i = 0; i < ss; ++i)
			{
				sd[i + ss] = phase[i] / TwoPi;
				sd[i + ss2] = modf((phase[i] - lp[i]) / TwoPi + 1.f, &intpart);
			}
			*/

			// update
			updateCausal(m_lastIndex++, 1);
		}
	}
	else if (m_capture)
	{
		if (m_capture)
			m_capture.reset();
		if (m_resampler)
		{
			resample_close(m_resampler);
			m_resampler = nullptr;
		}
		return false;
	}

	if (ui->actPlayCausal->isChecked() && m_lastIndex != (int)cursorIndex())
	{
		// do events until cursor.
		if (!((int)cursorIndex() < m_lastIndex || (int)cursorIndex() - m_lastIndex > 100))	// probably skipped.
			updateCausal(m_lastIndex + 1, cursorIndex() - m_lastIndex);
		m_lastIndex = cursorIndex();
	}

	return true;
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
	zoomTimeline((cursor() > earliestVisible() && cursor() < latestVisible()) ? positionOf(cursor()) : (activeWidth() / 2), 1.2);
}

void Noted::on_actZoomIn_activated()
{
	zoomTimeline((cursor() > earliestVisible() && cursor() < latestVisible()) ? positionOf(cursor()) : (activeWidth() / 2), 1 / 1.2);
}

void Noted::on_actPanBack_activated()
{
	setTimelineOffset(m_timelineOffset - visibleDuration() / 4);
}

void Noted::on_actPanForward_activated()
{
	setTimelineOffset(m_timelineOffset + visibleDuration() / 4);
}

void Noted::on_actPanic_activated()
{
	ui->actPlay->setChecked(false);
	ui->actPlayCausal->setChecked(false);
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
	QMainWindow::addDockWidget(Qt::BottomDockWidgetArea, dw, Qt::Horizontal);
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
				pb->setMaximumHeight(17);
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
			emit analysisFinished();
			info("WORK All finished");
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
	return (QMetaType::Type(_v.type()) == QMetaType::Float && _e.temperature == _v.toFloat() && _e.type >= Lightbox::Graph)
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

void Noted::info(QString const& _info, int _id)
{
	QString color = (_id == 255) ? "#700" : (_id == 254) ? "#007" : (_id == 253) ? "#440" : "#fff";
	QMutexLocker l(&x_infos);
	m_infos += "<div style=\"margin-top: 1px;\"><span style=\"background-color:" + color + ";\">&nbsp;</span> " + Qt::escape(_info) + "</div>";
}

void Noted::info(QString const& _info, QString const& _c)
{
	QMutexLocker l(&x_infos);
	m_infos += QString("<div style=\"margin-top: 1px;\"><span style=\"background-color:%1;\">&nbsp;</span> %2</div>").arg(_c).arg(_info);
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
	auto wasToBeAnalysed = m_toBeAnalyzed;
	m_toBeAnalyzed.clear();
	if (wasToBeAnalysed.size())
	{
		auto yetToBeAnalysed = wasToBeAnalysed;
		deque<AcausalAnalysisPtr> todo;	// will become a member.
		todo.push_back(nullptr);

		// OPTIMIZE: move into worker code; allow multiple workers.
		// OPTIMIZE: consider searching tree locally and completely, putting toBeAnalyzed things onto global todo, and skipping through otherwise.
		// ...keeping search local until RAAs needed to be done are found.
		while (todo.size())
		{
			AcausalAnalysisPtr aa = todo.front();
			todo.pop_front();
			if (yetToBeAnalysed.count(aa) && aa)
			{
				WorkerThread::setCurrentDescription(aa->name());
				cnote << "WORKER Working on " << aa->name().toStdString();
				aa->go(this, 0, hops());
				cnote << "WORKER Finished " << aa->name().toStdString();
				if (WorkerThread::quitting())
				{
					for (auto i: wasToBeAnalysed)
						m_toBeAnalyzed.insert(i);
					break;
				}
			}
			else if (aa)
			{
				cnote << "WORKER Skipping job " << aa->name().toStdString();
			}
			AcausalAnalysisPtrs ripe = ripeAcausalAnalysis(aa);
			if (yetToBeAnalysed.count(aa))
			{
				foreach (auto i, ripe)
					yetToBeAnalysed.insert(i);
				yetToBeAnalysed.erase(aa);
			}
			catenate(todo, ripe);
		}
		if (!WorkerThread::quitting() /*&& all other threads finished*/)
			m_finishUpAcAnalysis->go(this, 0, hops());
	}
}

CausalAnalysisPtrs Noted::ripeCausalAnalysis(CausalAnalysisPtr const& _finished)
{
	CausalAnalysisPtrs ret;
	if (_finished == nullptr)
		ret.push_back(m_compileEventsAnalysis);
	else if (dynamic_cast<CompileEvents*>(&*_finished) && eventsViews().size())
		foreach (EventsView* ev, eventsViews())
			ret.push_back(CausalAnalysisPtr(new CompileEventsView(ev)));
	else if ((dynamic_cast<CompileEvents*>(&*_finished) && !eventsViews().size()) || (dynamic_cast<CompileEventsView*>(&*_finished) && ++m_eventsViewsDone == eventsViews().size()))
		ret.push_back(m_collateEventsAnalysis);

	// Go through all other things that can give CAs; at this point, it's just the plugins
	CausalAnalysisPtrs acc;
	foreach (RealLibraryPtr const& l, m_libraries)
		if (l->p && (acc = l->p->ripeCausalAnalysis(_finished)).size())
			ret.insert(ret.end(), acc.begin(), acc.end());

	return ret;
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

	// Go through all other things that can give CAs; at this point, it's just the plugins
	AcausalAnalysisPtrs acc;
	foreach (RealLibraryPtr const& l, m_libraries)
		if (l->p && (acc = l->p->ripeAcausalAnalysis(_finished)).size())
			ret.insert(ret.end(), acc.begin(), acc.end());

	return ret;
}

void Noted::initializeCausal(CausalAnalysisPtr const& _lastComplete)
{
	deque<CausalAnalysisPtr> todo;	// will become a member.
	todo.push_back(_lastComplete);
	assert(m_causalQueue.empty());

	while (todo.size())
	{
		CausalAnalysisPtr ca = todo.front();
		todo.pop_front();
		if (ca != _lastComplete)
		{
			ca->init(this, false);

			m_causalQueue.push_back(ca);
		}
		catenate(todo, ripeCausalAnalysis(ca));
	}
	m_sequenceIndex = 0;
}

void Noted::finalizeCausal()
{
	for (auto ca: m_causalQueue)
		ca->fini(false);
	m_causalQueue.clear();
}

void Noted::updateCausal(int _from, int _count)
{
	(void)_from;
	Time h = hop();
	for (auto ca: m_causalQueue)
		ca->noteBatch(m_sequenceIndex, _count);
	for (int i = 0; i < _count; ++i, ++m_sequenceIndex)
	{
		if (m_isCausal)
			m_causalCursorIndex = clamp(_from + i, 0, (int)hops());
		for (auto ca: m_causalQueue)
			ca->process(m_sequenceIndex, h * m_sequenceIndex);
	}
}

foreign_vector<float> Noted::cursorWaveWindow() const
{
	if (isCausal())
		return waveWindow(m_causalCursorIndex);
	else if (isPassing())
		return foreign_vector<float>((vector<float>*)&m_currentWave);
	else
		return waveWindow(cursorIndex()); // NOTE: only approximate - no good for Analysers.
}

foreign_vector<float> Noted::cursorMagSpectrum() const
{
	if (isCausal())
		return magSpectrum(m_causalCursorIndex, 1);
	else if (isPassing())
		return foreign_vector<float>((vector<float>*)&m_currentMagSpectrum);
	else
		return magSpectrum(cursorIndex(), 1); // NOTE: only approximate - no good for Analysers.
}

foreign_vector<float> Noted::cursorPhaseSpectrum() const
{
	if (isCausal())
		return phaseSpectrum(m_causalCursorIndex, 1);			// FIXME: will return phase normalized to [0, 1] rather than [0, pi].
	else if (isPassing())
		return foreign_vector<float>((vector<float>*)&m_currentPhaseSpectrum);
	else
		return phaseSpectrum(cursorIndex(), 1); // NOTE: only approximate - no good for Analysers.
}
