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

using namespace Lightbox;
using namespace std;

Timeline::~Timeline()
{
}

void Timeline::initTimeline(NotedFace* _nf)
{
	m_nf = _nf;
}

void Timeline::finiTimeline()
{
	m_nf->timelineDead(this);
}

Lightbox::Time Timeline::earliestVisible() const
{
	return m_nf->earliestVisible();
}

Lightbox::Time Timeline::pixelDuration() const
{
	return m_nf->pixelDuration();
}

Lightbox::Time Timeline::highlightFrom() const
{
	return m_nf->cursor();
}

Lightbox::Time Timeline::highlightDuration() const
{
	return m_nf->hop();
}
