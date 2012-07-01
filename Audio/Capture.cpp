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

#include <iostream>
#include <string>
#include <portaudio.h>
using namespace std;

#include "Capture.h"
using namespace Audio;

Capture::Capture(int _device, unsigned _channels, unsigned _rate, unsigned long _frames, unsigned _periods, char const*)
{
	(void)_device;
	(void)_channels;
	(void)_rate;
	(void)_frames;
	(void)_periods;
}

vector<int16_t> const& Capture::read()
{
	return m_buffer;
}
