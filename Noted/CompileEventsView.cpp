/* BEGIN COPYRIGHT
 *
 * This file is part of Noted.
 *
 * Copyright ©2011, 2012, Lancaster Logic Response Limited.
 *
 * Noted is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Noted is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Noted.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Noted.h"
#include "EventsView.h"
#include "CompileEventsView.h"

using namespace std;
using namespace Lightbox;

CompileEventsView::CompileEventsView(EventsView* _ev):
	CausalAnalysis(QString("Compiling %1 events").arg(_ev->niceName())),
	m_ev(_ev)
{
}

void CompileEventsView::init(bool _willRecord)
{
	m_ev->clearEvents();
	if (!m_ev->eventCompiler().isNull())
	{
		auto ises = m_ev->m_eventCompiler.init(m_ev->c()->spectrumSize(), m_ev->c()->hop(), toBase(2, m_ev->c()->rate()));
		if (_willRecord)
			m_ev->setInitEvents(ises);
	}
}

void CompileEventsView::process(unsigned _i, Lightbox::Time)
{
	vector<float> mag(noted()->spectrumSize());
	vector<float> phase(noted()->spectrumSize());
	{
		if (auto mf = (noted()->isCausal() || noted()->isPassing()) ? dynamic_cast<Noted*>(noted())->cursorMagSpectrum() : dynamic_cast<Noted*>(noted())->magSpectrum(_i, 1))
			memcpy(mag.data(), mf.data(), sizeof(float) * mag.size());
		if (auto pf = (noted()->isCausal() || noted()->isPassing()) ? dynamic_cast<Noted*>(noted())->cursorPhaseSpectrum() : dynamic_cast<Noted*>(noted())->phaseSpectrum(_i, 1))
			memcpy(phase.data(), pf.data(), sizeof(float) * phase.size());
	}
	m_ev->m_current = m_ev->m_eventCompiler.compile(mag, phase, vector<float>());
}

void CompileEventsView::record()
{
	m_ev->appendEvents(m_ev->m_current);
}

