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

#include "PropertiesEditor.h"
#include "Noted.h"
#include "EventsView.h"

using namespace std;
using namespace Lightbox;

EventsView::EventsView(QWidget* _parent, EventCompiler const& _ec):
	PrerenderedTimeline	(new QSplitter(_parent)),
	m_eventCompiler		(_ec),
	m_use				(nullptr)
{
	m_actualWidget = dynamic_cast<QSplitter*>(parentWidget());
	m_propertiesEditor = new PropertiesEditor(m_actualWidget);
	m_actualWidget->addWidget(this);
	m_actualWidget->addWidget(m_propertiesEditor);
	m_propertiesEditor->setProperties(m_eventCompiler.properties());
	connect(m_propertiesEditor, SIGNAL(changed()), c(), SLOT(noteEventCompilersChanged()));

	connect(c(), SIGNAL(eventsChanged()), this, SLOT(sourceChanged()));
	auto oe = []() -> QGraphicsEffect* { auto ret = new QGraphicsOpacityEffect; ret->setOpacity(0.7); return ret; };

	m_label = new QLabel(this);
	m_label->setGeometry(0, 0, 98, 16);
	m_label->setStyleSheet("background: white");
	m_label->setText(name());

	QPushButton* b = new QPushButton(this);
	b->setGeometry(0, 18, 23, 23);
	b->setText("X");
	b->setGraphicsEffect(oe());
	connect(b, SIGNAL(clicked()), SLOT(deleteLater()));

	QPushButton* ex = new QPushButton(this);
	ex->setGeometry(25, 18, 23, 23);
	ex->setText(">");
	ex->setGraphicsEffect(oe());
	connect(ex, SIGNAL(clicked()), SLOT(exportEvents()));

	QPushButton* d = new QPushButton(this);
	d->setGeometry(50, 18, 23, 23);
	d->setText("+");
	d->setGraphicsEffect(oe());
	connect(d, SIGNAL(clicked()), SLOT(duplicate()));

	m_use = new QPushButton(this);
	m_use->setGeometry(75, 18, 23, 23);
	m_use->setText("U");
	m_use->setGraphicsEffect(oe());
	m_use->setCheckable(true);
	m_use->setChecked(true);
	connect(m_use, SIGNAL(toggled(bool)), SLOT(onUseChanged()));

	m_selection = new QComboBox(this);
	m_selection->setGeometry(0, 43, 98, 23);
	m_selection->setGraphicsEffect(oe());
	connect(m_selection, SIGNAL(currentIndexChanged(int)), SLOT(sourceChanged()));

	c()->noteEventCompilersChanged();
}

EventsView::~EventsView()
{
	quit();
	QWidget* w = parentWidget();
	setParent(0);
	delete w;
	clearEvents();
}

void EventsView::onUseChanged()
{
	c()->noteEventCompilersChanged();
}

void EventsView::clearEvents()
{
	QMutexLocker l(&x_events);
	m_initEvents.clear();
	m_events.clear();
	m_current.clear();
	m_graphEvents.clear();
	m_auxEvents.clear();
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
			if (e.type >= Graph)
			{
				auto it = m_graphEvents.insert(make_pair(e.temperature, vector<float>()));
				if (it.second)
					it.first->second = vector<float>(m_events.size());
				it.first->second[i] = e.strength;
			}
			else if (e.type > Comment && e.type < SyncPoint)
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
}

Lightbox::StreamEvents EventsView::events(int _i) const
{
	if (m_use && m_use->isChecked())
	{
		QMutexLocker l(&x_events);
		if (_i < m_events.size())
			return m_events[_i];
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
	QMutexLocker l(&x_events);
	m_events.clear();
	m_initEvents.clear();
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
	restore();
}

void EventsView::writeSettings(QSettings& _s, QString const& _id)
{
	_s.setValue(_id + "/name", name());
	_s.setValue(_id + "/properties", QString::fromStdString(m_eventCompiler.properties().serialized()));
}

void EventsView::exportEvents()
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
				if (eventVisible(v, e))
				{
					if (fn.endsWith(".xml"))
					{
						if (!timeout++)
							out << QString("\t<time value=\"%1\" ms=\"%2\">").arg(t).arg(toMsecs(t)).toStdString() << endl;
						out << QString("\t\t<%1 strength=\"%2\" character=\"%3\" temperature=\"%4\" surprise=\"%5\" position=\"%6\" period=\"%7\" channel=\"%8\" />").arg(QString::fromStdString(toString(e.type)).toLower()).arg(e.strength).arg(toString(e.character).c_str()).arg(e.temperature).arg(e.surprise).arg(e.position).arg(e.period).arg(e.channel).toStdString() << endl;
					}
					else if (fn.endsWith(".events"))
					{
						if (!timeout++)
							out << t << endl;
						out << (int)e.type << " " << e.strength << " " << (int)e.character << " " << e.temperature << " " << e.surprise << " " << e.position << " " << e.period << " " << e.channel << endl;
					}
					else
					{
						out << toSeconds(t) << " " << (int)e.type << " " << e.strength << " " << (int)e.character << " " << e.temperature << " " << e.surprise << " " << e.position << " " << e.period << " " << e.channel << endl;
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

//	int w = width();
	int h = height();
	QRect r(_dx, 0, _dw, h);

	auto isAlwaysVisible = [](EventType et) { return et == Lightbox::PeriodSet || et == Sustain || et == EndSustain || et == BackSustain || et == EndBackSustain; };

	p.setClipRect(r);

	p.setPen(QColor(224, 224, 224));
	p.drawLine(_dx, h / 2, _dx + _dw - 1, h / 2);

	QMutexLocker l(&x_events);

	QMap<float, float> mins;
	QMap<float, float> maxs;
	foreach (StreamEvents se, m_events)
		foreach (StreamEvent e, se)
			if (e.type >= Graph)
			{
				if (!mins.contains(e.temperature) || mins[e.temperature] > e.strength)
					mins[e.temperature] = e.strength;
				if (!maxs.contains(e.temperature) || maxs[e.temperature] < e.strength)
					maxs[e.temperature] = e.strength;
			}

	const unsigned c_maxElementWidth = 16;
	const int ySustain = 12;
	const int yBackSustain = 20;
	const int ySpike = 32;
	const int yChain = 40;
	const int yBar = h - 16;
	QLinearGradient barGrad(0, 0, 0, yBar);

//	for (int pass = 0; pass < 4; ++pass)
	{
		p.setCompositionMode(QPainter::CompositionMode_Source);
/*		if (pass == 0)
		{
			barGrad.setColorAt(0.25, Qt::white);
			barGrad.setColorAt(1, QColor(224, 224, 224));
		}
		else
		{
			barGrad.setColorAt(0.25, Qt::transparent);
			barGrad.setColorAt(1, QColor(96, 96, 96));
		}*/
		QMap<float, QPoint> lastComments;
		QMap<float, QPoint> lastGraphs;
		QPoint lastSpikeChain;
		QPoint lastSustain;
		QPoint lastBackSustain;
		StreamEvent lastSustainEvent;
		StreamEvent lastBackSustainEvent;

		int ifrom = c()->windowIndex(renderingTimeOf(_dx - c_maxElementWidth));
		int ito = min(c()->hops(), c()->windowIndex(renderingTimeOf(_dx + _dw)));

		auto hop = c()->hop();
		for (auto g: m_graphEvents)
		{
			float n = toHue(g.first) / 360.f;
			p.setPen(QColor::fromHsvF(n, 0.5f, 0.6f * Color::hueCorrection(n * 360)));
			QPoint lp;
			for (int i = ifrom; i < ito; ++i)
			{
				QPoint cp(renderingPositionOf(i * hop), height() - g.second[i] * height());
				if (i != ifrom)
					p.drawLine(lp, cp);
				lp = cp;
			}
		}

		for (int i = ifrom; i < ito; ++i)
		{
			Time t = i * c()->hop();
			int px = renderingPositionOf((i - 1) * c()->hop());
			int x = renderingPositionOf(t);
			int nx = renderingPositionOf((i + 1) * c()->hop());
//			int mx = (x + nx) / 2;
			bool inView = nx >= r.left() - 160 && px <= r.right() + 160;
			{
//				QMap<float, QPoint> comments;
//				QMap<float, QPoint> graphs;
				BOOST_FOREACH (Lightbox::StreamEvent e, m_events[i])
					if (eventVisible(m_selection->itemData(m_selection->currentIndex()), e) && (inView || isAlwaysVisible(e.type)))
					{
						float n = toHue(e.temperature) / 360.f;
						QPoint pt(x, height() - ((e.type >= Graph) ? ((e.strength - mins[e.temperature]) / (maxs[e.temperature] - mins[e.temperature])) : e.strength) * height());
						QColor c = QColor::fromHsvF(n, 1.f, 1.f * Color::hueCorrection(n * 360));
						QColor cDark = QColor::fromHsvF(n, 0.5f, 0.6f * Color::hueCorrection(n * 360));
						QColor cLight = QColor::fromHsvF(n, 0.5f, 1.0f * Color::hueCorrection(n * 360));
						QColor cPastel = QColor::fromHsvF(n, 0.25f, 1.0f * Color::hueCorrection(n * 360));
						float id = e.temperature;

						if (Lightbox::AuxLabel* al = dynamic_cast<AuxLabel*>(&*e.aux()))
						{
							p.setPen(cDark);
							p.setBrush(cPastel);
							p.drawRoundedRect(QRect(pt.x(), pt.y() - 7, p.fontMetrics().width(al->label.c_str()) + 8, 14), 4, 4);
							p.drawText(QRect(pt.x() + 4, pt.y() - 7, 160, 14), Qt::AlignLeft | Qt::AlignVCenter, al->label.c_str());
						}

//						if (pass == 3)
							switch (e.type)
							{
							case Lightbox::Spike:	// strength, surprise, temperature
							{
								int ox = qMax(4, nx - x);
								if (e.strength > 0)
								{
									p.setPen(cDark);
									p.setBrush(QBrush(cPastel));
									p.drawRect(x, ySpike, ox, 8);
									for (int j = 0; j < log2(e.strength) + 6; ++j)
										p.drawLine(QLine(x, ySpike + 10 + j * 2, qMax(x + 2, nx - 1), ySpike + 10 + j * 2));
								}
								else
								{
									p.setPen(cPastel);
									p.setBrush(Qt::NoBrush);
									p.drawRect(x, ySpike, ox, 8);
									for (int j = 0; j < log2(-e.strength) + 6; ++j)
										p.drawLine(QLine(x, ySpike + 10 + j * 2, qMax(x + 2, nx - 1), ySpike + 10 + j * 2));
								}
								p.setPen(cDark);
								if (e.surprise)
									for (int j = 0; j < log2(e.surprise) + 6; ++j)
									{
										p.drawLine(QLine(x + ox + (j + 1) * 2, ySpike, x + ox + (j + 1) * 2, ySpike + 5));
										p.drawLine(QLine(x + ox + (j + 1) * 2, ySpike + 7, x + ox + (j + 1) * 2, ySpike + 8));
									}
								lastSpikeChain = QPoint(x + ox, ySpike);

								break;
							}
							case Lightbox::Chain:
							{
								p.setPen(cDark);
								p.setBrush(QBrush(cPastel));
								int ox = qMax(4, nx - x);
								p.drawRect(x, yChain, ox, 4);
								for (int j = 0; j < log2(e.strength) + 6; ++j)
									p.drawLine(QLine(x, yChain + 6 + j * 2, qMax(x + 2, nx - 1), yChain + 6 + j * 2));
								p.setPen(QColor(0, 0, 0, 64));
								p.drawLine(lastSpikeChain, QPoint(x, yChain));
								p.drawLine(lastSpikeChain + QPoint(0, 4), QPoint(x, yChain + 4));
								lastSpikeChain = QPoint(x + ox, yChain);
								break;
							}
							case Lightbox::Sustain: case Lightbox::EndSustain:
							{
								if (lastSustainEvent.type == Sustain)
								{
									p.fillRect(lastSustain.x(), ySustain + 1, x - lastSustain.x(), 1 + lastSustainEvent.strength * 16, QBrush(QColor::fromHsvF(toHue(lastSustainEvent.temperature) / 360.f, 0.25f, 1.0f * Color::hueCorrection(toHue(lastSustainEvent.temperature)))));
									p.fillRect(lastSustain.x(), ySustain, x - lastSustain.x(), 1, QBrush(QColor::fromHsvF(toHue(lastSustainEvent.temperature) / 360.f, 0.5f, 0.6f * Color::hueCorrection(toHue(lastSustainEvent.temperature)))));
									lastSustain = QPoint(x, ySustain);
								}
								else
								{
									p.fillRect(x, ySustain, 2, 5, cDark);
									lastSustain = QPoint(x + 2, ySustain);
								}
								lastSustainEvent = e;
								break;
							}
							case Lightbox::BackSustain: case Lightbox::EndBackSustain:
							{
								if (lastBackSustainEvent.type == BackSustain)
								{
									p.fillRect(lastBackSustain.x(), yBackSustain + 1, x - lastBackSustain.x(), 4, QBrush(QColor::fromHsvF(toHue(lastBackSustainEvent.temperature) / 360.f, 0.25f, 1.0f * Color::hueCorrection(toHue(lastBackSustainEvent.temperature)))));
									p.fillRect(lastBackSustain.x(), yBackSustain, x - lastBackSustain.x(), 1, QBrush(QColor::fromHsvF(toHue(lastBackSustainEvent.temperature) / 360.f, 0.5f, 0.6f * Color::hueCorrection(toHue(lastBackSustainEvent.temperature)))));
									lastBackSustain = QPoint(x, yBackSustain);
								}
								else
								{
									p.fillRect(x, yBackSustain, 2, 5, cDark);
									lastBackSustain = QPoint(x + 2, yBackSustain);
								}
								lastBackSustainEvent = e;
								break;
							}
							case Lightbox::PeriodSet:
							{
								p.setPen(cDark);
								QRect r(x, yBar, renderingPositionOf(t + e.period) - x, 12);
								p.drawLine(r.left(), r.center().y(), r.right(), r.center().y());
								p.drawLine(r.topLeft(), r.bottomLeft());
								p.drawLine(r.topRight(), r.bottomRight());
								p.setBrush(cPastel);
								QString l = QString("%1 bpm").arg(round(10 * toBpm(e.period)) / 10);
								int tw = p.fontMetrics().width(l) + 8;
								QRect tr(r.center().x() - tw / 2, r.center().y() - 6, tw, 12);
								p.setCompositionMode(QPainter::CompositionMode_SourceOver);
								p.drawRoundedRect(tr, 4, 4);
								p.setPen(Qt::black);
								p.drawText(tr, Qt::AlignCenter, l);
								p.setCompositionMode(QPainter::CompositionMode_Darken);
								break;
							}
							default:
								break;
						}
/*
						if (pass == 0)
							switch (e.type)
							{
							case Lightbox::Beat:
							case Lightbox::Bar:
							case Lightbox::Cycle:
							{
								static int lastBeat = -100000;
								if (lastBeat > -100000)
								{
									barGrad.setColorAt(1, QColor::fromHsv(0, 0, e.position / 4 % 2 ? 224 : 240));
									p.fillRect(lastBeat, 12, x - lastBeat, h, barGrad);
								}
								lastBeat = x;
								break;
							}
							default:;
							}
						if (pass == 2)
							switch (e.type)
							{
							case Lightbox::Cycle:
								p.setPen(QPen(barGrad, 0, Qt::SolidLine));
								p.setBrush(Qt::white);
								p.drawRect(pt.x() - 6, yBar, 12, 12);
								p.drawLine(pt.x() - 1, 0, pt.x() - 1, yBar);
								p.drawLine(pt.x() + 1, 0, pt.x() + 1, yBar);
								p.drawLine(pt.x() - 1, yBar + 12, pt.x() - 1, h);
								p.drawLine(pt.x() + 1, yBar + 12, pt.x() + 1, h);
								p.drawText(pt.x() - 6, yBar, 12, 12, Qt::AlignCenter, "C");
								break;
							case Lightbox::Bar:
								p.setPen(QPen(barGrad, 0, Qt::SolidLine));
								p.setBrush(Qt::white);
								p.drawRect(pt.x() - 6, yBar, 12, 12);
								p.drawLine(pt.x() - 1, 0, pt.x() - 1, yBar);
								p.drawLine(pt.x() + 1, 0, pt.x() + 1, yBar);
								p.drawLine(pt.x() - 1, yBar + 12, pt.x() - 1, h);
								p.drawLine(pt.x() + 1, yBar + 12, pt.x() + 1, h);
								p.drawText(pt.x() - 6, yBar, 12, 12, Qt::AlignCenter, QString::number(e.position / 4 + 1));
								break;
							case Lightbox::Beat:
								p.setPen(QPen(barGrad, 0, Qt::DotLine));
								p.setBrush(Qt::white);
								p.drawLine(pt.x(), 0, pt.x(), yBar);
								p.drawLine(pt.x(), yBar + 12, pt.x(), h);
								p.drawRect(pt.x() - 6, yBar, 12, 12);
								p.drawText(pt.x() - 6, yBar, 12, 12, Qt::AlignCenter, QString::number(e.position / 4 + 1));
								break;
							case Lightbox::Tick:
								p.setPen(QPen(barGrad, 0, Qt::DotLine));
								p.drawLine(pt.x(), 0, pt.x(), yBar);
								p.drawLine(pt.x(), yBar, pt.x(), h);
								break;
							default:
								break;
							}*/
					} // END: Is visible.
				// END: Each event in set.
/*				if (pass == 1)
				{
					for (auto it = lastGraphs.begin(); it != lastGraphs.end(); ++it)
						if (!graphs.contains(it.key()))
						{
							p.setPen(Qt::NoPen);
							p.setBrush(QColor(it.key()));
							p.drawEllipse(it.value(), 3, 3);
						}
					lastGraphs = graphs;
				}*/
			} // END: Within view.
		} // END: Each set of events.
	} // END: Passes.
} // END: Function.
