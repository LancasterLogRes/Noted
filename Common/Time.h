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

#include <cmath>
#include <sstream>
#include <string>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <sys/time.h>

namespace Lightbox
{

// Is a multiple of 192000, 48000, 36000, 24000 &c.; 88200, 44100, 22050 &c.; and 1000000.
static const int64_t s_baseRate = 7056000000;

typedef int64_t Time;

static const Time UndefinedTime = std::numeric_limits<Time>::min();

static inline Time toBase(int64_t _srcTicks, uint64_t _srcRate)
{
	return Time(_srcTicks) * (s_baseRate / _srcRate);
}

static inline int64_t fromBase(Time _baseTicks, uint64_t _destRate)
{
	return _baseTicks / Time(s_baseRate / _destRate);
}

static inline int64_t toMsecs(Time _t)
{
	return fromBase(_t, 1000);
}

static inline Time fromMsecs(int64_t _msecs)
{
	return toBase(_msecs, 1000);
}

static inline double toSeconds(Time _baseTicks)
{
	return (long double)(_baseTicks) / s_baseRate;
}

static inline Time fromSeconds(double _seconds)
{
	return (long double)(_seconds) * s_baseRate + 0.5;
}

template <int64_t _SrcTicks, uint64_t _SrcRate> struct ToBase
{
	static const int64_t value = int64_t(_SrcTicks) * (s_baseRate / _SrcRate);
};

template <uint64_t _Seconds> struct FromSeconds
{
	static const int64_t value = ToBase<_Seconds, 1>::value;
};

template <uint64_t _Msecs> struct FromMsecs
{
	static const int64_t value = ToBase<_Msecs, 1000>::value;
};

template <uint64_t _Usecs> struct FromUsecs
{
	static const int64_t value = ToBase<_Usecs, 1000000>::value;
};

template <uint64_t _Bpm> struct FromBpm
{
	static const int64_t value = ToBase<60, _Bpm>::value;
};

static const Time OneHour = FromSeconds<3600>::value;
static const Time OneMinute = FromSeconds<60>::value;
static const Time OneSecond = FromSeconds<1>::value;
static const Time OneMsec = FromMsecs<1>::value;
static const Time ZeroTime = 0;

static inline uint64_t wallUsecs()
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return uint64_t(tv.tv_usec) + uint64_t(tv.tv_sec) * 1000000ull;
}

static inline Time wallTime()
{
	return toBase(wallUsecs(), 1000000);
}

inline double toBpm(Time _p)
{
	if (!_p)
		return std::numeric_limits<double>::infinity();
	return double(OneMinute) / _p;
}

inline uint64_t periodToBpm(Time _p)
{
	if (!_p)
		return 0u-1;
	Time oneMinute = toBase(60, 1);
	unsigned intBpm = oneMinute / _p;
	return intBpm;
}

std::string textualTime(Time _t, Time _largest = OneHour, Time _smallestOptional = ZeroTime, Time _smallestRequired = 2*OneHour);
float sensiblePrior(Time _period);

static const Time msecs = FromMsecs<1>::value;
static const Time secs = FromSeconds<1>::value;

inline float oscillator(Time _t, Time _cycle, float _min = 0.f, float _max = 1.f)
{
	float d = (_max - _min) / 2;
	return _min + d * (1 + sin(double(_t) / _cycle * 3.14159265358 * 2));
}

inline float random(Time _t, float _min = 0.f, float _max = 1.f)
{
    ::srand(_t);
    return ::rand() / double(RAND_MAX) * (_max - _min) + _min;
}

inline float decayed(float _f, Time _p, Time _hl)
{
	return _f / exp2(double(_p) / _hl);
}

inline float ofHalfLife(Time _halfLife, Time _unit)
{
	return 1.f / exp2(double(_unit) / _halfLife);
}

}
