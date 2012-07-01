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
#include <Common/Maths.h>
#include <NotedPlugin/NotedFace.h>

#include "SpectraView.h"

using namespace std;
using namespace Lightbox;

Lightbox::Time SpectraView::period() const { return c()->windowSize(); }

void SpectraView::doRender(QImage& _img, int _dx, int _dw)
{
//	int w = width();
	int h = height();
	unsigned bc = c()->spectrumSize();
	unsigned s = c()->hops();
	NotedFace* br = dynamic_cast<NotedFace*>(c());
	if (s && bc > 2)
	{
		for (int x = _dx; x < _dx + _dw; ++x)
		{
			int fi = timeOf(x) > 0 ? timeOf(x) / c()->hop() : -1;
			int ti = qMax<int>(fi + 1, timeOf(x + 1) / c()->hop());
//			float di = float(timeOf(x + 1) - timeOf(x)) / c()->hop();
			if (fi >= 0 && ti < (int)s)
			{
				auto ms = br->magSpectrum(fi, ti - fi, true);
				auto dps = br->deltaPhaseSpectrum(fi, 1, true);

				if (ms && dps)
				{
					for (int y = 0; y < h; ++y)
					{
						unsigned yi = y * bc / h;
						unsigned nyi = qMax<unsigned>(yi + 1, (y + 1) * bc / h);

//						unsigned mmj = 0;
						float mm = 0.f;
//						float mmp = 0.f;
						float mmdp = 0.f;
						for (unsigned j = yi; j < nyi; ++j)
							if (ms[j] > mm)
							{
								mm = ms[j];
//								mmp = ps[j];
								mmdp = dps[j];
//								mmj = j;
							}
#if 0
						if (mmp < 0.f || mmp > M_PI * 2)
							qDebug() << "Gaa! Phase out of range! (" << mmp << ")";
						if (di <= 2)
							_img.setPixel(x, y, QColor::fromHsvF(qMax(0.f, qMin(1.f, mmp / float(M_PI * 2))), qMax(0.f, qMin(1.f, 1.f + log(mm + 0.0001f) / 4.f)), 1.f).rgb());
						else if (di >= 4)
							_img.setPixel(x, y, QColor::fromHsvF(0, 0, 1.f - qMax(0.f, qMin(1.f, sqrt(mm)))).rgb());
						else
						{
							QRgb c1 = QColor::fromHsvF(qMax(0.f, qMin(1.f, mmp / float(M_PI * 2))), qMax(0.f, qMin(1.f, 1.f + log(mm + 0.0001f) / 4.f)), 1.f).rgb();
							QRgb c2 = QColor::fromHsvF(0, 0, 1.f - qMax(0.f, qMin(1.f, sqrt(mm)))).rgb();
							float f = (di - 2) / 2;
							_img.setPixel(x, y, qRgb(lerp(f, qRed(c1), qRed(c2)), lerp(f, qGreen(c1), qGreen(c2)), lerp(f, qBlue(c1), qBlue(c2))));
						}
#endif
//						float dp = Lightbox::withReflection(fabs(mmdp));
//						int phaseChanges = c()->hopSamples() / 2;//c()->windowSize() / c()->hop(); // number of times the phase changes 0->TwoPi over the spectrum.
//						int samplesPerPhaseCycle = bc / phaseChanges;

//						float standingWavePhaseChange = float(mmj % phaseChanges) * 2 * Pi / phaseChangeSamples;
//						if (standingWavePhaseChange > M_PI)
//							standingWavePhaseChange = 2 * Pi - standingWavePhaseChange;	// cyclic reflection
//						standingWavePhaseChange = 0;
//						float expectedUnitChange = 1.f - (mmj % samplesPerPhaseCycle) / float(samplesPerPhaseCycle);
						float unitChange = (mmdp + TwoPi) / TwoPi;//fabs(( - standingWavePhaseChange) / Pi);
						if (unitChange > 1.f)
							unitChange -= 1.f;
//						float deviation = 1.f - sqr(sqr(1.f - withReflection(fabs(unitChange - expectedUnitChange), 1.f)));

						_img.setPixel(x, y, QColor::fromHsvF(qMax(0.f, qMin(1.f, unitChange)), qMax(0.f, qMin(1.f, 1.f + log(mm + 0.0001f) / 4.f)), 1.f).rgb());
					}
				}
			}
		}
	}
}
