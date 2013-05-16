#include <Common/Global.h>
#include <NotedPlugin/AuxLibraryFace.h>
#include "ProcessEventCompiler.h"
#include "Global.h"
#include "Noted.h"
#include "PropertiesEditor.h"
#include "LibraryMan.h"
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

void LibraryMan::readSettings(QSettings& _settings)
{
	if (_settings.contains("libraryCount"))
		for (int i = 0; i < _settings.value("libraryCount").toInt(); ++i)
			addLibrary(_settings.value(QString("library%1").arg(i)).toString(), _settings.value(QString("library%1.enabled").arg(i)).toBool());
	else if (_settings.contains("libraries"))
		for (QString n: _settings.value("libraries").toStringList())
			addLibrary(n);

	for (auto l: m_libraries)
		if (l->plugin)
			l->plugin->readSettings(_settings);
}

void LibraryMan::writeSettings(QSettings& _settings)
{
	for (auto l: m_libraries)
		if (l->plugin)
			l->plugin->writeSettings(_settings);

	int lc = 0;
	for (auto l: m_libraries)
	{
		_settings.setValue(QString("library%1").arg(lc), l->filename);
		_settings.setValue(QString("library%1.enabled").arg(lc), l->enabled);
		++lc;
	}
	_settings.setValue("libraryCount", lc);
}

int LibraryMan::rowCount(QModelIndex const& _parent) const
{
	return _parent.isValid() ? 0 : m_libraries.size();
}

int LibraryMan::columnCount(QModelIndex const&) const
{
	return 3;
}

QModelIndex LibraryMan::index(int _row, int _column, QModelIndex const& _parent) const
{
	int r = 0;
	if (!_parent.isValid())
		for (auto p: m_libraries)
			if (r++ == _row)
				return createIndex(_row, _column, &*p);
	return QModelIndex();
}

QModelIndex LibraryMan::parent(QModelIndex const&) const
{
	return QModelIndex();
}

QString defaultNick(QString const& _filename)
{
	QString ret = _filename.section('/', -1);
	QRegExp re("(lib)?(.*)\\.[a-zA-Z]*");
	if (re.exactMatch(ret))
		ret = re.cap(2);
	return ret;
}

QVariant LibraryMan::data(QModelIndex const& _index, int _role) const
{
	auto l = (RealLibrary*)_index.internalPointer();
	if (_role == Qt::CheckStateRole && _index.column() == 0)
		return l->enabled ? Qt::Checked : Qt::Unchecked;
	if (_role == Qt::DisplayRole)
		switch (_index.column())
		{
			case 0: return l->nick.isEmpty() ? defaultNick(l->filename) : l->nick;
			case 1: return l->auxFace ? "Aux: " + l->parent : l->plugin ? "Plugin" : l->eventCompilerFactories.size() ? "EventCompilers" : l->required.size() ? "Plugin requires: " + l->required.join(" ") : l->error.size() ? "Load error: " + l->error : "Aux: ???";
			case 2: return l->filename;
		}
	return QVariant();
}

bool LibraryMan::setData(QModelIndex const& _index, QVariant const& _value, int _role)
{
	auto l = (RealLibrary*)_index.internalPointer();
	if (_role == Qt::CheckStateRole && _index.column() == 0)
	{
		setEnabled(m_libraries[l->filename], _value.toInt() == Qt::Checked);
		return true;
	}
	return false;
}

Qt::ItemFlags LibraryMan::flags(QModelIndex const& _index) const
{
	return (_index.column() == 0 ? Qt::ItemIsUserCheckable : Qt::ItemIsEnabled) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant LibraryMan::headerData(int _section, Qt::Orientation, int _role) const
{
	return _role == Qt::DisplayRole ? _section == 0 ? "Name" : _section == 1 ? "Type" : "Filename" : QVariant();
}

void LibraryMan::addLibrary(QString const& _name, bool _isEnabled)
{
	cnote << "Adding library" << _name.toLocal8Bit().data() << ".";
	if (m_libraries.contains(_name))
		cwarn << "Ignoring duplicate library" << _name.toLocal8Bit().data() << ".";
	else
	{
		cnote << "Not a duplicate - loading...";
		Noted::compute()->suspendWork();
		beginResetModel();

		RealLibraryPtr lp = make_shared<RealLibrary>(_name, _isEnabled);
		m_libraries.insert(_name, lp);
		if (_isEnabled)
			load(lp);

		endResetModel();
		Noted::compute()->noteEventCompilersChanged();
		Noted::compute()->resumeWork();
	}
}

void LibraryMan::removeLibrary(QModelIndex _index)
{
	auto l = (RealLibrary*)_index.internalPointer();
	if (l)
		removeLibrary(l->filename);
	else
		cwarn << "Can't remove library: Bad model index.";
}

void LibraryMan::removeLibrary(QString const& _name)
{
	if (m_libraries.contains(_name))
	{
		Noted::compute()->suspendWork();
		beginResetModel();

		auto l = m_libraries[_name];
		unload(l);
		m_libraries.remove(_name);

		endResetModel();
		Noted::compute()->noteEventCompilersChanged();
		Noted::compute()->resumeWork();
	}
	else
		cwarn << "Ignoring removal of non-existant library" << _name.toStdString() << ".";
}

void LibraryMan::reloadLibrary(QString const& _name)
{
	if (m_libraries.contains(_name))
	{
		auto l = m_libraries[_name];
		if (l->enabled)
		{
			Noted::compute()->suspendWork();
			beginResetModel();

			auto l = m_libraries[_name];
			unload(l);
			load(l);

			endResetModel();
			Noted::compute()->noteEventCompilersChanged();
			Noted::compute()->resumeWork();
		}
		else
			cwarn << "Ignoring reload of disabled library" << _name.toStdString() << ".";
	}
	else
		cwarn << "Ignoring reload of non-existant library" << _name.toStdString() << ".";
}

void LibraryMan::setEnabled(RealLibraryPtr const& _l, bool _enabled)
{
	if (_l->enabled == _enabled)
		return;
	_l->enabled = _enabled;

	Noted::compute()->suspendWork();
	beginResetModel();

	if (_l->enabled)
		load(_l);
	else
		unload(_l);

	endResetModel();
	Noted::compute()->noteEventCompilersChanged();
	Noted::compute()->resumeWork();
}

shared_ptr<NotedPlugin> LibraryMan::getPlugin(QString const& _mangledName)
{
	for (auto l: m_libraries)
		if (l->plugin && typeid(*l->plugin).name() == _mangledName)
			return l->plugin;
	return nullptr;
}

void LibraryMan::onLibraryChange(QString const& _name)
{
	reloadLibrary(_name);
}

bool LibraryMan::providesEventCompiler(QString const& _library, QString const& _ec)
{
	return m_libraries.contains(_library) && m_libraries[_library]->eventCompilerFactories.count(_ec.toStdString());
}

void LibraryMan::load(RealLibraryPtr const& _dl)
{
	cnote << "Loading:" << _dl->filename;
	m_libraryWatcher.addPath(_dl->filename);
	if (QLibrary::isLibrary(_dl->filename))
	{
		_dl->nick = defaultNick(_dl->filename);
		QString tempFile = QDir::tempPath() + "/Noted[" + _dl->nick + "]" + QDateTime::currentDateTime().toString("yyyyMMdd-hh.mm.ss.zzz");
		_dl->library.setFileName(tempFile);
		_dl->library.setLoadHints(QLibrary::ResolveAllSymbolsHint);
		QFile::copy(_dl->filename, tempFile);
		if (_dl->library.load())
		{
			typedef EventCompilerFactories&(*cf_t)();
			typedef NotedPlugin*(*pf_t)();
			typedef char const*(*pnf_t)();
			if (cf_t cf = (cf_t)_dl->library.resolve("eventCompilerFactories"))
			{
				cnote << "LOAD" << _dl->nick << " [ECF]";
				_dl->eventCompilerFactories = cf();
				for (auto f: _dl->eventCompilerFactories)
					emit eventCompilerFactoryAvailable(QString::fromStdString(f.first), f.second.version);
				cnote << _dl->eventCompilerFactories.size() << " event compiler factories";
			}
			else if (pf_t np = (pf_t)_dl->library.resolve("newPlugin"))
			{
				cnote << "LOAD" << _dl->nick << " [PLUGIN]";

				_dl->plugin = shared_ptr<NotedPlugin>(np());

				if (_dl->plugin->m_required.empty())
				{
					for (auto lib: m_libraries)
						if (!lib->library.isLoaded() && lib->enabled)
							load(lib);

					QSettings s("LancasterLogicResponse", "Noted");
					Members<NotedPlugin> props(_dl->plugin->propertyMap(), _dl->plugin, [=](std::string const&){_dl->plugin->onPropertiesChanged();});
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
						_dl->plugin->readSettings(s);
					}
					if (pe)
						pe->setProperties(props);
				}
				else
				{
					_dl->required = _dl->plugin->m_required;
					_dl->plugin.reset();
					_dl->unload();
				}
			}
			else
			{
				for (auto lib: m_libraries)
					if (lib->plugin)
						if (auto f = shared_ptr<AuxLibraryFace>(lib->plugin->newAuxLibrary()))
						{
							// tentatively add it, so the library can use it transparently.
							lib->plugin->m_auxLibraries.append(f);
							if (f->load(_dl))
							{
								_dl->auxFace = f;
								_dl->auxPlugin = lib->plugin;
								cnote << "LOAD" << _dl->nick << " [AUX:" << lib->nick << "]";
								_dl->parent = lib->nick;
								goto LOADED;
							}
							else
							{
								f.reset();
								lib->plugin->removeDeadAuxes();
							}
						}
				cnote << "Useless library - unloading" << _dl->nick;
				_dl->unload();
				LOADED:;
			}
		}
		else
		{
			cwarn << "ERROR on load: " << _dl->library.errorString();
			_dl->error = _dl->library.errorString();
		}
		if (_dl->library.isLoaded())
			emit doneLibraryLoad(_dl->filename);
	}
	else if (QFile::exists(_dl->filename))
	{
		_dl->eventCompilerFactories[_dl->filename.toStdString()] = {[=](){ return new ProcessEventCompiler(_dl->filename); }, 0};
		emit eventCompilerFactoryAvailable(_dl->filename, 0);
		emit doneLibraryLoad(_dl->filename);
	}
}

void RealLibrary::unload()
{
	if (bool** fed = (bool**)library.resolve("g_lightboxFinalized"))
	{
		bool isFinalized = false;
		*fed = &isFinalized;
		assert(library.unload());
		assert(isFinalized);
	}
	else
	{
		qWarning() << "Couldn't get the Lightbox 'finalized' symbol in " << filename;
		assert(library.unload());
	}
	QFile::remove(library.fileName());
}

void LibraryMan::unload(RealLibraryPtr const& _dl)
{
	if (_dl->library.isLoaded())
	{
		emit prepareLibraryUnload(_dl->filename);
		if (_dl->plugin)
		{
			// save state.
			{
				QSettings s("LancasterLogicResponse", "Noted");
				Noted::get()->writeBaseSettings(s);
				_dl->plugin->writeSettings(s);

				Members<NotedPlugin> props(_dl->plugin->propertyMap(), _dl->plugin);
				s.setValue(_dl->nick + "/properties", QString::fromStdString(props.serialized()));

				// kill the properties dock if there is one.
				if (auto pw = findChild<QDockWidget*>(_dl->nick + "/properties"))
				{
					s.setValue(_dl->nick + "/propertiesGeometry", pw->saveGeometry());
					delete pw;
				}
			}

			// unload dependents.
			for (auto l: m_libraries)
				if (l->auxFace && l->auxPlugin.lock() == _dl->plugin)
					unload(l);
			// if all dependents were successfully unloaded, then there should be nothing left in the auxLibrary list.
			assert(_dl->plugin->m_auxLibraries.isEmpty());
			// kill plugin.
			_dl->plugin.reset();
		}
		else if (_dl->eventCompilerFactories.size())
		{
			for (auto f: _dl->eventCompilerFactories)
				emit eventCompilerFactoryUnavailable(QString::fromStdString(f.first));

			_dl->eventCompilerFactories.clear();
		}
		else if (_dl->auxFace && _dl->auxPlugin.lock()) // check if we're a plugin's auxilliary
		{
			// remove ourselves from the plugin we're dependent on.
			_dl->auxFace->unload(_dl);
			_dl->auxFace.reset();
			_dl->auxPlugin.lock()->removeDeadAuxes();
			_dl->auxPlugin.reset();
		}
		cnote << "UNLOAD" << _dl->filename;

		_dl->unload();
	}
	m_libraryWatcher.removePath(_dl->filename);
}
