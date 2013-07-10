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

#include <iostream>
#include <fstream>
#include <cmath>
#include <QtGui>
#include <QtWidgets>
#include <QGLFramebufferObject>
#include <EventCompiler/StreamEvent.h>
#include <EventCompiler/GraphSpec.h>
#include <EventsEditor/EventsEditor.h>
#include <EventsEditor/EventsEditScene.h>
#include <NotedPlugin/DataSet.h>
#include "PropertiesEditor.h"
#include "Noted.h"
#include "LibraryMan.h"
#include "EventCompilerView.h"
using namespace std;
using namespace lb;

EventCompilerView::EventCompilerView(QWidget* _parent, EventCompiler const& _ec):
	EventsGraphicsView	(_parent),
	m_eventCompiler		(_ec)
{
	connect(NotedFace::libs(), SIGNAL(eventCompilerFactoryAvailable(QString, unsigned)), SLOT(onFactoryAvailable(QString, unsigned)));
	connect(NotedFace::libs(), SIGNAL(eventCompilerFactoryUnavailable(QString)), SLOT(onFactoryUnavailable(QString)));
	connect(NotedFace::data(), SIGNAL(dataReady(DataKey)), this, SLOT(onDataComplete(DataKey)));

	setMaximumHeight(48);
	setMinimumHeight(48);

	const float c_size = 16;
	const float c_margin = 0;

	m_label = new QLabel(this);
	m_label->setGeometry(0, 0, (c_size + c_margin) * 5 + c_size, 14);
	m_label->setStyleSheet("background: white");
	m_label->setText(name());

	m_channel = new QComboBox(this);
	m_channel->setGeometry((c_size + c_margin) * 4, m_label->height(), c_size * 2 + c_margin, c_size);
	connect(m_channel, SIGNAL(currentIndexChanged(int)), SLOT(channelChanged()));
	m_channel->addItem("-");
	m_channel->addItem("0");
	m_channel->addItem("1");
	m_channel->addItem("2");
	m_channel->addItem("3");

	setObjectName(name());

	NotedFace::events()->registerEventsView(this);
	NotedFace::events()->registerStore(this);
}

EventCompilerView::~EventCompilerView()
{
	NotedFace::graphs()->unregisterGraphs(objectName());
	NotedFace::events()->unregisterStore(this);
	NotedFace::events()->unregisterEventsView(this);
}

void EventCompilerView::onFactoryAvailable(QString _factory, unsigned)
{
	if (isArchived() && _factory == m_savedName)
		restore();
}

void EventCompilerView::onFactoryUnavailable(QString _factory)
{
	if (!isArchived() && _factory == name())
		save();
}

void EventCompilerView::onUseChanged()
{
	Noted::events()->notePluginDataChanged();
}

void EventCompilerView::clearCurrentEvents()
{
	QMutexLocker l(&x_eventsCache);
	m_current.clear();
}

lb::StreamEvents EventCompilerView::cursorEvents() const
{
	StreamEvents ret;
	int ch = m_channel->currentIndex() - 1;
	if (ch >= 0)
		for (StreamEvent e: m_current)
			ret.push_back(e.assignedTo(ch));
	return ret;
}

void EventCompilerView::channelChanged()
{
	onUseChanged();//for now...
}

class DataSetEventsStore: public EventsStore
{
public:
	DataSetEventsStore(DataSetPtr<lb::StreamEvent> const& _ses): m_streamEvents(_ses) {}
	virtual ~DataSetEventsStore();

	virtual lb::StreamEvents events(lb::Time _from, lb::Time _before) const
	{
		return m_streamEvents->getInterval(_from, _before);
	}
	virtual lb::SimpleKey hash() const { return m_streamEvents->operationKey(); }

private:
	DataSetPtr<lb::StreamEvent> m_streamEvents;
};

DataSetEventsStore::~DataSetEventsStore()
{
	cnote << "~DataSetEventsStore()";
}

void EventCompilerView::onDataComplete(DataKey _k)
{
	if (_k == DataKey(Noted::audio()->key(), m_operationKey))
	{
		// Finished computing events. Copy them into the scene.
		DataSetEventsStore es(m_streamEvents);
		scene()->clear();
		scene()->copyFrom(&es);
	}
}

lb::StreamEvents EventCompilerView::events(int _i) const
{
	if (isEnabled())
	{
		QMutexLocker l(&x_eventsCache);
		if (_i < m_eventsCache.size())
		{
			StreamEvents ret = m_eventsCache[_i];
			int ch = m_channel->currentIndex() - 1;
			if (ch >= 0)
				for (StreamEvent& e: ret)
					e.assign(ch);
			return ret;
		}
	}
	return StreamEvents();
}

QString EventCompilerView::name() const
{
	return QString::fromStdString(m_eventCompiler.name());
}

void EventCompilerView::save()
{
	cnote << "Saving EventCompilerView...";
	if (!m_eventCompiler.isNull())
	{
		m_savedName = name();
		m_savedProperties = m_eventCompiler.properties().serialized();
	}

	NotedFace::graphs()->unregisterGraphs(objectName());
	m_eventCompiler = EventCompiler();

	NotedFace::events()->notePluginDataChanged();
}

void EventCompilerView::restore()
{
	cnote << "Restoring EventCompilerView...";
	m_eventCompiler = NotedFace::libs()->newEventCompiler(m_savedName);
	m_eventCompiler.properties().deserialize(m_savedProperties);
//	m_propertiesEditor->setProperties(m_eventCompiler.properties());
	m_label->setText(name());

	NotedFace::events()->noteEventCompilersChanged();
}

void EventCompilerView::readSettings(QSettings& _s, QString const& _id)
{
	m_savedName = _s.value(_id + "/name").toString();
	m_savedProperties = _s.value(_id + "/properties").toString().toStdString();
	m_channel->setCurrentIndex(_s.value(_id + "/channel").toInt());
	setObjectName(m_savedName);
	restore();
}

void EventCompilerView::writeSettings(QSettings& _s, QString const& _id)
{
	_s.setValue(_id + "/name", name());
	_s.setValue(_id + "/properties", QString::fromStdString(m_eventCompiler.properties().serialized()));
	_s.setValue(_id + "/channel", m_channel->currentIndex());
}

/*void EventCompilerView::exportEvents()
{
	QString fn = QFileDialog::getSaveFileName(this, "Export a series of events", QDir::currentPath(), "Native format (*.events);;XML format (*.xml);;CSV format (*.csv *.txt)");
	ofstream out;
	out.open(fn.toLocal8Bit(), ios::trunc);
	if (fn.endsWith(".xml"))
		out << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << endl << "<events>" << endl;
	if (out)
	{
		Time t = 0;
		QVariant v = m_selection->itemData(m_selection->currentIndex());
		foreach (StreamEvents se, m_events)
		{
			int timeout = 0;
			foreach (StreamEvent e, se)
			{
				if (eventVisible(v, e))
				{
					if (fn.endsWith(".xml"))
					{
						if (!timeout++)
							out << QString("\t<time value=\"%1\" ms=\"%2\">").arg(t).arg(toMsecs(t)).toStdString() << endl;
						out << QString("\t\t<%1 strength=\"%2\" character=\"%3\" temperature=\"%4\" surprise=\"%5\" position=\"%6\" constancy=\"%7\" jitter=\"%8\" channel=\"%9\" />").arg(QString::fromStdString(toString(e.type)).toLower()).arg(e.strength).arg(toString(e.character).c_str()).arg(e.temperature).arg(e.surprise).arg(e.position).arg(e.constancy).arg(e.jitter).arg(e.channel).toStdString() << endl;
					}
					else if (fn.endsWith(".events"))
					{
						if (!timeout++)
							out << t << endl;
						out << (int)e.type << " " << e.strength << " " << (int)e.character << " " << e.temperature << " " << e.surprise << " " << (int)e.position << " " << e.jitter << " " << e.constancy << " " << (int)e.channel << endl;
					}
					else
					{
						out << toSeconds(t) << " " << (int)e.type << " " << e.strength << " " << (int)e.character << " " << e.temperature << " " << e.surprise << " " << (int)e.position << " " << e.jitter << " " << e.constancy << " " << (int)e.channel << endl;
					}

				}
			}
			if (timeout)
			{
				if (fn.endsWith(".xml"))
					out << "\t</time>" << endl;
				else if (fn.endsWith(".events"))
					out << endl;
			}
			t += NotedFace::audio()->hop();
		}
	}
	if (fn.endsWith(".xml"))
		out << "</events>" << endl;
}*/
