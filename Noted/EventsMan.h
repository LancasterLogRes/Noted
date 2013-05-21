#pragma once

#include <NotedPlugin/EventsManFace.h>
#include <NotedPlugin/ComputeManFace.h>

class EventsMan: public EventsManFace, public JobSource
{
	Q_OBJECT

public:
	EventsMan(QObject* _p = nullptr);
	virtual ~EventsMan();
	
	virtual void registerEventsView(EventsView* _ev);
	virtual void unregisterEventsView(EventsView* _ev);
	virtual QSet<EventsView*> eventsViews() const { return m_eventsViews; }
	virtual lb::EventCompiler findEventCompiler(QString const& _name) const;
	virtual QString getEventCompilerName(lb::EventCompilerImpl* _ec) const;

protected:
	virtual AcausalAnalysisPtrs ripeAcausalAnalysis(AcausalAnalysisPtr const&);
	virtual CausalAnalysisPtrs ripeCausalAnalysis(CausalAnalysisPtr const&);

private slots:
	void onAnalyzed(AcausalAnalysis* _aa);

private:
	// DEPRECATED.
	QSet<EventsView*> m_eventsViews;

	int m_eventsViewsDone = 0;
};
