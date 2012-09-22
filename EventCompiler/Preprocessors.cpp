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

#include "Preprocessors.h"
using namespace Lightbox;

void PhaseUnity::init(EventCompilerImpl*)
{
	m_buffer.clear();
	m_last = 0.f;
}

void PhaseUnity::execute(EventCompilerImpl* _eci, Time _t, vector<float> const&, vector<float> const& _phase, std::vector<float> const&)
{
	if (_t > _eci->windowSize())
	{
		unsigned b = _eci->bands();
		float currentPhaseUnity = 0.f;
		for (unsigned i = 1; i < b; ++i)
			currentPhaseUnity += withReflection(abs(_phase[i] - _phase[i - 1]));
		currentPhaseUnity /= _eci->bands() * Pi;
		m_buffer.push_back(currentPhaseUnity);
		if (m_buffer.size() > max<unsigned>(1, _eci->windowSize() / _eci->hop() / 2 - 1))
		{
			m_last = max(0.f, m_buffer.front() - currentPhaseUnity);
			m_buffer.pop_front();
		}
	}
}

