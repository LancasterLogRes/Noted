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

#include <QHash>
#include "Noted.h"
#include "EventsView.h"
#include "CompileEventsView.h"
using namespace std;
using namespace Lightbox;

DataSetDataStore::DataSetDataStore(std::string const& _name)
{
	m_key = qHash(QString::fromStdString(_name));
}

DataSetDataStore::~DataSetDataStore()
{
}

// Variable record length if 0. _dense if all hops are stored, otherwise will store sparsely.
void DataSetDataStore::init(unsigned _recordLength, bool _dense)
{
	m_s = DataMan::get()->dataSet(m_key);
	if (m_s->haveRaw() && m_s->haveDigest(MeanDigest) && m_s->haveDigest(MinMaxInOutDigest))
		m_s = nullptr;
	else
	{
		m_s->init(_recordLength, _dense ? Noted::audio()->hop(): 0, 0);
	}
}

void DataSetDataStore::shiftBuffer(unsigned _index, foreign_vector<float> const& _record)
{
	if (m_s)
		m_s->appendRecord(Noted::audio()->hop() * _index, _record);
}

void DataSetDataStore::fini(DigestFlags _digests)
{
	if (m_s)
	{
		for (DigestFlags f = _digests; f; f &= ~f.highestSet())
			m_s->digest(f.highestSet());
		m_s->done();
	}
	m_s = nullptr;
}

CompileEventsView::CompileEventsView(EventsView* _ev):
	CausalAnalysis(QString("Compiling %1 events").arg(_ev->niceName())),
	m_ev(_ev)
{
}

void CompileEventsView::init(bool _willRecord)
{
	m_ev->clearEvents();
	if (!ec().isNull())
	{
		for (auto const& i: ec().asA<EventCompilerImpl>().graphMap())
		{
			auto ds = new DataSetDataStore(i.first);
			m_dataStores.insert(i.second, ds);
			i.second->setStore(ds);
		}

		ec().init(Noted::get()->spectrumSize(), Noted::audio()->hop(), toBase(2, Noted::audio()->rate()));
	}
}

Lightbox::EventCompiler CompileEventsView::ec() const
{
	return m_ev->m_eventCompiler;
}

void CompileEventsView::process(unsigned _i, Lightbox::Time)
{
	vector<float> mag(noted()->spectrumSize());
	vector<float> phase(noted()->spectrumSize());
	{
		if (auto mf = noted()->isImmediate() ? Noted::get()->cursorMagSpectrum() : Noted::get()->magSpectrum(_i, 1))
			memcpy(mag.data(), mf.data(), sizeof(float) * mag.size());
		if (auto pf = noted()->isImmediate() ? Noted::get()->cursorPhaseSpectrum() : Noted::get()->phaseSpectrum(_i, 1))
			memcpy(phase.data(), pf.data(), sizeof(float) * phase.size());
	}
	m_ev->m_current = m_ev->m_eventCompiler.compile(mag, phase, vector<float>());
}

void CompileEventsView::record()
{
	m_ev->appendEvents(m_ev->m_current);
}

void CompileEventsView::fini(bool _didRecord)
{
	if (_didRecord)
	{
		for (auto i = m_dataStores.begin(); i != m_dataStores.end(); ++i)
		{
			if (dynamic_cast<GraphChart*>(i.key()))
				i.value()->fini(MeanDigest | MinMaxInOutDigest);
			else if (dynamic_cast<GraphDenseDenseFixed*>(i.key()))
				i.value()->fini(MeanDigest);
			else
				i.value()->fini(DigestFlags());
			delete i.value();
			i.key()->setStore(nullptr);
		}
		m_dataStores.clear();

		m_ev->finalizeEvents();
	}
}
