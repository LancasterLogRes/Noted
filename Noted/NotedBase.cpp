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
#include <sndfile.h>
#include <libresample.h>
#include <QDebug>
#include <QtGui>
#include <Common/Common.h>
#include "WorkerThread.h"
#include "NotedBase.h"
using namespace std;
using namespace Lightbox;

NotedBase::NotedBase(QWidget* _p):
	NotedFace					(_p),
	x_wave(QMutex::Recursive),
	x_waveProfile(QMutex::Recursive),
	x_spectra(QMutex::Recursive)
{
}

NotedBase::~NotedBase()
{
}

uint32_t NotedBase::calculateWaveFingerprint() const
{
	return uint32_t(qHash(m_sourceFileName)) ^ m_rate;
}

uint32_t NotedBase::calculateSpectraFingerprint(uint32_t _base) const
{
	uint32_t ret = _base;
	ret ^= (uint32_t&)m_windowFunction[m_windowFunction.size() * 7 / 13];
	if (!m_zeroPhase)
		ret *= 69;
	if (!m_floatFFT)
		ret *= 42;
	ret ^= m_hopSamples << 8;
	ret ^= m_windowFunction.size() << 16;
	return ret;
}

// returns true if it's pairs of max/rms, false if it's samples.
bool NotedBase::waveBlock(Time _from, Time _duration, Lightbox::foreign_vector<float> o_toFill, bool _forceSamples) const
{
	int samples = fromBase(_duration, m_rate);
	int items = _forceSamples ? o_toFill.size() : (o_toFill.size() / 2);
	int samplesPerItem = samples / items;
	if (samplesPerItem < (int)m_hopSamples || _forceSamples)
	{
		QMutexLocker l(&x_wave);
		int imin = -_from * items / _duration;
		int imax = (duration() - _from) * items / _duration;
		float const* d = m_wave.data<float>().data() + fromBase(_from, m_rate);
		for (int i = 0; i < items; ++i)
			o_toFill[i] = (i <= imin || i >= imax) ? 0 : d[samples * i / items];
		return false;
	}
	else
	{
		QMutexLocker l(&x_waveProfile);
		int hi = _from / hop();
		int hs = _duration / hop();
		for (int i = 0; i < items; ++i)
			if (hi + i * hs / items >= 0 && hi + i * hs / items < (int)hops())
			{
				auto d = m_waveProfile.items<float>(hi + i * hs / items, hs / items);
				o_toFill[i * 2] = d[0];
				o_toFill[i * 2 + 1] = d[1];
			}
			else
				o_toFill[i * 2] = o_toFill[i * 2 + 1] = 0;
		return true;
	}
}

Lightbox::foreign_vector<float const> NotedBase::waveWindow(int _window) const
{
	QMutexLocker l(&x_wave);

	_window = clamp<int, int>(_window, 0, hops());

	// 0th window begins at (1 - hopsPerWindow) hops; all negative samples are 0 values.
	int hopsPerWindow = m_windowFunction.size() / m_hopSamples;
	int hop = _window + 1 - hopsPerWindow;
	if (hop < 0)
	{
		shared_ptr<vector<float> > data = make_shared<vector<float> >(m_windowFunction.size(), 0.f);
		if (m_windowFunction.size() + hop * m_hopSamples > 0)
		{
			auto i = m_wave.data<float>().cropped(0, m_windowFunction.size() + hop * m_hopSamples);
			valcpy(data->data() + m_windowFunction.size() - i.count(), i.data(), i.count());
		}
		return foreign_vector<float const>(data->data(), m_windowFunction.size()).tied(data);
	}
	else
		// same page - just return
		return m_wave.data<float>().cropped(hop * m_hopSamples, m_windowFunction.size()).tied(std::make_shared<QMutexLocker>(&x_wave));
}

bool NotedBase::resampleWave()
{
	SF_INFO info;
	auto sndfile = sf_open(m_sourceFileName.toLocal8Bit().data(), SFM_READ, &info);
	if (sndfile)
	{
		QMutexLocker l1(&x_waveProfile);
		QMutexLocker l2(&x_wave);
		unsigned outHops = (fromBase(toBase(info.frames, info.samplerate), m_rate) + m_hopSamples - 1) / m_hopSamples;
		m_samples = outHops * m_hopSamples;
		bool waveOk = m_wave.init(calculateWaveFingerprint(), "wave", m_samples * sizeof(float));
		bool waveProfileOk = m_waveProfile.init(calculateWaveFingerprint(), "waveProfile", 2 * sizeof(float), outHops);
		if (!waveOk || !waveProfileOk)
		{
			sf_seek(sndfile, 0, SEEK_SET);
			vector<float> buffer(m_hopSamples * info.channels);

			float* cache = m_wave.data<float>().data();
			float* wave = m_waveProfile.data<float>().data();
			if (info.samplerate == (int)m_rate)
			{
				// Just copy across...
				for (unsigned i = 0; i < outHops; ++i, wave += 2, cache += m_hopSamples)
				{
					unsigned rc = sf_readf_float(sndfile, buffer.data(), m_hopSamples);
					valcpy<float>(cache, buffer.data(), rc, 1, info.channels);	// just take the channel 0.
					memset(cache + rc, 0, sizeof(float) * (m_hopSamples - rc));	// zeroify what's left.
					wave[0] = sigma(buffer);
					auto r = range(buffer);
					wave[1] = max(fabs(r.first), r.second);
					WorkerThread::setCurrentProgress(i * 100 / outHops);
				}
			}
			else
			{
				// Needs a resample
				double factor = double(m_rate) / info.samplerate;
				void* resampler = resample_open(1, factor, factor);
				unsigned bufferPos = m_hopSamples;

				for (unsigned i = 0; i < outHops; ++i, wave += 2, cache += m_hopSamples)
				{
					unsigned pagePos = 0;
					while (pagePos != m_hopSamples)
					{
						if (bufferPos == m_hopSamples)
						{
							// At end of current (input) buffer - refill and reset position.
							int rc = sf_readf_float(sndfile, buffer.data(), m_hopSamples);
							if (rc < 0)
								rc = 0;
							valcpy<float>(buffer.data(), buffer.data(), rc, 1, info.channels);	// just take the channel 0.
							memset(buffer.data() + rc, 0, sizeof(float) * (m_hopSamples - rc));	// zeroify what's left.
							bufferPos = 0;
						}
						int used = 0;
						pagePos += resample_process(resampler, factor, buffer.data() + bufferPos, m_hopSamples - bufferPos, i == outHops - 1, &used, cache + pagePos, m_hopSamples - pagePos);
						bufferPos += used;
					}
					for (unsigned i = 0; i < m_hopSamples; ++i)
						cache[i] = clamp(cache[i], -1.f, 1.f);
					wave[0] = sigma(cache, cache + m_hopSamples, 0.f);
					auto r = range(cache, cache + m_hopSamples);
					wave[1] = max(fabs(r.first), r.second);
					WorkerThread::setCurrentProgress(i * 100 / outHops);
				}
				resample_close(resampler);
			}
			m_wave.setGood();
			m_waveProfile.generate<float>();
		}
		sf_close(sndfile);
	}
	else
	{
		m_wave.init(calculateWaveFingerprint(), "wave", 0);
		m_waveProfile.init(calculateWaveFingerprint(), "waveProfile", 2 * sizeof(float), 0);
		return false;
	}
	return true;
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
	if (samples())
	{
		if (!m_spectra.init(calculateSpectraFingerprint(calculateWaveFingerprint()), "spectrum", ss * 3 * sizeof(float), hops()))
		{
			if (m_floatFFT)
			{
				FFT<float> fft(m_windowFunction.size());
				vector<float> lastPhase(spectrumSize(), 0);
				for (unsigned index = 0; index < hops(); ++index)
				{
					float* b = fft.in();
					foreign_vector<float const> win = waveWindow(index);
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
					WorkerThread::setCurrentProgress(index * 99 / hops());
				}
			}
			else
			{
				FFT<Fixed16> fft(m_windowFunction.size());
				vector<float> lastPhase(spectrumSize(), 0);
				for (unsigned index = 0; index < hops(); ++index)
				{
					Fixed<1, 15>* b = fft.in();
					foreign_vector<float const> win = waveWindow(index);
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
					WorkerThread::setCurrentProgress(index * 99 / hops());
				}
			}
			m_spectra.generate([&](Lightbox::foreign_vector<float> a, Lightbox::foreign_vector<float> b, Lightbox::foreign_vector<float> ret)
			{
				Lightbox::valcpy(ret.data(), a.data(), a.size());
				v4sf half = {.5f, .5f, .5f, .5f};
				Lightbox::packTransform(ret.data(), b.data(), ss, [=](v4sf& rv, v4sf bv)
				{
					rv = (rv + bv) * half;
				});// only do the mean combine with b for the mag spectrum.
			}, 0.f);
		}
	}
	else
	{
		m_spectra.init(calculateSpectraFingerprint(calculateWaveFingerprint()), "spectrum", ss * 3, 0);
	}
}
