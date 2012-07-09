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
#include <cmath>
#include <QSplitter>
#include <QGraphicsOpacityEffect>
#include <QPushButton>
#include <QDebug>
#include <QFrame>
#include <QPaintEvent>
#include <QPainter>
#include <EventCompiler/StreamEvent.h>

#include "Noted.h"
#include "EventsView.h"

using namespace std;
using namespace Lightbox;

// TODO: Properties editor. (Make general Properties edit widget?)

// TODO: Events export. Fix for individual EventsViews.
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

void updateCombo(QComboBox* _box, set<float> const& _natures, set<EventType> _types)
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

void EventsView::updateEventTypes()
{
	set<float> natures;
	set<EventType> types;
	QMutexLocker l(&x_events);
	foreach (auto es, m_events)
		foreach (auto e, es)
			if (e.type >= Graph)
				natures.insert(e.nature);
			else
				types.insert(e.type);
	updateCombo(m_selection, natures, types);
}

EventsView::EventsView(QWidget* _parent, EventCompiler const& _ec):
	PrerenderedTimeline	(_parent),
	m_eventCompiler		(_ec),
	m_use				(nullptr)
{
	connect(c(), SIGNAL(eventsChanged()), this, SLOT(sourceChanged()));
	auto oe = []() -> QGraphicsEffect* { auto ret = new QGraphicsOpacityEffect; ret->setOpacity(0.7); return ret; };

	QPushButton* b = new QPushButton(this);
	b->setGeometry(0, 0, 23, 23);
	b->setText("X");
	b->setGraphicsEffect(oe());
	connect(b, SIGNAL(clicked()), SLOT(deleteLater()));

	m_selection = new QComboBox(this);
	m_selection->setGeometry(0, 25, 98, 23);
	m_selection->setGraphicsEffect(oe());
	connect(m_selection, SIGNAL(currentIndexChanged(int)), SLOT(sourceChanged()));

	QPushButton* ex = new QPushButton(this);
	ex->setGeometry(25, 0, 23, 23);
	ex->setText(">");
	ex->setGraphicsEffect(oe());
	connect(ex, SIGNAL(clicked()), SLOT(exportEvents()));

	QPushButton* d = new QPushButton(this);
	d->setGeometry(50, 0, 23, 23);
	d->setText("+");
	d->setGraphicsEffect(oe());
	connect(d, SIGNAL(clicked()), SLOT(duplicate()));

	QPushButton* e = new QPushButton(this);
	e->setGeometry(75, 0, 23, 23);
	e->setText("E");
	e->setGraphicsEffect(oe());
	connect(e, SIGNAL(clicked()), SLOT(edit()));

	m_use = new QPushButton(this);
	m_use->setGeometry(100, 0, 23, 23);
	m_use->setText("U");
	m_use->setGraphicsEffect(oe());
	m_use->setCheckable(true);
	m_use->setChecked(true);
	connect(e, SIGNAL(toggled(bool)), SLOT(onUseChanged()));

	c()->noteEventCompilersChanged();
}

EventsView::~EventsView()
{
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
}

vector<float> EventsView::graphEvents(float _nature) const
{
	QMutexLocker l(&x_events);
	vector<float> ret;
	foreach (auto es, m_events)
		foreach (auto e, es)
			if (e.type >= Graph && e.nature == _nature)
				ret.push_back(e.strength);
	return ret;
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
		m_name = name();
	m_eventCompiler = EventCompiler();

	// Have to clear at the moment, since auxilliary StreamEvent data can have hooks into the shared library that will be unloaded.
	QMutexLocker l(&x_events);
	m_events.clear();
}

void EventsView::restore()
{
	m_eventCompiler = c()->newEventCompiler(m_name);
}

void EventsView::doRender(QImage& _img, int _dx, int _dw)
{
	if (_img.isNull())
		return;
	QPainter p(&_img);
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
				if (!mins.contains(e.nature) || mins[e.nature] > e.strength)
					mins[e.nature] = e.strength;
				if (!maxs.contains(e.nature) || maxs[e.nature] < e.strength)
					maxs[e.nature] = e.strength;
			}

	const int ySustain = 12;
	const int yBackSustain = 20;
	const int ySpike = 32;
	const int yChain = 40;
	const int yBar = h - 16;
	QLinearGradient barGrad(0, 0, 0, yBar);

	for (int pass = 0; pass < 4; ++pass)
	{
		p.setCompositionMode(QPainter::CompositionMode_Source);
		if (pass == 0)
		{
			barGrad.setColorAt(0.25, Qt::white);
			barGrad.setColorAt(1, QColor(224, 224, 224));
		}
		else
		{
			barGrad.setColorAt(0.25, Qt::transparent);
			barGrad.setColorAt(1, QColor(96, 96, 96));
		}
		QMap<float, QPoint> lastComments;
		QMap<float, QPoint> lastGraphs;
		QPoint lastSpikeChain;
		QPoint lastSustain;
		QPoint lastBackSustain;
		StreamEvent lastSustainEvent;
		StreamEvent lastBackSustainEvent;
		for (int i = 0; i < (int)m_events.size(); ++i)
		{
			Time t = i * c()->hop();
			int px = xOf((i - 1) * c()->hop());
			int x = xOf(t);
			int nx = xOf((i + 1) * c()->hop());
			int mx = (x + nx) / 2;
			bool inView = nx >= r.left() - 160 && px <= r.right() + 160;
			{
				QMap<float, QPoint> comments;
				QMap<float, QPoint> graphs;
				BOOST_FOREACH (Lightbox::StreamEvent e, m_events[i])
					if (eventVisible(m_selection->itemData(m_selection->currentIndex()), e) && (inView || isAlwaysVisible(e.type)))
					{
						float n = e.nature - int(e.nature);
						QPoint pt(x, height() - ((e.type >= Graph) ? ((e.strength - mins[e.nature]) / (maxs[e.nature] - mins[e.nature])) : e.strength) * height());
						QColor c = QColor::fromHsvF(n, 1.f, 1.f * Color::hueCorrection(n * 360));
						QColor cDark = QColor::fromHsvF(n, 0.5f, 0.6f * Color::hueCorrection(n * 360));
						QColor cLight = QColor::fromHsvF(n, 0.5f, 1.0f * Color::hueCorrection(n * 360));
						QColor cPastel = QColor::fromHsvF(n, 0.25f, 1.0f * Color::hueCorrection(n * 360));
						float id = e.nature;

						if (Lightbox::AuxLabel* al = dynamic_cast<AuxLabel*>(&*e.aux()))
						{
							p.setPen(cDark);
							p.setBrush(cPastel);
							p.drawRoundedRect(QRect(pt.x(), pt.y() - 7, p.fontMetrics().width(al->label.c_str()) + 8, 14), 4, 4);
							p.drawText(QRect(pt.x() + 4, pt.y() - 7, 160, 14), Qt::AlignLeft | Qt::AlignVCenter, al->label.c_str());
						}

						if (pass == 3)
							switch (e.type)
							{
							case Lightbox::Spike:	// strength, surprise, nature
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
									p.fillRect(lastSustain.x(), ySustain + 1, x - lastSustain.x(), 4, QBrush(QColor::fromHsvF(lastSustainEvent.nature, 0.25f, 1.0f * Color::hueCorrection(lastSustainEvent.nature * 360))));
									p.fillRect(lastSustain.x(), ySustain, x - lastSustain.x(), 1, QBrush(QColor::fromHsvF(lastSustainEvent.nature, 0.5f, 0.6f * Color::hueCorrection(lastSustainEvent.nature * 360))));
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
									p.fillRect(lastBackSustain.x(), yBackSustain + 1, x - lastBackSustain.x(), 4, QBrush(QColor::fromHsvF(lastBackSustainEvent.nature, 0.25f, 1.0f * Color::hueCorrection(lastBackSustainEvent.nature * 360))));
									p.fillRect(lastBackSustain.x(), yBackSustain, x - lastBackSustain.x(), 1, QBrush(QColor::fromHsvF(lastBackSustainEvent.nature, 0.5f, 0.6f * Color::hueCorrection(lastBackSustainEvent.nature * 360))));
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
							case Lightbox::SpikeA: case Lightbox::SpikeB: case Lightbox::SpikeC: case Lightbox::SpikeD: case Lightbox::SpikeE: case Lightbox::SpikeF:
								p.setBrush(QBrush(cPastel));
								p.setPen(cDark);
								for (int i = 0, y = 30; i < 5; ++i)
								{
									if (e.type - Lightbox::SpikeA == i)
										{
											p.drawEllipse(QRect(mx - 4, y, 8, 8));
											y += 9;
										}
										else
										{
											p.drawLine(mx, y, mx, y + 2);
											y += 3;
										}
								}
								break;
							case Lightbox::PeriodSet:
							{
								p.setPen(cDark);
								QRect r(x, yBar, xOf(t + e.period) - x, 12);
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

						if (pass == 1)
							switch (e.type)
							{
							case Lightbox::Comment:
								p.setPen(cLight);
								if (lastComments.contains(id))
								{
									if (lastComments[id].y() >= h && pt.y() >= h)
									{
										p.setPen(cDark);
										p.drawLine(lastComments[id].x(), h - 1, pt.x(), h - 1);
									}
									else if (lastComments[id].y() < 0 && pt.y() < 0)
									{
										p.setPen(cDark);
										p.drawLine(lastComments[id].x(), 0, pt.x(), 0);
									}
									else
										p.drawLine(lastComments[id], pt);
								}
								lastComments[id] = pt;
								break;
							case Lightbox::Graph:
							case Lightbox::GraphUnder:
							{
								p.setPen(cLight);
								if (lastGraphs.contains(id))
								{
									if (lastGraphs[id].y() >= h && pt.y() >= h)
									{
										p.setPen(cDark);
										p.drawLine(lastGraphs[id].x(), h - 1, pt.x(), h - 1);
									}
									else if (lastGraphs[id].y() < 0 && pt.y() < 0)
									{
										p.setPen(cDark);
										p.drawLine(lastGraphs[id].x(), 0, pt.x(), 0);
									}
									else
										p.drawLine(lastGraphs[id], pt);

									if (e.type == Lightbox::GraphUnder)
									{
										p.setPen(Qt::NoPen);
										p.setBrush(cPastel);
										p.drawPolygon(QPolygon(QVector<QPoint>() << QPoint(x, h) << pt << comments[id] << QPoint(comments[id].x(), h)));
									}
								}
								else
								{
									p.setPen(Qt::NoPen);
									p.setBrush(c);
									p.drawEllipse(pt, 3, 3);
								}
								graphs[id] = pt;
								break;
							}
							case Lightbox::GraphBar:
								break;
							default:
								break;
							}

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
							}
					} // END: Is visible.
				// END: Each event in set.
				if (pass == 1)
				{
					for (auto it = lastGraphs.begin(); it != lastGraphs.end(); ++it)
						if (!graphs.contains(it.key()))
						{
							p.setPen(Qt::NoPen);
							p.setBrush(QColor(it.key()));
							p.drawEllipse(it.value(), 3, 3);
						}
					lastGraphs = graphs;
				}
			} // END: Within view.
		} // END: Each set of events.
	} // END: Passes.
} // END: Function.
