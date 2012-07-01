#include "Noted.h"
#include "EventsView.h"
#include "CompileEventsView.h"

using namespace std;
using namespace Lightbox;

CompileEventsView::CompileEventsView(EventsView* _ev):
	CausalAnalysis(QString("Compiling %1 events").arg(_ev->m_eventCompiler.name().c_str())),
	m_ev(_ev)
{
}

void CompileEventsView::init(bool _willRecord)
{
	m_ev->initEvents();
	if (!m_ev->m_eventCompiler.isNull())
	{
		auto ises = m_ev->m_eventCompiler.init(m_ev->c()->spectrumSize(), m_ev->c()->hop(), toBase(2, m_ev->c()->rate()));
		if (_willRecord)
			dynamic_cast<Noted*>(m_ev->c())->appendInitEvents(ises);
	}
}

void CompileEventsView::process(unsigned _i, Lightbox::Time)
{
	vector<float> mag(noted()->spectrumSize());
	vector<float> phase(noted()->spectrumSize());
	{
		if (auto mf = noted()->magSpectrum(_i, 1, true))
			memcpy(mag.data(), mf.data(), sizeof(float) * mag.size());
		if (auto pf = noted()->phaseSpectrum(_i, 1, true))
			memcpy(phase.data(), pf.data(), sizeof(float) * phase.size());
	}
	m_ev->m_current = m_ev->m_eventCompiler.compile(mag, phase, vector<float>());
}

void CompileEventsView::record()
{
	m_ev->appendEvents(m_ev->m_current);
}

