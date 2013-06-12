#include <Common/Global.h>
#include "Global.h"
#include "WorkerThread.h"
#include "Noted.h"
#include "ComputeMan.h"
using namespace std;
using namespace lb;

ComputeMan::ComputeMan():
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

void ComputeMan::registerJobSource(JobSource* _js)
{
	if (!m_sources.contains(_js))
	{
		suspendWork();
		m_sources.insert(_js);
		resumeWork();
	}
}

void ComputeMan::unregisterJobSource(JobSource* _js)
{
	if (m_sources.contains(_js))
	{
		suspendWork();
		m_sources.remove(_js);
		resumeWork();
	}
}

void ComputeMan::invalidate(AcausalAnalysisPtr const& _a)
{
	if (!m_toBeAnalyzed.count(_a))
	{
		suspendWork();
		cnote << "WORK Invalidated " << (_a ? _a->name().toLatin1().data() : "(None)");
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
	if (m_computeThread && m_computeThread->isRunning())
	{
		lb::DebugOutputStream<lb::RightChannel, true>() << "SUSPEND";
		m_computeThread->quit();
		while (!m_computeThread->wait(1000))
			cwarn << "Worker thread not responding :-(";
		m_suspends = 0;
//		cnote << "WORK Suspended";
	}
	else
	{
		++m_suspends;
		lb::DebugOutputStream<lb::RightChannel, true>() << string(m_suspends + 1, '$');
	}
}

void ComputeMan::resumeWork(bool _force)
{
	if (!_force && m_suspends)
	{
		m_suspends--;
		lb::DebugOutputStream<lb::LeftChannel, true>() << string(m_suspends + 1, '$');
	}
	else
	{
		lb::DebugOutputStream<lb::LeftChannel, true>() << "RESUME";
		if (m_computeThread && !m_computeThread->isRunning())
		{
			m_computeThread->start(QThread::LowPriority);
		}
		m_suspends = 0;
	}
}

CausalAnalysisPtrs ComputeMan::ripeCausalAnalysis(CausalAnalysisPtr const& _finished)
{
	CausalAnalysisPtrs ret;

	// Go through all job sources...
	CausalAnalysisPtrs acc;
	for (JobSource* s: m_sources)
		if ((acc = s->ripeCausalAnalysis(_finished)).size())
			ret.insert(ret.end(), acc.begin(), acc.end());

	return ret;
}

AcausalAnalysisPtrs ComputeMan::ripeAcausalAnalysis(AcausalAnalysisPtr const& _finished)
{
	AcausalAnalysisPtrs ret;

	// Go through all job sources...
	AcausalAnalysisPtrs acc;
	for (JobSource* s: m_sources)
		if ((acc = s->ripeAcausalAnalysis(_finished)).size())
			ret.insert(ret.end(), acc.begin(), acc.end());

	return ret;
}

void ComputeMan::initializeCausal(CausalAnalysisPtr const& _lastComplete)
{
	QMutexLocker l(&x_analysis);
	deque<CausalAnalysisPtr> todo;	// will become a member.
	todo.push_back(_lastComplete);
	assert(m_causalQueueCache.empty());

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
