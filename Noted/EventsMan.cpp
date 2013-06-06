#include <Common/Global.h>
#include <NotedPlugin/Timeline.h>
#include "CompileEvents.h"
#include "CollateEvents.h"
#include "CompileEventsView.h"
#include "Global.h"
#include "Noted.h"
#include "EventsView.h"
#include "EventsMan.h"
using namespace std;
using namespace lb;

EventsMan::EventsMan(QObject* _p):
	EventsManFace				(_p)
{
	m_compileEventsAnalysis = CausalAnalysisPtr(new CompileEvents);
	m_collateEventsAnalysis = CausalAnalysisPtr(new CollateEvents);
	NotedFace::compute()->registerJobSource(this);
	connect(NotedFace::compute(), SIGNAL(analyzed(AcausalAnalysis*)), SLOT(onAnalyzed(AcausalAnalysis*)));
}

EventsMan::~EventsMan()
{
	NotedFace::compute()->unregisterJobSource(this);
	cnote << "Killing events views...";
	while (m_eventsViews.size())
		delete *m_eventsViews.begin();
	cnote << "Killed.";
}

void EventsMan::onAnalyzed(AcausalAnalysis* _aa)
{
	if (_aa == &*m_collateEventsAnalysis)
		for (EventsView* ev: m_eventsViews)
			if (!ev->isArchived())
				for (auto const& i: ev->eventCompiler().asA<EventCompilerImpl>().graphMap())
				{
					GraphSpec* gs = i.second;
					QString url = ev->objectName() + "/" + QString::fromStdString(i.first);
					GraphMetadata gm = GraphMetadata(DataSetDataStore::operationKey(gs), {}, gs->name());
					if (GraphChart* gc = dynamic_cast<GraphChart*>(gs))
						gm.setAxes({{ gc->ylabel(), gc->ytx(), gc->yrangeHint() }});
					else if (GraphDenseDenseFixed* gddf = dynamic_cast<GraphDenseDenseFixed*>(gs))
						gm.setAxes({ { gddf->ylabel(), gddf->ytx(), gddf->yrangeHint() }, { gddf->xlabel(), gddf->xtx(), gddf->xrangeHint() } });
					Noted::graphs()->registerGraph(url, gm);
				}
}

CausalAnalysisPtrs EventsMan::ripeCausalAnalysis(CausalAnalysisPtr const& _finished)
{
	CausalAnalysisPtrs ret;
	if (_finished == nullptr)
	{
		m_eventsViewsDone = 0;
		ret.push_back(m_compileEventsAnalysis);
	}
	else if (dynamic_cast<CompileEvents*>(&*_finished) && m_eventsViews.size())
		for (EventsView* ev: m_eventsViews)
			ret.push_back(CausalAnalysisPtr(new CompileEventsView(ev)));
	else if ((_finished == m_compileEventsAnalysis && !m_eventsViews.size()) || (dynamic_cast<CompileEventsView*>(&*_finished) && ++m_eventsViewsDone == m_eventsViews.size()))
		ret.push_back(m_collateEventsAnalysis);
	return ret;
}

AcausalAnalysisPtrs EventsMan::ripeAcausalAnalysis(AcausalAnalysisPtr const& _finished)
{
	AcausalAnalysisPtrs ret;
	if (_finished == Noted::get()->spectraAcAnalysis())
	{
		m_eventsViewsDone = 0;
		ret.push_back(m_compileEventsAnalysis);
	}
	else if (_finished == m_compileEventsAnalysis && m_eventsViews.size())
		for (EventsView* ev: m_eventsViews)
			ret.push_back(AcausalAnalysisPtr(new CompileEventsView(ev)));
	else if ((_finished == m_compileEventsAnalysis && !m_eventsViews.size()) ||
			 (dynamic_cast<CompileEventsView*>(&*_finished) && ++m_eventsViewsDone == m_eventsViews.size()))
		ret.push_back(m_collateEventsAnalysis);
	return ret;
}

void EventsMan::registerEventsView(EventsView* _ev)
{
	cnote << "REGISTER eventsView";
	NotedFace::compute()->suspendWork();
	m_eventsViews.insert(_ev);
	noteEventCompilersChanged();	// OPTIMIZE: Heavy-handed, should only need to recompile the new one.
	NotedFace::compute()->resumeWork();
}

void EventsMan::unregisterEventsView(EventsView* _ev)
{
	cnote << "UNREGISTER eventsView";
	NotedFace::compute()->suspendWork();
	m_eventsViews.remove(_ev);
	notePluginDataChanged();
	NotedFace::compute()->resumeWork();
}

EventCompiler EventsMan::findEventCompiler(QString const& _name) const
{
	for (auto ev: m_eventsViews)
		if (!ev->isArchived() && ev->name() == _name)
			return ev->eventCompiler();
	return EventCompiler();
}

QString EventsMan::getEventCompilerName(EventCompilerImpl* _ec) const
{
	for (auto ev: m_eventsViews)
		if (!ev->isArchived() && &ev->eventCompiler().asA<EventCompilerImpl>() == _ec)
			return ev->name();
	return QString();
}
