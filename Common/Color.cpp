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
#include "Color.h"
using namespace std;
using namespace Lightbox;

float Color::hueCorrection(float _h)
{
	static const std::map<float, float> c_brightnessCurve{{0, .9f}, {.166666, .75f}, {.333333, .85f}, {.5, .85f}, {.666666, 1.f}, {.8333333, .9f}, {1, .9f}};
	return lerpLookup(c_brightnessCurve, _h);
}
