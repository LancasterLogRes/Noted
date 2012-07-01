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

#include "Playback.h"

using namespace std;
using namespace Audio;

Playback::Playback(int _device, int _channels, int _rate, unsigned long _frames, int _periods, char const*)
{
	init();
/*
	int numDevices = Pa_GetDeviceCount();
	for (int i = 0; i < numDevices; ++i)
	{
		PaDeviceInfo const* deviceInfo = Pa_GetDeviceInfo(i);
		PaHostApiInfo const* apiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
		cerr << apiInfo->name << "/" << deviceInfo->name << ": " << (deviceInfo->defaultLowOutputLatency * 1000) << " ms; " << deviceInfo->defaultSampleRate << " Hz; #" << deviceInfo->maxOutputChannels << ((Pa_GetDefaultOutputDevice() == i) ? "[DEFAULT]" : "") << endl;
	}*/

	m_params = new PaStreamParameters;

	m_params->device = (_device == -1) ? Pa_GetDefaultOutputDevice() : _device;
	m_device = m_params->device;
	PaDeviceInfo const* deviceInfo = Pa_GetDeviceInfo(m_device);
	PaHostApiInfo const* apiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
	m_deviceName = string(apiInfo->name) + "/" + deviceInfo->name;

	m_params->channelCount = _channels;
	m_channels = m_params->channelCount;
	m_params->sampleFormat = paInt16;

	m_rate = (_rate == -1) ? deviceInfo->defaultSampleRate : _rate;
	m_frames = _frames;

	m_params->suggestedLatency = (_periods > -1) ? double(m_frames * _periods) / m_rate : deviceInfo->defaultLowOutputLatency;
	m_params->hostApiSpecificStreamInfo = nullptr;

	if (int err = Pa_OpenStream(&m_stream, NULL, m_params, m_rate, m_frames, paClipOff, NULL, NULL))
		throw CannotOpenDevice(err);

	PaStreamInfo const* si = Pa_GetStreamInfo(m_stream);
	m_rate = si->sampleRate;
	m_periods = si->outputLatency / m_frames * si->sampleRate;

//	cerr << "Device " << Pa_GetDeviceInfo(m_device)->name << " open: " << m_frames << " x" << m_periods << " @" << m_rate << "Hz, " << m_channels << "#" << endl;

	if (int err = Pa_StartStream(m_stream))
		throw Exception(err);
}

void Playback::write(vector<int16_t> const& _frame)
{
	if (_frame.size() != m_frames * m_channels)
		throw IncorrectNumberOfFrames();
	write(_frame.data());
}

void Playback::write(int16_t const* _frame)
{
	if (int rc = Pa_WriteStream(m_stream, _frame, m_frames))
	{
		if (rc == paOutputUnderflowed)
		{
			Pa_StopStream(m_stream);
			Pa_StartStream(m_stream);
		}
		else
			throw UnhandledIOError(rc);
	}
}
