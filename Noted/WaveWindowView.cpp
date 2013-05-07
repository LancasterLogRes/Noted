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
#include <QGLFramebufferObject>
#include <NotedPlugin/NotedFace.h>
#include "Noted.h"
#include "WaveWindowView.h"

using namespace std;
using namespace Lightbox;

void WaveWindowView::renderGL()
{
	foreign_vector<float const> d = dynamic_cast<Noted*>(c())->cursorWaveWindow();
	float const* data = d.data();

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

			QOpenGLPaintDevice glpd(size());
	QPainter p(&glpd);

		p.fillRect(rect(), Qt::white);

		p.setPen(QColor(224, 224, 224));
		p.drawLine(0, h / 2, w, h / 2);

		unsigned off = c()->isZeroPhase() ? windowSize / 2 : 0;
		for (unsigned i = 0; i < windowSize; ++i)
		{
			p.fillRect(i * w / windowSize, h / 2, max<int>(1, (i + 1) * w / windowSize - i * w / windowSize), h * data[(i + off) % windowSize] * wf[(i + off) % windowSize] / -2, QColor(224, 224, 255));
			//p.setPen();
			//p.drawLine(QLine((i - 1) * w / windowSize, (h - h * data[(i - 1 + off) % windowSize] * wf[(i - 1 + off) % windowSize]) / 2, i * w / windowSize, (h - h * data[(i + off) % windowSize] * wf[(i + off) % windowSize]) / 2));
			if (i)
			{
				p.setPen(QColor(224, 224, 224));
				p.drawLine(QLine((i - 1) * w / windowSize, (h - h * wf[(i - 1 + off) % windowSize]) / 2, i * w / windowSize, (h - h * wf[(i + off) % windowSize]) / 2));
				p.setPen(QColor(0, 0, 127));
				p.drawLine(QLine((i - 1) * w / windowSize, (h - h * data[(i - 1 + off) % windowSize]) / 2, i * w / windowSize, (h - h * data[(i + off) % windowSize]) / 2));
			}
		}

		p.setPen(QColor(127, 127, 255));
		p.drawLine(0, (h + h * rms / 65536) / 2, w, (h + h * rms / 65536) / 2);
		p.drawLine(0, (h - h * rms / 65536) / 2, w, (h - h * rms / 65536) / 2);
	}
}
