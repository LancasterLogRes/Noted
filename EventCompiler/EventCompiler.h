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
#include <memory>
#include <exception>

#include <Common/Global.h>
#include <Common/Members.h>
#include "EventCompilerImpl.h"
#include "EventCompilerLibrary.h"

namespace Lightbox
{

class EventCompiler
{
public:
	static EventCompiler create(EventCompilerImpl* _i) { EventCompiler ret; ret.m_impl = std::shared_ptr<EventCompilerImpl>(_i); return ret; }

	StreamEvents init(unsigned _bands, Time _hop, Time _nyquist) { if (m_impl) { m_impl->m_t = 0; m_impl->m_bands = _bands; m_impl->m_hop = _hop; m_impl->m_nyquist = _nyquist; m_impl->initPres(); return m_impl->init(); } return StreamEvents(); }
	StreamEvents compile(std::vector<float> const& _mag, std::vector<float> const& _phase, std::vector<float> const& _wave) { if (m_impl) { m_impl->executePres(m_impl->m_t, _mag, _phase, _wave); auto ret = m_impl->compile(m_impl->m_t, _mag, _phase, _wave); m_impl->m_t += m_impl->m_hop; return ret; } else return StreamEvents(); }
	Members<EventCompilerImpl> properties() { return m_impl ? Members<EventCompilerImpl>(m_impl->propertyMap(), m_impl) : Members<EventCompilerImpl>(); }
	Members<EventCompilerImpl> state() { return m_impl ? Members<EventCompilerImpl>(m_impl->stateMap(), m_impl) : Members<EventCompilerImpl>(); }

	std::string name() const { return m_impl ? demangled(typeid(*m_impl).name()) : ""; }
	template <class T> bool isA() const { return !!dynamic_cast<T*>(m_impl.get()); }
	template <class T> T const& asA() const { /*if (!isA<T>()) throw std::exception();*/ return *dynamic_cast<T const*>(m_impl.get()); }

	bool operator<(EventCompiler const& _c) const { return m_impl < _c.m_impl; }
	bool operator==(EventCompiler const& _c) const { return m_impl == _c.m_impl; }
	bool operator!=(EventCompiler const& _c) const { return !operator==(_c); }

	bool isNull() const { return !m_impl; }

private:
	std::shared_ptr<EventCompilerImpl> m_impl;
};

}
