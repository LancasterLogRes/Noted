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

#include <utility>
#include <cmath>
#include <QDebug>
#include <QFrame>
#include <QPaintEvent>
#include <QPainter>
#include <Common/Common.h>
#include <NotedPlugin/NotedFace.h>

#include "SpectrumView.h"

using namespace std;
using namespace Lightbox;

void SpectrumView::doRender(QGLFramebufferObject* _fbo)
{
	auto mag = c()->magSpectrum(m_i, 1);
	auto phase = c()->phaseSpectrum(m_i, 1);
	unsigned s = c()->spectrumSize();

	if (mag && phase)
	{
		int w = width();
		int ho = height() / 5;
		int h = height() - 16 - ho;

		QPainter p(_fbo);
		p.fillRect(rect(), qRgb(255, 255, 255));

		float sc = qMax(1.f, Lightbox::range(mag.begin(), mag.end()).second);

		GraphParameters<float> minorParams(make_pair(0.f, sc), h / 18, ForceMinor);
		p.setPen(QColor(236, 236, 236));
		for (float f = minorParams.from; f < minorParams.to; f += minorParams.incr)
			p.drawLine(20, h - h * f / sc + ho, w, h - h * f / sc + ho);

		GraphParameters<float> majorParams(make_pair(0.f, sc), h / 18);
		for (float f = majorParams.from; f < majorParams.to; f += majorParams.incr)
		{
			int y = h - h * f / sc + ho;
			p.setPen(QColor(208, 208, 208));
			p.drawLine(18, y, w, y);
			p.setPen(QColor(144, 144, 144));
			p.drawText(2, y + 4, QString::number(f));
		}

		p.setPen(QColor(192,192,192));
		p.drawLine(12, h + ho, w, h + ho);

		w -= 24;

		float nyquist = c()->rate() / 2;
		GraphParameters<float> freqMinor(make_pair(0.f, nyquist), w / 48, ForceMinor);
		p.setPen(QColor(236, 236, 236));
		for (float f = freqMinor.from; f < freqMinor.to; f += freqMinor.incr)
			p.drawLine(24 + f / nyquist * w, 0, 24 + f / nyquist * w, h + 4 + ho);

		GraphParameters<float> freqMajor(make_pair(0.f, nyquist), w / 48);
		for (float f = freqMajor.from; f < freqMajor.to; f += freqMajor.incr)
		{
			int x = 24 + f / nyquist * w;
			p.setPen(QColor(208, 208, 208));
			p.drawLine(x, 0, x, h + 4 + ho);
			p.setPen(QColor(144, 144, 144));
			p.drawText(QRect(x - 40, h + 2 + ho, 80, 16), Qt::AlignHCenter|Qt::AlignBottom, f ? QString::number(f) + ((f == freqMajor.from + freqMajor.incr) ? "Hz" : "") : "DC");
		}

		QVector<QLine> pps;
		pps.reserve(s - 1);
		p.setPen(Qt::NoPen);
		for (unsigned i = 1; i < s; ++i)
		{
			int lx = 24 + (i - 1) * w / s;
			int ly = h - h * mag[i - 1] / sc + ho;
			int x = 24 + i * w / s;
			int y = h - h * mag[i] / sc + ho;
			float dp = abs(phase[i] - phase[i - 1]);	// delta-phase
			if (dp > Pi)
				dp = 2 * Pi - dp;	// cyclic reflection
			dp /= Pi;	// normalize
			p.fillRect(lx, 0, x - lx + 1, (dp * h) / 5, QBrush(QColor::fromHsvF(phase[i] / (Pi * 2), 0.25f, 0.9f)));
			pps.push_back(QLine(lx, ly, x, y));
			p.setPen(Qt::NoPen);
			p.setBrush(QColor::fromHsvF(0, 0, 0.75f + 0.25f * dp));
			p.drawPolygon(QPolygon(QVector<QPoint>() << QPoint(x, h + ho) << QPoint(x, y) << QPoint(lx, ly) << QPoint(lx, h + ho)));
		}

		p.setPen(QColor(128, 128, 128));
		p.setBrush(Qt::NoBrush);
		p.drawLines(pps);

		if (!c()->isPlaying())
			drawPeaks(p, parabolicPeaks(mag.data(), s), ho, [&](float _x){ return 24 + _x * w / s; }, [&](float _y) { return h - h * _y / sc + ho; }, [&](float _x) { return QString::number(round(_x * nyquist / s)) + "Hz"; } );
	}
}
