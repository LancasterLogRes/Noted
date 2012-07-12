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
class CompileEventsView;

class EventsView: public PrerenderedTimeline, public EventsStore
{
	Q_OBJECT
	friend class CompileEventsView;

public:
	EventsView(QWidget* _parent = 0, Lightbox::EventCompiler const& _c = Lightbox::EventCompiler());
	~EventsView();

	virtual QWidget* widget() { return m_actualWidget; }

	void save();
	void restore();
	QString name() const;
	virtual QString niceName() const { return name(); }

	Lightbox::EventCompiler const& eventCompiler() const { return m_eventCompiler; }

	QMutex* mutex() const { return &x_events; }
	void clearEvents();
	void setInitEvents(Lightbox::StreamEvents const& _se);
	void appendEvents(Lightbox::StreamEvents const& _se);
	virtual Lightbox::StreamEvents events(int _i) const;
	virtual Lightbox::StreamEvents initEvents() const { return m_initEvents; }
	std::vector<float> graphEvents(float _nature) const;

	void updateEventTypes();

public slots:
	void duplicate();
	void onUseChanged();
	void exportEvents();

private:
	virtual void doRender(QImage& _img, int _dx, int _dw);

	Lightbox::EventCompiler m_eventCompiler;

	QSplitter* m_actualWidget;
	PropertiesEditor* m_propertiesEditor;

	Lightbox::StreamEvents m_initEvents;
	QList<Lightbox::StreamEvents> m_events;
	mutable QMutex x_events;

	Lightbox::StreamEvents m_current;
	QComboBox* m_selection;

	QString m_name;
	QPushButton* m_use;
};
