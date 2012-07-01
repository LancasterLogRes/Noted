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

#include <cstdint>

namespace Lightbox
{

template <class C>
class BoolArray
{
public:
	BoolArray(): m_val(0) {}
	BoolArray(C _c): m_val(uint64_t(_c)) {}
	BoolArray(BoolArray const& _f): m_val(_f.m_val) {}

	bool operator[](C _c) const { return (m_val & uint64_t(1 << (unsigned(_c) & 0x3f))) != 0; }

	BoolArray& operator|=(C _c) { m_val |= uint64_t(1 << (unsigned(_c) & 0x3f)); return *this; }
	BoolArray operator|(C _c) const { BoolArray ret; ret.m_val = (m_val | uint64_t(1 << (unsigned(_c) & 0x3f))); return ret; }
	BoolArray& operator-=(C _c) { m_val &= ~uint64_t(1 << (unsigned(_c) & 0x3f)); return *this; }
	BoolArray operator-(C _c) const { BoolArray ret; ret.m_val = (m_val & ~uint64_t(1 << (unsigned(_c) & 0x3f))); return ret; }

	inline friend BoolArray operator|(C _c, C _d) { return BoolArray(_c) | _d; }

	bool operator==(BoolArray const& _c) const { return m_val == _c.m_val; }
	bool operator!=(BoolArray const& _c) const { return !operator==(_c); }

private:
	uint64_t m_val;
};

}
