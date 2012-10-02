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
#include <Common/Global.h>
#include <Common/Time.h>
#include "StreamEvent.h"

namespace Lightbox
{

struct LIGHTBOX_API Track
{
	std::multimap<Time, StreamEvent> events;
	std::vector<Time> syncPoints;

	bool isNull() const { return !(syncPoints.size() && events.size()); }

	StreamEvents eventsBetween(Time _from, Time _to) const	///< Get the stream events in range [_from, _to);
	{
		StreamEvents ret;
		auto end = events.lower_bound(_to);
		for (auto it = events.lower_bound(_from); it != end; ++it)
			ret.push_back(it->second);
		return ret;
	}

	template <class _F>
	void streamIn(_F const& _read)	// _F must be a function (void*, size_t)
	{
		events.clear();
		syncPoints.clear();
		syncPoints.push_back(Time(0));
		// data stream is intel-encoded for simplicity.
		uint32_t testSwap = 69;
		bool doSwap = !*(char*)&testSwap;	// swap if first-byte is zero (for intel first byte would be 69).

		uint32_t count;
		_read(&count, sizeof(count));
		if (doSwap)
			count = __bswap_32(count);
		for (unsigned i = 0; i < count; ++i)
		{
			Time t;
			_read(&t, sizeof(Time));
			if (doSwap)
				t = __bswap_64(t);
			StreamEvent se;
			_read(&se, sizeof(StreamEvent));
			if (se.type == SyncPoint)
				syncPoints.push_back(t);
			memset(&se.m_aux, 0, sizeof(se.m_aux));		// shouldn't be set, but just in case...
			events.insert(std::make_pair(t, se));
		}
	}

	template <class _F>	// _F must be a function (void const*, size_t)
	void streamOut(_F const& _write) const
	{
		int32_t count = events.size();
		_write(&count, sizeof(count));
		for (auto i: events)
		{
			_write(&(i.first), sizeof(Time));
			_write(&(i.second), sizeof(StreamEvent));
		}
	}

	LIGHTBOX_API void readFile(std::string const& _filename);
	LIGHTBOX_API void updateSyncPoints();
};

}
