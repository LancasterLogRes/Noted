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

class EventsView: public PrerenderedTimeline, public EventsStore
{
	Q_OBJECT
	friend class CompileEventsView;

public:
	EventsView(QWidget* _parent = 0, Lightbox::EventCompiler const& _c = Lightbox::EventCompiler());
	~EventsView();

	virtual QWidget* widget() { return m_actualWidget; }

	void readSettings(QSettings& _s, QString const& _id);
	void writeSettings(QSettings& _s, QString const& _id);

	void save();
	void restore();
	QString name() const;
	virtual QString niceName() const { return name(); }

	Lightbox::EventCompiler const& eventCompiler() const { return m_eventCompiler; }

	QMutex* mutex() const { return &x_events; }
	void clearEvents();
	void setInitEvents(Lightbox::StreamEvents const& _se);
	void appendEvents(Lightbox::StreamEvents const& _se);
	void finalizeEvents();
	virtual Lightbox::StreamEvents events(int _i) const;
	virtual Lightbox::StreamEvents initEvents() const { return m_initEvents; }
	virtual Lightbox::StreamEvents cursorEvents() const { return m_current; }
	virtual unsigned eventCount() const { return 0; }
	std::vector<float> const& graphEvents(float _temperature) const { auto it = m_graphEvents.find(_temperature); if (it != m_graphEvents.end()) return it->second; else return Lightbox::NullVectorFloat; }
	std::shared_ptr<Lightbox::StreamEvent::Aux> auxEvent(float _temperature, int _pos) const;

	void updateEventTypes();

public slots:
	void duplicate();
	void onUseChanged();
	void exportEvents();

private:
	virtual void doRender(QGLFramebufferObject* _fbo, int _dx, int _dw);
	void filterEvents();

	Lightbox::EventCompiler m_eventCompiler;

	QSplitter* m_actualWidget;
	PropertiesEditor* m_propertiesEditor;
	QComboBox* m_selection;
	QPushButton* m_use;
	QLabel* m_label;

	Lightbox::StreamEvents m_initEvents;
	QList<Lightbox::StreamEvents> m_events;
	mutable QMutex x_events;

	Lightbox::StreamEvents m_current;
	std::map<float, std::vector<float> > m_graphEvents;
	std::map<float, std::map<int, std::shared_ptr<Lightbox::StreamEvent::Aux> > > m_auxEvents;

	QString m_savedName;
	std::string m_savedProperties;
};
