#pragma once

#include "CausalAnalysis.h"
#include "AcausalAnalysis.h"

class JobSource
{
public:
	virtual AcausalAnalysisPtrs ripeAcausalAnalysis(AcausalAnalysisPtr const& _finished) { AcausalAnalysisPtrs ret; for (auto p: ripeAnalysis(_finished)) ret.push_back(p); return ret; }
	virtual CausalAnalysisPtrs ripeCausalAnalysis(CausalAnalysisPtr const& _finished) { return ripeAnalysis(_finished); }

	virtual CausalAnalysisPtrs ripeAnalysis(AcausalAnalysisPtr const&) { return CausalAnalysisPtrs(); }
};
