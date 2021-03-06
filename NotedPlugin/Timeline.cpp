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

#include "NotedFace.h"
#include "Timeline.h"

using namespace lb;
using namespace std;

Timeline::Timeline()
{
//	installEventFilter(NotedFace::get());
}

Timeline::~Timeline()
{
}

lb::Time Timeline::earliestVisible() const
{
	return NotedFace::view()->earliestVisible();
}

lb::Time Timeline::pitch() const
{
	return NotedFace::view()->pitch();
}

lb::Time Timeline::highlightFrom() const
{
	return NotedFace::audio()->cursor();
}

lb::Time Timeline::highlightDuration() const
{
	return NotedFace::audio()->hop();
}
