#pragma once

#include <map>
#include <set>
#include <vector>
#include <QMutex>
#include <NotedPlugin/ComputeManFace.h>

class WorkerThread;

class ComputeMan: public ComputeManFace
{
	Q_OBJECT

public:
	ComputeMan();
	~ComputeMan();

	virtual void suspendWork();
	virtual void abortWork();
	virtual void resumeWork(bool _force = false);

	virtual AcausalAnalysisPtr spectraAcAnalysis() const { return m_spectraAcAnalysis; }
	virtual CausalAnalysisPtr compileEventsAnalysis() const { return m_compileEventsAnalysis; }
	virtual CausalAnalysisPtr collateEventsAnalysis() const { return m_collateEventsAnalysis; }
	virtual AcausalAnalysisPtrs ripeAcausalAnalysis(AcausalAnalysisPtr const&);
	virtual CausalAnalysisPtrs ripeCausalAnalysis(CausalAnalysisPtr const&);
	virtual void noteLastValidIs(AcausalAnalysisPtr const& _a = nullptr);

	virtual int causalCursorIndex() const { return m_causalCursorIndex; }

	virtual bool carryOn(int _progress);

	void initializeCausal(CausalAnalysisPtr const& _lastComplete);
	void finalizeCausal();
	void updateCausal(int _from, int _count);

private:
	bool serviceCompute();
	void finishUp();

	QMutex x_analysis;
	std::set<AcausalAnalysisPtr> m_toBeAnalyzed;						// TODO? Needs a lock?
	AcausalAnalysisPtr m_spectraAcAnalysis;				// TODO: register with Noted until it can be simple plugin.
	AcausalAnalysisPtr m_finishUpAcAnalysis;			// TODO: what is this?
	CausalAnalysisPtr m_compileEventsAnalysis;		// TODO: register with EventsMan
	CausalAnalysisPtr m_collateEventsAnalysis;		// TODO: register with EventsMan
	int m_eventsViewsDone = 0;											// TODO: move to EventsMan
	std::map<float, std::vector<float> > m_collatedGraphEvents;			// TODO: move to EventsMan

	// Causal playback...
	unsigned m_causalSequenceIndex;
	CausalAnalysisPtrs m_causalQueueCache;
	int m_causalCursorIndex;

	int m_suspends = 0;
	WorkerThread* m_computeThread;
};
