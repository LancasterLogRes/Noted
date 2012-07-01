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

#include <unordered_map>
#include <cassert>
#include <cstdint>
#include <vector>
#include <typeinfo>
#include <string>
#include <functional>

#include "Global.h"

namespace Lightbox
{

struct Property
{
	Property(intptr_t _offset = 0, std::type_info const* _type = nullptr): offset(_offset), type(_type) {}
	intptr_t offset;
	std::type_info const* type;
	template <class _Val, class _Class> void set(_Class* _that, _Val const& _v) const
	{
		if (!type)
			return;
		if (std::string(typeid(_Val).name()) != type->name())
			std::cerr << "WARNING: Bad property cast; real type is " << demangled(type->name()) << ", cast to " << demangled(typeid(_Val).name()) << std::endl;
		*reinterpret_cast<_Val*>(intptr_t(_that) + offset) = _v;
	}
	template <class _Val, class _Class> _Val const& get(_Class const* _that, _Val const& _default = _Val()) const
	{
		if (!type)
			return _default;
		if (std::string(typeid(_Val).name()) != type->name())
			std::cerr << "WARNING: Bad property cast; real type is " << demangled(type->name()) << ", cast to " << demangled(typeid(_Val).name()) << std::endl;
		return *reinterpret_cast<_Val const*>(intptr_t(_that) + offset);
	}
	template <class _Val, class _Class> _Val& get(_Class* _that, _Val& _default) const
	{
		if (!type)
			return _default;
		if (std::string(typeid(_Val).name()) != type->name())
			std::cerr << "WARNING: Bad property cast; real type is " << demangled(type->name()) << ", cast to " << demangled(typeid(_Val).name()) << std::endl;
		return *reinterpret_cast<_Val*>(intptr_t(_that) + offset);
	}
};

typedef std::unordered_map<std::string, Property> PropertyMap;

static const PropertyMap NullPropertyMap;

#define LIGHTBOX_MEMBERS(COLLECTOR, ...) \
	template <class C> char const* COLLECTOR(C& _c) const \
	{ \
		_c, __VA_ARGS__; \
		return #__VA_ARGS__; \
	}

#define LIGHTBOX_S_P_AUX(N, Base, ...) \
	LIGHTBOX_MEMBERS(_collect_ ## N, __VA_ARGS__) \
	virtual Lightbox::PropertyMap N() const \
	{ \
		if (m_lightbox_properties.empty()) \
		{ \
			m_lightbox_properties = Lightbox::PropertyCatcher::populate(*static_cast<Base const*>(this), [=](Lightbox::PropertyCatcher& _pc){ return _collect_ ## N(_pc); }); \
		} \
		return m_lightbox_properties; \
	} \
	mutable Lightbox::PropertyMap m_lightbox_properties;

//foreach (auto p, m_lightbox_properties) std::cerr << p.first << ", " << p.second.offset << " " << Lightbox::demangled(p.second.type->name()) << endl;


#define LIGHTBOX_STATE(Base, ...) LIGHTBOX_S_P_AUX(state, Base, __VA_ARGS__)
#define LIGHTBOX_PROPERTIES(Base, ...) LIGHTBOX_S_P_AUX(properties, Base, __VA_ARGS__)

class PropertyCatcher
{
public:
	PropertyCatcher(intptr_t _that): m_that(_that) {}
	template <class T> PropertyCatcher& operator,(T& _m)
	{
//		std::cerr << "Property found: " << (void*)&typeid(T) << " " << typeid(T).name() << " @" << (intptr_t(&_m) - m_that) << std::endl;
		m_p.push_back(Property(intptr_t(&_m) - m_that, &typeid(T)));
		return *this;
	}

	template <class T, class F> static PropertyMap populate(T const& _c, F const& _f)
	{
		PropertyCatcher pc((intptr_t(&_c)));
		char const* ns = _f(pc);
		PropertyMap ret;
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
	std::vector<Property> m_p;
	intptr_t m_that;
};

}
