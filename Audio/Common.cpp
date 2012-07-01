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
#include <portaudio.h>

#include "Common.h"

using namespace std;
using namespace Audio;

int Audio::Common::s_initCount = 0;

Common::Exception::Exception(int _rc):
	msg(Pa_GetErrorText(_rc))
{
	cerr << msg << endl;
}

Common::Common():
	m_stream	(0),
	m_params	(0)
{
}

Common::~Common()
{
	if (m_stream)
	{
		if (int err = Pa_StopStream(m_stream))
			throw Exception(err);

		if (int err = Pa_CloseStream(m_stream))
			throw Exception(err);

		fini();
		delete m_params;
	}
}

void Common::init()
{
	if (++s_initCount == 1)
		if (int err = Pa_Initialize())
			throw Exception(err);
}

void Common::fini()
{
	if (--s_initCount == 0)
		Pa_Terminate();
}

std::map<int, std::string> Common::devices(bool _playback)
{
	init();

	std::map<int, std::string> ret;
	int numDevices = Pa_GetDeviceCount();
	for (int i = 0; i < numDevices; ++i)
	{
		PaDeviceInfo const* deviceInfo = Pa_GetDeviceInfo(i);
		PaHostApiInfo const* apiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
		if (deviceInfo->maxInputChannels > 0 && !_playback || deviceInfo->maxOutputChannels > 0 && _playback)
			ret[i] = string(apiInfo->name) + "/" + deviceInfo->name;
	}

	fini();

	return ret;
}
