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

class QPushButton;

class EventsView: public PrerenderedTimeline, public EventsStore
{
	Q_OBJECT

public:
	explicit EventsView(QWidget* _parent = 0);
	~EventsView();

	void save();
	void restore();
	QString name() const;

	void initEvents();
	void shiftEvents(unsigned _n);
	void appendEvents(Lightbox::StreamEvents const& _se);
	virtual Lightbox::StreamEvents events(int _i) const;

	Lightbox::EventCompiler m_eventCompiler;
	QList<Lightbox::StreamEvents> m_events;
	mutable QMutex x_events;

	Lightbox::StreamEvents m_current;
	QComboBox* m_selection;

public slots:
	void duplicate();
	void edit();

	void onUseChanged();

private:
	virtual void doRender(QImage& _img, int _dx, int _dw);

	QString m_name;
	QPushButton* m_use;
};
