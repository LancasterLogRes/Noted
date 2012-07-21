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
#include <boost/algorithm/string.hpp>

#include "Global.h"
#include "StreamIO.h"
#include "MemberMap.h"

namespace Lightbox
{

template <class _Owner>
class Members
{
	template <class _OtherOwner> friend class Members;

public:
	Members() {}

	Members(MemberMap const& _map, std::weak_ptr<_Owner> const& _object, std::function<void(std::string const&)> const& _onChanged = std::function<void(std::string const&)>()):
		m_map		(_map),
		m_object	(_object),
		m_onChanged	(_onChanged)
	{}

	// Templated copy-constructor
	template <class _OtherOwner>
	Members(Members<_OtherOwner> const& _ps):
		m_map		(_ps.m_map),
		m_object	(_ps.m_object),
		m_onChanged	(_ps.m_onChanged)
	{}

	unsigned size() const { return m_map.size(); }
	std::vector<std::string> names() const { std::vector<std::string> ret; for (auto k : m_map) ret.push_back(k.first); return ret; }
	std::type_info const& typeinfo(std::string const& _name) const { auto i = m_map.find(_name); if (i != m_map.end()) return *i->second.type; else return typeid(void); }

	template <class _Val> void set(std::string const& _name, _Val const& _v) { if (std::shared_ptr<_Owner> p = m_object.lock()) { auto i = m_map.find(_name); if (i != m_map.end()) { i->second.set<_Val, _Owner>(p.get(), _v); if (m_onChanged) m_onChanged(_name); } } }
	template <class _Val> _Val const& get(std::string const& _name, _Val const& _default = _Val()) const { if (std::shared_ptr<_Owner> p = m_object.lock()) { auto i = m_map.find(_name); if (i != m_map.end()) return i->second.getValue<_Val, _Owner>(p.get(), _default); } return _default; }
	template <class _Val> Members& operator()(std::string const& _name, _Val const& _v) { set(_name, _v); return *this; }

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
#define	DO(X) if (tn == typeid(X).name()) { typedef X T; T old = get<X>(_name); T nw = _l(old); if (nw == old) return false; set<X>(_name, nw); return true; }
#include "DoTypes.h"
#undef DO
		return false;
	}

	std::string serialized() const
	{
		// Output is a valid C++ properties () string.
		std::stringstream out;
		if (auto p = m_object.lock())
			for (auto i: m_map)
			{
				std::string tn(i.second.type->name());
				if (false) {}
#define	DO(T) else if (tn == typeid(T).name()) { std::string s = toString(i.second.get<T>(p.get())); out << "(\"" << i.first << "\",/*" << s.size() << "*/" << s << ")"; }
#include "DoTypes.h"
#undef DO
			}
		return out.str();
	}

	void deserialize(std::string const& _s)
	{
		std::istringstream in(_s);
		while (in.peek() != -1)
		{
			std::string n;
			assert(in.get() == '(');
			assert(in.get() == '"');
			std::getline(in, n, '"');
			assert(in.get() == ',');
			assert(in.get() == '/');
			assert(in.get() == '*');
			unsigned valSize;
			in >> valSize;
			assert(in.get() == '*');
			assert(in.get() == '/');
			std::string val(valSize, ' ');
			in.read(&val[0], valSize);
			assert(in.get() == ')');
			if (auto p = m_object.lock())
			{
				auto i = m_map.find(n);
				if (i != m_map.end())
				{
					std::string tn(i->second.type->name());
					if (false) {}
#define	DO(T) else if (tn == typeid(T).name()) { T t; fromString(i->second.get(p.get(), t), val); }
#include "DoTypes.h"
#undef DO
				}
			}
		}
	}

private:
	MemberMap m_map;
	std::weak_ptr<_Owner> m_object;
	std::function<void(std::string const&)> m_onChanged;
};

typedef Members<void> VoidMembers;

}
