#pragma once

#include <QObject>
#include "JobSource.h"

/**
 * @brief Acausal/Causal audio computation manager.
 * Object exists in its own worker thread.
 * This does all the work on the timeline - resamples, calculates spectra, compiles events &c.
 * It picks up the Analysis objects from the central objects and plugins.
 * Analysis objects may have dependencies on other Analysis objects.
 */
class ComputeManFace: public QObject
{
	Q_OBJECT

public:
	ComputeManFace() {}

	virtual AcausalAnalysisPtrs ripeAcausalAnalysis(AcausalAnalysisPtr const&) = 0;
	virtual CausalAnalysisPtrs ripeCausalAnalysis(CausalAnalysisPtr const&) = 0;
	virtual void invalidate(AcausalAnalysisPtr const& _a = nullptr) = 0;

	virtual int causalCursorIndex() const = 0;	///< -1 when !isCausal()

	virtual bool carryOn(int _progress) = 0;

	virtual void registerJobSource(JobSource* _js) = 0;
	virtual void unregisterJobSource(JobSource* _js) = 0;

public slots:
	virtual void suspendWork() = 0;
	virtual void abortWork() = 0;
	virtual void resumeWork(bool _force = false) = 0;

signals:
	void progressed(QString, int);
	void aboutToAnalyze(AcausalAnalysis*);
	void analyzed(AcausalAnalysis*);
	void finished();
};

