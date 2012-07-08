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

#include <tuple>
using namespace std;

#include <QDebug>
#include <QtGui>

#include "Noted.h"
#include "Cursor.h"

Cursor::Cursor(Noted* _c, int _id): QWidget(_c, Qt::FramelessWindowHint|Qt::Tool), m_c(_c), m_id(_id)
{
#if !defined(Q_OS_WIN)
    setWindowHint(Qt::WindowStaysOnBottomHint);
#endif
    setStyleSheet("background:transparent;");
	setAttribute(Qt::WA_TranslucentBackground, true);
	setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
	setAttribute(Qt::WA_X11DoNotAcceptFocus, true);
    setEnabled(false);
    setAutoFillBackground(false);
	connect(m_c, SIGNAL(cursorChanged()), SLOT(updateGeo()));
	connect(m_c, SIGNAL(offsetChanged()), SLOT(updateGeo()));
	connect(m_c, SIGNAL(durationChanged()), SLOT(updateGeo()));
	connect(m_c, SIGNAL(viewSizesChanged()), SLOT(updateGeo()));
	connect(m_c, SIGNAL(viewSizesChanged()), SLOT(update()));
	updateGeo();
	show();
}

Cursor::~Cursor() {}

bool Cursor::event(QEvent* _e)
{
	if (m_c->cursorEvent(_e, m_id))
		return true;
	return QWidget::event(_e);
}

void Cursor::updateGeo()
{
	QRect geo = m_c->cursorGeo(m_id);
	if (pos() != geo.topLeft())
		move(geo.topLeft());
	if (size() != geo.size())
	{
		if (geo.size().isEmpty())
			resize(1, 1);
		else
			resize(geo.size());
	}
}

void Cursor::paintEvent(QPaintEvent*)
{
    QPainter p(this);
	m_c->paintCursor(p, m_id);
}
