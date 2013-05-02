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

#include <QtGui/QApplication>
#include <Common/Common.h>
#include "Noted.h"

extern "C"
{
void XInitThreads();
}

int main(int argc, char *argv[])
{
	if (!Lightbox::UnitTesting<100>::go())
		return -1;
//	QApplication::setAttribute(Qt::AA_X11InitThreads);
#ifdef Q_WS_X11
	XInitThreads();
#endif
	QApplication a(argc, argv);
	Noted w;
#if defined(Q_WS_S60)
	w.showMaximized();
#else
	w.show();
#endif
	return a.exec();
}
