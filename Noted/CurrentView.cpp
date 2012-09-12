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

#include <NotedPlugin/NotedFace.h>

#include "CurrentView.h"

CurrentView::CurrentView(QWidget* _parent): Prerendered(_parent), m_i(-1)
{}

bool CurrentView::needsRepaint() const
{
	if (Prerendered::needsRepaint() && (m_i != (int)c()->cursorIndex() || c()->isImmediate()))
	{
		m_i = c()->cursorIndex();
		return true;
	}
	return false;
}
