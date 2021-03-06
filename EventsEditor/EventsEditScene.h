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

#pragma once

#include <QMap>
#include <QGraphicsScene>
#include <QMutex>

#include <EventCompiler/StreamEvent.h>
#include <NotedPlugin/EventsStore.h>
#include <Common/Time.h>

class EventsStore;
class EventsGraphicsView;
class NotedFace;
class Chained;
class StreamEventItem;

class EventsEditScene: public QGraphicsScene//, public EventsStore
{
	Q_OBJECT

public:
	explicit EventsEditScene(QObject *parent = 0);
	
	void copyFrom(EventsStore* _ev);
	void setEvents(QList<lb::StreamEvents> const& _es, int _forceChannel = -1);

	EventsGraphicsView* view() const;
	void itemChanged(StreamEventItem* _it);

	bool isDirty() const { return m_isDirty; }
	void setDirty(bool _requiresRecompile);
	void loadFrom(QString _filename);
	void saveTo(QString _filename) const;
	QList<lb::StreamEvents> events(lb::Time _hop) const;

//	virtual lb::SimpleKey hash() const;
	virtual lb::StreamEvents events(lb::Time _from, lb::Time _before) const;

	virtual void wheelEvent(QGraphicsSceneWheelEvent* _wheelEvent);

public slots:
	void rejigEvents();

signals:
	void newScale();

private:
	mutable bool m_isDirty = false;
	bool m_willRejig = true;
	mutable QMutex x_events;
};
