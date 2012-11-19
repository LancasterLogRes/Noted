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

#include <chrono>
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
static const Time BigBang = std::numeric_limits<Time>::min() + 1;

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

/// Pseudo-random number, determined entirely by _t.
inline uint32_t random(Time _t)
{
	uint32_t w = (_t & 0x55555555) | ((_t & 0xaaaaaaaa00000000) >> 32);    /* must not be zero */
	uint32_t z = (_t & 0xaaaaaaaa) | ((_t & 0x5555555500000000) >> 32);    /* must not be zero */

	if (!w)
		++w;
	if (!z)
		++z;

	z = 36969 * (z & 65535) + (z >> 16);
	w = 18000 * (w & 65535) + (w >> 16);
	return (z << 16) + w;
}

/// Pseudo-random factor in interval [_min, _max], determined entire by _t.
inline float random(Time _t, float _min, float _max)
{
	return random(_t) / double(UINT32_MAX) * (_max - _min) + _min;
}

/// Pseudo-random factor in interval [0, _delta - 1], determined entire by _t.
inline int random(Time _t, int _delta)
{
	return _delta ? _delta > 0 ? random(_t) / (UINT32_MAX / _delta) : -(random(_t) / (UINT32_MAX / -_delta)) : 0;
}

inline float halfLifeDecay(Time _halfLife, Time _unit, float _factor = 1.f)
{
	return _factor / exp2(double(_unit) / _halfLife);
}

/// @deprecated Use halfLifeDecay() instead.
inline float decayed(float _f, Time _hl, Time _p)
{
	return halfLifeDecay(_p, _hl, _f);
}

class Timer
{
	typedef std::chrono::high_resolution_clock Clock;
public:
	Timer() { reset(); }
	void reset() { m_start = Clock::now(); }
	Time elapsed() const
	{
		Clock::time_point t = Clock::now();
		Clock::duration d = t - m_start;
//			if (m_start > t)
//				d = t + Clock::duration() - m_start;
		return std::chrono::duration_cast<std::chrono::duration<int64_t, std::ratio<1, s_baseRate>>>(d).count();
	}
private:
	Clock::time_point m_start;
};

class AccTimer: public Timer
{
public:
	AccTimer(Time& _acc): m_acc(_acc) {}
	~AccTimer() { m_acc += elapsed(); }
private:
	Time& m_acc;
};

}
