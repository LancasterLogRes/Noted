#include <Common/Global.h>
#include "NotedFace.h"
#include "ComputeAnalysis.h"
using namespace std;
using namespace lb;

ComputeAnalysis::ComputeAnalysis(std::vector<ComputeTask> const& _c, std::string const& _name):
	CausalAnalysis("Compute Analyzer"),
	m_computes(_c)
{
	for (auto const& c: m_computes)
	{
		std::string url = _name + "/" + c.compute.name();
		m_graphs.push_back(url);
		NotedFace::graphs()->registerGraph(QString::fromStdString(url), GraphMetadata(c.compute.hash(), c.axes, c.compute.name()));
	}
}
ComputeAnalysis::~ComputeAnalysis()
{
	for (auto g: m_graphs)
		NotedFace::graphs()->unregisterGraph(QString::fromStdString(g));
}

bool ComputeAnalysis::init(bool _willRecord)
{
	if (!_willRecord)
		return true;
	bool shouldCompute = false;
	for (auto const& c: m_computes)
		shouldCompute = !lb::ComputeRegistrar::get()->store(c.compute) || shouldCompute;
	return shouldCompute;
}

void ComputeAnalysis::fini(bool _completed, bool _didRecord)
{
	cnote << "ComputeRegistrar: Finishing all DSs";
	if (_completed && _didRecord)
		for (auto const& c: m_computes)
			if (auto ds = NotedFace::data()->get(DataKey(NotedFace::audio()->key(), c.compute.hash())))
			{
				for (auto d: c.digests)
					ds->ensureHaveDigest(d);
				ds->done();
			}
}

void ComputeAnalysis::process(unsigned, Time)
{
	for (auto c: m_computes)
		lb::ComputeRegistrar::get()->compute(c.compute);
}
