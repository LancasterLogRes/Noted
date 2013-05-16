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

#include <QGraphicsSceneWheelEvent>
#include <QTimer>
#include <fstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <EventCompiler/StreamEvent.h>
#include <NotedPlugin/NotedFace.h>
#include "AttackItem.h"
#include "PeriodItem.h"
#include "SustainItem.h"
#include "SyncPointItem.h"
#include "EventsEditor.h"
#include "EventsEditScene.h"

using namespace std;
using namespace lb;

EventsEditScene::EventsEditScene(QObject* _parent):
	QGraphicsScene	(_parent),
	m_isDirty		(false),
	m_c				(nullptr)
{
}

void EventsEditScene::copyFrom(EventsStore* _ev)
{
	int s = c()->hops();
	double hs = toSeconds(c()->hop()) * 1000;
	for (int i = 0; i < s; ++i)
	{
		foreach (StreamEvent const& se, _ev->events(i))
		{
			switch (se.type)
			{
#define DO(X) \
			case X: \
			{ \
				auto it = new X ## Item(se); \
				addItem(it); \
				it->setPos(QPointF(0, (se.channel == -1) ? 0 : (se.channel * 32 + 16)) + it->evenUp(QPointF(i * hs, 0))); \
				break; \
			}
#include "DoEventTypes.h"
#undef DO
			default:;
			}
		}
	}
	rejigEvents();
	m_isDirty = false;
	emit newScale();
}

void EventsEditScene::setEvents(QList<lb::StreamEvents> const& _es, int _forceChannel)
{
	int s = c()->hops();
	double hs = toSeconds(c()->hop()) * 1000;
	clear();
	for (int i = 0; i < min(_es.size(), s); ++i)
		foreach (StreamEvent se, _es[i])
		{
			if (_forceChannel != -1)
				se.assign(_forceChannel);
			switch (se.type)
			{
#define DO(X) \
			case X: \
			{ \
				auto it = new X ## Item(se); \
				addItem(it); \
				it->setPos(QPointF(0, (se.channel == -1) ? 0 : (se.channel * 32 + 16)) + it->evenUp(QPointF(i * hs, 0))); \
				break; \
			}
#include "DoEventTypes.h"
#undef DO
			default:;
			}
		}
	m_isDirty = false;
	emit newScale();
	if (!m_willRejig)
	{
		m_willRejig = true;
		QTimer::singleShot(0, this, SLOT(rejigEvents()));
	}
}

void EventsEditScene::rejigEvents()
{
	m_willRejig = false;
	QMap<int, StreamEventItem*> lastPSI;
	QMap<int, StreamEventItem*> lastSI;
	int spOrder = 1;
	foreach (auto it, items(Qt::AscendingOrder))
		if (auto sei = dynamic_cast<StreamEventItem*>(it))
		{
			int ch = sei->streamEvent().channel;
			if ((dynamic_cast<PeriodResetItem*>(sei) || dynamic_cast<PeriodTweakItem*>(sei)) && lastPSI[ch])
			{
				PeriodBarItem* pbi = new PeriodBarItem(lastPSI[ch]->pos(), sei->pos(), lastPSI[ch]->streamEvent().constancy * 1000);
				pbi->setZValue(-1);
				addItem(pbi);
			}
			if (dynamic_cast<SustainSuperItem*>(sei) /*&& !dynamic_cast<AttackItem*>(sei)*/ && lastSI[ch])
			{
				SustainBarItem* sbi = new SustainBarItem(lastSI[ch]->pos(), sei->pos(), lastSI[ch]->streamEvent(), sei->streamEvent());
				sbi->setZValue(-1);
				addItem(sbi);
			}
			if (auto psi = dynamic_cast<PeriodSetItem*>(sei))
				lastPSI[ch] = psi;
			if (dynamic_cast<ReleaseItem*>(sei))
				lastSI[ch] = nullptr;
			else if (auto ssi = dynamic_cast<SustainSuperItem*>(sei))
				lastSI[ch] = ssi;
			if (auto c = dynamic_cast<SyncPointItem*>(it))
			{
				c->setOrder(spOrder);
				spOrder++;
			}
		}
		else if (auto c = dynamic_cast<SustainBarItem*>(it))
			delete c;
		else if (auto c = dynamic_cast<PeriodBarItem*>(it))
			delete c;
}

void EventsEditScene::wheelEvent(QGraphicsSceneWheelEvent* _wheelEvent)
{
	QGraphicsScene::wheelEvent(_wheelEvent);
	if (!_wheelEvent->isAccepted())
	{
		int x = view()->mapFromScene(_wheelEvent->scenePos()).x();
		lb::Time t = c()->timeOf(x);
		c()->setPixelDuration(c()->pixelDuration() * exp(-_wheelEvent->delta() / 240.0));
		c()->setTimelineOffset(t - x * c()->pixelDuration());
	}
	_wheelEvent->accept();
}

void EventsEditScene::itemChanged(StreamEventItem* _it)
{
	setDirty(_it->isCausal());
	if (!m_willRejig)
	{
		m_willRejig = true;
		QTimer::singleShot(0, this, SLOT(rejigEvents()));
	}
}

EventsEditor* EventsEditScene::view() const
{
	return dynamic_cast<EventsEditor*>(views().first());
}

NotedFace* EventsEditScene::c() const
{
	return m_c ? m_c : (m_c = view()->c());
}

QList<StreamEvents> EventsEditScene::events(Time _hop) const
{
	QList<StreamEvents> ret;
	Time last = UndefinedTime;
	foreach (QGraphicsItem* it, items(Qt::AscendingOrder))
		if (auto sei = dynamic_cast<StreamEventItem*>(it))
		{
			if (last != fromSeconds(sei->pos().x() / 1000))
			{
				assert(last < fromSeconds(sei->pos().x() / 1000));
				last = fromSeconds(sei->pos().x() / 1000);
				while (ret.size() * _hop <= last)
					ret.push_back(StreamEvents());
			}
			if (ret.size())
				ret.last().push_back(sei->streamEvent());
		}
	return ret;
}

void EventsEditScene::saveTo(QString _filename) const
{
	using boost::property_tree::ptree;
	ptree pt;
	Time last = UndefinedTime;
	ptree* lt = nullptr;
	foreach (QGraphicsItem* it, items(Qt::AscendingOrder))
		if (auto sei = dynamic_cast<StreamEventItem*>(it))
		{
			if (last != fromSeconds(sei->pos().x() / 1000))
			{
				assert(last < fromSeconds(sei->pos().x() / 1000));
				last = fromSeconds(sei->pos().x() / 1000);
				lt = &pt.add("events.time", "");
				lt->put("<xmlattr>.value", last);
				lt->put("<xmlattr>.ms", toMsecs(last));
			}
			StreamEvent const& se = sei->streamEvent();
			ptree& e = lt->add(boost::algorithm::to_lower_copy(toString(se.type)), "");
			e.put("<xmlattr>.strength", se.strength);
			e.put("<xmlattr>.temperature", se.temperature);
			e.put("<xmlattr>.position", se.position);
			e.put("<xmlattr>.channel", se.channel);
			e.put("<xmlattr>.surprise", se.surprise);
			e.put("<xmlattr>.constancy", se.constancy);
			e.put("<xmlattr>.jitter", se.jitter);
			e.put("<xmlattr>.character", toString(se.character));
		}

	ofstream out;
	out.open(_filename.toLocal8Bit().data());
	if (out.is_open())
	{
		write_xml(out, pt);
		m_isDirty = false;
	}
}

void EventsEditScene::setDirty(bool _requiresRecompile)
{
	m_isDirty = true;
	view()->onChanged(_requiresRecompile);
}

void EventsEditScene::loadFrom(QString _filename)
{
	clear();
	ifstream in;
	in.open(_filename.toLocal8Bit().data());
	if (in)
	{
		using boost::property_tree::ptree;
		ptree pt;
		read_xml(in, pt);

		foreach (ptree::value_type const& v, pt.get_child("events"))
			if (v.first == "time")
			{
				int64_t ms = v.second.get<int64_t>("<xmlattr>.ms", 0);
				Time t = v.second.get<Time>("<xmlattr>.value", fromMsecs(ms));
				foreach (ptree::value_type const& w, v.second)
					if (w.first != "<xmlattr>")
					{
						StreamEvent se;
						se.type = toEventType(w.first, false);
						se.strength = w.second.get<float>("<xmlattr>.strength", 1.f);
						se.temperature = w.second.get<float>("<xmlattr>.temperature", 0.f);
						se.jitter = w.second.get<float>("<xmlattr>.jitter", 0.f);
						se.constancy = w.second.get<float>("<xmlattr>.constancy", 0.f);
						se.position = w.second.get<int>("<xmlattr>.position", -1);
						se.surprise = w.second.get<float>("<xmlattr>.surprise", 1.f);
						se.character = toCharacter(w.second.get<string>("<xmlattr>.character", "Dull"));
						se.assign(w.second.get<int>("<xmlattr>.channel", 0));
						if (StreamEventItem* sei = StreamEventItem::newItem(se))
						{
							sei->setPos(toSeconds(t) * 1000, sei->pos().y());
							addItem(sei);
							sei->setZValue(toSeconds(t) * 1000);
						}
					}
			}
	}
	rejigEvents();
	m_isDirty = false;
	emit newScale();
}
