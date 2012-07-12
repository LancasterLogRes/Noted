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
	template <class _Val, class _Class> _Val const& getValue(_Class const* _that, _Val const& _default = _Val()) const
	{
		if (!type)
			return _default;
		if (std::string(typeid(_Val).name()) != type->name())
			std::cerr << "WARNING: Bad property cast; real type is " << demangled(type->name()) << ", cast to " << demangled(typeid(_Val).name()) << std::endl;
		return *reinterpret_cast<_Val const*>(intptr_t(_that) + offset);
	}
	template <class _Val, class _Class> _Val const& get(_Class const* _that) const { return getValue<_Val>(_that); }
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

template <class _T>
class TypedProperties
{
	template <class _T2> friend class TypedProperties;

public:
	TypedProperties() {}

	TypedProperties(PropertyMap const& _map, std::weak_ptr<_T> const& _object):
		m_map		(_map),
		m_object	(_object)
	{}

	// Templated copy-constructor
	template <class _T2>
	TypedProperties(TypedProperties<_T2> const& _ps):
		m_map		(_ps.m_map),
		m_object	(_ps.m_object)
	{}

	std::vector<std::string> names() const { std::vector<std::string> ret; for (auto k : m_map) ret.push_back(k.first); return ret; }
	std::type_info const& typeinfo(std::string const& _name) const { auto i = m_map.find(_name); if (i != m_map.end()) return *i->second.type; else return typeid(void); }

	template <class _Val> void set(std::string const& _name, _Val const& _v) { if (std::shared_ptr<_T> p = m_object.lock()) { auto i = m_map.find(_name); if (i != m_map.end()) i->second.set<_Val, _T>(p.get(), _v); } }
	template <class _Val> _Val const& get(std::string const& _name, _Val const& _default = _Val()) const { if (std::shared_ptr<_T> p = m_object.lock()) { auto i = m_map.find(_name); if (i != m_map.end()) return i->second.getValue<_Val, _T>(p.get(), _default); } return _default; }
	template <class _Val> TypedProperties& operator()(std::string const& _name, _Val const& _v) { set(_name, _v); return *this; }

	template <class _Getter>
	typename _Getter::Returns typedGet(std::string const& _name, _Getter& _l) const
	{
		std::string tn(typeinfo(_name).name());
#define	DO(X) if (tn == typeid(X).name()) return _l(get<X>(_name));
#include "DoTypes.h"
#undef DO
		return typename _Getter::Returns(0);
	}

	template <class _Setter>
	bool typedSet(std::string const& _name, _Setter& _l)
	{
		std::string tn(typeinfo(_name).name());
#define	DO(X) if (tn == typeid(X).name()) { typedef X T; T old = get<X>(_name); T nw = _l(old); cdebug << nw << old << (nw == old); if (nw == old) return false; set<X>(_name, nw); cdebug << get<X>(_name); return true; }
#include "DoTypes.h"
#undef DO
		return false;
	}

private:
	PropertyMap m_map;
	std::weak_ptr<_T> m_object;
};

typedef TypedProperties<void> Properties;

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
	mutable Lightbox::PropertyMap m_lightbox_properties

//foreach (auto p, m_lightbox_properties) std::cerr << p.first << ", " << p.second.offset << " " << Lightbox::demangled(p.second.type->name()) << endl;


#define LIGHTBOX_STATE(...) LIGHTBOX_S_P_AUX(state, LIGHTBOX_STATE_BaseClass, __VA_ARGS__)
#define LIGHTBOX_PROPERTIES(...) LIGHTBOX_S_P_AUX(propertyMap, LIGHTBOX_PROPERTIES_BaseClass, __VA_ARGS__)

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
