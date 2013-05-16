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

#include <iostream>
#include <cmath>

#include <EventCompiler/EventCompiler.h>
#include <EventCompiler/StreamEvent.h>
#include <NotedPlugin/PrerenderedTimeline.h>

#include <QComboBox>
#include <QDebug>
#include <QFrame>
#include <QPaintEvent>
#include <QPainter>
#include <QSplitter>

class PropertiesEditor;
class QPushButton;
class QLabel;
class QSettings;
class CompileEventsView;
class EventsEditor;

class EventsView: public PrerenderedTimeline, public EventsStore
{
	Q_OBJECT
	friend class CompileEventsView;

public:
	EventsView(QWidget* _parent = 0, lb::EventCompiler const& _c = lb::EventCompiler());
	~EventsView();

	virtual QWidget* widget() { return m_actualWidget; }

	void readSettings(QSettings& _s, QString const& _id);
	void writeSettings(QSettings& _s, QString const& _id);

	bool isArchived() const { return eventCompiler().isNull(); }
	void save();
	void restore();
	QString name() const;
	virtual QString niceName() const { return name(); }

	lb::EventCompiler const& eventCompiler() const { return m_eventCompiler; }

	QMutex* mutex() const { return &x_events; }
	void clearEvents();
	void appendEvents(lb::StreamEvents const& _se);
	void finalizeEvents();
	virtual lb::StreamEvents events(int _i) const;
	virtual lb::StreamEvents cursorEvents() const;
	virtual unsigned eventCount() const { return 0; }

public slots:
	void duplicate();
	void onUseChanged();
	void exportGraph();
	void channelChanged();
	void setNewEvents();

private slots:
	void onLibraryUnload(QString _lib);
	void onLibraryLoad(QString _lib);

private:
	lb::EventCompiler m_eventCompiler;

	QSplitter* m_actualWidget;
	QSplitter* m_verticalSplitter;
	EventsEditor* m_eventsEditor;
	PropertiesEditor* m_propertiesEditor;
	QComboBox* m_channel;
	QPushButton* m_use;
	QLabel* m_label;

	QList<lb::StreamEvents> m_events;
	mutable QMutex x_events;

	lb::StreamEvents m_current;

	QString m_savedName;
	std::string m_savedProperties;
};
