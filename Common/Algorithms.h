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

#include <cassert>
#include <string>
#include <algorithm>
#include <tuple>
#include <list>
#include <map>
#include <set>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>
#include <type_traits>

#include "Global.h"
#include "Maths.h"
#include "UnitTesting.h"

namespace Lightbox
{

template <class _T>
inline unsigned maxInRange(_T const& _t)
{
	unsigned ret = -1;
	for (unsigned i = 0; i < _t.size(); ++i)
		if (ret == -1 || _t[ret] < _t[i])
			ret = i;
	return ret;
}

template <class _T>
inline unsigned minInRange(_T const& _t)
{
	unsigned ret = -1;
	for (unsigned i = 0; i < _t.size(); ++i)
		if (ret == -1 || _t[ret] > _t[i])
			ret = i;
	return ret;
}

inline void widenToFit(std::pair<float, float>& _x, float _b)
{
	if (_b < _x.first || !isFinite(_x.first))
		_x.first = _b;
	if (_b > _x.second || !isFinite(_x.second))
		_x.second = _b;
}

template <class _Map, class _U>
typename _Map::mapped_type lerpLookup(_Map const& _m, _U _u)
{
	typedef typename _Map::mapped_type R;
	if (_m.empty())
		return R(0);
	auto ge = _m.upper_bound(_u);
	if (ge == _m.begin())
		return ge->second;
	if (ge == _m.end())
		return std::prev(ge)->second;
	if (_U(ge->first) == _u)
		return ge->second;

	auto lt = std::prev(ge);
	return lerp<R>(double(_u - lt->first) / (ge->first - lt->first), lt->second, ge->second);
}

LIGHTBOX_UNITTEST(2, "lerpLookup")
{
	std::map<unsigned, float> m;
	for (unsigned i = 1; i < 10; ++i)
		m[i * i] = float(i) + 0.5f;
	LIGHTBOX_REQUIRE_EQUAL(lerpLookup(m, 0), 1.5f);
	LIGHTBOX_REQUIRE_EQUAL(lerpLookup(m, 100), 9.5f);
	LIGHTBOX_REQUIRE_EQUAL(lerpLookup(m, 16), 4.5f);
	LIGHTBOX_REQUIRE_EQUAL(lerpLookup(m, 42.5), 7.f);
	LIGHTBOX_REQUIRE_EQUAL(lerpLookup(m, 45.75), 7.25f);
}

template <class T, class enable = void>
struct _sorted
{
	typedef T return_type;
	static T sorted(T const& _unsorted)
	{
		T ret = _unsorted;
		sort(ret.begin(), ret.end());
		return ret;
	}
};

template <class> struct is_sorted_container: public std::false_type {};
template <class U> struct is_sorted_container< std::set<U> >: public std::true_type {};
template <class U> struct is_sorted_container< std::multiset<U> >: public std::true_type {};
template <class U, class V> struct is_sorted_container< std::map<U, V> >: public std::true_type {};
template <class U, class V> struct is_sorted_container< std::multimap<U, V> >: public std::true_type {};

template <class U>
struct _sorted<U, typename std::enable_if<is_sorted_container<U>::value>::type>
{
	typedef U return_type;
	static U go(U const& _u) { return _u; }
};

template <class T> typename _sorted<T>::return_type sorted(T const& _s) { return _sorted<T>::go(_s); }

template <class _C, class _P>
unsigned count_if(_C const& _c, _P const& _p) { return std::count_if(_c.begin(), _c.end(), _p); }

template <class _T, class _U>
bool contains(_T const& _c, _U const& _el)
{
	foreach (typename _T::value_type const& i, _c)
		if (_el == i)
			return true;
	return false;
}

template <class _T, class _F>
_T& transform(_T& _r, _F const& _f)
{
	transform(_r.begin(), _r.end(), _r.begin(), _f);
	return _r;
}

template <class _T1, class _T2>
std::map<_T2, _T1> transpose(std::map<_T1, _T2> const& _m)
{
	std::map<_T2, _T1> ret;
	foreach (auto i, _m)
		ret.insert(std::make_pair(i.second, i.first));
	return ret;
}

template <class _Iterator, class _Enable = void> struct has_contiguous_storage: public std::false_type {};
template <> struct has_contiguous_storage<std::vector<float>::iterator, void>: public std::true_type {};
template <> struct has_contiguous_storage<std::vector<float>::const_iterator, void>: public std::true_type {};
template <class _E> struct has_contiguous_storage<_E*, void>: public std::true_type {};
template <class _E> struct has_contiguous_storage<_E const*, void>: public std::true_type {};

template <class _Iterator> struct element_of { typedef typename _Iterator::value_type type; };
template <class _E> struct element_of<_E*> { typedef _E type; };
template <class _E> struct element_of<_E const*> { typedef _E type; };

template <class _A, class _B, class _Fbrew, class _Fdistill>
typename element_of<_A>::type packCombine(_A const& _a, _B const& _b, unsigned _s, _Fbrew const& _brew, _Fdistill const& _distill)
{
	v4sf atv = {0, 0, 0, 0};
	float* at = (float*)&atv;
	v4sf btv = {0, 0, 0, 0};
	float* bt = (float*)&btv;
	v4sf accv = {0, 0, 0, 0};
	float* acc = (float*)&accv;

	if (false)
	{
		_A a = _a;
		_B b = _b;
		_A ae = next(a, _s);
		for (; a != ae; ++a, ++b)
		{
			at[0] = *a;
			bt[0] = *b;
			_brew(accv, atv, btv);
		}
	}
	else if (has_contiguous_storage<_A>::value)
	{
		float const* a = &_a[0];
		float const* b = &_b[0];
		v4sf const* av = (v4sf const*)(intptr_t(a + 3) & ~intptr_t(15));
		float const* ae = a + _s;
		v4sf const* aev = (v4sf const*)(intptr_t(ae) & ~intptr_t(15));

		for (; a < (float const*)av; ++a, ++b)
		{
			at[0] = *a;
			bt[0] = *b;
			_brew(accv, atv, btv);
		}

		if ((intptr_t(b) & 15) == 0)
		{
			v4sf* pav = (v4sf*)a;
			v4sf* pbv = (v4sf*)b;
			for (; pav != aev; ++pav, ++pbv)
				_brew(accv, *pav, *pbv);
		}
		else
		{
			v4sf* pav = (v4sf*)a;
			for (; pav != aev; ++pav, b+=4)
			{
				bt[0] = b[0];
				bt[1] = b[1];
				bt[2] = b[2];
				bt[3] = b[3];
				_brew(accv, *pav, btv);
			}
		}

		at[1] = at[2] = at[3] = 0.f;
		bt[1] = bt[2] = bt[3] = 0.f;
		for (a = (float const*)aev, b = (float const*)aev - &_a[0] + &_b[0]; a < ae; ++a, ++b)
		{
			at[0] = *a;
			bt[0] = *b;
			_brew(accv, atv, btv);
		}
	}
	else
	{
		_A a = _a;
		_B b = _b;
		_A ae = next(a, _s);

		if (_s > 7)
		{
			_A ap3 = next(a, 3);
			_B bp3 = next(b, 3);
			_A aem3 = next(a, _s - 3);
			for (; ap3 < aem3;)
			{
				bool agood = (&*a + 3 == &*ap3) && (intptr_t(&*a) & 15) == 0;
				bool bgood = (&*b + 3 == &*bp3) && (intptr_t(&*b) & 15) == 0;

				if (agood)
				{
					if (bgood)
					{
						_brew(accv, *(v4sf const*)&*a, *(v4sf const*)&*b);
						advance(a, 4);
						advance(b, 4);
						advance(ap3, 4);
						advance(bp3, 4);
					}
					else
					{
						bt[0] = *b++;
						bt[1] = *b++;
						bt[2] = *b++;
						bt[3] = *b++;
						_brew(accv, *(v4sf const*)&*a, btv);
						advance(a, 4);
						advance(ap3, 4);
						advance(bp3, 4);
					}
				}
				else if (bgood)
				{
					at[0] = *a++;
					at[1] = *a++;
					at[2] = *a++;
					at[3] = *a++;
					_brew(accv, atv, *(v4sf const*)&*b);
					advance(b, 4);
					advance(ap3, 4);
					advance(bp3, 4);
				}
				else
				{
					at[0] = *a;
					at[1] = 0;
					at[2] = 0;
					at[3] = 0;
					bt[0] = *b;
					bt[1] = 0;
					bt[2] = 0;
					bt[3] = 0;
					_brew(accv, atv, btv);
					++a;
					++b;
					++ap3;
					++bp3;
				}
			}
		}
		for (int i = 0; i < 4; ++i)
			if (a == ae)
				at[i] = 0, bt[i] = 0;
			else
				at[i] = *a++, bt[i] = *b++;
		_brew(accv, atv, btv);
	}
	return _distill(acc);
}

template <class _A, class _Fbrew, class _Fdistill>
typename element_of<_A>::type packEvaluate(_A const& _a, unsigned _s, _Fbrew const& _brew, _Fdistill const& _distill)
{
	v4sf atv = {0, 0, 0, 0};
	float* at = (float*)&atv;
	v4sf accv = {0, 0, 0, 0};
	float* acc = (float*)&accv;

	assert(has_contiguous_storage<_A>::value);
	{
		float const* a = &_a[0];
		v4sf const* av = (v4sf const*)(intptr_t(a + 3) & ~intptr_t(15));
		float const* ae = a + _s;
		v4sf const* aev = (v4sf const*)(intptr_t(ae) & ~intptr_t(15));

		for (; a < (float const*)av; ++a)
		{
			at[0] = *a;
			_brew(accv, atv);
		}

		v4sf* pav = (v4sf*)a;
		for (; pav != aev; ++pav)
			_brew(accv, *pav);

		at[1] = at[2] = at[3] = 0.f;
		for (a = (float const*)aev; a < ae; ++a)
		{
			at[0] = *a;
			_brew(accv, atv);
		}
	}

	return _distill(acc);
}

template <class _A, class _Fxform>
void packTransform(_A const& _a, unsigned _s, _Fxform const& _xform)
{
	v4sf atv = {0, 0, 0, 0};
	float* at = (float*)&atv;

	assert (has_contiguous_storage<_A>::value);
	{
		float* a = &*_a;
		v4sf* av = (v4sf*)(intptr_t(a + 3) & ~intptr_t(15));
		float* ae = a + _s;
		v4sf* aev = (v4sf*)(intptr_t(ae) & ~intptr_t(15));

		for (; a < (float*)av; ++a)
		{
			at[0] = *a;
			_xform(atv);
			*a = at[0];
		}

		v4sf* pav = (v4sf*)a;
		for (; pav != aev; ++pav)
			_xform(*pav);

		at[1] = at[2] = at[3] = 0.f;
		for (a = (float*)aev; a < ae; ++a)
		{
			at[0] = *a;
			_xform(atv);
			*a = at[0];
		}
	}
}

template <class _A, class _B, class _Fxform>
void packTransform(_A _a, _B _b, unsigned _s, _Fxform const& _xform)
{
	typedef typename std::remove_const<typename std::remove_reference<decltype(*_a)>::type>::type ElementType;
	typedef ElementType VectorType __attribute__ ((vector_size (sizeof(ElementType) * 4)));
	static const int VectorSize = sizeof(ElementType) * 4;
	static const int VectorSizeLess1 = VectorSize - 1;

	VectorType atv = {0, 0, 0, 0};
	ElementType* at = (ElementType*)&atv;
	VectorType btv = {0, 0, 0, 0};
	ElementType* bt = (ElementType*)&btv;

	if (false)
	{
		_A a = _a;
		_B b = _b;
		_A ae = next(a, _s);
		for (; a != ae; ++a, ++b)
		{
			at[0] = *a;
			bt[0] = *b;
			_xform(atv, btv);
			*a = at[0];
		}
	}
	else if (has_contiguous_storage<_A>::value)
	{
		ElementType* a = &_a[0];
		ElementType const* b = &_b[0];
		VectorType* av = (VectorType*)(intptr_t(a + 3) & ~intptr_t(VectorSizeLess1));
		ElementType* ae = a + _s;
		VectorType* aev = (VectorType*)(intptr_t(ae) & ~intptr_t(VectorSizeLess1));

		for (; a < (ElementType*)av; ++a, ++b)
		{
			at[0] = *a;
			bt[0] = *b;
			_xform(atv, btv);
			*a = at[0];
		}

		if ((intptr_t(b) & VectorSizeLess1) == 0)
		{
			VectorType* pav = (VectorType*)a;
			VectorType* pbv = (VectorType*)b;
			for (; pav != aev; ++pav, ++pbv)
				_xform(*pav, *pbv);
		}
		else
		{
			VectorType* pav = (VectorType*)a;
			for (; pav != aev; ++pav, b+=4)
			{
				bt[0] = b[0];
				bt[1] = b[1];
				bt[2] = b[2];
				bt[3] = b[3];
				_xform(*pav, btv);
			}
		}

		at[1] = at[2] = at[3] = 0.f;
		bt[1] = bt[2] = bt[3] = 0.f;
		for (a = (ElementType*)aev, b = (ElementType*)aev - &_a[0] + &_b[0]; a < ae; ++a, ++b)
		{
			at[0] = *a;
			bt[0] = *b;
			_xform(atv, btv);
			*a = at[0];
		}
	}
	else
	{
		_A a = _a;
		_B b = _b;
		_A ae = next(a, _s);
		_A ae4 = next(a, _s & ~3u);

		for (; a != ae4;)
		{
			bool agood = (&*a + 3 == &*next(a, 3)) && (intptr_t(&*a) & VectorSizeLess1) == 0;
			if (agood)
			{
				bool bgood = (&*b + 3 == &*next(b, 3)) && (intptr_t(&*b) & VectorSizeLess1) == 0;
				if (bgood)
				{
					_xform(*(VectorType*)&*a, *(VectorType const*)&*b);
					a += 4;
					b += 4;
				}
				else
				{
					bt[0] = *b++;
					bt[1] = *b++;
					bt[2] = *b++;
					bt[3] = *b++;
					_xform(*(VectorType*)&*a, btv);
					a += 4;
				}
			}
			else
			{
				at[0] = *a;
				at[1] = 0;
				at[2] = 0;
				at[3] = 0;
				bt[0] = *b;
				bt[1] = 0;
				bt[2] = 0;
				bt[3] = 0;
				_xform(atv, btv);
				*a = at[0];
				++a;
				++b;
			}
		}
		at[1] = at[2] = at[3] = 0.f;
		bt[1] = bt[2] = bt[3] = 0.f;
		for (; a != ae; ++a, ++b)
		{
			at[0] = *a;
			bt[0] = *b;
			_xform(atv, btv);
			*a = at[0];
		}
	}
}

template <class _A, class _B>
typename element_of<_A>::type similarity(_A _a, _B _b, unsigned _s)
{
	return packCombine(_a, _b, _s,
	   [](v4sf& acc, v4sf const& a, v4sf const& b) { v4sf ab = a - b; acc = acc + ab * ab; },
	   [=](float const* acc) { return -(acc[0] + acc[1] + acc[2] + acc[3]); });
}

template <class _A, class _B>
typename element_of<_A>::type correlation(_A _a, _B _b, unsigned _s)
{
	return packCombine(_a, _b, _s,
	   [](v4sf& acc, v4sf const& a, v4sf const& b) { acc = acc + a * b; },
	   [=](float const* acc) { return acc[0] + acc[1] + acc[2] + acc[3]; });
}

template <class _A, class _B>
typename element_of<_A>::type dissimilarity(_A _a, _B _b, unsigned _s)
{
	return packCombine(_a, _b, _s,
	   [](v4sf& acc, v4sf const& a, v4sf const& b) { v4sf ab = a - b; acc = acc + ab * ab; },
	   [=](float const* acc) { return acc[0] + acc[1] + acc[2] + acc[3]; });
}

template <class _T>
_T correlation(std::vector<_T> _a, std::vector<_T> _b)
{
	return correlation(_a.begin(), _b.begin(), min(_a.size(), _b.size()));
}

template <class _F, class _It>
std::vector<typename element_of<_It>::type> autocross(_It _begin, int _s, _F const& _f, unsigned _maxPeriod)
{
	typedef typename element_of<_It>::type T;
	int rs = std::min<int>(_maxPeriod, _s - 1);
	std::vector<T> ret(rs);
	_It ap = _begin;
	for (int p = 0; p < rs; ++p, ++ap)
		ret[p] = _f(_begin, ap, (_s - p)) / (_s - p);
	return ret;
}

template <class _F, class _It>
void autocross(_It _begin, int _s, _F const& _f, unsigned _maxPeriod, unsigned _movingBy, std::vector<typename element_of<_It>::type>& _ret)
{
	typedef typename element_of<_It>::type T;
	int rs = std::min<int>(_maxPeriod, _s - 1);
	std::vector<T> ret(rs);
	_It ap = _begin;
	for (int p = 0; p < rs; ++p, ++ap)
		// Add new, subtract old.
		_ret[p] += (_f(next(_begin, _s - p), next(ap, _s - p), _movingBy) - _f(_begin, ap, _movingBy)) / (_s - p);
}

template <class _F, class _U>
std::vector<typename _U::value_type> autocross(_U const& _b, _F const& _f, unsigned _s, unsigned _maxPeriod)
{
	return autocross(_b.begin(), _s, _f, _maxPeriod);
}

template <class _T>
std::vector<typename _T::value_type> autocorrelation(_T const& _b, unsigned _size = 0, unsigned _maxPeriod = 0)
{
	if (!_size)
		_size = _b.size();
	if (!_maxPeriod)
		_maxPeriod = _size / 4;
	return autocross(_b, [](typename _T::const_iterator a, typename _T::const_iterator b, unsigned s){return correlation(a, b, s);}, _size, _maxPeriod);
}

template <class _T = std::vector<float> >
std::vector<typename _T::value_type> selfdissimilarity(_T const& _b, unsigned _size = 0, unsigned _maxPeriod = 0)
{
	if (!_size)
		_size = _b.size();
	if (!_maxPeriod)
		_maxPeriod = _size / 4;
	return autocross(_b, [](typename _T::const_iterator a, typename _T::const_iterator b, unsigned s){return dissimilarity(a, b, s);}, _size, _maxPeriod);
}

template <class _T = std::vector<float> >
std::vector<typename _T::value_type> selfsimilarity(_T const& _b, unsigned _size = 0, unsigned _maxPeriod = 0)
{
	if (!_size)
		_size = _b.size();
	if (!_maxPeriod)
		_maxPeriod = _size / 4;
	return autocross(_b, [](typename _T::const_iterator a, typename _T::const_iterator b, unsigned s){return similarity(a, b, s);}, _size, _maxPeriod);
}

inline void makeTotalUnit(std::vector<float>& _v)
{
	float integral = packEvaluate(_v.begin(), _v.size(), [](v4sf& acc, v4sf const& a){ acc = acc + a; }, [](float const* acc){ return acc[0] + acc[1] + acc[2] + acc[3];});
	float div[4] = {integral, integral, integral, integral };
	packTransform(_v.begin(), _v.size(), [&](v4sf& a){ a = a / *(v4sf*)div; });
}

inline void makeTotalMinusUnit(std::vector<float>& _v)
{
	float integral = packEvaluate(_v.begin(), _v.size(), [](v4sf& acc, v4sf const& a){ acc = acc + a; }, [](float const* acc){ return acc[0] + acc[1] + acc[2] + acc[3];});
	float div[4] = {-integral, -integral, -integral, -integral };
	packTransform(_v.begin(), _v.size(), [&](v4sf& a){ a = a / *(v4sf*)div; });
}


template <class _It>
std::tuple<typename element_of<_It>::type, typename element_of<_It>::type> meanVariance(_It _begin, int _s)
{
	float m = packEvaluate(_begin, _s, [](v4sf& acc, v4sf const& a){ acc += a; }, [&](float a[4]){ return (a[0] + a[1] + a[2] + a[3]) / float(_s); });
	float ma[4] = {m,m,m,m};
	float v = packEvaluate(_begin, _s, [&](v4sf& acc, v4sf const& a){ acc += sqr(a - *(v4sf*)ma); }, [&](float a[4]){ return (a[0] + a[1] + a[2] + a[3]) / float(_s); });
	return std::make_tuple(m, v);
}

}
