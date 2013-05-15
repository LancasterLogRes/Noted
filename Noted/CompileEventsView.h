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
#include "DataSet.h"

class EventsView;

class DataSetDataStore
{
public:
	DataSetDataStore(std::string const& _name);
	virtual ~DataSetDataStore();

	void fini();

protected:
	// Variable record length if 0. _dense if all hops are stored, otherwise will store sparsely.
	virtual void init(unsigned _recordLength, bool _dense);
	virtual void shiftBuffer(unsigned _index, Lightbox::foreign_vector<float> const& _record);

private:
	DataKey m_key = 0;
	DataSet* m_s = nullptr;
};

class CompileEventsView: public CausalAnalysis
{
public:
	CompileEventsView(EventsView* _ev);
	virtual ~CompileEventsView() {}

	virtual void init(bool _willRecord);
	virtual void process(unsigned _i, Lightbox::Time);
	virtual void record();
	virtual void fini(bool _didRecord);

private:
	EventsView* m_ev;
	Lightbox::EventCompiler m_ec;

};

