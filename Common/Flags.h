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

#include <iostream>
#include <type_traits>
#include <algorithm>
#include <boost/preprocessor/seq.hpp>
#include <boost/algorithm/string.hpp>
#include "Global.h"

namespace Lightbox
{

template <class _T> struct is_flag: public std::false_type {};

template <class _T> typename is_flag<_T>::FlagsType operator|(_T _a, typename is_flag<_T>::FlagsType _b);
template <class _T> _T operator&(_T _a, typename is_flag<_T>::FlagsType _b);

template <class _Enum>
class Flags
{
public:
		typedef _Enum Flag;

		Flags(): v(_Enum(0)) {}
		Flags(_Enum _v): v(_v) {}
		Flags(Flags const& _f): v(_f.v) {}
		explicit Flags(long _i): v(_Enum(_i)) {}

		Flags& operator=(Flags const& _f) { v = _f.v; return *this; }

		_Enum operator&(_Enum _x) const { return _Enum(long(v) & long(_x)); }
		Flags operator&(Flags _x) const { return Flags(long(v) & long(_x.v)); }
		Flags operator|(_Enum _x) const { return Flags(long(v) | long(_x)); }
		Flags operator|(Flags _x) const { return Flags(long(v) | long(_x.v)); }
		Flags operator^(_Enum _x) const { return Flags(long(v) ^ long(_x)); }
		Flags operator^(Flags _x) const { return Flags(long(v) ^ long(_x.v)); }
		Flags& operator&=(_Enum _x) { return (*this) = operator&(_x); }
		Flags& operator&=(Flags _x) { return (*this) = operator&(_x); }
		Flags& operator|=(_Enum _x) { return (*this) = operator|(_x); }
		Flags& operator|=(Flags _x) { return (*this) = operator|(_x); }
		Flags& operator^=(_Enum _x) { return (*this) = operator^(_x); }
		Flags& operator^=(Flags _x) { return (*this) = operator^(_x); }
		Flags operator~() { return Flags(~long(v)); }
		Flags operator<<(int _i) const { return Flags(long(v) << _i); }
		Flags operator>>(int _i) const { return Flags(long(v) >> _i); }
		Flags& operator<<=(int _i) { return (*this) = operator<<(_i); }
		Flags& operator>>=(int _i) { return (*this) = operator>>(_i); }

		template <class _T> friend _T operator&(_T _a, typename is_flag<_T>::FlagsType _b);
		template <class _T> friend typename is_flag<_T>::FlagsType operator|(_T _a, typename is_flag<_T>::FlagsType _b);

		bool operator==(Flags _c) const { return v == _c.v; }
		bool operator!=(Flags _c) const { return v != _c.v; }
		bool operator<(Flags _c) const { return v < _c.v; }

		operator unsigned() const { return (unsigned)v; }
		_Enum highestSet() const { return _Enum(Lightbox::highestBitOnly(long(v))); }

private:
		_Enum v;
};

template <class _T> inline typename is_flag<_T>::FlagsType operator|(_T _a, _T _b) { return typename is_flag<_T>::FlagsType(long(_a) | long(_b)); }
template <class _T> inline typename is_flag<_T>::FlagsType operator|(_T _a, typename is_flag<_T>::FlagsType _b) { return typename is_flag<_T>::FlagsType(long(_a) | long(_b.v)); }
template <class _T> inline _T operator&(_T _a, typename is_flag<_T>::FlagsType::Flag _b) { return _T(long(_a) & long(_b)); }
template <class _T> inline _T operator&(_T _a, typename is_flag<_T>::FlagsType _b) { return _T(long(_a) & long(_b.v)); }
template <class _T> inline typename is_flag<_T>::FlagsType operator~(_T _a) { return typename is_flag<_T>::FlagsType(~long(_a)); }
template <class _T> inline unsigned toIndex(_T _a, typename is_flag<_T>::FlagsType::Flag = _T(0)) { return log2(long(_a)); }
template <class _T> inline typename is_flag<_T>::FlagsType::Flag fromIndex(unsigned _index) { return typename is_flag<_T>::FlagsType::Flag(1 << _index); }

#define LIGHTBOX_ENUM_S2F(R, DataType, Flag) \
	if (_v == LIGHTBOX_ENUM_MAKESTRING(Flag)) \
		return Flag;
#define LIGHTBOX_ENUM_F2S(R, DataType, Flag) \
	if ((_v & Flag) == Flag) \
		return LIGHTBOX_FLAGS_MAKESTRING(Flag);
#define LIGHTBOX_ENUM_F2N(R, DataType, Flag) \
	if ((_v & Flag) == Flag) \
		return shortened(LIGHTBOX_FLAGS_MAKESTRING(Flag));
#define LIGHTBOX_ENUM_MAKESTRING(Flag) #Flag
#define LIGHTBOX_ENUM_STRINGABLE(N, X) \
	inline char const* toString(N _v) \
	{ \
		BOOST_PP_SEQ_FOR_EACH(LIGHTBOX_ENUM_F2S, N, X) \
		return #N "(0)"; \
	} \
	inline std::string toNick(N _v) \
	{ \
		BOOST_PP_SEQ_FOR_EACH(LIGHTBOX_ENUM_F2N, N, X) \
		return #N "(0)"; \
	} \
	inline N to ## N(std::string const& _v) \
	{ \
		BOOST_PP_SEQ_FOR_EACH(LIGHTBOX_ENUM_S2F, N, X) \
		return N(0); \
	} \
    template <class _S> \
    inline _S& operator<<(_S& _out, N _p) \
	{ \
		return _out << toString(_p); \
	} \
    template <class _S> \
    inline _S& operator>>(_S& _in, N& _p) \
	{ \
		std::string s; \
		_in >> s; \
		_p = to ## N(s); \
		return _in; \
	}

#define LIGHTBOX_FLAGS_TYPE(_Enum, _FlagsType) \
	typedef Flags<_Enum> _FlagsType; \
	template <> struct is_flag<_Enum>: public std::true_type { typedef _FlagsType FlagsType; }

#define LIGHTBOX_FLAGS_S2F(R, DataType, Flag) \
	if (n == LIGHTBOX_FLAGS_MAKESTRING(Flag)) \
		ret |= Flag;
#define LIGHTBOX_FLAGS_F2S(R, DataType, Flag) \
	if ((_v & Flag) == Flag) \
		ret += ret.empty() ? LIGHTBOX_FLAGS_MAKESTRING(Flag) : "|" LIGHTBOX_FLAGS_MAKESTRING(Flag);
#define LIGHTBOX_FLAGS_F2N(R, DataType, Flag) \
	if ((_v & Flag) == Flag) \
		ret += ret.empty() ? shortened(LIGHTBOX_FLAGS_MAKESTRING(Flag)) : ("|" + shortened(LIGHTBOX_FLAGS_MAKESTRING(Flag)));
#define LIGHTBOX_FLAGS_MAKESTRING(Flag) #Flag
#define LIGHTBOX_FLAGS_STRINGABLE(N, X) \
	inline std::string toString(N _v) \
	{ \
		std::string ret = ""; \
		BOOST_PP_SEQ_FOR_EACH(LIGHTBOX_FLAGS_F2S, N, X) \
		return ret.empty() ? std::string(LIGHTBOX_FLAGS_MAKESTRING(N) "(0)") : ret; \
	} \
	inline std::string toNick(N _v) \
	{ \
		std::string ret = ""; \
		BOOST_PP_SEQ_FOR_EACH(LIGHTBOX_FLAGS_F2N, N, X) \
		return ret.empty() ? std::string(LIGHTBOX_FLAGS_MAKESTRING(N) "(0)") : ret; \
	} \
	inline N to ## N(std::string const& _v) \
	{ \
		N ret = N(0); \
		for (unsigned i = 0, j = 0; i < _v.size(); ++i) \
			if (_v[i] == '|' || i == _v.size() - 1) \
			{ \
				std::string n = _v.substr(j, i - j); \
				boost::algorithm::trim(n); \
				BOOST_PP_SEQ_FOR_EACH(LIGHTBOX_FLAGS_S2F, N, X) \
				j = i + 1; \
			} \
		return ret; \
	} \
	inline std::ostream& operator<<(std::ostream& _out, N _p) \
	{ \
		return _out << toString(_p); \
	}
#define LIGHTBOX_FLAGS_BR(R, D, I, ELEM) { ELEM, I },
#define LIGHTBOX_FLAGS_ARRAYED(N, X) \
	static const std::array<N, BOOST_PP_SEQ_SIZE(X)> N ## Values = { { BOOST_PP_SEQ_ENUM(X) } }; \
	static std::map<N, unsigned> N ## Lookup = { BOOST_PP_SEQ_FOR_EACH_I(LIGHTBOX_FLAGS_BR, i, X) };

#define LIGHTBOX_FLAGS(N, Ns, X) \
	LIGHTBOX_FLAGS_TYPE(N, Ns); \
	LIGHTBOX_ENUM_STRINGABLE(N, X) \
	LIGHTBOX_FLAGS_STRINGABLE(Ns, X) \
	LIGHTBOX_FLAGS_ARRAYED(N, X)

}
