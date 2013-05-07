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

#include <cmath>
#include <vector>
#include <QDebug>
#include <QFrame>
#include <QPaintEvent>
#include <QPainter>
#include <QGLFramebufferObject>
#include <NotedPlugin/NotedFace.h>
#include "WaveView.h"

using namespace std;
using namespace Lightbox;

Lightbox::Time WaveView::highlightFrom() const
{
	return m_nf->cursor() - m_nf->windowSize() + m_nf->hop();
}

Lightbox::Time WaveView::highlightDuration() const
{
	return m_nf->windowSize();
}

void WaveView::renderGL()
{
	PrerenderedTimeline::renderGL();

	if (width() < 1)
		return;

	vector<float> wave(width() * 2);

	bool isAbsolute = c()->waveBlock(c()->timeOf(0), c()->durationOf(width()), foreign_vector<float>(wave.data(), wave.size()));

	int h = height();
	QOpenGLPaintDevice glpd(size());
	QPainter p(&glpd);

	int _dx = 0;
	int _dw = width();

	QRect r(_dx, 0, _dw, h);
	p.setClipRect(r);
	p.translate(0, 10);
	h -= 10;

	unsigned sd = sigma(wave, 0.f);
	p.fillRect(_dx, (h - h * sd * 3 / 32767.f) / 2, _dw, h * sd * 3 / 32767.f, QColor(0, 0, 0, 16));
	p.fillRect(_dx, (h - h * sd * 2 / 32767.f) / 2, _dw, h * sd * 2 / 32767.f, QColor(0, 0, 0, 16));
	p.fillRect(_dx, (h - h * sd / 32767.f) / 2, _dw, h * sd / 32767.f, QColor(0, 0, 0, 16));

	p.setPen(QColor(0, 0, 0, 16));
	p.drawLine(_dx, h / 2 - 1, _dx + _dw - 1, h / 2 - 1);

	p.setPen(QColor(0, 0, 0));
	if (isAbsolute)
	{
		for (int x = _dx; x < _dx + _dw; ++x)
		{
			int bhMax = max(1.f, h * wave[(x - _dx) * 2 + 1]);
			p.fillRect(x, (h - bhMax) / 2, 1, bhMax, QColor(0, 0, 0));
			int bhRms = max(1.f, h * wave[(x - _dx) * 2]);
			p.fillRect(x, (h - bhRms) / 2, 1, bhRms, QColor(127, 127, 127));
		}
	}
	else
	{
		int ly;
		for (int x = _dx; x < _dx + _dw; ++x)
		{
			int ty = (h + h * wave[x - _dx]) / 2;
			if (x != _dx)
				p.drawLine(x - 1, ly, x, ty);
			ly = ty;
		}
	}
}

