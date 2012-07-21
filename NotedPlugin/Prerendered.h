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

#include <utility>
#include <map>
#include <Common/Common.h>
#include <QThread>
#include <QGLWidget>
#include <QMutex>
#include <QPainter>
#include <QDebug>
#include <QMouseEvent>
#include <QWidget>

class NotedFace;
class Prerendered;
class QGLFramebufferObject;

template <class _FX, class _FY, class _FL>
void drawPeaks(QPainter& _p, std::map<float, float> const& _ps, int _yoffset, _FX _x, _FY _y, _FL _l, int _maxCount = 5)
{
	int ly = _yoffset;
	int pc = 0;
	BOOST_REVERSE_FOREACH (auto peak, _ps)
		if (peak.first > (--_ps.end())->first * .25 && pc++ < _maxCount)
		{
			int x = _x(peak.second);
			int y = _y(peak.first);
			_p.setPen(QColor::fromHsvF(float(pc) / _maxCount, 1.f, 0.5f, 0.5f));
			_p.drawEllipse(QPoint(x, y), 4, 4);
			_p.drawLine(x, y - 4, x, ly + 6);
			QString f = _l(peak.second);
			int fw = _p.fontMetrics().width(f);
			_p.drawLine(x + 16 + fw + 2, ly + 6, x, ly + 6);
			_p.setPen(QColor::fromHsvF(0, 0.f, .35f));
			_p.fillRect(QRect(x+12, ly-6, fw + 8, 12), QBrush(QColor(255, 255, 255, 160)));
			_p.drawText(QRect(x+16, ly-6, 160, 12), Qt::AlignVCenter, f);
			ly += 14;
		}
		else
			break;
}

class DisplayThread: public QThread
{
public:
	DisplayThread(Prerendered* _p): m_p(_p) {}

	virtual void run();

	using QThread::msleep;

private:
	Prerendered* m_p;
};

class Prerendered: public QGLWidget
{
	Q_OBJECT

	friend class DisplayThread;

public:
	Prerendered(QWidget* _p);
	~Prerendered();

	NotedFace* c() const;

public slots:
	void rerender();

protected:
	virtual bool needsRepaint() const;
	virtual void doRender(QGLFramebufferObject*) {}

	virtual void initializeGL();
	virtual void resizeGL(int _w, int _h);
	virtual void paintGL();

	virtual void run();

	virtual void paintEvent(QPaintEvent*);
	virtual void hideEvent(QShowEvent*);
	virtual void closeEvent(QCloseEvent*);
	virtual void resizeEvent(QResizeEvent* _e);

protected:
	QGLFramebufferObject* m_fbo;

private:
	void quit();

	DisplayThread m_display;
	bool m_quitting;

	mutable NotedFace* m_c;
	QSize m_newSize;
};
