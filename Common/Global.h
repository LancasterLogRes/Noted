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

#include <vector>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdint>
#include <tuple>
#include <functional>
#include <type_traits>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

#undef foreach
#define foreach BOOST_FOREACH

#define LIGHTBOX_API __attribute__ ((visibility ("default")))

#define LIGHTBOX_BITS_QUOTE(A) #A
#define LIGHTBOX_BITS_STRINGIFY(A) LIGHTBOX_BITS_QUOTE(A)
#define LIGHTBOX_BITS_EXPAND(A) A
#define LIGHTBOX_BITS_CAT(A, B) A ## B
#define LIGHTBOX_BITS_EXPAND_CAT(A, B) LIGHTBOX_BITS_CAT( A, B )
#define LIGHTBOX_BITS_TARGETIFY(A) LIGHTBOX_BITS_EXPAND_CAT(A, LIGHTBOX_TARGET_NAME)

/// Define an enumeration together with a output stream operator. The values may not be assigned integers explicitly.
#define LIGHTBOX_ENUM_TOSTRING(Name, ...) \
	static const std::vector<Name> Name ## Values = { { __VA_ARGS__ } }; \
	static std::string g_lightbox_upperNamesOf ## Name;\
	inline std::string toString(Name _n)\
	{\
		return ::Lightbox::afterComma(#__VA_ARGS__, (uint16_t)_n);\
	}\
	template <class T> inline T& operator<<(T& _o, Name _e)\
	{\
		_o << toString(_e);\
		return _o;\
	}\
	inline Name to ## Name(std::string const& _s, bool _caseSensitive = true)\
	{\
		std::string ucs = boost::algorithm::to_upper_copy(_s);\
		if (g_lightbox_upperNamesOf ## Name.empty())\
			g_lightbox_upperNamesOf ## Name = boost::algorithm::to_upper_copy(std::string(#__VA_ARGS__));\
		for (unsigned i = 0; ; ++i)\
		{\
			std::string s = ::Lightbox::afterComma(_caseSensitive ? #__VA_ARGS__ : g_lightbox_upperNamesOf ## Name.c_str(), i);\
			if (s.empty())\
				return Name(0);\
			else if ((_caseSensitive ? _s : ucs) == s)\
				return Name(i);\
		}\
	}\
	template <class T> inline T& operator>>(T& _o, Name& _e)\
	{\
		std::string ino;\
		_o >> ino;\
		_e = to ## Name (ino);\
		return _o;\
	}

#define LIGHTBOX_TEXTUAL_ENUM(Name, First, ...) \
	enum Name { First = 0, __VA_ARGS__ }; \
	LIGHTBOX_ENUM_TOSTRING(Name, First, __VA_ARGS__)

#define LIGHTBOX_TEXTUAL_ENUM_INHERITS(Name, Inherits, First, ...) \
	enum Name: Inherits { First = 0, __VA_ARGS__ }; \
	LIGHTBOX_ENUM_TOSTRING(Name, First, __VA_ARGS__)

#define LIGHTBOX_STRUCT_INTERNALS_2(Name, T1, M1, T2, M2) \
	Name& operator=(Name const& _s) { M1 = _s.M1; M2 = _s.M2; return *this; } \
	bool operator<(Name const& _c) const { return M1 < _c.M1 || (M1 == _c.M1 && M2 < _c.M2); } \
	bool operator==(Name const& _c) const { return _c.M1 == M1 && _c.M2 == M2; } \
	bool operator!=(Name const& _c) const { return !operator==(_c); } \
	template <class S> friend S& operator<<(S& _out, Name const& _this) { _out << #Name << "(" #M1 "=" << _this.M1 << ", " #M2 "=" << _this.M2 << ")"; return _out; } \
	operator std::tuple<T1, T2>() const { return std::make_tuple(M1, M2); } \
	std::tuple<T1, T2> toTuple() const { return std::make_tuple(M1, M2); }

#define LIGHTBOX_STRUCT_BASE_2(Name, T1, M1, T2, M2) \
	T1 M1; \
	T2 M2; \
	Name() {} \
	Name(T1 _ ## M1, T2 _ ## M2): M1(_ ## M1), M2(_ ## M2) {} \
	Name(std::tuple<T1, T2> const& _t): M1(std::get<0>(_t)), M2(std::get<1>(_t)) {} \
	Name(Name const& _s): M1(_s.M1), M2(_s.M2) {}

#define LIGHTBOX_STRUCT_INTERNALS_3(Name, T1, M1, T2, M2, T3, M3) \
	Name& operator=(Name const& _s) { M1 = _s.M1; M2 = _s.M2; M3 = _s.M3; return *this; } \
	bool operator<(Name const& _c) const { return M1 < _c.M1 || (M1 == _c.M1 && (M2 < _c.M2 || (M2 == _c.M2 && M3 < _c.M3))); } \
	bool operator==(Name const& _c) const { return _c.M1 == M1 && _c.M2 == M2 && _c.M3 == M3; } \
	bool operator!=(Name const& _c) const { return !operator==(_c); } \
	template <class S> friend S& operator<<(S& _out, Name const& _this) { _out << #Name << "(" #M1 "=" << _this.M1 << ", " #M2 "=" << _this.M2 << ", " #M3 "=" << _this.M3 << ")"; return _out; } \
	operator std::tuple<T1, T2, T3>() const { return std::make_tuple(M1, M2, M3); } \
	std::tuple<T1, T2, T3> toTuple() const { return std::make_tuple(M1, M2, M3); }

#define LIGHTBOX_STRUCT_BASE_3(Name, T1, M1, T2, M2, T3, M3) \
	T1 M1; \
	T2 M2; \
	T3 M3; \
	Name() {} \
	Name(T1 _ ## M1, T2 _ ## M2, T3 _ ## M3): M1(_ ## M1), M2(_ ## M2), M3(_ ## M3) {} \
	Name(std::tuple<T1, T2, T3> const& _t): M1(std::get<0>(_t)), M2(std::get<1>(_t)), M3(std::get<2>(_t)) {} \
	Name(Name const& _s): M1(_s.M1), M2(_s.M2), M3(_s.M3) {}

#define LIGHTBOX_STRUCT_INTERNALS_4(Name, T1, M1, T2, M2, T3, M3, T4, M4) \
	Name& operator=(Name const& _s) { M1 = _s.M1; M2 = _s.M2; M3 = _s.M3; M4 = _s.M4; return *this; } \
	bool operator<(Name const& _c) const { return M1 < _c.M1 || (M1 == _c.M1 && (M2 < _c.M2 || (M2 == _c.M2 && (M3 < _c.M3 || (M3 == _c.M3 && M4 < _c.M4))))); } \
	bool operator==(Name const& _c) const { return _c.M1 == M1 && _c.M2 == M2 && _c.M3 == M3 && _c.M4 == M4; } \
	bool operator!=(Name const& _c) const { return !operator==(_c); } \
	template <class S> friend S& operator<<(S& _out, Name const& _this) { _out << #Name << "(" #M1 "=" << _this.M1 << ", " #M2 "=" << _this.M2 << ", " #M3 "=" << _this.M3 << ", " #M4 "=" << _this.M4 << ")"; return _out; } \
	operator std::tuple<T1, T2, T3, T4>() const { return std::make_tuple(M1, M2, M3, M4); } \
	std::tuple<T1, T2, T3, T4> toTuple() const { return std::make_tuple(M1, M2, M3, M4); }

#define LIGHTBOX_STRUCT_BASE_4(Name, T1, M1, T2, M2, T3, M3, T4, M4) \
	T1 M1; \
	T2 M2; \
	T3 M3; \
	T4 M4; \
	Name() {} \
	Name(T1 _ ## M1, T2 _ ## M2, T3 _ ## M3, T4 _ ## M4): M1(_ ## M1), M2(_ ## M2), M3(_ ## M3), M4(_ ## M4) {} \
	Name(std::tuple<T1, T2, T3, T4> const& _t): M1(std::get<0>(_t)), M2(std::get<1>(_t)), M3(std::get<2>(_t)), M4(std::get<3>(_t)) {} \
	Name(Name const& _s): M1(_s.M1), M2(_s.M2), M3(_s.M3), M4(_s.M4) {}

#define LIGHTBOX_STRUCT_INTERNALS_5(Name, T1, M1, T2, M2, T3, M3, T4, M4, T5, M5) \
	Name& operator=(Name const& _s) { M1 = _s.M1; M2 = _s.M2; M3 = _s.M3; M4 = _s.M4; M5 = _s.M5; return *this; } \
	bool operator<(Name const& _c) const { return M1 < _c.M1 || (M1 == _c.M1 && (M2 < _c.M2 || (M2 == _c.M2 && (M3 < _c.M3 || (M3 == _c.M3 && (M4 < _c.M4 || (M4 == _c.M4 && M5 < _c.M5))))))); } \
	bool operator==(Name const& _c) const { return _c.M1 == M1 && _c.M2 == M2 && _c.M3 == M3 && _c.M4 == M4 && _c.M5 == M5; } \
	bool operator!=(Name const& _c) const { return !operator==(_c); } \
	template <class S> friend S& operator<<(S& _out, Name const& _this) { _out << #Name << "(" #M1 "=" << _this.M1 << ", " #M2 "=" << _this.M2 << ", " #M3 "=" << _this.M3 << ", " #M4 "=" << _this.M4 << ", " #M5 "=" << _this.M5 << ")"; return _out; } \
	operator std::tuple<T1, T2, T3, T4, T5>() const { return std::make_tuple(M1, M2, M3, M4, M5); } \
	std::tuple<T1, T2, T3, T4, T5> toTuple() const { return std::make_tuple(M1, M2, M3, M4, M5); }

#define LIGHTBOX_STRUCT_BASE_5(Name, T1, M1, T2, M2, T3, M3, T4, M4, T5, M5) \
	T1 M1; \
	T2 M2; \
	T3 M3; \
	T4 M4; \
	T5 M5; \
	Name() {} \
	Name(T1 _ ## M1, T2 _ ## M2, T3 _ ## M3, T4 _ ## M4, T5 _ ## M5): M1(_ ## M1), M2(_ ## M2), M3(_ ## M3), M4(_ ## M4), M5(_ ## M5) {} \
	Name(std::tuple<T1, T2, T3, T4, T5> const& _t): M1(std::get<0>(_t)), M2(std::get<1>(_t)), M3(std::get<2>(_t)), M4(std::get<3>(_t)), M5(std::get<4>(_t)) {} \
	Name(Name const& _s): M1(_s.M1), M2(_s.M2), M3(_s.M3), M4(_s.M4), M5(_s.M5) {}

#define LIGHTBOX_STRUCT_INTERNALS_6(Name, T1, M1, T2, M2, T3, M3, T4, M4, T5, M5, T6, M6) \
	Name& operator=(Name const& _s) { M1 = _s.M1; M2 = _s.M2; M3 = _s.M3; M4 = _s.M4; M5 = _s.M5; M6 = _s.M6; return *this; } \
	bool operator<(Name const& _c) const { return M1 < _c.M1 || (M1 == _c.M1 && (M2 < _c.M2 || (M2 == _c.M2 && (M3 < _c.M3 || (M3 == _c.M3 && (M4 < _c.M4 || (M4 == _c.M4 && (M5 < _c.M5 || (M5 == _c.M5 && M6 < _c.M6))))))))); } \
	bool operator==(Name const& _c) const { return _c.M1 == M1 && _c.M2 == M2 && _c.M3 == M3 && _c.M4 == M4 && _c.M5 == M5 && _c.M6 == M6; } \
	bool operator!=(Name const& _c) const { return !operator==(_c); } \
	template <class S> friend S& operator<<(S& _out, Name const& _this) { _out << #Name << "(" #M1 "=" << _this.M1 << ", " #M2 "=" << _this.M2 << ", " #M3 "=" << _this.M3 << ", " #M4 "=" << _this.M4 << ", " #M5 "=" << _this.M5 << ", " #M6 "=" << _this.M6 << ")"; return _out; } \
	operator std::tuple<T1, T2, T3, T4, T5, T6>() const { return std::make_tuple(M1, M2, M3, M4, M5, M6); } \
	std::tuple<T1, T2, T3, T4, T5, T6> toTuple() const { return std::make_tuple(M1, M2, M3, M4, M5, M6); }

#define LIGHTBOX_STRUCT_BASE_6(Name, T1, M1, T2, M2, T3, M3, T4, M4, T5, M5, T6, M6) \
	T1 M1; \
	T2 M2; \
	T3 M3; \
	T4 M4; \
	T5 M5; \
	T6 M6; \
	Name() {} \
	Name(T1 _ ## M1, T2 _ ## M2, T3 _ ## M3, T4 _ ## M4, T5 _ ## M5, T6 _ ## M6): M1(_ ## M1), M2(_ ## M2), M3(_ ## M3), M4(_ ## M4), M5(_ ## M5), M6(_ ## M6) {} \
	Name(std::tuple<T1, T2, T3, T4, T5, T6> const& _t): M1(std::get<0>(_t)), M2(std::get<1>(_t)), M3(std::get<2>(_t)), M4(std::get<3>(_t)), M5(std::get<4>(_t)), M6(std::get<5>(_t)) {} \
	Name(Name const& _s): M1(_s.M1), M2(_s.M2), M3(_s.M3), M4(_s.M4), M5(_s.M5), M6(_s.M6) {}

#define LIGHTBOX_STRUCT_INTERNALS_7(Name, T1, M1, T2, M2, T3, M3, T4, M4, T5, M5, T6, M6, T7, M7) \
	Name& operator=(Name const& _s) { M1 = _s.M1; M2 = _s.M2; M3 = _s.M3; M4 = _s.M4; M5 = _s.M5; M6 = _s.M6; M7 = _s.M7; return *this; } \
	bool operator<(Name const& _c) const { return M1 < _c.M1 || (M1 == _c.M1 && (M2 < _c.M2 || (M2 == _c.M2 && (M3 < _c.M3 || (M3 == _c.M3 && (M4 < _c.M4 || (M4 == _c.M4 && (M5 < _c.M5 || (M5 == _c.M5 && (M6 < _c.M6 || (M6 == _c.M6 && M7 < _c.M7))))))))))); } \
	bool operator==(Name const& _c) const { return _c.M1 == M1 && _c.M2 == M2 && _c.M3 == M3 && _c.M4 == M4 && _c.M5 == M5 && _c.M6 == M6 && _c.M7 == M7; } \
	bool operator!=(Name const& _c) const { return !operator==(_c); } \
	template <class S> friend S& operator<<(S& _out, Name const& _this) { _out << #Name << "(" #M1 "=" << _this.M1 << ", " #M2 "=" << _this.M2 << ", " #M3 "=" << _this.M3 << ", " #M4 "=" << _this.M4 << ", " #M5 "=" << _this.M5 << ", " #M6 "=" << _this.M6 <<", " #M7 "=" << _this.M7 << ")"; return _out; } \
	operator std::tuple<T1, T2, T3, T4, T5, T6, T7>() const { return std::make_tuple(M1, M2, M3, M4, M5, M6, M7); } \
	std::tuple<T1, T2, T3, T4, T5, T6, T7> toTuple() const { return std::make_tuple(M1, M2, M3, M4, M5, M6, M7); }

#define LIGHTBOX_STRUCT_BASE_7(Name, T1, M1, T2, M2, T3, M3, T4, M4, T5, M5, T6, M6, T7, M7) \
	T1 M1; \
	T2 M2; \
	T3 M3; \
	T4 M4; \
	T5 M5; \
	T6 M6; \
	T7 M7; \
	Name() {} \
	Name(T1 _ ## M1, T2 _ ## M2, T3 _ ## M3, T4 _ ## M4, T5 _ ## M5, T6 _ ## M6, T7 _ ## M7): M1(_ ## M1), M2(_ ## M2), M3(_ ## M3), M4(_ ## M4), M5(_ ## M5), M6(_ ## M6), M7(_ ## M7) {} \
	Name(std::tuple<T1, T2, T3, T4, T5, T6, T7> const& _t): M1(std::get<0>(_t)), M2(std::get<1>(_t)), M3(std::get<2>(_t)), M4(std::get<3>(_t)), M5(std::get<4>(_t)), M6(std::get<5>(_t)), M7(std::get<6>(_t)) {} \
	Name(Name const& _s): M1(_s.M1), M2(_s.M2), M3(_s.M3), M4(_s.M4), M5(_s.M5), M6(_s.M6), M7(_s.M7) {}

#define LIGHTBOX_STRUCT(N, Name, ...) \
struct Name \
{ \
	LIGHTBOX_STRUCT_BASE_ ## N (Name, __VA_ARGS__) \
	LIGHTBOX_STRUCT_INTERNALS_ ## N (Name, __VA_ARGS__) \
}

#define LIGHTBOX_STRUCT_INTERNALS(N, ...) \
	LIGHTBOX_STRUCT_INTERNALS_ ## N (__VA_ARGS__)

#if defined(LIGHTBOX_SHARED_LIBRARY)
#define LIGHTBOX_FINALIZING_LIBRARY \
	extern "C" { __attribute__ ((visibility ("default"))) bool* g_lightboxFinalized = nullptr; } \
	struct LightboxFinalizer { ~LightboxFinalizer() { if (g_lightboxFinalized) *g_lightboxFinalized = true; } } g_lightboxFinalizer;
#else
#define LIGHTBOX_FINALIZING_LIBRARY
#endif

typedef int16_t v4hi __attribute__ ((vector_size (8)));
typedef float v4sf __attribute__ ((vector_size (16)));
typedef int32_t i4sf __attribute__ ((vector_size (16)));
typedef double d4sf __attribute__ ((vector_size (32)));

namespace Lightbox
{

template <class _T> _T const& NullReturn() { static const _T s_ret = _T(); return s_ret; }

/// Determine nth item in comma+space delimited string.
std::string afterComma(char const* _s, unsigned _i);
std::string demangled(char const* _n);
std::string shortened(std::string const& _s);

constexpr inline bool static_strcmp(char const* a, char const* b)
{
	return *a == *b && (!*a || ::Lightbox::static_strcmp(a + 1, b + 1));
}

template <class _T> struct Packed { static const bool value = false; };
template <> struct Packed<float> { static const bool value = true; typedef v4sf type; };
template <> struct Packed<double> { static const bool value = true; typedef d4sf type; };
template <> struct Packed<int32_t> { static const bool value = true; typedef i4sf type; };

static const float Pi = 3.1415926535898;
static const float TwoPi = Pi * 2.0;
static const float HalfPi = Pi / 2.0;

template <class _T>
inline _T highestBitOnly(_T _w)
{
	_T v = _w >> 1;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	if (sizeof(_T) > 1)
		v |= v >> 8;
	if (sizeof(_T) > 2)
		v |= v >> 16;
	if (sizeof(_T) > 4)
		v |= v >> 32;
	return _w & ~v;
}

inline unsigned log2(unsigned _i)
{
	unsigned ret = 0;
	while (_i >>= 1)
	  ret++;
	return ret;
}

template <class _Iterator> struct element_of { typedef typename _Iterator::value_type type; };
template <class _E> struct element_of<_E*> { typedef _E type; };
template <class _E> struct element_of<_E const*> { typedef _E type; };

template <class _T> std::vector<_T>& operator+=(std::vector<_T>& _v, _T const& _t) { _v.push_back(_t); return _v; }
template <class _T> std::vector<_T>& operator+=(std::vector<_T>& _v1, std::vector<_T> const& _v2) { catenate(_v1, _v2); return _v1; }
template <class _T> std::vector<_T> operator+(std::vector<_T> const& _v, _T const& _t) { auto v = _v; v.push_back(_t); return v; }
template <class _T> std::vector<_T> operator+(_T const& _t, std::vector<_T> const& _v) { return operator+(_v, _t); }
template <class _T> std::vector<_T> operator+(std::vector<_T> const& _v1, std::vector<_T> const& _v2) { auto v = _v1; catenate(v, _v2); return v; }

template <class _T>
class foreign_vector
{
public:
	typedef _T value_type;
	typedef _T element_type;

	foreign_vector(): m_data(nullptr), m_count(0) {}
	foreign_vector(std::vector<typename std::remove_const<_T>::type>* _data): m_data(_data->data()), m_count(_data->size()) {}
	foreign_vector(_T* _data, unsigned _count): m_data(_data), m_count(_count) {}

	explicit operator bool() const { return m_data && m_count; }

	_T* data() { return m_data; }
	_T const* data() const { return m_data; }
	unsigned count() const { return m_count; }
	unsigned size() const { return m_count; }
	foreign_vector& tied(std::shared_ptr<void> const& _lock) { m_lock = _lock; return *this; }
	foreign_vector<_T> next() const { return foreign_vector<_T>(m_data + m_count, m_count).tied(m_lock); }
	foreign_vector<_T> cropped(unsigned _begin, unsigned _count) const { return foreign_vector<_T>(m_data + _begin, _count).tied(m_lock); }

	_T* begin() { return m_data; }
	_T* end() { return m_data + m_count; }
	_T const* begin() const { return m_data; }
	_T const* end() const { return m_data + m_count; }

	_T& operator[](unsigned _i) { assert(m_data); assert(_i < m_count); return m_data[_i]; }
	_T const& operator[](unsigned _i) const { assert(m_data); assert(_i < m_count); return m_data[_i]; }

private:
	_T* m_data;
	unsigned m_count;
	std::shared_ptr<void> m_lock;
};

class NullOutputStream
{
public:
	template <class T> NullOutputStream& operator<<(T const&) { return *this; }
};

static const std::vector<float> NullVectorFloat;

extern bool g_debugEnabled[256];
extern std::function<void(std::string const&, unsigned char)> g_debugPost;

void simpleDebugOut(std::string const&, unsigned char);

template <unsigned char _Id = 0, bool _AutoSpacing = true>
class DebugOutputStream
{
public:
	DebugOutputStream(char const* _start = "    ") { sstr << _start; }
	~DebugOutputStream() { g_debugPost(sstr.str(), _Id); }
	template <class T> DebugOutputStream& operator<<(T const& _t) { if (_AutoSpacing && sstr.str().size() && sstr.str().back() != ' ') sstr << " "; sstr << _t; return *this; }
	std::stringstream sstr;
};

}

// Dirties the global namespace, but oh so convenient...
#define nbug(X) if (true) {} else Lightbox::NullOutputStream()
#define nsbug(X) if (true) {} else Lightbox::NullOutputStream()
#define ndebug if (true) {} else Lightbox::NullOutputStream()
#define cbug(X) Lightbox::DebugOutputStream<X>()
#define csbug(X) Lightbox::DebugOutputStream<X, false>()
#define cdebug Lightbox::DebugOutputStream<253, true>()
#define cnote Lightbox::DebugOutputStream<254, true>()
#define cwarn Lightbox::DebugOutputStream<255, true>()
