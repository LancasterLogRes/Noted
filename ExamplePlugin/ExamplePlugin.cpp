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

#include <NotedPlugin/NotedFace.h>
#include <Preprocessors/Spectral.h>
#include "ExamplePlugin.h"
using namespace std;
using namespace lb;

NOTED_PLUGIN(ExamplePlugin);

class ExampleAnalysis: public CausalAnalysis
{
public:
	ExampleAnalysis(): CausalAnalysis("ExampleAnalysis") {}
	virtual void init(bool _willRecord)
	{
		m_ds.reset();
		m_lastRecord.resize(1);
		if (_willRecord)
		{
			m_ds = NotedFace::data()->dataSet(DataKeys(NotedFace::audio()->key(), qHash("ExamplePlugin/ZeroCrossings")));
			m_ds->init(1, NotedFace::audio()->hop());
		}
	}

	virtual void fini(bool _completed, bool _didRecord)
	{
		if (_completed && _didRecord && m_ds)
		{
			m_ds->ensureHaveDigest(MeanDigest);
			m_ds->done();
		}
	}

	virtual void process(unsigned, Time _t)
	{
		std::vector<float> vs(NotedFace::audio()->hopSamples());
		NotedFace::audio()->wave()->populateRaw(_t, &vs);

		// Count zero-crossings.
		int zeroXs = 0;
		for (auto v: vs)
			if ((v > 0) == !(zeroXs % 2))
				++zeroXs;

		m_lastRecord[0] = zeroXs / float(vs.size() / 2);
	}

	virtual void record(unsigned, Time _t)
	{
		cnote << m_lastRecord;
		if (m_ds && !m_ds->haveRaw())
			m_ds->appendRecord(_t, &m_lastRecord);
	}

private:
	DataSetPtr m_ds;
	vector<float> m_lastRecord;
};

ExamplePlugin::ExamplePlugin():
	m_analysis(new ExampleAnalysis)
{
	m_graph = GraphMetadata(qHash("ExamplePlugin/ZeroCrossings"), { { "Proportion", XOf(), Range(0, 1) } }, "Zero-Crossings", false );
	NotedFace::compute()->registerJobSource(this);
	NotedFace::graphs()->registerGraph("ExamplePlugin/ZeroCrossings", m_graph);
}

ExamplePlugin::~ExamplePlugin()
{
	NotedFace::graphs()->unregisterGraph("ExamplePlugin/ZeroCrossings");
	NotedFace::compute()->unregisterJobSource(this);
}

CausalAnalysisPtrs ExamplePlugin::ripeCausalAnalysis(CausalAnalysisPtr const& _finished)
{
	if (_finished == NotedFace::audio()->resampleWaveAcAnalysis())
		return { m_analysis };
	return CausalAnalysisPtrs();
}

AcausalAnalysisPtrs ExamplePlugin::ripeAcausalAnalysis(AcausalAnalysisPtr const& _finished)
{
	if (_finished == NotedFace::audio()->resampleWaveAcAnalysis())
		return { m_analysis };
	return AcausalAnalysisPtrs();
}
