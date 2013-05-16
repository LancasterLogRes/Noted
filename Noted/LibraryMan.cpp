#include <Common/Global.h>
#include <NotedPlugin/AuxLibraryFace.h>
#include "ProcessEventCompiler.h"
#include "Global.h"
#include "Noted.h"
#include "PropertiesEditor.h"
#include "LibraryMan.h"



#include "EventsView.h"		// TODO: Remove
#include "ui_Noted.h"		// TODO: Remove


using namespace std;
using namespace lb;

LibraryMan::LibraryMan()
{
	connect(&m_libraryWatcher, SIGNAL(fileChanged(QString)), this, SLOT(onLibraryChange(QString)));
}

LibraryMan::~LibraryMan()
{
	cnote << "Unloading libraries...";
	while (m_libraries.size())
	{
		unload(*m_libraries.begin());
		m_libraries.erase(m_libraries.begin());
	}
	cnote << "Unloaded all libraries.";
}

QString defaultNick(QString const& _filename)
{
	QString ret = _filename.section('/', -1);
	QRegExp re("(lib)?(.*)\\.[a-zA-Z]*");
	if (re.exactMatch(ret))
		ret = re.cap(2);
	return ret;
}

void LibraryMan::addLibrary(QString const& _name, bool _isEnabled)
{
	cnote << "Adding library" << _name.toLocal8Bit().data() << ".";
	if (m_libraries.contains(_name))
		cwarn << "Ignoring duplicate library" << _name.toLocal8Bit().data() << ".";
	else
	{
		cnote << "Not a duplicate - loading...";
		auto lp = make_shared<RealLibrary>(_name);
		m_libraries.insert(_name, lp);
		lp->item = new QTreeWidgetItem(Noted::get()->ui->loadedLibraries, QStringList() << defaultNick(_name) << "Unknown" << _name);
		lp->item->setFlags(lp->item->flags() | Qt::ItemIsUserCheckable);
		lp->item->setCheckState(0, _isEnabled ? Qt::Checked : Qt::Unchecked);
		if (_isEnabled)
		{
			m_dirtyLibraries.insert(_name);
			reloadDirties();
		}
	}
}

void LibraryMan::killLibrary(QTreeWidgetItem* _it)
{
	QString s = _it->text(2);
	delete _it;
	m_dirtyLibraries.insert(s);
	reloadDirties();
}

shared_ptr<NotedPlugin> LibraryMan::getPlugin(QString const& _mangledName)
{
	for (auto l: m_libraries)
		if (l->p && typeid(*l->p).name() == _mangledName)
			return l->p;
	return nullptr;
}

void LibraryMan::load(RealLibraryPtr const& _dl)
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
			typedef NotedPlugin*(*pf_t)();
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
					Noted::get()->ui->eventCompilersList->addItem(li);
					Noted::compute()->noteEventCompilersChanged();
				}
				cnote << _dl->cf.size() << " event compiler factories";
			}
			else if (pf_t np = (pf_t)_dl->l.resolve("newPlugin"))
			{
				_dl->item->setText(1, "Plugin");
				cnote << "LOAD" << _dl->nick << " [PLUGIN]";

				_dl->p = shared_ptr<NotedPlugin>(np());

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
						QDockWidget* propsDock = new QDockWidget(QString("%1 Properties").arg(_dl->nick), NotedFace::get());
						propsDock->setObjectName(_dl->nick + "/properties");
						pe = new PropertiesEditor(propsDock);
						propsDock->setWidget(pe);
						propsDock->setFeatures(propsDock->features()|QDockWidget::DockWidgetVerticalTitleBar);
						NotedFace::get()->addDockWidget(Qt::RightDockWidgetArea, propsDock);
						if (s.contains(_dl->nick + "/propertiesGeometry"))
							propsDock->restoreGeometry(s.value(_dl->nick + "/propertiesGeometry").toByteArray());
					}
					if (Noted::get()->m_constructed)
					{
						Noted::get()->readBaseSettings(s);
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
		Noted::get()->ui->eventCompilersList->addItem(li);
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

void LibraryMan::unload(RealLibraryPtr const& _dl)
{
	if (_dl->l.isLoaded())
	{
		if (_dl->p)
		{
			// save state.
			{
				QSettings s("LancasterLogicResponse", "Noted");
				Noted::get()->writeBaseSettings(s);
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
			// TODO: Kill off any events that may be lingering (hopefully none).
			foreach (auto f, _dl->cf)
			{
				// NOTE: A bit messy, this?
				for (EventsView* ev: Noted::get()->eventsViews())
					if (ev->name() == QString::fromStdString(f.first))
						ev->save();
				delete Noted::get()->ui->eventCompilersList->findItems(QString::fromStdString(f.first), 0).front();
			}
			_dl->cf.clear();
			// TODO: update whatever has changed re: event compilers being available.
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

void LibraryMan::onLibraryChange(QString const& _name)
{
	m_dirtyLibraries.insert(_name);
	reloadDirties();
}

void LibraryMan::reloadLibrary(QTreeWidgetItem* _it)
{
	for (auto l: m_libraries)
		if (l->item == _it)
		{
			m_dirtyLibraries.insert(l->filename);
			break;
		}
	reloadDirties();
}

void LibraryMan::reloadDirties()
{
	if (!m_dirtyLibraries.empty())
	{
		Noted::compute()->suspendWork();

		// OPTIMIZE: only bother saving for EVs whose EC is given by a dirty library.
		for (EventsView* ev: Noted::get()->eventsViews())
			ev->save();

		foreach (QString const& name, m_dirtyLibraries)
		{
			bool doLoad = false;
			bool doKill = true;
			for (auto i: Noted::get()->ui->loadedLibraries->findItems(name, Qt::MatchExactly, 2))
			{
				doKill = false;
				doLoad = (i->checkState(0) == Qt::Checked);
				break;
			}

			if (doLoad)
			{
				RealLibraryPtr dl = m_libraries[name];
				assert(dl);
				assert(dl->filename == name);
				unload(dl);
				load(dl);
			}
			else
			{
				unload(m_libraries[name]);
				if (doKill)
					m_libraries.remove(name);
			}
		}

		foreach (EventsView* ev, Noted::get()->eventsViews())
			ev->restore();

		m_dirtyLibraries.clear();

		// OPTIMIZE: Something finer grained?
		Noted::compute()->noteEventCompilersChanged();

		Noted::compute()->resumeWork();
	}
}
