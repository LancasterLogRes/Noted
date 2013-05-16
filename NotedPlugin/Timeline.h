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

#include <Common/Time.h>
#include <EventCompiler/StreamEvent.h>
#include "Prerendered.h"

class QWidget;
class NotedFace;

class EventsStore
{
public:
	virtual ~EventsStore() {}

	virtual QString niceName() const = 0;
	virtual lb::StreamEvents events(int _i) const = 0;
	virtual lb::StreamEvents cursorEvents() const = 0;
	virtual unsigned eventCount() const = 0;
	inline bool isPredetermined() const { return eventCount(); }
};

class Timeline
{
public:
	virtual ~Timeline();

	void initTimeline(NotedFace* _nf);
	void finiTimeline();

	virtual QWidget* widget() = 0;
	virtual QColor cursorColor() { return Qt::black; }
	virtual lb::Time highlightDuration() const;
	virtual lb::Time highlightFrom() const;

	lb::Time earliestVisible() const;
	lb::Time pixelDuration() const;

protected:
	NotedFace* m_nf;
};
