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
#include <QDebug>
#include <QFrame>
#include <QPaintEvent>
#include <QPainter>
#include <NotedPlugin/NotedFace.h>

#include "WaveWindowView.h"

using namespace std;
using namespace Lightbox;

void WaveWindowView::doRender(QImage& _img)
{
	foreign_vector<float> d = c()->waveWindow(c()->cursorIndex());
	float* data = d.data();

	int mean = 0;
	float rms = 0.f;

	vector<float> const& wf = c()->windowFunction();
	unsigned windowSize = d.count();
	if (windowSize && wf.size())
	{
		double total = 0;
		for (unsigned i = 0; i < windowSize; ++i)
			total += data[i];
		mean = total / windowSize;

		double totalsqd = 0;
		for (unsigned i = 0; i < windowSize; ++i)
		{
			int d = data[i] - mean;
			totalsqd += d * d;
		}
		rms = sqrt(totalsqd / windowSize);

		int w = width();
		int h = height();

		QPainter p(&_img);
		p.fillRect(rect(), Qt::white);

		QVector<QLine> pps;
		pps.reserve(windowSize - 1);
		unsigned off = c()->isZeroPhase() ? windowSize / 2 : 0;
		for (unsigned i = 1; i < windowSize; ++i)
			pps.push_back(QLine((i - 1) * w / windowSize, (h + h * data[(i - 1 + off) % windowSize] * wf[(i - 1 + off) % windowSize]) / 2, i * w / windowSize, (h + h * data[(i + off) % windowSize] * wf[(i + off) % windowSize]) / 2));

		p.setPen(QColor(224, 224, 224));
		p.drawLine(0, h / 2, w, h / 2);

		p.setPen(QColor(0, 0, 255));
		p.drawLines(pps);

		p.setPen(QColor(127, 127, 255));
		p.drawLine(0, (h + h * rms / 65536) / 2, w, (h + h * rms / 65536) / 2);
		p.drawLine(0, (h - h * rms / 65536) / 2, w, (h - h * rms / 65536) / 2);
	}
}
