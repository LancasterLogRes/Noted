#include <Common/Global.h>
#include <NotedPlugin/PrerenderedTimeline.h>	// TODO: KILL
#include "Global.h"
#include "WorkerThread.h"
#include "CompileEvents.h"	// TODO: KILL
#include "CollateEvents.h"	// TODO: KILL
#include "CompileEventsView.h"	// TODO: KILL
#include "Noted.h"
#include "ComputeMan.h"
using namespace std;
using namespace lb;

// TODO: Move to Noted.
// OPTIMIZE: allow reanalysis of spectra to be data-parallelized.
class SpectraAc: public AcausalAnalysis
{
public:
	SpectraAc(): AcausalAnalysis("Analyzing spectra") {}

	virtual void init()
	{
	}
	virtual unsigned prepare(unsigned _from, unsigned _count, lb::Time _hop)
	{
		(void)_from; (void)_count; (void)_hop;
		return 100;
	}
	virtual void analyze(unsigned _from, unsigned _count, lb::Time _hop)
	{
		(void)_from; (void)_count; (void)_hop;
		Noted::get()->rejigSpectra();
	}
	virtual void fini()
	{
	}
};

ComputeMan::ComputeMan():
	m_spectraAcAnalysis			(new SpectraAc),				// TODO: register with Noted until it can be simple plugin.
	m_compileEventsAnalysis		(new CompileEvents),		// TODO: register with EventsMan
	m_collateEventsAnalysis		(new CollateEvents),		// TODO: register with EventsMan
	m_computeThread				(createWorkerThread([=](){return serviceCompute();}))
{
	moveToThread(m_computeThread);
	connect(m_computeThread, SIGNAL(progressed(QString,int)), SIGNAL(progressed(QString,int)));
}

ComputeMan::~ComputeMan()
{
	delete m_computeThread;
}

bool ComputeMan::carryOn(int _progress)
{
	WorkerThread::setCurrentProgress(_progress);
	return !WorkerThread::quitting();
}

void ComputeMan::noteLastValidIs(AcausalAnalysisPtr const& _a)
{
	if (!m_toBeAnalyzed.count(_a))
	{
		suspendWork();
		cnote << "WORK Last valid is now " << (_a ? _a->name().toLatin1().data() : "(None)");
		m_toBeAnalyzed.insert(_a);
		resumeWork();
	}
}

class Sleeper: QThread { public: using QThread::usleep; };

bool ComputeMan::serviceCompute()
{
	if (m_toBeAnalyzed.size())
	{
		QMutexLocker l(&x_analysis);
		auto wasToBeAnalysed = m_toBeAnalyzed;
		m_toBeAnalyzed.clear();
		if (wasToBeAnalysed.size())
		{
			auto yetToBeAnalysed = wasToBeAnalysed;
			deque<AcausalAnalysisPtr> todo;	// will become a member.
			todo.push_back(nullptr);

			// OPTIMIZE: move into worker code; allow multiple workers.
			// OPTIMIZE: consider searching tree locally and completely, putting toBeAnalyzed things onto global todo, and skipping through otherwise.
			// ...keeping search local until RAAs needed to be done are found.
			m_eventsViewsDone = 0;
			while (todo.size())
			{
				AcausalAnalysisPtr aa = todo.front();
				todo.pop_front();
				if (yetToBeAnalysed.count(aa) && aa)
				{
					WorkerThread::setCurrentDescription(aa->name());
					emit aboutToAnalyze(&*aa);
					cnote << "WORKER Working on " << aa->name().toStdString();
					aa->go(0);
					cnote << "WORKER Finished " << aa->name().toStdString();
					if (WorkerThread::quitting())
					{
						for (auto i: wasToBeAnalysed)
							m_toBeAnalyzed.insert(i);
						break;
					}
					else
						emit analyzed(&*aa);
				}
				else if (aa)
				{
					cnote << "WORKER Skipping job " << aa->name().toStdString();
				}
				AcausalAnalysisPtrs ripe = ripeAcausalAnalysis(aa);
				if (yetToBeAnalysed.count(aa))
				{
					foreach (auto i, ripe)
						yetToBeAnalysed.insert(i);
					yetToBeAnalysed.erase(aa);
				}
				catenate(todo, ripe);
			}
			if (!WorkerThread::quitting() /*&& all other threads finished*/)
			{
				m_computeThread->setProgress(100);
				emit finished();
			}
		}
	}
	else
		Sleeper::usleep(100000);
	return true;
}

void ComputeMan::abortWork()
{
	cnote << "WORK Aborted... Not sure what to do here :-(";
}

void ComputeMan::suspendWork()
{
	cnote << "WORK Suspending..." << m_suspends;
	if (m_computeThread && m_computeThread->isRunning())
	{
		m_computeThread->quit();
		while (!m_computeThread->wait(1000))
			cwarn << "Worker thread not responding :-(";
		m_suspends = 0;
		cnote << "WORK Suspended";
	}
	else
	{
		++m_suspends;
		cnote << "WORK Additional suspended" << m_suspends;
	}
}

void ComputeMan::resumeWork(bool _force)
{
	if (!_force && m_suspends)
	{
		m_suspends--;
		cnote << "WORK One fewer suspend" << m_suspends;
	}
	else
	{
		if (m_computeThread && !m_computeThread->isRunning())
		{
			m_computeThread->start();//QThread::LowPriority);
			cnote << "WORK Resumed" << m_suspends;
		}
		m_suspends = 0;
	}
}

CausalAnalysisPtrs ComputeMan::ripeCausalAnalysis(CausalAnalysisPtr const& _finished)
{
	CausalAnalysisPtrs ret;
	if (_finished == nullptr)
		ret.push_back(m_compileEventsAnalysis);
	else if (dynamic_cast<CompileEvents*>(&*_finished) && Noted::get()->eventsViews().size())
		foreach (EventsView* ev, Noted::get()->eventsViews())
			ret.push_back(CausalAnalysisPtr(new CompileEventsView(ev)));
	else if ((dynamic_cast<CompileEvents*>(&*_finished) && !Noted::get()->eventsViews().size()) || (dynamic_cast<CompileEventsView*>(&*_finished) && ++m_eventsViewsDone == Noted::get()->eventsViews().size()))
		ret.push_back(m_collateEventsAnalysis);

	// Go through all other things that can give CAs; at this point, it's just the plugins
	CausalAnalysisPtrs acc;
	foreach (RealLibraryPtr const& l, Noted::libs()->libraries())
		if (l->plugin && (acc = l->plugin->ripeCausalAnalysis(_finished)).size())
			ret.insert(ret.end(), acc.begin(), acc.end());

	return ret;
}

AcausalAnalysisPtrs ComputeMan::ripeAcausalAnalysis(AcausalAnalysisPtr const& _finished)
{
	AcausalAnalysisPtrs ret;

	// TODO: register this rather than hardcoded here.
	if (_finished == nullptr)
		ret.push_back(Noted::audio()->resampleWaveAcAnalysis());
	else if (_finished == Noted::audio()->resampleWaveAcAnalysis())
		ret.push_back(m_spectraAcAnalysis);
	else if (dynamic_cast<SpectraAc*>(&*_finished))
		ret.push_back(m_compileEventsAnalysis);
	else if (dynamic_cast<CompileEvents*>(&*_finished) && Noted::get()->eventsViews().size())
		foreach (EventsView* ev, Noted::get()->eventsViews())
			ret.push_back(AcausalAnalysisPtr(new CompileEventsView(ev)));
	else if ((dynamic_cast<CompileEvents*>(&*_finished) && !Noted::get()->eventsViews().size()) ||
			 (dynamic_cast<CompileEventsView*>(&*_finished) && ++m_eventsViewsDone == Noted::get()->eventsViews().size()))
		ret.push_back(m_collateEventsAnalysis);

	// Go through all other things that can give CAs; at this point, it's just the plugins
	AcausalAnalysisPtrs acc;
	foreach (RealLibraryPtr const& l, Noted::libs()->libraries())
		if (l->plugin && (acc = l->plugin->ripeAcausalAnalysis(_finished)).size())
			ret.insert(ret.end(), acc.begin(), acc.end());

	return ret;
}

void ComputeMan::initializeCausal(CausalAnalysisPtr const& _lastComplete)
{
	QMutexLocker l(&x_analysis);
	deque<CausalAnalysisPtr> todo;	// will become a member.
	todo.push_back(_lastComplete);
	assert(m_causalQueueCache.empty());

	m_eventsViewsDone = 0;
	while (todo.size())
	{
		CausalAnalysisPtr ca = todo.front();
		todo.pop_front();
		if (ca != _lastComplete)
		{
			ca->initialize(false);

			m_causalQueueCache.push_back(ca);
		}
		auto ripe = ripeCausalAnalysis(ca);
//		cdebug << m_eventsViewsDone << ca << " leads to " << ripe;
		catenate(todo, ripe);
	}

//	cdebug << "Causal queue: " << m_causalQueueCache;
	m_causalSequenceIndex = 0;
	m_causalCursorIndex = Noted::audio()->isCausal() ? 0 : -1;
}

void ComputeMan::finalizeCausal()
{
	for (auto ca: m_causalQueueCache)
		ca->fini(false);
	m_causalQueueCache.clear();
}

void ComputeMan::updateCausal(int _from, int _count)
{
	// TODO: consider reorganising causal stuff to avoid needing audio() here.
	(void)_from;
	Time h = Noted::audio()->hop();
	for (auto ca: m_causalQueueCache)
		ca->noteBatch(m_causalSequenceIndex, _count);
	for (int i = 0; i < _count; ++i, ++m_causalSequenceIndex)
	{
		if (Noted::audio()->isCausal())
			m_causalCursorIndex = clamp(_from + i, 0, (int)Noted::audio()->hops());
		for (auto ca: m_causalQueueCache)
			ca->process(m_causalSequenceIndex, h * m_causalSequenceIndex);
	}
}
