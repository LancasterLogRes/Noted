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

#include <QWidget>

class Noted;

class Cursor: public QWidget
{
	Q_OBJECT

public:
	Cursor(Noted* _c, int _id = 0);
	virtual ~Cursor();

public slots:
	void updateGeo();

private:
	virtual void paintEvent(QPaintEvent* _e);
	virtual bool event(QEvent* _e);

	Noted* m_c;
	int m_id;
};
