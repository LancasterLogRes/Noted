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
#include <cstdint>

#include "Common.h"

namespace Audio
{

class Capture: public Common
{
public:
	Capture(int _device = -1, unsigned _channels = 1, unsigned _rate = 44100, unsigned long _frames = 1024, unsigned _periods = 4, char const* _elem = "");
	std::vector<int16_t> const& read();

	static std::map<int, std::string> devices() { return Common::devices(false); }

private:
	std::vector<int16_t>			m_buffer;
};

}
