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
#include <Common/Maths.h>
#include <NotedPlugin/NotedFace.h>

#include "SpectraView.h"

using namespace std;
using namespace Lightbox;

Lightbox::Time SpectraView::period() const { return c()->windowSize(); }

void SpectraView::doRender(QGLFramebufferObject* _fbo, int _dx, int _dw)
{
//	int w = width();
//	int h = _fbo->height();
//	QPainter p(_fbo);
	// OPTIMIZE: !!!
	unsigned bc = c()->spectrumSize();
	unsigned s = c()->hops();
	NotedFace* br = dynamic_cast<NotedFace*>(c());

	_fbo->bind();
	glEnable(GL_TEXTURE_1D);
	glPushMatrix();
	glLoadIdentity();
	glScalef(1, _fbo->height(), 1);
	glColor3f(1, 1, 1);
	if (s && bc > 2)
	{
		for (int x = _dx; x < _dx + _dw; ++x)
		{
			int fi = renderingTimeOf(x) > 0 ? renderingTimeOf(x) / c()->hop() : -1;
			int ti = qMax<int>(fi + 1, renderingTimeOf(x + 1) / c()->hop());
//			float di = float(timeOf(x + 1) - timeOf(x)) / c()->hop();
			if (fi >= 0 && ti < (int)s)
			{
				auto mus = br->multiSpectrum(fi, ti - fi);
				if (!m_texture[0])
				{
					glGenTextures(1, m_texture);
					glBindTexture(GL_TEXTURE_1D, m_texture[0]);
					glTexParameteri (GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri (GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					float bias = 1.f;
					float scale = -1.f;
					glPixelTransferf(GL_RED_SCALE, scale);
					glPixelTransferf(GL_GREEN_SCALE, scale);
					glPixelTransferf(GL_BLUE_SCALE, scale);
					glPixelTransferf(GL_RED_BIAS, bias);
					glPixelTransferf(GL_GREEN_BIAS, bias);
					glPixelTransferf(GL_BLUE_BIAS, bias);
				}
				else
					glBindTexture(GL_TEXTURE_1D, m_texture[0]);
				glTexImage1D(GL_TEXTURE_1D, 0, 1, bc * 3, 0, GL_LUMINANCE, GL_FLOAT, mus.data());
				glBegin(GL_TRIANGLE_STRIP);
				glTexCoord1f(1.f / 3);
				glVertex3i(x, 0, 0);
				glTexCoord1f(1.f / 3);
				glVertex3i(x + 1, 0, 0);
				glTexCoord1f(0.f);
				glVertex3i(x, 1, 0);
				glTexCoord1f(0.f);
				glVertex3i(x + 1, 1, 0);
				glEnd();
#if 0
				auto ms = br->magSpectrum(fi, ti - fi);
				auto dps = br->deltaPhaseSpectrum(fi, 1);

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
						if (mmp < 0.f || mmp > Pi * 2)
							qDebug() << "Gaa! Phase out of range! (" << mmp << ")";
						if (di <= 2)
							p.fillRect(QRect(x, y, 1, 1), QColor::fromHsvF(qMax(0.f, qMin(1.f, mmp / float(Pi * 2))), qMax(0.f, qMin(1.f, 1.f + log(mm + 0.0001f) / 4.f)), 1.f).rgb());
						else if (di >= 4)
							p.fillRect(QRect(x, y, 1, 1), QColor::fromHsvF(0, 0, 1.f - qMax(0.f, qMin(1.f, sqrt(mm)))).rgb());
						else
						{
							QRgb c1 = QColor::fromHsvF(qMax(0.f, qMin(1.f, mmp / float(Pi * 2))), qMax(0.f, qMin(1.f, 1.f + log(mm + 0.0001f) / 4.f)), 1.f).rgb();
							QRgb c2 = QColor::fromHsvF(0, 0, 1.f - qMax(0.f, qMin(1.f, sqrt(mm)))).rgb();
							float f = (di - 2) / 2;
							p.fillRect(QRect(x, y, 1, 1), qRgb(lerp(f, qRed(c1), qRed(c2)), lerp(f, qGreen(c1), qGreen(c2)), lerp(f, qBlue(c1), qBlue(c2))));
						}
#endif
//						float dp = Lightbox::withReflection(fabs(mmdp));
//						int phaseChanges = c()->hopSamples() / 2;//c()->windowSize() / c()->hop(); // number of times the phase changes 0->TwoPi over the spectrum.
//						int samplesPerPhaseCycle = bc / phaseChanges;

//						float standingWavePhaseChange = float(mmj % phaseChanges) * 2 * Pi / phaseChangeSamples;
//						if (standingWavePhaseChange > Pi)
//							standingWavePhaseChange = 2 * Pi - standingWavePhaseChange;	// cyclic reflection
//						standingWavePhaseChange = 0;
//						float expectedUnitChange = 1.f - (mmj % samplesPerPhaseCycle) / float(samplesPerPhaseCycle);
						float unitChange = (mmdp + TwoPi) / TwoPi;//fabs(( - standingWavePhaseChange) / Pi);
						if (unitChange > 1.f)
							unitChange -= 1.f;
//						float deviation = 1.f - sqr(sqr(1.f - withReflection(fabs(unitChange - expectedUnitChange), 1.f)));

						p.fillRect(QRect(x, y, 1, 1), QColor::fromHsvF(qMax(0.f, qMin(1.f, unitChange)), qMax(0.f, qMin(1.f, 1.f + log(mm + 0.0001f) / 4.f)), 1.f).rgb());
					}
				}
#endif
			}
		}
	}
	glPopMatrix();
	_fbo->release();
}
