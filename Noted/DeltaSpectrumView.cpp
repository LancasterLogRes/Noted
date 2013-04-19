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
#include <QGLFramebufferObject>
#include <Common/Common.h>
#include <NotedPlugin/NotedFace.h>
using namespace std;
using namespace Lightbox;

#include "DeltaSpectrumView.h"

void DeltaSpectrumView::doRender(QGLFramebufferObject* _fbo)
{
	if (m_i < 0)
		return;
	auto phase = c()->phaseSpectrum(m_i, 1);
	auto lPhase = c()->phaseSpectrum(m_i - 1, 1);
	auto llPhase = c()->phaseSpectrum(m_i - 2, 1);
	unsigned s = c()->spectrumSize();

	if (phase && lPhase && llPhase)
	{
		int w = width();
		int h = height() - 16;

		QPainter p(_fbo);
		p.fillRect(rect(), qRgb(255, 255, 255));

		float sc = twoPi<float>();

		GraphParameters<float> minorParams(make_pair(0.f, sc), h / 18, ForceMinor);
		p.setPen(QColor(236, 236, 236));
		for (float f = minorParams.from; f < minorParams.to; f += minorParams.incr)
			p.drawLine(20, h - h * f / sc, w, h - h * f / sc);

		GraphParameters<float> majorParams(make_pair(0.f, sc), h / 18);
		for (float f = majorParams.from; f < majorParams.to; f += majorParams.incr)
		{
			int y = h - h * f / sc;
			p.setPen(QColor(208, 208, 208));
			p.drawLine(18, y, w, y);
			p.setPen(QColor(144, 144, 144));
			p.drawText(2, y + 4, QString::number(f));
		}

		p.setPen(QColor(192,192,192));
		p.drawLine(12, h, w, h);

		w -= 24;

		float nyquist = c()->rate() / 2;
		GraphParameters<float> freqMinor(make_pair(0.f, nyquist), w / 48, ForceMinor);
		p.setPen(QColor(236, 236, 236));
		for (float f = freqMinor.from; f < freqMinor.to; f += freqMinor.incr)
			p.drawLine(24 + f / nyquist * w, 0, 24 + f / nyquist * w, h + 4);

		GraphParameters<float> freqMajor(make_pair(0.f, nyquist), w / 48);
		for (float f = freqMajor.from; f < freqMajor.to; f += freqMajor.incr)
		{
			int x = 24 + f / nyquist * w;
			p.setPen(QColor(208, 208, 208));
			p.drawLine(x, 0, x, h + 4);
			p.setPen(QColor(144, 144, 144));
			p.drawText(QRect(x - 40, h + 2, 80, 16), Qt::AlignHCenter|Qt::AlignBottom, f ? QString::number(f) + ((f == freqMajor.from + freqMajor.incr) ? "Hz" : "") : "DC");
		}


		p.setPen(QColor(192, 192, 192));
		unsigned rate = c()->rate();
		unsigned incr = rate / c()->hopSamples();
		for (unsigned r = 0; r < rate; r += incr)
		{
			p.drawLine(24 + r * w / nyquist, h, 24 + (r + incr / 2) * w / nyquist, h / 2);
			p.drawLine(24 + (r + incr) * w / nyquist, h, 24 + (r + incr / 2) * w / nyquist, h / 2);
		}

		int lx = 0;
		int ly = 0;
		int ldsy = 0;
		int lddsy = 0;
		float pdRms = 0.f;
		for (int i = 0; i < (int)s; ++i)
		{
			float dp = Lightbox::withReflection(fabs(phase[i] - lPhase[i]));		// delta-phase
			float ddp = Lightbox::withReflection(fabs(phase[i] + llPhase[i] - 2 * lPhase[i]));		// delta-delta-phase

			int x = 24 + i * w / s;
			int y = h - h * dp / sc;

			int phaseChangeSamples = (2 * s / c()->hopSamples());
			float standingWavePhaseChange = float(i % phaseChangeSamples) * twoPi<float>() / phaseChangeSamples;
			if (standingWavePhaseChange > pi<float>())
				standingWavePhaseChange = twoPi<float>() - standingWavePhaseChange;	// cyclic reflection

			float changeOverStandingWaveSqr = sqr((dp - standingWavePhaseChange) / pi<float>());
			pdRms += changeOverStandingWaveSqr;
			int dsy = h * changeOverStandingWaveSqr / sc;
			int ddsy = h * ddp / pi<float>() / sc;
			if (i)
			{
				p.setPen(Qt::NoPen);
				p.setBrush(QColor::fromHsvF(0, 0.25f, 1.f));
				p.drawPolygon(QPolygon(QVector<QPoint>() << QPoint(x, 0) << QPoint(x, dsy) << QPoint(lx, ldsy) << QPoint(lx, 0)));
				p.setBrush(QColor::fromHsvF(0.66f, 0.25f, 1.f));
				p.drawPolygon(QPolygon(QVector<QPoint>() << QPoint(x, 0) << QPoint(x, ddsy) << QPoint(lx, lddsy) << QPoint(lx, 0)));
				p.setPen(QColor::fromHsvF(0, 0.5f, 1.f));
				p.drawLine(QLine(lx, ldsy, x, dsy));
				p.setPen(QColor::fromHsvF(0.66f, 0.5f, 1.f));
				p.drawLine(QLine(lx, lddsy, x, ddsy));
				p.setPen(QColor::fromHsvF(0, 0, 0.5f));
				p.drawLine(QLine(lx, ly, x, y));
			}

			lx = x;
			ly = y;
			ldsy = dsy;
			lddsy = ddsy;
		}
		pdRms /= s;
		p.setPen(QColor::fromHsvF(0, 0.5f, 1.f));
		p.drawLine(QLine(24, h * pdRms / sc, w, h * pdRms / sc));
	}
}

