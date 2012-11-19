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
#include <array>
#include <map>
#include "Global.h"
#include "Flags.h"

namespace Lightbox
{

enum ColorSetItem
{
	ChromaToggle =			0x00000000,
	ChromaMin =				0x00000001,
	ChromaSelection =		0x00000002, // Use colorSelection() to determine actual set.
	ChromaContinuous =		0x00000008,
	ValueMax =				0x00000000,
	ValueToggle =			0x00000010,
	ValueSelection =		0x00000020,
	ValueContinuous =		0x00000080,
	HueSelection =			0x00000200, // Use colorSelection() to determine actual set.
	HueContinuous =			0x00000800,
	SingleColor =			0x00001000	// Forces RGB to a value channel-wise <= to single color in selection.
};

LIGHTBOX_FLAGS(ColorSetItem, ColorSet, (ChromaToggle)(ChromaMin)(ChromaSelection)(ChromaContinuous)(ValueMax)(ValueToggle)(ValueSelection)(ValueContinuous)(HueSelection)(HueContinuous)(SingleColor));

static const ColorSet DimmableSelection =		HueSelection|ChromaSelection|ValueContinuous;
static const ColorSet DimmableMono =			SingleColor|ValueContinuous;
static const ColorSet FullColorSet =			HueContinuous|ChromaContinuous|ValueContinuous;

LIGHTBOX_STRUCT(6, Meter, float, period, float, Wstrength, float, Hstrength, float, Qstrength, float, Dstrength, float, Tstrength);

LIGHTBOX_STRUCT(7, Phase, float, phase, float, forebeat, float, qBackbeat, float, eBackbeat, float, sBackbeat, float, trailGap, float, mltAnchor);

LIGHTBOX_STRUCT(6, Tempo, float, prior, float, Wstrength, float, Hstrength, float, Qstrength, float, Dstrength, float, Tstrength);

LIGHTBOX_STRUCT(6, Start, float, forebeat, float, qBackbeat, float, eBackbeat, float, sBackbeat, float, trailGap, float, mltAnchor);

}
