#pragma once

#include <vector>
#include <QString>
#include <Common/GraphMetadata.h>
#include <Compute/Compute.h>
#include "DataSet.h"
#include "CausalAnalysis.h"

struct ComputeTask
{
	lb::GenericCompute compute;
	lb::GraphMetadata::Axes axes;
	DigestTypes digests;
};

class ComputeAnalysis: public CausalAnalysis
{
public:
	ComputeAnalysis(std::vector<ComputeTask> const& _c, std::string const& _name = "ComputeAnalysis");
	virtual ~ComputeAnalysis();
	virtual bool init(bool _willRecord);
	virtual void fini(bool _completed, bool _didRecord);
	virtual void process(unsigned, lb::Time);

private:
	std::vector<ComputeTask> m_computes;
	std::vector<std::string> m_graphs;
};

