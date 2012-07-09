/* BEGIN COPYRIGHT
 *
 * This file is part of Noted.
 *
 * Copyright Â©2011, 2012, Lancaster Logic Response Limited.
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
#include <QDebug>
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
#include "Cursor.h"
#include "CompileEvents.h"
#include "CollateEvents.h"
#include "CompileEventsView.h"
#include "Noted.h"
#include "NotedGLWidget.h"
#include "ui_Noted.h"

using namespace std;
using namespace Lightbox;

template <class _T, class _U>
void catenate(_T& _target, _U const& _extra)
{
	foreach (auto i, _extra)
		_target.push_back(i);
}

Noted::Noted(QWidget* _p) :
	NotedBase				(_p),
	ui						(new Ui::Noted),
	m_dataStatus			(Dirty),
	m_fineCursor			(0),
	m_offset				(0),
	m_duration				(1),
	m_workerThread			(nullptr),
	m_compileEventsAnalysis	(new CompileEvents),
	m_collateEventsAnalysis	(new CollateEvents)
{
	ui->setupUi(this);

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
	m_alsaThread = createWorkerThread([=](){return serviceAudio();});

	{
		QLabel* l = new QLabel("No audio");
		l->setObjectName("alsa");
		l->setMinimumWidth(220);
		ui->statusBar->addPermanentWidget(l);
	}

	{
		QLabel* l = new QLabel;
		l->setObjectName("cursor");
		l->setAlignment(Qt::AlignRight);
		l->setMinimumWidth(80);
		ui->statusBar->addPermanentWidget(l);
	}

	connect(&m_libraryWatcher, SIGNAL(fileChanged(QString)), this, SLOT(onLibraryChange(QString)));
	connect(ui->dataToExport, SIGNAL(currentIndexChanged(int)), this, SIGNAL(eventsChanged()));

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
	for (m_alsaThread->quit(); !m_alsaThread->wait(1000); m_alsaThread->terminate()) {}
	delete m_alsaThread;

	delete ui;
}

void Noted::on_actAbout_activated()
{
    QMessageBox::about(this, "About Noted!", "<h1>Noted!</h1>Copyright ©2011, 2012 Lancaster Logic Response Limited. This code is released under version 2 of the GNU General Public Licence.");
}

void Noted::updateAudioDevices()
{
	int pd = ui->playDevice->currentIndex();
	ui->playDevice->clear();
	foreach (auto d, Audio::Playback::devices())
		ui->playDevice->addItem(QString::fromStdString(d.second), d.first);
	ui->playDevice->setCurrentIndex(pd);
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
	qDebug() << "Loading:" << _dl->name;
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
			if (cf_t cf = (cf_t)_dl->l.resolve("eventCompilerFactories"))
			{
				qDebug() << "LOAD" << _dl->name << " [ECF]";
				_dl->cf = cf();
				foreach (auto f, _dl->cf)
				{
					auto li = new QListWidgetItem(QString::fromStdString(f.first));
					li->setData(0, QString::fromStdString(f.first));
					ui->eventCompilersList->addItem(li);
					noteEventCompilersChanged();
				}
				qDebug() << _dl->cf.size() << " event compiler factories";
			}
			else if (pf_t np = (pf_t)_dl->l.resolve("newPlugin"))
			{
				qDebug() << "LOAD" << _dl->name << " [PLUGIN]";
				_dl->p = shared_ptr<NotedPlugin>(np(this));

				foreach (auto lib, m_libraries)
					if (!lib->l.isLoaded())
						load(lib);

				// load state.
				QSettings s("LancasterLogicResponse", "Noted");
				readBaseSettings(s);
				_dl->p->readSettings(s);
			}
			else
			{
				foreach (auto lib, m_libraries)
					if (lib->p)
					{
						shared_ptr<AuxLibraryFace> f = shared_ptr<AuxLibraryFace>(lib->p->newAuxLibrary());
						// tentatively add it, so the library can use it transparently.
						lib->p->m_auxLibraries.append(f);
						if (f->load(_dl->l))
						{
							_dl->auxFace = f;
							_dl->aux = lib->p;
							qDebug() << "LOAD" << _dl->name << " [AUX:" << lib->name << "]";
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
			qDebug() << "ERROR on load: " << _dl->l.errorString();
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
	if (bool** fed = (bool**)l.resolve("g_lightboxFinilized"))
	{
		bool isFinilized = false;
		*fed = &isFinilized;
		assert(l.unload());
		assert(isFinilized);
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
			foreach (auto f, _dl->cf)
			{
				m_initEvents.clear();
                qDebug() << "Killing events views...";
				foreach (EventsView* ev, eventsViews())
					if (ev->name() == QString::fromStdString(f.first))
                    {
                        qDebug() << "Killing.";
                        delete ev;
                    }
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
		qDebug() << "UNLOAD" << _dl->name;
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

		// TODO: only bother saving for EVs whose EC is given by a dirty library.
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

		// TODO: Something finer grained?
		noteEventCompilersChanged();

		if (doSuspend)
			resumeWork();
	}
}

QWidget* Noted::addGLWidget(GLView* _v, QWidget* _p)
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
	vector<float> ret;
	foreach (EventsView* ev, eventsViews())
	{
		QMutexLocker l(&ev->x_events);
		foreach (auto es, ev->m_events)
			foreach (auto e, es)
				if (e.type >= Graph && e.nature == _nature)
					ret.push_back(e.strength);
	}
	return ret;
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
		foreach (EventsView* ev, evs)
			ev->x_events.lock();
		for (int i = windowIndex(_t); i >= 0; --i)
			foreach (EventsView* ev, evs)
				if (i < ev->m_events.size())
					foreach (StreamEvent const& e, ev->m_events[i])
						if (e.type == _et && (e.nature == _nature || !careAboutNature))
						{
							ret = e;
							goto OK;
						}
		OK:
		foreach (EventsView* ev, evs)
			ev->x_events.unlock();
	}
	return ret;
}

QVector<StreamEvent> Noted::eventsOf(EventType _et, float _nature, Time _t) const
{
	if (_t == UndefinedTime)
		_t = cursor();
	QVector<StreamEvent> ret;
	if (_t < duration())
	{
		bool careAboutNature = isFinite(_nature);
		auto evs = eventsViews();
		foreach (EventsView* ev, evs)
			ev->x_events.lock();
		for (int i = windowIndex(_t); i >= 0 && ret.isEmpty(); --i)
			foreach (EventsView* ev, evs)
				if (i < ev->m_events.size())
					foreach (StreamEvent const& e, ev->m_events[i])
						if (e.type == _et && (e.nature == _nature || !careAboutNature))
							ret.push_back(e);
		foreach (EventsView* ev, evs)
			ev->x_events.unlock();
	}
	return ret;
}

QVector<StreamEvent> Noted::initEventsOf(EventType _et, float _nature) const
{
	QVector<StreamEvent> ret;
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
	QString t = (m_audioFile.isOpen() ? m_audioFile.fileName() : QString("New Recording"));
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
	QString s = ui->loadedLibraries->currentItem()->data(Qt::UserRole).toString();
	delete ui->loadedLibraries->currentItem();
	m_dirtyLibraries.insert(s);
	reloadDirties();
}

void Noted::readBaseSettings(QSettings& _s)
{
	restoreGeometry(_s.value("geometry").toByteArray());
	restoreState(_s.value("windowState").toByteArray());
	foreach (QSplitter* s, findChildren<QSplitter*>())
		s->restoreState(_s.value(s->objectName() + "State").toByteArray());
}

void Noted::writeBaseSettings(QSettings& _s)
{
	_s.setValue("geometry", saveGeometry());
	_s.setValue("windowState", saveState());
	foreach (QSplitter* s, findChildren<QSplitter*>())
		_s.setValue(s->objectName() + "State", s->saveState());
}

void Noted::readSettings()
{
	QSettings settings("LancasterLogicResponse", "Noted");
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
		DO(normalize, setChecked, toBool);
	}

	if (settings.contains("realtimeGraph"))
	{
		DO(playChunkSamples, setValue, toInt);
		DO(playChunks, setValue, toInt);
		DO(playRate, setCurrentIndex, toInt);
		DO(playDevice, setCurrentIndex, toInt);
	}
#undef DO
	if (settings.contains("eventViews"))
		foreach (QString n, settings.value("eventViews").toStringList())
		{
			EventsView* ev = new EventsView(ui->dataDisplay);
			ev->m_eventCompiler = newEventCompiler(n);
			addTimeline(ev);
		}

	setAudio(settings.value("fileName").toString());

	if (settings.contains("duration"))
	{
        m_duration = max<int>(settings.value("duration").toLongLong(), 1);
		m_offset = settings.value("offset").toLongLong();
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

	QStringList evns;
	foreach (EventsView* ev, eventsViews())
		evns.append(ev->name());
	settings.setValue("eventViews", evns);

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
	DO(normalize, isChecked);
	DO(playChunkSamples, value);
	DO(playChunks, value);
	DO(playRate, currentIndex);
	DO(playDevice, currentIndex);
#undef DO

	settings.setValue("fileName", m_audioFile.fileName());
	settings.setValue("duration", (qlonglong)m_duration);
	settings.setValue("offset", (qlonglong)m_offset);
	settings.setValue("cursor", (qlonglong)m_fineCursor);
}

void Noted::on_addEventsView_clicked()
{
	EventsView* ev = new EventsView(ui->dataDisplay);
	ev->m_eventCompiler = newEventCompiler(ui->eventCompilersList->currentItem()->data(0).toString());
	addTimeline(ev);
}

void Noted::on_actNewEvents_activated()
{
	EventsEditor* ev = new EventsEditor(ui->dataDisplay);
	addTimeline(ev);
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

void Noted::newEventsEdit(EventsView* _copy)
{
	// create and insert new QGraphicsItem-based events view from _copy.
	// copy _copy's events into it.
	EventsEditor* ev = new EventsEditor(ui->dataDisplay);
	ev->scene()->copyFrom(_copy);
	addTimeline(ev);
}

void Noted::on_exportEvents_clicked()
{
/*	QString fn = QFileDialog::getSaveFileName(this, "Save a series of events", QDir::currentPath(), "Native format (*.events);;XML format (*.xml);;CSV format (*.csv *.txt)");
	ofstream out;
	out.open(fn.toLocal8Bit(), ios::trunc);
	if (out)
	{
		Time t = 0;
		QVariant v = ui->dataToExport->itemData(ui->dataToExport->currentIndex());
		foreach (StreamEvents se, m_events)
		{
			int timeout = 0;
			foreach (StreamEvent e, se)
				if (eventVisible(v, e))
				{
					if (!timeout++)
					{
						if (fn.endsWith(".xml"))
							out << QString("<events time=\"%1\">").arg(toSeconds(t)).toStdString() << endl;
						else if (!fn.endsWith(".events"))
							out << toSeconds(t);
					}
					if (fn.endsWith(".events"))
						out << t << " " << (int)e.type << " " << e.strength << " " << e.nature << endl;
					else if (fn.endsWith(".xml"))
						out << QString("<event type=\"%1\" strength=\"%2\" nature=\"%3\" />").arg(toString(e.type).c_str()).arg(e.strength).arg(e.nature).arg(e.subNature).toStdString() << endl;
					else
				}
						out << " " << e.strength;
			if (timeout)
			{
				if (fn.endsWith(".xml"))
					out << QString("</events>").toStdString() << endl;
				else if (!fn.endsWith(".events"))
					out << endl;
			}
			t += hop();
		}
	}*/
}

void Noted::on_sampleRate_currentIndexChanged(int)
{
	m_rate = ui->sampleRate->currentText().left(ui->sampleRate->currentText().size() - 3).toInt();
	ui->hopPeriod->setText(QString("%1ms %2%").arg(fromBase(toBase(ui->hop->value(), rate()), 100000) / 100.0).arg(((ui->windowSize->value() - ui->hop->value()) * 1000 / ui->windowSize->value()) / 10.0));
	ui->windowPeriod->setText(QString("%1ms %2Hz").arg(fromBase(toBase(ui->windowSize->value(), rate()), 100000) / 100.0).arg((rate() * 10 / ui->windowSize->value()) / 10.0));
	noteDataChanged(Dirty);
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

bool Noted::work()
{
	if (m_dataStatus < Fresh)
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
		noteDataChanged(Dirty);
        m_workerThread->quit();
        if (!m_workerThread->wait(5000))
        {
            m_workerThread->terminate();
            qWarning() << "Worker thread not responding. Terminating. If it had a lock, everything will break.";
        }
	}
}

void Noted::resumeWork()
{
	if (m_workerThread)
		m_workerThread->start(QThread::LowPriority);
}

Lightbox::StreamEvents Noted::eventsAt(int _index) const
{
	StreamEvents ses;
	QMutexLocker l(&x_timelines);
	foreach (Timeline* i, m_timelines)
		if (EventsStore* es = dynamic_cast<EventsStore*>(i))
			merge(ses, es->events(_index));
	return ses;
}

vector<EventsStore*> Noted::eventsStores() const
{
	vector<EventsStore*> ret;
	QMutexLocker l(&x_timelines);
	foreach (Timeline* i, m_timelines)
		if (EventsStore* es = dynamic_cast<EventsStore*>(i))
			ret.push_back(es);
	return ret;
}

void Noted::appendInitEvents(Lightbox::StreamEvents const& _ses)
{
	foreach (StreamEvent const& se, _ses)
		m_initEvents.push_back(se);
}

void Noted::noteLastValidIs(AcausalAnalysisPtr const& _a)
{
	m_toBeAnalyzed.insert(_a);
	noteDataChanged(RejiggingSpectra);
}

void Noted::on_actFollow_changed()
{
}

void Noted::on_actOpen_activated()
{
	QString s = QFileDialog::getOpenFileName(this, "Open a Wave File", QDir::homePath(), "*.wav");
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
		if (!m_alsaThread->isRunning())
			m_alsaThread->start(QThread::TimeCriticalPriority);
	}
	else
	{
		ui->dockPlay->setEnabled(true);
		ui->actOpen->setEnabled(true);
	}
}

bool Noted::serviceAudio()
{
	if (ui->actPlay->isChecked() && m_audioHeader)
	{
		if (!m_alsa)
		{
			try {
				m_alsa = shared_ptr<Audio::Playback>(new Audio::Playback(ui->playDevice->itemData(ui->playDevice->currentIndex()).toInt(), 2, (ui->playRate->currentText() == "Default") ? -1 : ui->playRate->currentText().section(' ', 0, 0).toInt(), ui->playChunkSamples->value(), (ui->playChunks->value() == 2) ? -1 : ui->playChunks->value()));
			} catch (...) {}
		}
		int sams = m_audioHeader->dataBytes / m_audioHeader->bytesPerFrame;
        if (m_alsa)
		{
			vector<int16_t> out(m_alsa->frames() * m_alsa->channels());
            if (m_fineCursor >= 0 && m_fineCursor < toBase(sams, m_audioHeader->rate) - toBase(m_alsa->frames(), m_alsa->rate()))
			{
				auto d = [=](int i){ return *(int16_t const*)(m_audioData + (fromBase(m_fineCursor, m_audioHeader->rate) + i) * m_audioHeader->bytesPerFrame); };
				unsigned s = sams;
				unsigned r = m_alsa->rate();
				unsigned f = m_alsa->frames();
				if (m_alsa->isInterleaved())
					for (unsigned i = 0; i < f; i++)
						out[i * 2] = out[i * 2 + 1] = d(qMin(s - 1, i * m_audioHeader->rate / r));
				else
					for (unsigned i = 0; i < f; i++)
						out[i] = out[i + f] = d(qMin(s - 1, i * m_audioHeader->rate / r));
			}
			m_alsa->write(out);
			setCursor(m_fineCursor + toBase(m_alsa->frames(), m_alsa->rate()));
			return true;
		}
    }
	else
	{
		if (m_alsa)
			m_alsa.reset();
		updateAudioDevices();
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

QList<EventsEditor*> Noted::eventsEditors() const
{
	QList<EventsEditor*> ret;
	QMutexLocker l(&x_timelines);
	foreach (Timeline* i, m_timelines)
		if (EventsEditor* ev = dynamic_cast<EventsEditor*>(i))
			ret.push_back(ev);
	return ret;
}

void Noted::on_actZoomOut_activated()
{
	Time centre = m_offset + m_duration / 2;
	setDuration(m_duration *= 1.2);
	setOffset(centre - m_duration / 2);
}

void Noted::on_actZoomIn_activated()
{
	Time centre = m_offset + m_duration / 2;
	setDuration(m_duration /= 1.2);
	setOffset(centre - m_duration / 2);
}

void Noted::on_actPanBack_activated()
{
	setOffset(m_offset - m_duration / 2);
}

void Noted::on_actPanForward_activated()
{
	setOffset(m_offset + m_duration / 2);
}

void Noted::on_actPanic_activated()
{
	ui->actPlay->setChecked(false);
}

bool Noted::proceedTo(DataStatus _s, DataStatus _from)
{
	QMutexLocker l(&m_lock);
	qDebug() << "STATUS" << m_dataStatus << "--> (" << _s << "from" << _from << ") =" << ((m_dataStatus == _from) ? _s : m_dataStatus);
	if (m_dataStatus != _from)
		return false;
	m_dataStatus = _s;
	return true;
}

void Noted::noteDataChanged(DataStatus _s)
{
	QMutexLocker l(&m_lock);
	qDebug() << "STATUS" << m_dataStatus << "<--" << _s << "=" << ((m_dataStatus > _s) ? _s : m_dataStatus);
	if (m_dataStatus > _s)
		m_dataStatus = _s;
}


void Noted::setAudio(QString const& _filename)
{
	if (ui->actPlay->isChecked())
		ui->actPlay->setChecked(false);
	suspendWork();

	m_fineCursor = 0;
	m_offset = 0;
	m_duration = 1;

	emit audioChanged();

	m_audioFile.unmap((uint8_t*)m_audioHeader);
	m_audioHeader = nullptr;
	m_audioData = nullptr;
	m_audioFile.close();
	m_audioFile.setFileName(_filename);
	if (!m_audioFile.open(QFile::ReadOnly))
		m_audioFile.setFileName("");
	noteDataChanged(Dirty);
	updateWindowTitle();
	resumeWork();
}

static void updateCombo(QComboBox* _box, set<float> const& _natures, set<EventType> _types)
{
	QString s =  _box->currentText();
	_box->clear();
	foreach (float n, _natures)
	{
		QPixmap pm(16, 16);
		pm.fill(QColor::fromHsvF(n, 0.5, 0.85));
		_box->insertItem(_box->count(), pm, QString("Graph events of nature %1").arg(n), n);
	}
	_box->insertItem(_box->count(), "All Graph events", true);
	foreach (EventType e, _types)
		_box->insertItem(_box->count(), QString("All %1 events").arg(toString(e).c_str()), int(e));
	_box->insertItem(_box->count(), "All non-Graph events", false);
	_box->insertItem(_box->count(), "All events");
	for (int i = 0; i < _box->count(); ++i)
		if (_box->itemText(i) == s)
		{
			_box->setCurrentIndex(i);
			goto OK;
		}
	_box->setCurrentIndex(_box->count() - 1);
	OK:;
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
	QMutexLocker l(&m_lock);
	set<float> gnatures;
	set<EventType> gtypes;
	vector< shared_ptr<AuxGraphsSpec> > gspecs;
	foreach (auto e, m_initEvents)
		if (e.type == GraphSpecComment)
			gspecs.push_back(dynamic_pointer_cast<AuxGraphsSpec>(e.aux()));

	foreach (EventsView* ev, eventsViews())
	{
		set<float> natures;
		set<EventType> types;
		foreach (auto es, ev->m_events)
			foreach (auto e, es)
				if (e.type == GraphSpecComment)
				{
					gspecs.push_back(dynamic_pointer_cast<AuxGraphsSpec>(e.aux()));
				}
				else if (e.type >= Graph)
				{
					natures.insert(e.nature);
					gnatures.insert(e.nature);
				}
				else
				{
					types.insert(e.type);
					gtypes.insert(e.type);
				}
		updateCombo(ev->m_selection, natures, types);
	}

	updateGraphs(gspecs);

	updateCombo(ui->dataToExport, gnatures, gtypes);
}

void Noted::timerEvent(QTimerEvent*)
{
	static int i = 0;
	if (++i % 10 == 0)
	{
		QString msg;
		int prog;
		{
			QMutexLocker l(&m_lock);
			msg = m_showMessage;
			prog = m_progress;
		}

		QProgressBar* pb = ui->statusBar->findChild<QProgressBar*>();
		if (m_progress < 100)
		{
			if (!pb)
			{
				pb = new QProgressBar;
				pb->setMaximum(100);
				pb->setObjectName("progress");
				pb->setMaximumWidth(128);
				ui->statusBar->addPermanentWidget(pb);
			}
			pb->setValue(prog);
		}
		else if (pb)
			delete pb;

		if (m_dataStatus == Fresh)
		{
			m_dataStatus = Clean;
			qDebug() << "DATA" << Fresh << " cleans to " << Clean;
			emit audioChanged();
			m_cursorDirty = true;
			updateEventStuff();
			emit analysisFinished();
		}

		statusBar()->showMessage(msg);

		if (m_alsa)
			ui->statusBar->findChild<QLabel*>("alsa")->setText(QString("%1 %2# @ %3Hz, %4x%5 frames").arg(m_alsa->deviceName().c_str()).arg(m_alsa->channels()).arg(m_alsa->rate()).arg(m_alsa->periods()).arg(m_alsa->frames()));
		else
			ui->statusBar->findChild<QLabel*>("alsa")->setText("No audio");

		QMutexLocker l(&x_timelines);
		foreach (Timeline* t, m_timelines)
			if (PrerenderedTimeline* pt = dynamic_cast<PrerenderedTimeline*>(t))
				pt->updateIfNeeded();
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
		if (ui->actFollow->isChecked() && (m_fineCursor < m_offset || m_fineCursor > m_offset + m_duration * 7 / 8))
			setOffset(m_fineCursor - m_duration / 8);
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

bool Noted::isVisible(Lightbox::StreamEvent const& _e) const
{
	return eventVisible(ui->dataToExport->itemData(ui->dataToExport->currentIndex()), _e);
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

pair<QRect, QRect> Noted::cursorGeoOffset(int _id) const
{
	QWidget* mthis = const_cast<QWidget*>((QWidget const*)this);
	QRect geo;		// where the cursor window will be placed on the screen.
	QRect canvas;	// the offset
	if (_id == 0)
	{
		// quite arbitrary limits placed on where the cursor may draw...
		int f = xOf(cursor() - windowSize() + hop()) - 4;
		int t = xOf(cursor() + hop()) + 4;

		int cf = clamp(f, 0, ui->dataDisplay->width() - 1);
		int ct = clamp(t, 0, ui->dataDisplay->width() - 1);
		geo = QRect(mapToGlobal(ui->dataDisplay->mapTo(mthis, QPoint(cf, 0))), QSize(ct - cf, ui->dataDisplay->height()));
		canvas = QRect(QPoint(-cf, 0), QSize(0, geo.height()));
	}
	else if (_id == 1)
	{
		int f = ui->overview->xOf(timelineOffset());
		int t = ui->overview->xOf(timelineOffset() + timelineDuration()) + 2;
		int cf = clamp(f, 0, ui->overview->width() - 1);
		int ct = clamp(t, 0, ui->overview->width() - 1);
		geo = QRect(mapToGlobal(ui->overview->mapTo(mthis, QPoint(cf, 0))), QSize(ct - cf, ui->overview->height()));
		canvas = QRect(QPoint(f - cf, 0), QSize(t - f, geo.height()));
	}
	else if (_id == 2)
	{
		int f = ui->overview->xOf(cursor());
		int cf = clamp(f, 0, ui->overview->width() - 1);
		geo = QRect(mapToGlobal(ui->overview->mapTo(mthis, QPoint(cf, 0))), QSize(1, ui->overview->height()));
		canvas = QRect(QPoint(f - cf, 0), geo.size());
	}
	return make_pair(geo, canvas);
}

void Noted::info(QString const& _info)
{
	ui->infoView->setHtml(ui->infoView->toHtml() + "<div>" + _info + "</div>");
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
				int x = xOf(t->highlightFrom());
				int w = screenWidth(t->highlightDuration());
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
		int f = ui->overview->xOf(timelineOffset());
		int t = ui->overview->xOf(timelineOffset() + timelineDuration()) + 2;
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

	m_windowSizeSamples = ui->windowSize->value();	// TODO: Kill in favour of m_baseWindowFunction.size()
	m_windowFunction = Lightbox::windowFunction(m_windowSizeSamples, WindowFunction(ui->windowFunction->currentIndex()));

	m_zeroPhase = ui->zeroPhase->isChecked();
	m_normalize = ui->normalize->isChecked();
}

void Noted::rejigAudio()
{
	updateParameters();

	if (proceedTo(ResamplingAudio, Dirty))
	{
        qDebug() << "Resampling wave...";
        if (!resampleWave([&](int _progress){return carryOn("Resampling Wave", _progress, ResamplingAudio);}))
			proceedTo(Fresh, ResamplingAudio);
        qDebug() << "OK.";
        if (m_duration == 1)
		{
			m_fineCursor = 0;
			normalizeView();
		}
	}

	if (proceedTo(RejiggingSpectra, ResamplingAudio))
	{
        qDebug() << "Rejigging spectra...";
		rejigSpectra([&](int _progress){return carryOn("Rejigging Spectra", _progress, RejiggingSpectra);});
        qDebug() << "OK.";
        m_toBeAnalyzed.insert(nullptr);
	}

	if (proceedTo(Analyzing, RejiggingSpectra))
	{
		// TODO: restart worker from next(nullptr), skipping through work until item in mustReanalyzeAfter is found.

		deque<AcausalAnalysisPtr> todo;	// will become a member.
		todo.push_back(nullptr);

		// TODO: move into worker code; allow multiple workers.
		// TODO: consider searching tree locally and completely, putting toBeAnalyzed things onto global todo, and skipping through otherwise.
		// ...keeping search local until RAAs needed to be done are found.
		while (todo.size())
		{
			AcausalAnalysisPtr aa = todo.front();
			todo.pop_front();
			if (m_toBeAnalyzed.count(aa) && aa)
			{
				aa->go(this, 0, hops());
				if (!carryOn("", 0, Analyzing))
					break;
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

	if (proceedTo(Fresh, Analyzing) && carryOn("Ready", 100, Fresh))
	{
		// TODO: Move to individual timelines for more immediate view.
		QMutexLocker l(&x_timelines);
		foreach (Timeline* t, m_timelines)
			if (PrerenderedTimeline* pt = dynamic_cast<PrerenderedTimeline*>(t))
				pt->sourceChanged();
	}
}

AcausalAnalysisPtrs Noted::ripeAcausalAnalysis(AcausalAnalysisPtr const& _finished)
{
	AcausalAnalysisPtrs ret;

	if (_finished == nullptr)
		ret.push_back(m_compileEventsAnalysis);
	else if (dynamic_cast<CompileEvents*>(&*_finished))
		foreach (EventsView* ev, eventsViews())
			ret.push_back(AcausalAnalysisPtr(new CompileEventsView(ev)));
	else if (dynamic_cast<CompileEventsView*>(&*_finished) && ++m_eventsViewsDone == eventsViews().size())
		ret.push_back(m_collateEventsAnalysis);

	// Go through all other things that can give CAs; at this point, it's just the plugins
	AcausalAnalysisPtrs acc;
	foreach (LibraryPtr const& l, m_libraries)
		if (l->p && (acc = l->p->ripeAnalysis(_finished)).size())
			ret.insert(ret.end(), acc.begin(), acc.end());

	return ret;
}
