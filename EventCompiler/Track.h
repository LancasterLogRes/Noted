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
#include <map>
#include <byteswap.h>
#include <Common/Time.h>
#include "StreamEvent.h"

namespace Lightbox
{

struct Track
{
	std::vector<StreamEvents> events;
	Time pitch;
	std::map<uint32_t, Time> syncPoints;

	StreamEvents eventsBetween(Time _from, Time _to) const	///< Get the stream events in range [_from, _to);
	{
		unsigned begin = (_from + pitch - 1) / pitch;
		unsigned end = _to / pitch;
		StreamEvents ret;
		for (unsigned i = begin; i < end; ++i)
			catenate(ret, events[i]);
		return ret;
	}

	template <class _F>
	void streamIn(_F const& _read)
	{
		syncPoints.clear();
		// data stream is intel-encoded for simplicity.
		uint32_t testSwap = 69;
		bool doSwap = !*(char*)&testSwap;	// swap if first-byte is zero (for intel first byte would be 69).

		uint32_t count;
		_read(&count, sizeof(count));
		if (doSwap)
			count = __bswap_32(count);
		events.resize(count);
		for (unsigned i = 0; i < count; ++i)
		{
			uint32_t evCount;
			_read(&evCount, sizeof(evCount));
			if (doSwap)
				evCount = __bswap_32(evCount);
			events[i].resize(evCount);
			_read(events[i].data(), sizeof(StreamEvent) * evCount);
			for (StreamEvent& se: events[i])
			{
				if (se.type == SyncPoint)
					syncPoints[(uint32_t)se.strength] = i * pitch;
				memset(&se.m_aux, 0, sizeof(se.m_aux));		// shouldn't be set, but just in case...
				if (doSwap)
					se.period = __bswap_64(se.period);	// period needs twiddling as the only multi-byte integer.
			}
		}
		_read(&pitch, sizeof(pitch));
		if (doSwap)
			pitch = __bswap_64(pitch);
	}

	template <class _F>
	void streamOut(_F const& _write)
	{
		int32_t count = events.size();
		_write(&count, sizeof(count));
		for (unsigned i = 0; i < count; ++i)
		{
			int32_t evCount = events[i].size();
			_write(&evCount, sizeof(evCount));
			_write(events[i].data(), sizeof(StreamEvent) * evCount);
		}
		_write(&pitch, sizeof(pitch));
	}
};

}
