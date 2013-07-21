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
		m_graphs.push_back(_name + "/" + c.compute.name());
}

ComputeAnalysis::ComputeAnalysis(std::vector<GenericCompute> const& _c, std::string const& _name):
	CausalAnalysis("Compute Analyzer")
{
	for (auto const& c: _c)
		m_computes.push_back(c);
	for (auto const& c: m_computes)
		m_graphs.push_back(_name + "/" + c.compute.name());
}

ComputeAnalysis::~ComputeAnalysis()
{
	for (auto g: m_graphs)
		NotedFace::graphs()->unregisterGraph(QString::fromStdString(g));
}

void ComputeAnalysis::onAnalyzed()
{
	for (unsigned i = 0; i < m_computes.size(); ++i)
		if (m_computes[i].axes)
			NotedFace::graphs()->registerGraph(QString::fromStdString(m_graphs[i]), GraphMetadata(m_computes[i].compute.hash(), m_computes[i].axes(), m_computes[i].compute.name()));
}

bool ComputeAnalysis::init(bool _willRecord)
{
	if (!_willRecord)
		return true;
	bool shouldCompute = false;
	for (auto const& c: m_computes)
		shouldCompute = !lb::ComputeRegistrar::get()->store(c.compute, true) || shouldCompute;
	return shouldCompute;
}

void ComputeAnalysis::fini(bool _completed, bool _didRecord)
{
	cnote << "ComputeRegistrar: Finishing all DSs";
	if (_completed && _didRecord)
		for (auto const& c: m_computes)
			if (c.axes)
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
