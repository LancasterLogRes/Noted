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

#include <cstdint>

namespace Lightbox
{

// e.g. in the case of 16-bit stereo @ 44100 Hz:
struct WavHeader
{
	uint32_t riffMagic: 32;			// i.e. "RIFF"
	uint32_t bytesToFollow: 32;
	uint32_t waveMagic: 32;			// i.e. "WAVE"
	uint32_t fmtMagic: 32;			// i.e. "fmt "
	uint32_t fmtBytesToFollow: 32;	// e.g. 16	- 16 bytes to follow
	uint16_t format: 16;			// e.g. 1	- LPCM
	uint16_t channels: 16;			// e.g. 2
	uint32_t rate: 32;				// e.g. 44100
	uint32_t bytesPerSecond: 32;	// e.g. 176400
	uint16_t bytesPerFrame: 16;		// e.g. 4
	uint16_t bitsPerSample: 16;		// e.g. 16
	uint32_t dataMagic: 32;			// i.e. "data"
	uint32_t dataBytes: 32;

	bool verify() const { return riffMagic == *(uint32_t*)"RIFF" && waveMagic == *(uint32_t*)"WAVE" && fmtMagic == *(uint32_t*)"fmt " && dataMagic == *(uint32_t*)"data"; }
	void init() { riffMagic = *(uint32_t*)"RIFF"; waveMagic = *(uint32_t*)"WAVE"; fmtMagic = *(uint32_t*)"fmt "; dataMagic = *(uint32_t*)"data"; fmtBytesToFollow = 16; }
	void setCosyLpcm(unsigned _channels, unsigned _rate, unsigned _frames, unsigned _bitsPerSample = 16) { init(); format = 1; channels = _channels; rate = _rate; bitsPerSample = _bitsPerSample; bytesPerFrame = channels * bitsPerSample / 8; bytesPerSecond = bytesPerFrame * rate; dataBytes = _frames * bytesPerFrame; bytesToFollow = dataBytes + sizeof(WavHeader) - 12; }
	WavHeader(unsigned _channels, unsigned _rate, unsigned _frames, unsigned _bitsPerSample = 16) { setCosyLpcm(_channels, _rate, _frames, _bitsPerSample); }
};

}
