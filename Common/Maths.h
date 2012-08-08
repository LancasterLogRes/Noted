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

#include <type_traits>
#include <cmath>
#include <vector>
#include <map>
#include <boost/math/special_functions/fpclassify.hpp>

#include "Global.h"
#include "UnitTesting.h"

namespace Lightbox
{

/// Linear interpolate with templated fixed-point (thousandths) factor.
template <int _x> inline float lerp(float _a, float _b) { return _a * (1.f - _x / 1000.f) + _b * _x / 1000.f; }

template <class T> inline T lerp(double _x, T _a, T _b) { return _a + (_b - _a) * _x; }

/// Square a number.
template <class T> inline T sqr(T _t) { return _t * _t; }

/// Sign of a number
template <class T> inline T sign(T _t) { return _t ? _t > 0 ? 1 : -1 : 0; }

template <class _T>
typename std::remove_reference<decltype(_T()[0])>::type sumOf(_T const& _r)
{
	typedef typename std::remove_reference<decltype(_T()[0])>::type R;
	R ret = R(0);
	foreach (auto i, _r)
		ret += i;
	return ret;
}

template <class _T>
typename std::remove_reference<decltype(_T()[0])>::type productOf(_T const& _r)
{
	typedef typename std::remove_reference<decltype(_T()[0])>::type R;
	R ret = R(1);
	foreach (auto i, _r)
		ret *= i;
	return ret;
}

template <class _T>
typename std::remove_reference<decltype(_T()[0])>::type magnitudeOf(_T const& _r)
{
	typedef typename std::remove_reference<decltype(_T()[0])>::type R;
	R ret = R(0);
	for (auto i: _r)
		ret += sqr(i);
	return sqrt(ret);
}

template <class _Ta, class _Tb>
typename std::remove_reference<decltype(_Ta()[0] * _Tb()[0])>::type cosineSimilarity(_Ta const& _a, _Tb const& _b)
{
	assert(_a.size() == _b.size());
	typedef typename std::remove_reference<decltype(_Ta()[0] * _Tb()[0])>::type R;
	float ma = magnitudeOf(_a);
	float mb = magnitudeOf(_b);
	if (ma == 0.f || mb == 0.f)
		return 0.f;
	R ret = R(0);
	auto bii = _b.begin();
	for (auto ai: _a)
		ret += ai * *(bii++);
	return ret /= (ma * mb);
}

template <class _T>
bool similar(_T _a, _T _b, double _ratio = 0.05)
{
	return (max(_a, _b) - min(_a, _b)) / double(_a + _b) < _ratio / 2;
}

template <class _T>
double dissimilarity(_T _a, _T _b)
{
	return (max(_a, _b) - min(_a, _b)) / double(_a + _b) * 2;
}

template <class _T, class _U>
typename _T::iterator nearest(_T& _map, _U const& _v)
{
	auto it1 = _map.lower_bound(_v);
	auto it2 = it1;
	if (it2 != _map.begin())
		it2--;
	return (it1 == _map.end() || fabs(it2->first - _v) < fabs(it1->first - _v)) ? it2 : it1;
}

template <class _T>
void normalize(_T& _v)
{
	auto r = range(_v);
	auto d = r.second - r.first;
	if (d > 0)
		for (auto v: _v)
			v = (v - r.first) / d;
}

template <class _T>
decltype(_T()[0]) makeUnitMagnitude(_T& _v)
{
	auto m = magnitudeOf(_v);
	if (m)
		for (auto i: _v)
			i /= m;
}

inline double fracPart(double _f) { double r; return modf(_f, &r); }

/// Is the number finite?
inline bool isFinite(float _x)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
//	return _x == _x && !isinf(_x);
	return (reinterpret_cast<uint32_t&>(_x) & 0x7fffffff) < 0x7f800000;
#pragma GCC diagnostic pop
}

template <class T, class U> inline T clamp(T _v, U _min, U _max)
{
	return _v < _min ? _min : _v < _max ? _v : _max;
}

template <class T, class U> inline T clamp(T _v, std::pair<U, U> const& _minMax)
{
	return _v < _minMax.first ? _minMax.first : _v < _minMax.second ? _v : _minMax.second;
}

/// Get the "io" of a number.
template <class T> inline T io(T x)
{
	T const t = 1.e-08;
	T y = 0.5 * x;
	T e = 1.0;
	T de = 1.0;
	T xi;
	T sde;
	for (int i = 1; i < 26; ++i)
	{
		xi = T(i);
		de *= y / xi;
		sde = de * de;
		e += sde;
		if (e * t - sde > 0)
			break;
	}
	return e;
}

/// Different window functions supported.
LIGHTBOX_TEXTUAL_ENUM(WindowFunction, RectangularWindow, HannWindow, HammingWindow, TukeyWindow, KaiserWindow, BlackmanWindow, GaussianWindow)

/// Build a window function as a vector.
std::vector<float> windowFunction(unsigned _size, WindowFunction _f, float _parameter = 0.5f);

/// Transform a window function into its zero-phase variant (i.e. rotate by half its width).
std::vector<float> zeroPhaseWindow(std::vector<float> const& _w);

/// Returns a window that is the given window element-wise scaled by the factor _f.
std::vector<float> scaledWindow(std::vector<float> const& _w, float _f);

}
