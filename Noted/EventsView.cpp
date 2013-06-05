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
#include "PropertiesEditor.h"
#include "Noted.h"
#include "LibraryMan.h"
#include "EventsView.h"
using namespace std;
using namespace lb;

EventsView::EventsView(QWidget* _parent, EventCompiler const& _ec):
	EventsGraphicsView	(_parent),
	m_eventCompiler		(_ec)
{
	connect(NotedFace::libs(), SIGNAL(eventCompilerFactoryAvailable(QString, unsigned)), SLOT(onFactoryAvailable(QString, unsigned)));
	connect(NotedFace::libs(), SIGNAL(eventCompilerFactoryUnavailable(QString)), SLOT(onFactoryUnavailable(QString)));

//	m_verticalSplitter = dynamic_cast<QSplitter*>(parentWidget());
//	m_actualWidget = dynamic_cast<QSplitter*>(m_verticalSplitter->parentWidget());
//	m_propertiesEditor = new PropertiesEditor(m_actualWidget);
//	m_actualWidget->addWidget(m_propertiesEditor);
//	m_eventsEditor = new EventsEditor(m_verticalSplitter);
	/*m_eventsEditor->*/setMaximumHeight(48);
	/*m_eventsEditor->*/setMinimumHeight(48);
//	m_verticalSplitter->addWidget(m_eventsEditor);
//	m_verticalSplitter->addWidget(this);
//	m_propertiesEditor->setProperties(m_eventCompiler.properties());

//	connect(m_propertiesEditor, SIGNAL(changed()), NotedFace::events(), SLOT(noteEventCompilersChanged()));
	connect(NotedFace::events(), SIGNAL(eventsChanged()), SLOT(rerender()));

	const float c_size = 16;
	const float c_margin = 0;

	m_label = new QLabel(this);
	m_label->setGeometry(0, 0, (c_size + c_margin) * 5 + c_size, 14);
	m_label->setStyleSheet("background: white");
	m_label->setText(name());

/*	QPushButton* b = new QPushButton(this);
	b->setGeometry(0, m_label->height(), c_size, c_size);
	b->setText("X");
	connect(b, SIGNAL(clicked()), SLOT(deleteLater()));
*/
/*	QPushButton* ex = new QPushButton(this);
	ex->setGeometry(c_size + c_margin, m_label->height(), c_size, c_size);
	ex->setText(">");
	connect(ex, SIGNAL(clicked()), SLOT(exportGraph()));

	QPushButton* d = new QPushButton(this);
	d->setGeometry((c_size + c_margin) * 2, m_label->height(), c_size, c_size);
	d->setText("+");
	connect(d, SIGNAL(clicked()), SLOT(duplicate()));
*/
/*	m_use = new QPushButton(this);
	m_use->setGeometry((c_size + c_margin) * 3, m_label->height(), c_size, c_size);
	m_use->setText("U");
	m_use->setCheckable(true);
	m_use->setChecked(true);
	connect(m_use, SIGNAL(toggled(bool)), SLOT(onUseChanged()));*/

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
}

EventsView::~EventsView()
{
	NotedFace::graphs()->unregisterGraphs(objectName());
	NotedFace::events()->unregisterEventsView(this);
/*	quit();
	QWidget* w = parentWidget()->parentWidget();
	setParent(0);
	delete w;*/
}

void EventsView::onFactoryAvailable(QString _factory, unsigned)
{
	if (isArchived() && _factory == m_savedName)
		restore();
}

void EventsView::onFactoryUnavailable(QString _factory)
{
	if (!isArchived() && _factory == name())
		save();
}

void EventsView::onUseChanged()
{
	Noted::events()->notePluginDataChanged();
}

void EventsView::clearEvents()
{
	QMutexLocker l(&x_events);
	m_current.clear();
}

lb::StreamEvents EventsView::cursorEvents() const
{
	StreamEvents ret;
	int ch = m_channel->currentIndex() - 1;
	if (ch >= 0)
		for (StreamEvent e: m_current)
			ret.push_back(e.assignedTo(ch));
	return ret;
}

void EventsView::channelChanged()
{
	onUseChanged();//for now...
}

void EventsView::finalizeEvents()
{
	QTimer::singleShot(0, this, SLOT(setNewEvents()));
}

void EventsView::setNewEvents()
{
	setEvents(m_events, 0);
}

lb::StreamEvents EventsView::events(int _i) const
{
	if (isEnabled())
	{
		QMutexLocker l(&x_events);
		if (_i < m_events.size())
		{
			StreamEvents ret = m_events[_i];
			int ch = m_channel->currentIndex() - 1;
			if (ch >= 0)
				for (StreamEvent& e: ret)
					e.assign(ch);
			return ret;
		}
	}
	return StreamEvents();
}

void EventsView::appendEvents(StreamEvents const& _se)
{
	QMutexLocker l(&x_events);
	m_events.push_back(_se);
}

void EventsView::duplicate()
{
	EventsView* ev = new EventsView(parentWidget());
	ev->m_eventCompiler = m_eventCompiler;
	ev->m_events = m_events;
	auto p = dynamic_cast<QSplitter*>(parentWidget());
	p->insertWidget(p->indexOf(this) + 1, ev);
}

QString EventsView::name() const
{
	return QString::fromStdString(m_eventCompiler.name());
}

void EventsView::save()
{
	cnote << "Saving EventsView...";
	if (!m_eventCompiler.isNull())
	{
		m_savedName = name();
		m_savedProperties = m_eventCompiler.properties().serialized();
	}

	NotedFace::graphs()->unregisterGraphs(objectName());
	m_eventCompiler = EventCompiler();

	NotedFace::events()->notePluginDataChanged();
}

void EventsView::restore()
{
	cnote << "Restoring EventsView...";
	m_eventCompiler = NotedFace::libs()->newEventCompiler(m_savedName);
	m_eventCompiler.properties().deserialize(m_savedProperties);
//	m_propertiesEditor->setProperties(m_eventCompiler.properties());
	m_label->setText(name());

	NotedFace::events()->noteEventCompilersChanged();
}

void EventsView::readSettings(QSettings& _s, QString const& _id)
{
	m_savedName = _s.value(_id + "/name").toString();
	m_savedProperties = _s.value(_id + "/properties").toString().toStdString();
//	m_use->setChecked(_s.value(_id + "/use").toBool());
	m_channel->setCurrentIndex(_s.value(_id + "/channel").toInt());
	setObjectName(m_savedName);
	restore();
}

void EventsView::writeSettings(QSettings& _s, QString const& _id)
{
	_s.setValue(_id + "/name", name());
	_s.setValue(_id + "/properties", QString::fromStdString(m_eventCompiler.properties().serialized()));
	_s.setValue(_id + "/use", isEnabled());
	_s.setValue(_id + "/channel", m_channel->currentIndex());
}

/*void EventsView::exportEvents()
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

void EventsView::exportGraph()
{
	QString fn = QFileDialog::getSaveFileName(this, "Export the graphs", QDir::currentPath(), "CSV format (*.csv *.txt)");
	ofstream out;
	out.open(fn.toLocal8Bit(), ios::trunc);
	if (out)
	{
		out << "index,time";
		Time h = NotedFace::audio()->hop();
		unsigned tiMax = 0;

		vector<vector<float> const*> charts;
		for (GraphSpec* g: m_eventCompiler.asA<EventCompilerImpl>().graphs())
			if (GraphChart* s = dynamic_cast<GraphChart*>(g))
			{
				charts.push_back(&s->data());
				tiMax = max<unsigned>(tiMax, s->data().size());
				out << "," << s->name();
			}

		vector<map<int, vector<float>> const*> spectra;
		for (GraphSpec* g: m_eventCompiler.asA<EventCompilerImpl>().graphs())
			if (GraphSpectrum* s = dynamic_cast<GraphSpectrum*>(g))
			{
				if (s->data().size())
				{
					spectra.push_back(&s->data());
					tiMax = max<unsigned>(tiMax, prev(s->data().end())->first * h + 1);
					for (unsigned i = 0; i < s->bandCount(); ++i)
						out << "," << s->name() << "_" << i;
				}
			}

		out << endl;

		Time t = 0;
		unsigned ti = 0;
		auto inner = [&]()
		{
			out << ti << "," << toSeconds(t);
			for (vector<float> const* s: charts)
				out << "," << (ti < s->size() ? s->at(ti) : 0);
			for (map<int, vector<float>> const* s: spectra)
			{
				auto si = s->upper_bound(t / h);
				if (si != s->begin())
					--si;
				for (auto f: si->second)
					out << "," << f;
			}
			out << endl;
		};

		if (charts.size())
			for (ti = 0; ti < tiMax; ++ti, t += h)
				inner();
	}
}
