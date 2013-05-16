#pragma once

#include <QObject>
#include "CausalAnalysis.h"
#include "AcausalAnalysis.h"

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

	virtual void noteLastValidIs(AcausalAnalysisPtr const& _a = nullptr) = 0;
	virtual AcausalAnalysisPtr spectraAcAnalysis() const = 0;
	virtual CausalAnalysisPtr compileEventsAnalysis() const = 0;
	virtual CausalAnalysisPtr collateEventsAnalysis() const = 0;
	virtual AcausalAnalysisPtrs ripeAcausalAnalysis(AcausalAnalysisPtr const&) = 0;
	virtual CausalAnalysisPtrs ripeCausalAnalysis(CausalAnalysisPtr const&) = 0;

	virtual int causalCursorIndex() const = 0;	///< -1 when !isCausal()

public slots:
	virtual void suspendWork() = 0;
	virtual void abortWork() = 0;
	virtual void resumeWork(bool _force = false) = 0;

	inline void noteEventCompilersChanged() { noteLastValidIs(spectraAcAnalysis()); }
	inline void notePluginDataChanged() { noteLastValidIs(collateEventsAnalysis()); }

signals:
	void progressed(QString, int);
	void finished();
};

