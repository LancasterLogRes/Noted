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

#include <EventCompiler/StreamEvent.h>
#include <Common/Time.h>

class EventsStore;
class EventsEditor;
class NotedFace;
class ChainItem;
class SpikeChainItem;
class Chained;
class StreamEventItem;

class EventsEditScene: public QGraphicsScene
{
	Q_OBJECT

public:
	explicit EventsEditScene(QObject *parent = 0);
	
	void copyFrom(EventsStore* _ev);
	void rejigEvents();

	NotedFace* c() const;
	EventsEditor* view() const;
	void itemChanged(StreamEventItem* _it);

	bool isDirty() const { return m_isDirty; }
	void setDirty(bool _requiresRecompile);
	void loadFrom(QString _filename);
	void saveTo(QString _filename) const;
	QList<Lightbox::StreamEvents> events(Lightbox::Time _hop) const;

	virtual void wheelEvent(QGraphicsSceneWheelEvent* _wheelEvent);

signals:
	void newScale();

private:
	QMap<ChainItem*, Chained*> m_toChains;
	QMap<SpikeChainItem*, Chained*> m_fromChains;
	mutable bool m_isDirty;
	mutable NotedFace* m_c;
};
