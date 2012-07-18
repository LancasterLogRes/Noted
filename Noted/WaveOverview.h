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

#include <NotedPlugin/Prerendered.h>

class Noted;

class WaveOverview: public Prerendered
{
	Q_OBJECT
	friend class Noted;

public:
	WaveOverview(QWidget* _parent = 0);

	int positionOf(Lightbox::Time _t);
	Lightbox::Time timeOf(int _x);

signals:
	void resized();

private:
	using Prerendered::event;

	virtual void doRender(QImage& _img);
	virtual void mousePressEvent(QMouseEvent* _e);
	virtual void mouseMoveEvent(QMouseEvent* _e);
	virtual void paintGL();
};
