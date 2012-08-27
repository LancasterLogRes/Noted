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

#include <Common/Global.h>
#include "Common.h"

namespace Audio
{

class Capture: public Common
{
public:
	class IncorrectNumberOfFrames: public std::exception {};

	Capture(int _device = -1, unsigned _channels = 1, int _rate = -1, unsigned long _frames = 1024, int _periods = -1, bool _force16Bit = false);
	void read(Lightbox::foreign_vector<float> o_destination);

	static std::map<int, std::string> devices() { return Common::devices(false); }
};

}
