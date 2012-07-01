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

#include <vector>
#include <map>
using namespace std;

#include "Algorithms.h"
#include "Maths.h"
#include "Peaks.h"
using namespace Lightbox;

template <unsigned _S, unsigned _PeriodX1000>
vector<float> const& parabolaAC()
{
	static vector<float> s_ret(0);
	if (s_ret.empty())
	{
		s_ret.resize(_S);
		for (unsigned i = 0; i < _S; ++i)
			s_ret[i] = sqr(fmod(float(i * 2000) / _PeriodX1000, 2) - 1);
	}
	return s_ret;
}

template <class _T>
std::map<typename element_of<_T>::type, typename element_of<_T>::type> parabolicPeaksT(_T const& _s, unsigned _n)
{
	typedef typename element_of<_T>::type T;
	std::map<T, T> ret;
	int start = -1;
	int peak = -1;
	for (unsigned i = 1; i < _n; ++i)
	{
		if (_s[i] < _s[i - 1]) // descending
		{
			if (start != -1 && peak == -1) // were climbing
				peak = i - 1;
		}
		if (_s[i] > _s[i - 1] || i == _n - 1)	// climbing
		{
			if (start != -1 && peak != -1)
			{
				// found a peak
//				ret[_s[peak] - min(_s[start], _s[i - 1])] = peak;
				std::pair<T, T> pk = parabolicPeakOf(_s[peak - 1], _s[peak], _s[peak + 1]);
				// get local-relative strength +/- 5% of the width.
				ret[pk.second - (_s[start] + _s[i - 1]) / 2] = pk.first + peak;
//				ret[_s[peak]] = peak;

				start = -1;
				peak = -1;
			}
			if (start == -1)
				start = i - 1;
		}
	}
	return ret;
}

std::map<float, float> Lightbox::parabolicPeaks(std::vector<float> const& _s)
{
	return parabolicPeaksT(_s, _s.size());
}

std::map<float, float> Lightbox::parabolicPeaks(float const* _s, unsigned _n)
{
	return parabolicPeaksT(_s, _n);
}

vector<float> const& Lightbox::parabolaAC(unsigned _s, float _period)
{
	static __thread map<unsigned, map<float, vector<float> > >* s_savedP = nullptr;
	if (!s_savedP)
		s_savedP = new map<unsigned, map<float, vector<float> > >;
	map<unsigned, map<float, vector<float> > >& s_saved = *s_savedP;

	auto sp = s_saved[_s].find(_period);
	if (sp == s_saved[_s].end())
	{
		vector<float> r(_s);
		for (unsigned i = 0; i < _s; ++i)
			r[i] = (i > _period / 2) ? sqr(sqr(sqr(fmod(float(i * 2) / _period, 2) - 1)))/* * 2.f - 1.f*/ : 0;
		return s_saved[_s][_period] = r;
	}
	return sp->second;
}

float Lightbox::parabolaACf(float _x, float _period, unsigned _lobe)
{
	return (_x > _period * (_lobe + 0.5f) && _x < _period * (_lobe + 1.5f)) ? sqr(sqr(sqr(fmod(float(_x * 2) / _period, 2) - 1)))/* * 2.f - 1.f*/ : 0;
}

