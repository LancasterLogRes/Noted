#pragma once

#include <vector>
#include <functional>
#include <QString>
#include <Common/GraphMetadata.h>
#include <Compute/Compute.h>
#include "DataSet.h"
#include "CausalAnalysis.h"

struct ComputeTask
{
	ComputeTask() = default;
	ComputeTask(lb::GenericCompute const& _c): compute(_c) {}
	ComputeTask(lb::GenericCompute const& _c, std::function<lb::GraphMetadata::Axes()> const& _axes, DigestTypes const& _ds): compute(_c), axes(_axes), digests(_ds) {}
	lb::GenericCompute compute;
	std::function<lb::GraphMetadata::Axes()> axes;
	DigestTypes digests;
};

class ComputeAnalysis: public CausalAnalysis
{
public:
	ComputeAnalysis(std::vector<ComputeTask> const& _c, std::string const& _name = "ComputeAnalysis");
	ComputeAnalysis(std::vector<lb::GenericCompute> const& _c, std::string const& _name = "ComputeAnalysis");
	virtual ~ComputeAnalysis();
	virtual bool init(bool _willRecord);
	virtual void fini(bool _completed, bool _didRecord);
	virtual void process(unsigned, lb::Time);
	virtual void onAnalyzed();

private:
	std::vector<ComputeTask> m_computes;
	std::vector<std::string> m_graphs;
};

