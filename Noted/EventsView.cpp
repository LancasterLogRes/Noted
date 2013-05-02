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
#include <QGLFramebufferObject>
#include <EventCompiler/StreamEvent.h>
#include <EventsEditor/EventsEditor.h>
#include <EventsEditor/EventsEditScene.h>

#include "PropertiesEditor.h"
#include "Noted.h"
#include "EventsView.h"

using namespace std;
using namespace Lightbox;

EventsView::EventsView(QWidget* _parent, EventCompiler const& _ec):
	PrerenderedTimeline	(new QSplitter(Qt::Vertical, new QSplitter(Qt::Horizontal, _parent))),
	m_eventCompiler		(_ec),
	m_use				(nullptr)
{
	m_verticalSplitter = dynamic_cast<QSplitter*>(parentWidget());
	m_actualWidget = dynamic_cast<QSplitter*>(m_verticalSplitter->parentWidget());
	m_propertiesEditor = new PropertiesEditor(m_actualWidget);
	m_actualWidget->addWidget(m_propertiesEditor);
	m_eventsEditor = new EventsEditor(m_verticalSplitter);
	m_eventsEditor->setMaximumHeight(48);
	m_eventsEditor->setMinimumHeight(48);
	m_verticalSplitter->addWidget(m_eventsEditor);
	m_verticalSplitter->addWidget(this);
	m_propertiesEditor->setProperties(m_eventCompiler.properties());

	qDebug() << m_verticalSplitter->orientation() << m_actualWidget->orientation();

	connect(m_propertiesEditor, SIGNAL(changed()), c(), SLOT(noteEventCompilersChanged()));

	connect(c(), SIGNAL(eventsChanged()), this, SLOT(sourceChanged()));

	const float c_size = 16;
	const float c_margin = 0;

	m_label = new QLabel(this);
	m_label->setGeometry(0, 0, (c_size + c_margin) * 5 + c_size, 14);
	m_label->setStyleSheet("background: white");
	m_label->setText(name());

	QPushButton* b = new QPushButton(this);
	b->setGeometry(0, m_label->height(), c_size, c_size);
	b->setText("X");
	connect(b, SIGNAL(clicked()), SLOT(deleteLater()));

	QPushButton* ex = new QPushButton(this);
	ex->setGeometry(c_size + c_margin, m_label->height(), c_size, c_size);
	ex->setText(">");
	connect(ex, SIGNAL(clicked()), SLOT(exportGraph()));

	QPushButton* d = new QPushButton(this);
	d->setGeometry((c_size + c_margin) * 2, m_label->height(), c_size, c_size);
	d->setText("+");
	connect(d, SIGNAL(clicked()), SLOT(duplicate()));

	m_use = new QPushButton(this);
	m_use->setGeometry((c_size + c_margin) * 3, m_label->height(), c_size, c_size);
	m_use->setText("U");
	m_use->setCheckable(true);
	m_use->setChecked(true);
	connect(m_use, SIGNAL(toggled(bool)), SLOT(onUseChanged()));

	m_channel = new QComboBox(this);
	m_channel->setGeometry((c_size + c_margin) * 4, m_label->height(), c_size * 2 + c_margin, c_size);
	connect(m_channel, SIGNAL(currentIndexChanged(int)), SLOT(channelChanged()));
	m_channel->addItem("-");
	m_channel->addItem("0");
	m_channel->addItem("1");
	m_channel->addItem("2");
	m_channel->addItem("3");

	m_selection = new QComboBox(this);
	m_selection->setGeometry(0, m_label->height() + c_size + c_margin * 2, (c_size + c_margin) * 5 + c_size, c_size);
	connect(m_selection, SIGNAL(currentIndexChanged(int)), SLOT(sourceChanged()));

	initTimeline(c());

	c()->noteEventCompilersChanged();
}

EventsView::~EventsView()
{
	finiTimeline();
	quit();
	clearEvents();
	QWidget* w = parentWidget()->parentWidget();
	setParent(0);
	delete w;
}

void EventsView::onUseChanged()
{
	c()->noteEventCompilersChanged();
}

void EventsView::clearEvents()
{
	cnote << "CLEARING EVENTS OF" << (void*)this << m_savedName;
	m_actualWidget->setOrientation(Qt::Horizontal);
	QMutexLocker l(&x_events);
	m_initEvents.clear();
	m_events.clear();
	m_current.clear();
	m_graphEvents.clear();
	m_auxEvents.clear();
	if (m_eventsEditor)
		m_eventsEditor->scene()->clear();
}

void EventsView::filterEvents()
{
	QMutexLocker l(&x_events);
	int i = 0;
	QList<StreamEvents> filtered;
	filtered.reserve(m_events.size());
	for (StreamEvents const& es: m_events)
	{
		filtered.push_back(StreamEvents());
		for (StreamEvent const& e: es)
			if (isGraph(e.type))
			{
				auto it = m_graphEvents.insert(make_pair(e.temperature, vector<float>()));
				if (it.second)
					it.first->second = vector<float>(m_events.size());
				it.first->second[i] = e.strength;
			}
			else if (isComment(e.type))
			{
				auto it = m_auxEvents.insert(make_pair(e.temperature, map<int, shared_ptr<StreamEvent::Aux> >()));
				it.first->second[i] = e.aux();
			}
			else
				filtered.back().push_back(e);
		++i;
	}
	m_events = filtered;
}

Lightbox::StreamEvents EventsView::cursorEvents() const
{
	StreamEvents ret;
	int ch = m_channel->currentIndex() - 1;
	if (ch >= 0)
		for (StreamEvent e: m_current)
			if (!isGraph(e.type) && !isComment(e.type))
				ret.push_back(e.assignedTo(ch));
	return ret;
}

void EventsView::channelChanged()
{
	onUseChanged();//for now...
}

shared_ptr<StreamEvent::Aux> EventsView::auxEvent(float _temperature, int _pos) const
{
	auto it = m_auxEvents.find(_temperature);
	if (it == m_auxEvents.end())
		return shared_ptr<StreamEvent::Aux>();
	auto rit = it->second.upper_bound(_pos);
	if (rit != it->second.begin())
		rit = prev(rit);
	return rit->second;
}

void EventsView::finalizeEvents()
{
	filterEvents();
	QTimer::singleShot(0, this, SLOT(setNewEvents()));
}

void EventsView::setNewEvents()
{
	m_eventsEditor->setEvents(m_events, 0);
}

Lightbox::StreamEvents EventsView::events(int _i) const
{
	if (m_use && m_use->isChecked())
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

void EventsView::setInitEvents(StreamEvents const& _se)
{
	QMutexLocker l(&x_events);
	m_initEvents = _se;
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
	if (!m_eventCompiler.isNull())
	{
		m_savedName = name();
		m_savedProperties = m_eventCompiler.properties().serialized();
	}
	m_eventCompiler = EventCompiler();

	// Have to clear at the moment, since auxilliary StreamEvent data can have hooks into the shared library that will be unloaded.
	clearEvents();
}

void EventsView::restore()
{
	m_eventCompiler = c()->newEventCompiler(m_savedName);
	m_eventCompiler.properties().deserialize(m_savedProperties);
	m_propertiesEditor->setProperties(m_eventCompiler.properties());
	m_label->setText(name());
}

void EventsView::readSettings(QSettings& _s, QString const& _id)
{
	m_savedName = _s.value(_id + "/name").toString();
	m_savedProperties = _s.value(_id + "/properties").toString().toStdString();
	m_use->setChecked(_s.value(_id + "/use").toBool());
	m_channel->setCurrentIndex(_s.value(_id + "/channel").toInt());
	restore();
}

void EventsView::writeSettings(QSettings& _s, QString const& _id)
{
	_s.setValue(_id + "/name", name());
	_s.setValue(_id + "/properties", QString::fromStdString(m_eventCompiler.properties().serialized()));
	_s.setValue(_id + "/use", m_use->isChecked());
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
			t += c()->hop();
		}
	}
	if (fn.endsWith(".xml"))
		out << "</events>" << endl;
}*/

void EventsView::exportGraph()
{
	QString fn = QFileDialog::getSaveFileName(this, "Export a series of events", QDir::currentPath(), "Native format (*.graph);;CSV format (*.csv *.txt)");
	ofstream out;
	out.open(fn.toLocal8Bit(), ios::trunc);
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
					if (fn.endsWith(".events"))
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
			t += c()->hop();
		}
	}
	if (fn.endsWith(".xml"))
		out << "</events>" << endl;
}


void updateCombo(QComboBox* _box, set<float> const& _temperatures, set<EventType> _types)
{
	QString s =  _box->currentText();
	_box->clear();
	foreach (float n, _temperatures)
	{
		QPixmap pm(16, 16);
		pm.fill(QColor::fromHsvF(n, 0.5, 0.85));
		_box->insertItem(_box->count(), pm, QString("Graph events of temperature %1").arg(n), n);
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

void EventsView::updateEventTypes()
{
	set<float> temperatures;
	set<EventType> types;
	QMutexLocker l(&x_events);
	foreach (auto es, m_events)
		foreach (auto e, es)
			if (e.type >= Graph)
				temperatures.insert(e.temperature);
			else
				types.insert(e.type);
	updateCombo(m_selection, temperatures, types);
}

void EventsView::doRender(QGLFramebufferObject* _fbo, int _dx, int _dw)
{
	if (_fbo->size().isEmpty())
		return;
	QPainter p(_fbo);
	if (!p.isActive())
		return;

	int y = 0;
	int h = height() / (m_eventCompiler.asA<EventCompilerImpl>().graphs().size() + 1);

	auto hop = c()->hop();

	for (CompilerGraph* g: m_eventCompiler.asA<EventCompilerImpl>().graphs())
		if (GraphSpectrum* s = dynamic_cast<GraphSpectrum*>(g))
		{
			auto d = s->data();
			auto ifrom = d.lower_bound(renderingTimeOf(_dx - 3));
			if (ifrom != d.begin())
				--ifrom;
			auto ito = d.upper_bound(renderingTimeOf(_dx + _dw + 3));
			if (ito != d.end())
				++ito;
			int lx = 0;
			bool post = s->isPost();
			auto li = d.begin();
			for (auto i = ifrom; i != ito; ++i)
			{
				int x = renderingPositionOf(i->first);
				if (i != ifrom)
					for (unsigned b = 0, bs = i->second.size(); b < bs; ++b)
					{
						float v = clamp(((post ? i : li)->second[b] - s->min()) / s->delta());
						p.fillRect(QRect(lx, y + b * h / bs, x - lx, (b + 1) * h / bs - b * h / bs), QBrush(QColor(clamp(v * 767, 0, 255), clamp(v * 767 - 256, 0, 255), clamp(v * 767 - 512, 0, 255))));
					}
				lx = x;
				li = i;
			}
			y += h;
		}

	h = height() - y;	// don't waste any space :)
	int ifrom = c()->windowIndex(renderingTimeOf(_dx - 3)) - 1;
	int ito = min(c()->hops(), c()->windowIndex(renderingTimeOf(_dx + _dw + 3))) + 1;
	for (auto g: m_graphEvents)
		if (eventVisible(m_selection->itemData(m_selection->currentIndex()), StreamEvent(1.f, g.first)))
		{
			float n = toHue(g.first);
			p.setPen(QColor::fromHsvF(n, 0.5f, 0.6f * Color::hueCorrection(n)));
			QPoint lp;
			for (int i = ifrom; i < ito; ++i)
			{
				QPoint cp(renderingPositionOf(i * hop), y + h - g.second[i] * h);
				if (i != ifrom)
					p.drawLine(lp, cp);
				lp = cp;
			}
		}
}
