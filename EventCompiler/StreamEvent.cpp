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

#include <Common/StreamIO.h>
#include "StreamEvent.h"

namespace Lightbox
{

std::string id(float _y) { return toString(_y); }
std::string ms(float _x){ return toString(std::round(_x * 1000)) + (!_x ? "ms" : ""); }
std::string msL(float _x, float _y) { return toString(std::round(_x * 1000)) + "ms (" + toString(std::round(_y * 100)) + "%)"; }
std::string bpm(float _x) { return _x ? toString(std::round(toBpm(fromSeconds(_x)) * 10) / 10) : std::string("inf bpm"); }
std::string bpmL(float _x, float _y) { return toString(std::round(toBpm(fromSeconds(_x)) * 10) / 10) + "bpm (" + toString(std::round(_y * 100)) + "%)"; }

}
