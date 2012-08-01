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

#include <memory>
#include <unordered_map>
#include <cassert>
#include <cstdint>
#include <vector>
#include <typeinfo>
#include <string>
#include <functional>

#include "MemberMap.h"
#include "Global.h"

namespace Lightbox
{

#define LIGHTBOX_MEMBERS(COLLECTOR, ...) \
	template <class C> char const* COLLECTOR(C& _c) const \
	{ \
		_c, __VA_ARGS__; \
		return #__VA_ARGS__; \
	}

#define LIGHTBOX_MEMBERS_AUX(N, Base, ...) \
	LIGHTBOX_MEMBERS(_collect_ ## N, __VA_ARGS__) \
	virtual Lightbox::MemberMap N() const \
	{ \
		if (m_lightbox_ ## N.empty()) \
			m_lightbox_ ## N = Lightbox::MemberCatcher::populate(*static_cast<Base const*>(this), [=](Lightbox::MemberCatcher& _pc){ return _collect_ ## N(_pc); }); \
		return m_lightbox_ ## N; \
	} \
	mutable Lightbox::MemberMap m_lightbox_ ## N

#define LIGHTBOX_STATE(...) LIGHTBOX_MEMBERS_AUX(state, LIGHTBOX_STATE_BaseClass, __VA_ARGS__)
#define LIGHTBOX_PROPERTIES(...) LIGHTBOX_MEMBERS_AUX(propertyMap, LIGHTBOX_PROPERTIES_BaseClass, __VA_ARGS__)

class MemberCatcher
{
public:
	MemberCatcher(intptr_t _that): m_that(_that) {}
	template <class T> MemberCatcher& operator,(T& _m)
	{
//		std::cerr << "MemberEntry found: " << (void*)&typeid(T) << " " << typeid(T).name() << " @" << (intptr_t(&_m) - m_that) << std::endl;
		m_p.push_back(MemberEntry(intptr_t(&_m) - m_that, &typeid(T)));
		return *this;
	}

	template <class T, class F> static MemberMap populate(T const& _c, F const& _f)
	{
		MemberCatcher pc((intptr_t(&_c)));
		char const* ns = _f(pc);
		MemberMap ret;
		int j = 0;
		int n = 0;
		for (int i = 0; ; ++i)
			if (ns[i] == ',' || ns[i] == 0)
			{
				std::string s = ns + j;
				s.resize(i - j);
				ret.insert(make_pair(s, pc.m_p[n]));
				n++;
				j = i + 1;
				if (!ns[i])
					break;
			}
			else if (ns[i] == ' ')
				j = i + 1;
		assert(n == (int)pc.m_p.size());
		return ret;
	}

private:
	std::vector<MemberEntry> m_p;
	intptr_t m_that;
};

}
