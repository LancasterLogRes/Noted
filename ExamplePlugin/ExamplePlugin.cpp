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

#include <Compute/Compute.h>
#include <NotedPlugin/NotedFace.h>
#include <NotedPlugin/NotedComputeRegistrar.h>
#include <Preprocessors/Spectral.h>
#include "ExamplePlugin.h"
using namespace std;
using namespace lb;

NOTED_PLUGIN(ExamplePlugin);

class ZeroCrossingsImpl: public ComputeImpl<VoidInfo, float>
{
public:
	ZeroCrossingsImpl(Compute<PCMInfo, float> const& _input): input(_input) {}
	virtual SimpleKey hash() { return generateKey("ZeroCrossingsImpl", input.hash()); }
	virtual void compute(std::vector<float>& _v)
	{
		_v.resize(1);
		auto vs = input.get();
		int zeroXs = 0;	// even->-ve, odd->+ve
		for (auto v: vs)
			if ((v > 0) == !(zeroXs % 2))
				++zeroXs;
		_v[0] = (zeroXs - (vs[0] > 0 ? 1 : 0)) / float(vs.size());
	}

	Compute<PCMInfo, float> input;
};
using ZeroCrossings = ComputeBase<ZeroCrossingsImpl>;


/*
class ZeroCrossingsAnalysis: public CausalAnalysis
{
public:
	ZeroCrossingsAnalysis(): CausalAnalysis("Zero-Crossings Analyser") {}
	virtual bool init(bool _willRecord)
	{
		m_ds.reset();
		m_lastRecord.resize(1);
		if (_willRecord)
			m_ds = NotedFace::data()->create(DataKey(NotedFace::audio()->key(), m_operationKey));
		return m_ds && !m_ds->isComplete();
	}

	virtual void fini(bool _completed, bool _didRecord)
	{
		if (_completed && _didRecord && m_ds)
		{
			m_ds->ensureHaveDigest(MinMaxInOutDigest);
			m_ds->done();
		}
	}

	virtual void process(unsigned, Time _t)
	{
		std::vector<float> vs(NotedFace::audio()->hopSamples());
		NotedFace::audio()->wave()->populateSeries(_t - toBase(vs.size() - 1, NotedFace::audio()->rate()), &vs);

		// Count zero-crossings.
		int zeroXs = 0;	// even->-ve, odd->+ve
		for (auto v: vs)
			if ((v > 0) == !(zeroXs % 2))
				++zeroXs;
		m_lastRecord[0] = (zeroXs - (vs[0] > 0 ? 1 : 0)) / float(vs.size());
	}

	virtual void record(unsigned, Time _t)
	{
		if (m_ds && !m_ds->isComplete())
			m_ds->appendRecord(_t, &m_lastRecord);
	}

	SimpleKey operationKey() const { return m_operationKey; }

private:
	DataSetFloatPtr m_ds;
	vector<float> m_lastRecord;
	SimpleKey m_operationKey = qHash(QString("ExamplePlugin/ZeroCrossings"));
};
*/

class ZeroCrossingsAnalysis: public CausalAnalysis
{
public:
	ZeroCrossingsAnalysis(): CausalAnalysis("Zero-Crossings Analyser") {}

	virtual bool init(bool _willRecord)
	{
		return _willRecord ? !ComputeRegistrar::get()->store(zc) : true;
	}

	virtual void fini(bool _completed, bool _didRecord)
	{
		if (_completed && _didRecord)
			if (auto ds = NotedFace::data()->get(DataKey(NotedFace::audio()->key(), zc.hash())))
				ds->ensureHaveDigest(MinMaxInOutDigest);
	}

	virtual void process(unsigned, Time)
	{
		//zc.get();
		ComputeRegistrar::get()->compute(zc);
	}

	SimpleKey operationKey() const { return zc.hash(); }

private:
	ZeroCrossings zc = ZeroCrossings(ComputeRegistrar::feeder());
};


ExamplePlugin::ExamplePlugin()
{
	NotedComputeRegistrar::get();
	auto a = new ZeroCrossingsAnalysis;
	m_analysis = CausalAnalysisPtr(a);
	m_graph = GraphMetadata(a->operationKey(), { { "Proportion", XOf(), Range(0, 1) } }, "Zero-Crossings", false );
	NotedFace::compute()->registerJobSource(this);
	NotedFace::graphs()->registerGraph("ExamplePlugin/ZeroCrossings", m_graph);
}

ExamplePlugin::~ExamplePlugin()
{
	NotedFace::graphs()->unregisterGraph("ExamplePlugin/ZeroCrossings");
	NotedFace::compute()->unregisterJobSource(this);
}

CausalAnalysisPtrs ExamplePlugin::ripeAnalysis(AcausalAnalysisPtr const& _finished)
{
	if (_finished == NotedFace::audio()->resampleWaveAcAnalysis())
		return { m_analysis };
	return CausalAnalysisPtrs();
}

