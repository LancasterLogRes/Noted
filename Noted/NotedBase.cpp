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

#include <cstdlib>
#include <memory>
#include <unordered_map>
#include <QDebug>
#include <QtGui>
#include <Common/Common.h>
#include "WorkerThread.h"
#include "NotedBase.h"
using namespace std;
using namespace lb;

NotedBase::NotedBase(QWidget* _p):
	NotedFace					(_p),
	x_spectra(QMutex::Recursive)
{
}

NotedBase::~NotedBase()
{
}

uint32_t NotedBase::calculateSpectraFingerprint(uint32_t _base) const
{
	uint32_t ret = _base;
	ret ^= (uint32_t&)m_windowFunction[m_windowFunction.size() * 7 / 13];
	if (!m_zeroPhase)
		ret *= 69;
	if (!m_floatFFT)
		ret *= 42;
	ret ^= m_windowFunction.size() << 16;
	return ret;
}

foreign_vector<float const> NotedBase::multiSpectrum(int _i, int _n) const
{
//	qDebug() << "multiSpectrum";
	QMutexLocker l(&x_spectra);
//	qDebug() << "multiSpectrum L";
	return m_spectra.items<float>(max(0, _i), _n).tied(std::make_shared<QMutexLocker>(&x_spectra));
}

foreign_vector<float const> NotedBase::magSpectrum(int _i, int _n) const
{
//	qDebug() << "magSpectrum";
	QMutexLocker l(&x_spectra);
//	qDebug() << "magSpectrum L";
	return m_spectra.items<float>(max(0, _i), _n).cropped(0, spectrumSize()).tied(std::make_shared<QMutexLocker>(&x_spectra));
}

foreign_vector<float const> NotedBase::phaseSpectrum(int _i, int _n) const
{
//	qDebug() << "phaseSpectrum";
	QMutexLocker l(&x_spectra);
//	qDebug() << "phaseSpectrum L";
	return m_spectra.items<float>(max(0, _i), _n).cropped(spectrumSize(), spectrumSize()).tied(std::make_shared<QMutexLocker>(&x_spectra));
}

foreign_vector<float const> NotedBase::deltaPhaseSpectrum(int _i, int _n) const
{
//	qDebug() << "dpSpectrum";
	QMutexLocker l(&x_spectra);
//	qDebug() << "dpSpectrum L";
	return m_spectra.items<float>(max(0, _i), _n).cropped(spectrumSize() * 2, spectrumSize()).tied(std::make_shared<QMutexLocker>(&x_spectra));
}

void NotedBase::rejigSpectra()
{
//	qDebug() << "rejigSpectra";
	QMutexLocker l(&x_spectra);
//	qDebug() << "rejigSpectra L";
	int ss = spectrumSize();
	if (audio()->samples())
	{
		if (!m_spectra.init(calculateSpectraFingerprint(audio()->key()), "spectrum", ss * 3 * sizeof(float), audio()->hops()))
		{
			unsigned hops = audio()->hops();
			if (m_floatFFT)
			{
				FFT<float> fft(m_windowFunction.size());
				vector<float> lastPhase(spectrumSize(), 0);
				for (unsigned index = 0; index < hops; ++index)
				{
					float* b = fft.in();
					foreign_vector<float const> win = audio()->waveWindow(index);
					assert(win.data());
					float const* d = win.data();
					float* w = m_windowFunction.data();
					unsigned off = m_zeroPhase ? m_windowFunction.size() / 2 : 0;
					for (unsigned j = 0; j < m_windowFunction.size(); ++d, ++j, ++w)
						b[(j + off) % m_windowFunction.size()] = *d * *w;
					fft.process();

					auto sd = m_spectra.item<float>(0, index);
					valcpy(sd.data(), fft.mag().data(), ss);
					float const* phase = fft.phase().data();
					float intpart;
					for (int i = 0; i < ss; ++i)
					{
						sd[i + ss] = phase[i] / twoPi<float>();
						sd[i + ss*2] = modf((phase[i] - lastPhase[i]) / twoPi<float>() + 1.f, &intpart);
					}
					lastPhase = fft.phase();
					WorkerThread::setCurrentProgress(index * 99 / hops);
				}
			}
			else
			{
				FFT<Fixed16> fft(m_windowFunction.size());
				vector<float> lastPhase(spectrumSize(), 0);
				for (unsigned index = 0; index < hops; ++index)
				{
					Fixed<1, 15>* b = fft.in();
					foreign_vector<float const> win = audio()->waveWindow(index);
					assert(win.data());
					float const* d = win.data();
					float* w = m_windowFunction.data();
					unsigned off = m_zeroPhase ? m_windowFunction.size() / 2 : 0;
					for (unsigned j = 0; j < m_windowFunction.size(); ++d, ++j, ++w)
						b[(j + off) % m_windowFunction.size()] = *d * *w;
					fft.process();

					auto sd = m_spectra.item<float>(0, index);
					Fixed16 const* mag = fft.mag().data();
					for (int i = 0; i < ss; ++i)
						sd[i] = (float)mag[i];

					Fixed16 const* phase = fft.phase().data();
					float intpart;
					for (int i = 0; i < ss; ++i)
					{
						sd[i + ss] = (float)phase[i] / twoPi<float>();
						sd[i + ss*2] = modf(((float)phase[i] - lastPhase[i]) / twoPi<float>() + 1.f, &intpart);
						lastPhase[i] = (float)phase[i];
					}
					WorkerThread::setCurrentProgress(index * 99 / hops);
				}
			}
			m_spectra.generate([&](lb::foreign_vector<float> a, lb::foreign_vector<float> b, lb::foreign_vector<float> ret)
			{
				lb::valcpy(ret.data(), a.data(), a.size());
				v4sf half = {.5f, .5f, .5f, .5f};
				lb::packTransform(ret.data(), b.data(), ss, [=](v4sf& rv, v4sf bv)
				{
					rv = (rv + bv) * half;
				});// only do the mean combine with b for the mag spectrum.
			}, 0.f);
		}
	}
	else
	{
		m_spectra.init(calculateSpectraFingerprint(audio()->key()), "spectrum", ss * 3, 0);
	}
}
