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
#include "EventCompilerView.h"
#include "CompileEventCompilerView.h"
using namespace std;
using namespace lb;

CompileEventCompilerView::CompileEventCompilerView(EventCompilerView* _ev):
	CausalAnalysis(QString("Compiling %1 events").arg(_ev->niceName())),
	m_ev(_ev)
{
}

bool CompileEventCompilerView::init(bool _willRecord)
{
	m_ev->m_streamEvents.reset();
	m_ev->clearCurrentEvents();

	if (!ec().isNull() && _willRecord)
	{
		m_ev->m_operationKey = qHash(qMakePair(qMakePair(QString::fromStdString(ec().name()), NotedFace::libs()->eventCompilerVersion(QString::fromStdString(ec().name()))), QString::fromStdString(ec().properties().serialized())));
		m_ev->m_streamEvents = NotedFace::data()->create<StreamEvent>(DataKey(NotedFace::audio()->key(), m_ev->m_operationKey));

		for (auto const& i: ec().asA<EventCompilerImpl>().graphMap())
		{
			auto ds = new DataSetDataStore(i.second, m_ev->m_operationKey);
			m_dataStores.insert(i.second, ds);
			i.second->setStore(ds);
		}

		ec().init(Noted::audio()->hop(), toBase(2, Noted::audio()->rate()));

		for (auto i = m_dataStores.begin(); i != m_dataStores.end(); ++i)
			if (!i.value()->isActive())
				i.key()->setStore(nullptr);
	}
	return m_ev->m_streamEvents && !m_ev->m_streamEvents->isComplete();
}

lb::EventCompiler CompileEventCompilerView::ec() const
{
	return m_ev->m_eventCompiler;
}

void CompileEventCompilerView::process(unsigned _i, lb::Time)
{
	vector<float> wave(Noted::audio()->hopSamples());
	if (Noted::audio()->isImmediate())
		(void)0;// TODO
	else
		Noted::audio()->populateHop(_i, wave);
	m_ev->m_current = m_ev->m_eventCompiler.compile(wave);
}

void CompileEventCompilerView::record(unsigned, Time _t)
{
	m_ev->m_streamEvents->appendRecord(_t, &m_ev->m_current);
}

void CompileEventCompilerView::fini(bool _completed, bool _didRecord)
{
	if (_completed && _didRecord)
	{
		for (auto i = m_dataStores.begin(); i != m_dataStores.end(); ++i)
		{
			if (dynamic_cast<GraphChart*>(i.key()))
				i.value()->fini({MeanDigest, MinMaxInOutDigest});
			else if (dynamic_cast<GraphDenseDenseFixed*>(i.key()))
				i.value()->fini({MeanDigest});
			else
				i.value()->fini({});
			delete i.value();
			i.key()->setStore(nullptr);
		}
		m_dataStores.clear();

		if (m_ev->m_streamEvents && !m_ev->m_streamEvents->isComplete())
			m_ev->m_streamEvents->done();

		for (GraphMetadata const& g: ec().asA<EventCompilerImpl>().graphsX())
			if (auto ds = NotedFace::data()->get(DataKey(NotedFace::audio()->key(), g.operationKey())))
			{
				if (ds->isMonotonic())
					if (ds->isFixed())
						if ((ds->isScalar() && (g.hints() & Acyclic)) || !ds->isScalar())
							ds->ensureHaveDigest(MeanDigest);
						else if (ds->isScalar())
							ds->ensureHaveDigest(MinMaxInOutDigest);
				ds->done();
			}
	}
	else
		m_ev->m_streamEvents.reset();
}
