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

Playback::Playback(int _device, int _channels, int _rate, unsigned long _frames, int _periods, bool _force16Bit)
{
	init();

	m_params = new PaStreamParameters;

	m_device = m_params->device = (_device == -1) ? Pa_GetDefaultOutputDevice() : _device;
	PaDeviceInfo const* deviceInfo = Pa_GetDeviceInfo(m_device);
	if (!deviceInfo)
	{
		m_device = m_params->device = Pa_GetDefaultOutputDevice();
		deviceInfo = Pa_GetDeviceInfo(m_device);
	}

	PaHostApiInfo const* apiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
	m_deviceName = string(apiInfo->name) + "/" + deviceInfo->name;

	m_params->channelCount = _channels;
	m_channels = m_params->channelCount;
	m_params->sampleFormat = (_force16Bit ? paInt16 : paFloat32) | paNonInterleaved;

	m_rate = (_rate == -1) ? deviceInfo->defaultSampleRate : _rate;
	m_frames = _frames;

	m_params->suggestedLatency = (_periods > -1) ? double(m_frames * _periods) / m_rate : deviceInfo->defaultLowOutputLatency;
	m_params->hostApiSpecificStreamInfo = nullptr;

	if (int err = Pa_OpenStream(&m_stream, NULL, m_params, m_rate, m_frames, paClipOff, NULL, NULL))
		throw CannotOpenDevice(err);

	PaStreamInfo const* si = Pa_GetStreamInfo(m_stream);
	m_rate = si->sampleRate;
	m_periods = si->outputLatency / m_frames * si->sampleRate;

	if (int err = Pa_StartStream(m_stream))
		throw Exception(err);
}

void Playback::write(vector<float> const& _frame)
{
	if (_frame.size() != m_frames * m_channels)
		throw IncorrectNumberOfFrames();
	write(_frame.data());
}

void Playback::write(float const* _frame)
{
	void const* buffers[m_channels];
	int rc;
	if ((m_params->sampleFormat & paInt16) == paInt16)
	{
		int cf = m_channels * m_frames;
		int16_t buffer[cf];
		for (int i = 0; i < cf; ++i)
			buffer[i] = _frame[i] * 32767;
		for (unsigned i = 0; i < m_channels; ++i)
			buffers[i] = buffer + m_frames * i;
		rc = Pa_WriteStream(m_stream, buffers, m_frames);
	}
	else
	{
		for (unsigned i = 0; i < m_channels; ++i)
			buffers[i] = _frame + m_frames * i;
		rc = Pa_WriteStream(m_stream, buffers, m_frames);
	}
	if (rc)
	{
		if (rc == paOutputUnderflowed)
		{
			cerr << "*** AUDIO: Buffer underrun." << endl;
			Pa_StopStream(m_stream);
			Pa_StartStream(m_stream);
		}
		else
			throw UnhandledIOError(rc);
	}
}
