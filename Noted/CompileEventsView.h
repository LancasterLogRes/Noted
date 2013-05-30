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
#include <EventCompiler/EventCompiler.h>
#include <NotedPlugin/CausalAnalysis.h>
#include <NotedPlugin/DataSet.h>
#include <EventCompiler/GraphSpec.h>

class EventsView;

class CompileEventsView: public CausalAnalysis
{
public:
	CompileEventsView(EventsView* _ev);
	virtual ~CompileEventsView() {}

	virtual void init(bool _willRecord);
	virtual void process(unsigned _i, lb::Time);
	virtual void record();
	virtual void fini(bool _completed, bool _didRecord);

	lb::EventCompiler ec() const;

private:
	EventsView* m_ev;
	QMap<lb::GraphSpec*, DataSetDataStore*> m_dataStores;
};

