#pragma once

#include "CausalAnalysis.h"
#include "AcausalAnalysis.h"

class JobSource
{
public:
	virtual AcausalAnalysisPtrs ripeAcausalAnalysis(AcausalAnalysisPtr const&) { return AcausalAnalysisPtrs(); }
	virtual CausalAnalysisPtrs ripeCausalAnalysis(CausalAnalysisPtr const&) { return CausalAnalysisPtrs(); }
};
