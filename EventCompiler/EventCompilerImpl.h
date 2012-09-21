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

#include <string>
#include <vector>
#include <cstdlib>
#include <unordered_map>
#include <boost/variant.hpp>
#include <Common/Global.h>
#include <Common/MemberCollection.h>
#include <Common/Time.h>
#include <Common/StreamIO.h>
#include "StreamEvent.h"

namespace Lightbox
{

inline std::string id(float _y) { return toString(_y); }
inline std::string ms(float _x){ return toString(round(_x * 1000)) + (!_x ? "ms" : ""); }
inline std::string msL(float _x, float _y) { return toString(round(_x * 1000)) + "ms (" + toString(round(_y * 100)) + "%)"; }

#define LIGHTBOX_PREPROCESSORS(...) \
	virtual void initPres() { Initer(this) , __VA_ARGS__; } \
	virtual void executePres(Time _t, std::vector<float> const& _mag, std::vector<float> const& _phase, std::vector<float> const& _wave) { Executor(this, _t, _mag, _phase, _wave) , __VA_ARGS__; } \
	class Lightbox_Preprocessors_Macro_End {}

class EventCompiler;
class EventCompilerImpl;

struct Initer
{
	Initer(EventCompilerImpl* _eci): m_eci(_eci) {}
	template <class _T> Initer& operator,(_T& _t) { _t.init(m_eci); return *this; }
	EventCompilerImpl* m_eci;
};

struct Executor
{
	Executor(EventCompilerImpl* _eci, Time _t, std::vector<float> const& _mag, std::vector<float> const& _phase, std::vector<float> const& _wave): m_eci(_eci), m_t(_t), m_mag(_mag), m_phase(_phase), m_wave(_wave) {}
	template <class _T> Executor& operator,(_T& _t) { _t.execute(m_eci, m_t, m_mag, m_phase, m_wave); return *this; }
	EventCompilerImpl* m_eci;
	Time m_t;
	std::vector<float> const& m_mag;
	std::vector<float> const& m_phase;
	std::vector<float> const& m_wave;
};

class EventCompilerImpl
{
	friend class EventCompiler;

public:
	typedef EventCompilerImpl LIGHTBOX_STATE_BaseClass;		// Used by the LIGHTBOX_STATE State collector.
	typedef EventCompilerImpl LIGHTBOX_PROPERTIES_BaseClass;	// Used for the LIGHTBOX_PROPERTIES Members collector.

	EventCompilerImpl() {}
	virtual ~EventCompilerImpl() {}

	inline unsigned bands() const { return m_bands; }
	inline Time windowSize() const { return toBase((m_bands - 1) * 2, rate()); }
	inline Time hop() const { return m_hop; }
	inline Time nyquist() const { return m_nyquist; }
	inline unsigned rate() const { return s_baseRate / m_nyquist * 2; }

	virtual StreamEvents init() { return StreamEvents(); }
	virtual StreamEvents compile(Time _t, std::vector<float> const& _mag, std::vector<float> const& _phase, std::vector<float> const& _wave) { (void)_t; (void)_mag; (void)_phase; (void)_wave; return StreamEvents(); }
	virtual MemberMap propertyMap() const { return NullMemberMap; }
	virtual MemberMap stateMap() const { return NullMemberMap; }

protected:
	virtual void initPres() {}
	virtual void executePres(Time, std::vector<float> const&, std::vector<float> const&, std::vector<float> const&) {}

private:
	Time m_hop;
	Time m_nyquist;
	unsigned m_bands;
	Time m_t;
};

}
