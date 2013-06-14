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
#include <NotedPlugin/EventsStore.h>

#include <QMutex>
#include <QComboBox>
#include <QDebug>
#include <QFrame>
#include <QPaintEvent>
#include <QPainter>
#include <QSplitter>

#include <EventsEditor/EventsEditor.h>

class PropertiesEditor;
class QPushButton;
class QLabel;
class QMutex;
class QSettings;
class CompileEventCompilerView;

// Relies on objectName for composing graph's URLs - if this ever changes, make sure all graphs are first unregistered.
class EventCompilerView: public EventsGraphicsView
{
	Q_OBJECT
	friend class CompileEventCompilerView;

public:
	EventCompilerView(QWidget* _parent = 0, lb::EventCompiler const& _c = lb::EventCompiler());
	~EventCompilerView();

	void readSettings(QSettings& _s, QString const& _id);
	void writeSettings(QSettings& _s, QString const& _id);

	bool isArchived() const { return eventCompiler().isNull(); }
	SimpleKey operationKey() const { return m_operationKey; }
	void save();
	void restore();
	QString name() const;
	virtual QString niceName() const { return name(); }

	virtual bool isMutable() const { return false; }

	lb::EventCompiler const& eventCompiler() const { return m_eventCompiler; }

	virtual lb::StreamEvents events(int _i) const;
	virtual lb::StreamEvents cursorEvents() const;
	virtual unsigned eventCount() const { return 0; }

public slots:
	void clearCurrentEvents();
	void onUseChanged();
	void channelChanged();
	void onDataComplete(DataKey);

private slots:
	void onFactoryAvailable(QString _lib, unsigned _version);
	void onFactoryUnavailable(QString _lib);

private:
	lb::EventCompiler m_eventCompiler;
	DataSetPtr<lb::StreamEvent> m_streamEvents;
	SimpleKey m_operationKey = 0;

	QComboBox* m_channel;
	QLabel* m_label;

	lb::StreamEvents m_current;

	QString m_savedName;
	std::string m_savedProperties;
};
