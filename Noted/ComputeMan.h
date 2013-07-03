#pragma once

#include <set>
#include <QSet>
#include <QMutex>
#include <NotedPlugin/ComputeManFace.h>

class WorkerThread;
class JobSource;

class ComputeManGuiDispatch: public QObject
{
	Q_OBJECT

public slots:
	void onAnalyzed(AcausalAnalysisPtr _a) { _a->onAnalyzed(); }
};

class ComputeMan: public ComputeManFace
{
	Q_OBJECT

public:
	ComputeMan();
	~ComputeMan();

	virtual void suspendWork();
	virtual void abortWork();
	virtual void resumeWork(bool _force = false);

	virtual AcausalAnalysisPtrs ripeAcausalAnalysis(AcausalAnalysisPtr const&);
	virtual CausalAnalysisPtrs ripeCausalAnalysis(CausalAnalysisPtr const&);
	virtual void invalidate(AcausalAnalysisPtr const& _a = nullptr);

	virtual int causalCursorIndex() const { return m_causalCursorIndex; }

	virtual bool carryOn(int _progress);

	virtual void registerJobSource(JobSource* _js);
	virtual void unregisterJobSource(JobSource* _js);

	void initializeCausal(CausalAnalysisPtr const& _lastComplete);
	void finalizeCausal();
	void updateCausal(int _from, int _count);

private:
	bool serviceCompute();
	void finishUp();

	QMutex x_analysis;
	std::set<AcausalAnalysisPtr> m_toBeAnalyzed;

	QSet<JobSource*> m_sources;

	// Causal playback...
	unsigned m_causalSequenceIndex;
	CausalAnalysisPtrs m_causalQueueCache;
	int m_causalCursorIndex;

	int m_suspends = 0;
	WorkerThread* m_computeThread;

	ComputeManGuiDispatch m_guiDispatch;
};
