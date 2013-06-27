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

#include "NotedComputeRegistrar.h"
#include "CausalAnalysis.h"
using namespace std;
using namespace lb;

void CausalAnalysis::init()
{
	NotedComputeRegistrar::get()->init();
	m_willCompute = init(true);
}

void CausalAnalysis::fini(bool _completed)
{
	if (m_willCompute && _completed)
		NotedComputeRegistrar::get()->fini();
	fini(_completed, m_willCompute);
}

unsigned CausalAnalysis::prepare(unsigned _from, unsigned _count, lb::Time)
{
	if (m_willCompute)
		noteBatch(_from, _count);
	return _count;
}

void CausalAnalysis::analyze(unsigned _from, unsigned _count, lb::Time _hop)
{
	if (m_willCompute)
		for (unsigned i = 0; i < _count && done(i); ++i)
		{
			lb::Time t = _hop * (_from + i);
			NotedComputeRegistrar::get()->beginTime(t);
			process(_from + i, t);
			record(_from + i, t);
			NotedComputeRegistrar::get()->endTime(t);
		}
}
