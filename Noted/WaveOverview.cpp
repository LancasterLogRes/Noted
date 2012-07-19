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

#include <QPainter>
#include <Common/Common.h>
#include <NotedPlugin/NotedFace.h>

#include "WaveOverview.h"

using namespace std;
using namespace Lightbox;

WaveOverview::WaveOverview(QWidget* _parent): Prerendered(_parent)
{
	connect(c(), SIGNAL(offsetChanged()), SLOT(updateGL()));
	connect(c(), SIGNAL(durationChanged()), SLOT(updateGL()));
	connect(c(), SIGNAL(analysisFinished()), SLOT(rerender()));
	connect(c(), SIGNAL(analysisFinished()), SLOT(updateGL()));
	connect(c(), SIGNAL(cursorChanged()), SLOT(updateGL()));
}

int WaveOverview::positionOf(Lightbox::Time _t)
{
	return ((double(_t) / c()->duration()) * .95 + .025) * width();
}

Time WaveOverview::timeOf(int _x)
{
	return (double(_x) / width() - .025) / .95 * c()->duration();
}

void WaveOverview::mousePressEvent(QMouseEvent* _e)
{
	if (_e->button() == Qt::LeftButton)
		c()->setCursor(timeOf(_e->x()));
}

void WaveOverview::mouseMoveEvent(QMouseEvent* _e)
{
	if (_e->buttons() & Qt::LeftButton)
		c()->setCursor(timeOf(_e->x()));
}

void WaveOverview::initializeGL()
{
	Prerendered::initializeGL();
}

void WaveOverview::paintGL()
{
	Prerendered::paintGL();

	int cursorL = positionOf(c()->earliestVisible());
	int cursorR = positionOf(c()->latestVisible());
	int cursorM = positionOf(c()->cursor());

	glColor4f(0.f, .3f, 1.f, .2f);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBegin(GL_TRIANGLE_STRIP);
	glVertex2i(cursorL, 0);
	glVertex2i(cursorR, 0);
	glVertex2i(cursorL, height());
	glVertex2i(cursorR, height());
	glEnd();

	glBegin(GL_LINES);
	glColor4f(0.f, .3f, 1.f, .5f);
	glVertex2i(cursorL, 0);
	glVertex2i(cursorL, height());
	glVertex2i(cursorR, 0);
	glVertex2i(cursorR, height());
	glColor4f(0.f, 0.f, 0.f, .5f);
	glVertex2i(cursorM, 0);
	glVertex2i(cursorM, height());
	glEnd();
}

void WaveOverview::doRender(QImage& _img)
{
	int w = width();
	int h = height();

	if (w < 1 || h < 1)
		return;
	int ww = w * .95;
	int wx = w * .025;

	vector<float> wave(ww);
	bool isAbsolute = c()->waveBlock(Time(0), c()->duration(), foreign_vector<float>(wave.data(), wave.size()));

	_img.fill(Qt::white);

	QPainter p(&_img);

	GraphParameters<Time> nor(make_pair(0, c()->duration()), width() / 80, toBase(1, 1000000));
	for (Time t = nor.from; t < nor.to; t += nor.incr)
	{
		int x = wx + t * ww / c()->duration();
		if (nor.isMajor(t))
		{
			p.setPen(QColor(160, 160, 160));
			p.drawText(QRect(x - 40, 0, 80, 12), Qt::AlignHCenter | Qt::AlignBottom, ::Lightbox::textualTime(t, nor.delta, nor.major).c_str());
			p.setPen(QColor(192, 192, 192));
			p.drawLine(x, 14, x, height());
		}
		else
		{
			p.setPen(QColor(224, 224, 224));
			p.drawLine(x, 0, x, height());
		}
	}

	p.translate(0, 10);
	h -= 10;

	unsigned sd = sigma(wave, 0.f);
	p.fillRect(0, (h - h * sd * 3 / 32767.f) / 2, w, h * sd * 3 / 32767.f, QColor(0, 0, 0, 16));
	p.fillRect(0, (h - h * sd * 2 / 32767.f) / 2, w, h * sd * 2 / 32767.f, QColor(0, 0, 0, 16));
	p.fillRect(0, (h - h * sd / 32767.f) / 2, w, h * sd / 32767.f, QColor(0, 0, 0, 16));

	p.setPen(QColor(0, 0, 0, 16));
	p.drawLine(0, h / 2 - 1, w, h / 2 - 1);

	p.setPen(QColor(0, 0, 0));
	if (isAbsolute)
	{
		for (int x = wx; x < wx + ww; ++x)
		{
			int bh = max(1.f, h * wave[x - wx]);
			p.fillRect(x, (h - bh) / 2, 1, bh, QColor(0, 0, 127));
			p.fillRect(x, (h - bh / 2) / 2, 1, bh / 2, QColor(127, 127, 192));
		}
	}
	else
	{
		int ly;
		for (int x = wx; x < wx + ww; ++x)
		{
			int ty = (h + h * wave[x - wx]) / 2;
			if (x != wx)
				p.drawLine(x - 1, ly, x, ty);
			ly = ty;
		}
	}
}

