#include <Common/Global.h>
#include <NotedPlugin/Timeline.h>
#include "CompileEvents.h"
#include "CollateEvents.h"
#include "CompileEventCompilerView.h"
#include "Global.h"
#include "Noted.h"
#include "EventCompilerView.h"
#include "EventsMan.h"
using namespace std;
using namespace lb;

EventsMan::EventsMan(QObject* _p):
	EventsManFace				(_p)
{
	m_compileEventsAnalysis = CausalAnalysisPtr(new CompileEvents);
	m_collateEventsAnalysis = CausalAnalysisPtr(new CollateEvents);
	NotedFace::compute()->registerJobSource(this);
	connect(NotedFace::compute(), SIGNAL(analyzed(AcausalAnalysisPtr)), SLOT(onAnalyzed(AcausalAnalysisPtr)));
}

EventsMan::~EventsMan()
{
	NotedFace::compute()->unregisterJobSource(this);
	cnote << "Killing events views...";
	while (m_eventsViews.size())
		delete *m_eventsViews.begin();
	cnote << "Killed.";
}

template <class _EventsStores>
inline lb::StreamEvents mergedAt(_EventsStores const& _s, int _i)
{
	StreamEvents ses;
	for (EventsStore* es: _s)
		merge(ses, es->events(_i));
	return ses;
}

template <class _EventsStores>
inline lb::StreamEvents mergedAtCursor(_EventsStores const& _s, bool _force, bool _usePre, int _trackPos)
{
	StreamEvents ses;
	for (EventsStore* es: _s)
		if (_force)
			merge(ses, es->cursorEvents());
		else if (_usePre && es->isPredetermined())
			merge(ses, es->events(_trackPos));
		else if (!_usePre && !es->isPredetermined())
			merge(ses, es->cursorEvents());
	return ses;
}

StreamEvents EventsMan::inWindow(unsigned _i, bool _usePredetermined) const
{
	StreamEvents ses;
	if (NotedFace::audio()->isImmediate())
		ses = mergedAtCursor(m_stores, NotedFace::audio()->isCausal(), _usePredetermined, _i);
	else
		ses = mergedAt(m_stores, _i);
	if (ses.size())
		cerr << "Window " << _i << " yields " << ses << endl;
	return ses;
}

lb::SimpleKey EventsMan::hash() const
{
	SimpleKey ret = 0;
	for (auto s: m_stores)
		ret = generateKey(ret, s->hash());
	return ret;
}

void EventsMan::onAnalyzed(AcausalAnalysisPtr _aa)
{
	if (_aa == m_collateEventsAnalysis)
		for (EventCompilerView* ev: m_eventsViews)
			if (!ev->isArchived())
			{
				for (auto const& i: ev->eventCompiler().asA<EventCompilerImpl>().graphMap())
				{
					GraphSpec* gs = i.second;
					QString url = ev->objectName() + "/" + QString::fromStdString(i.first);
					GraphMetadata gm = GraphMetadata(DataSetDataStore::operationKey(gs, ev->operationKey()), {}, gs->name());
					if (GraphChart* gc = dynamic_cast<GraphChart*>(gs))
						gm.setAxes({{ gc->ylabel(), gc->ytx(), gc->yrangeHint() }});
					else if (GraphDenseDenseFixed* gddf = dynamic_cast<GraphDenseDenseFixed*>(gs))
						gm.setAxes({ { gddf->xlabel(), gddf->xtx(), gddf->xrangeHint() }, { gddf->ylabel(), gddf->ytx(), gddf->yrangeHint() } });
					Noted::graphs()->registerGraph(url, gm);
				}
				for (GraphMetadata const& i: ev->eventCompiler().asA<EventCompilerImpl>().graphsX())
				{
					QString url = ev->objectName() + "/" + QString::fromStdString(i.title());
					Noted::graphs()->registerGraph(url, i);
				}
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
		for (EventCompilerView* ev: m_eventsViews)
			ret.push_back(CausalAnalysisPtr(new CompileEventCompilerView(ev)));
	else if ((_finished == m_compileEventsAnalysis && !m_eventsViews.size()) || (dynamic_cast<CompileEventCompilerView*>(&*_finished) && ++m_eventsViewsDone == m_eventsViews.size()))
		ret.push_back(m_collateEventsAnalysis);
	return ret;
}

AcausalAnalysisPtrs EventsMan::ripeAcausalAnalysis(AcausalAnalysisPtr const& _finished)
{
	AcausalAnalysisPtrs ret;
	if (_finished == Noted::audio()->resampleWaveAcAnalysis())
	{
		m_eventsViewsDone = 0;
		ret.push_back(m_compileEventsAnalysis);
	}
	else if (_finished == m_compileEventsAnalysis && m_eventsViews.size())
		for (EventCompilerView* ev: m_eventsViews)
			ret.push_back(AcausalAnalysisPtr(new CompileEventCompilerView(ev)));
	else if ((_finished == m_compileEventsAnalysis && !m_eventsViews.size()) ||
			 (dynamic_cast<CompileEventCompilerView*>(&*_finished) && ++m_eventsViewsDone == m_eventsViews.size()))
		ret.push_back(m_collateEventsAnalysis);
	return ret;
}

void EventsMan::registerEventsView(EventCompilerView* _ev)
{
	cnote << "REGISTER eventsView";
	NotedFace::compute()->suspendWork();
	m_eventsViews.insert(_ev);
	noteEventCompilersChanged();	// OPTIMIZE: Heavy-handed, should only need to recompile the new one.
	NotedFace::compute()->resumeWork();
}

void EventsMan::unregisterEventsView(EventCompilerView* _ev)
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
