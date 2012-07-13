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

struct MemberEntry
{
	MemberEntry(intptr_t _offset = 0, std::type_info const* _type = nullptr): offset(_offset), type(_type) {}
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

typedef std::unordered_map<std::string, MemberEntry> MemberMap;
static const MemberMap NullMemberMap;

}
