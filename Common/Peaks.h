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

#pragma once

#include <cmath>
#include <vector>
#include <map>

#include "Global.h"
#include "Maths.h"

namespace Lightbox
{

std::vector<float> const& parabolaAC(unsigned _s, float _period);
float parabolaACf(float _x, float _period, unsigned _lobe = 0);

template <class T>
std::pair<T, T> parabolicPeakOf(T _a, T _b, T _c)
{
	T p = (_a - _c) / (2 * (_a - 2 * _b + _c));
	return make_pair(p, _b - (_a - _c) * p / 4.f);
}

std::map<float, float> parabolicPeaks(std::vector<float> const& _s);
std::map<float, float> parabolicPeaks(float const* _s, unsigned _n);

template <class T> std::map<T, unsigned> peaks(std::vector<T> const& _s)
{
	std::map<float, unsigned> ret;
	int start = -1;
	int peak = -1;
	for (unsigned i = 1; i < _s.size(); ++i)
	{
		if (_s[i] < _s[i - 1]) // descending
		{
			if (start != -1 && peak == -1) // were climbing
				peak = i - 1;
		}
		if (_s[i] > _s[i - 1] || i == _s.size() - 1)	// climbing
		{
			if (start != -1 && peak != -1)
			{
				// found a peak
//				ret[_s[peak] - min(_s[start], _s[i - 1])] = peak;
				ret[_s[peak]] = peak;
				start = -1;
				peak = -1;
			}
			if (start == -1)
				start = i - 1;
		}
	}
	return ret;
}

}
