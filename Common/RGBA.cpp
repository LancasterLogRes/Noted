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

#include <cmath>
#include "RGBA.h"
using namespace std;
using namespace Lightbox;

float RGBA::hueCorrection(unsigned _h)
{
	static const std::map<unsigned, float> c_brightnessCurve{{0, .9f}, {60, .75f}, {120, .85f}, {180, .85f}, {240, 1.f}, {300, .9f}, {360, .9f}};
	return lerpLookup(c_brightnessCurve, _h);
}

std::vector<uint8_t> Lightbox::gammaTable(float _g)
{
	std::vector<uint8_t> ret(256);
	for (int i = 0; i < 256; ++i)
		ret[i] = uint8_t(pow(float(i) / 255.f, _g) * 255.f);
	return ret;
}
