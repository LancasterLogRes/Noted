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

#include <map>
#include "Time.h"
#include "Algorithms.h"
using namespace std;
using namespace Lightbox;

float Lightbox::sensiblePrior(Time _period)
{
	static const std::map<Time, float> c_curve{{+FromBpm<220>::value, 0.f}, {+FromBpm<170>::value, .2f}, {+FromBpm<153>::value, 1.f}, {+FromBpm<88>::value, 1.f}, {+FromBpm<65>::value, .2f}, {+FromBpm<40>::value, 0.f}};
	return lerpLookup(c_curve, _period);
}

std::string Lightbox::textualTime(Time _t, Time _d, Time _i, Time _m)
{
	std::stringstream ss;
	int h = std::abs(_t / OneHour);
	int m = std::abs(_t / OneMinute % int64_t(60));
	int s = std::abs(_t / OneSecond % int64_t(60));
	int ms = std::abs(_t / OneMsec % int64_t(1000));
	int us = std::abs(fromBase(_t, 1000000) % int64_t(1000));

	if (_t < 0)
		ss << "-";
	if (_d < OneMsec && !h && !m && !s && !ms && us)
		ss << us << "us";
	else if (_d < OneSecond && !h && !m && !s)
		if ((_i >= OneMsec || !us) && _m >= OneMsec)
			ss << ms << "ms";
		else
			ss << us << "us";
	else if (_d < OneMinute && !h && !m)
		if ((_i >= OneSecond || (!ms && !us)) && _m >= OneSecond)
			ss << s << "s";
		else if ((_i >= OneMsec || !us) && _m >= OneMsec)
			ss << s << ((ms < 10) ? "'00" : (ms < 100) ? "'0" : "'") << ms;
		else
			ss << s << ((ms < 10) ? "'00" : (ms < 100) ? "'0" : "'") << ms << ((us < 10) ? ".00" : (us < 100) ? ".0" : ".") << us;
	else if (_d < OneHour && !h)
		if ((_i >= OneMinute || (!s && !ms && !us)) && _m >= OneMinute)
			ss << m << "m";
		else if ((_i >= OneSecond || (!ms && !us)) && _m >= OneSecond)
			ss << m << ((s < 10) ? ".0" : ".") << s;
		else if ((_i >= OneMsec || !us) && _m >= OneMsec)
			ss << m << ((s < 10) ? ".0" : ".") << s << ((ms < 10) ? "'00" : (ms < 100) ? "'0" : "'") << ms;
		else
			ss << m << ((s < 10) ? ".0" : ".") << s << ((ms < 10) ? "'00" : (ms < 100) ? "'0" : "'") << ms << ((us < 10) ? ".00" : (us < 100) ? ".0" : ".") << us;
	else
		if ((_i >= OneHour || (!m && !s && !ms && !us)) && _m >= OneHour)
			ss << h << "h";
		else if ((_i >= OneMinute || (!s && !ms && !us)) && _m >= OneMinute)
			ss << h << ((m < 10) ? ":0" : ":") << m;
		else if ((_i >= OneSecond || (!ms && !us)) && _m >= OneSecond)
			ss << h << ((m < 10) ? ":0" : ":") << m << ((s < 10) ? ".0" : ".") << s;
		else if ((_i >= OneMsec || !us) && _m >= OneMsec)
			ss << h << ((m < 10) ? ":0" : ":") << m << ((s < 10) ? ".0" : ".") << s << ((ms < 10) ? "'00" : (ms < 100) ? "'0" : "'") << ms;
		else
			ss << h << ((m < 10) ? ":0" : ":") << m << ((s < 10) ? ".0" : ".") << s << ((ms < 10) ? "'00" : (ms < 100) ? "'0" : "'") << ms << ((us < 10) ? ".00" : (us < 100) ? ".0" : ".") << us;
	return ss.str();
}


