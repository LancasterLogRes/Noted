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

#include <map>
#include <string>
#include <exception>

struct PaStreamParameters;

namespace Audio
{

class Common
{
public:
	struct Exception: public std::exception { Exception(int _rc); char const* msg; };
	struct CannotOpenDevice: public Exception { CannotOpenDevice(int _rc): Exception(_rc) {} };
	struct InvalidParameters: public Exception { InvalidParameters(int _rc): Exception(_rc) {} };
	struct UnhandledIOError: public Exception { UnhandledIOError(int _rc): Exception(_rc) {} };

	Common();
	~Common();

	static void init();
	static void fini();

	unsigned				frames() const { return m_frames; }
	unsigned				rate() const { return m_rate; }
	unsigned				channels() const { return m_channels; }
	unsigned				periods() const { return m_periods; }
	bool					isInterleaved() const { return false; }
	int						device() const { return m_device; }
	std::string				deviceName() const { return m_deviceName; }

protected:
	static std::map<int, std::string> devices(bool _playback);

	void*					m_stream;
	PaStreamParameters*		m_params;
	unsigned long			m_frames;
	unsigned				m_rate;
	unsigned				m_channels;
	unsigned				m_periods;
	int						m_device;
	std::string				m_deviceName;

private:
	static int				s_initCount;
};

}

