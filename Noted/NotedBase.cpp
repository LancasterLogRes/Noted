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

#include "Page.h"
#include "NotedBase.h"

using namespace std;
using namespace Lightbox;

NotedBase::NotedBase(QWidget* _p):
	NotedFace					(_p),
	m_wave						("wave"),
	m_blockSamples				(128),
	m_pageBlocks				(128)
{
}

NotedBase::~NotedBase()
{
}

uint32_t NotedBase::calculateWaveFingerprint() const
{
	return uint32_t(qHash(m_sourceFileName)) ^ m_rate ^ m_blockSamples;
}

uint32_t NotedBase::calculateSpectraFingerprint(uint32_t _base) const
{
	uint32_t ret = _base;
	ret ^= (uint32_t&)m_windowFunction[m_windowFunction.size() * 7 / 13];
	if (!m_zeroPhase)
		ret *= 69;
	ret ^= m_hopSamples << 8;
	ret ^= m_windowFunction.size() << 16;
	return ret;
}

bool NotedBase::waveBlock(Time _from, Time _duration, Lightbox::foreign_vector<float> o_toFill) const
{
	QMutexLocker l(&x_wave);
	IndexLevel il(-1, 0);
	unsigned samplesRequired = o_toFill.count();
	int pageBlocksRaisedLevel = 1;
	int samplesPerPage = m_pageBlocks * m_blockSamples;

	for (unsigned samples = fromBase(_duration, m_rate); samples / m_pageBlocks * 1.2 > samplesRequired; )
	{
		samples /= m_pageBlocks;
		++il.level;
		pageBlocksRaisedLevel *= m_pageBlocks;
	}
	// il.level is the best sampling level.

	PagePtr page;
	foreign_vector<float> data;
	int fromSample = fromBase(_from, m_rate);
	int samples = fromBase(_duration, m_rate);
	int pageBlocksRaisedLevelTimesSamplesPerPage = pageBlocksRaisedLevel * samplesPerPage;
	int lPageOffset = -1;
	for (int i = 0; i < (int)o_toFill.count(); ++i)
	{
		// global sample index...
		int si = fromSample + i * samples / o_toFill.count();

		if (si < 0 || si >= (int)m_samples)	// OPTIMIZE: by memset zero and loop cropping.
			o_toFill[i] = 0;
		else
		{
			// calculate required il.index and page offset from that.
			int pageIndex = si / pageBlocksRaisedLevelTimesSamplesPerPage;
			int pageOffset = si / pageBlocksRaisedLevel % samplesPerPage;

			if (il != IndexLevel(pageIndex, il.level))
			{
				il.index = pageIndex;
				page = m_wave.page(il);
				data = page->data<float>();
			}

			if (data)
			{
				o_toFill[i] = data[pageOffset];
				bool lastPositive = i ? o_toFill[i - 1] > 0 : true;

				if (pageOffset > lPageOffset + 1 && lPageOffset != -1)
					for (int j = lPageOffset + 1; j < pageOffset; ++j)
						o_toFill[i] = (fabs(data[j]) + (lastPositive == data[j] < 0 ? .15f : 0.f) > fabs(o_toFill[i])) ? data[j] : o_toFill[i];
				lPageOffset = pageOffset;
			}
		}
	}
	return il.level > 0;
}

Lightbox::foreign_vector<float> NotedBase::waveWindow(int _window) const
{
	QMutexLocker l(&x_wave);

	// 0th window begins at (1 - hopsPerWindow) hops; all negative samples are 0 values.
	int hopsPerWindow = m_windowFunction.size() / m_hopSamples;
	int hop = _window + 1 - hopsPerWindow;
	if (hop < 0)
	{
		shared_ptr<vector<float> > data = make_shared<vector<float> >(m_windowFunction.size(), 0.f);
		auto i = m_wave.item(0, 0, 0, m_windowFunction.size() + hop * m_hopSamples);
		valcpy(data->data() + m_windowFunction.size() - i.count(), i.data(), i.count());
		return foreign_vector<float>(data->data(), m_windowFunction.size()).tied(data);
	}
	else if ((hop * m_hopSamples) / (m_pageBlocks * m_blockSamples) == (hop * m_hopSamples + m_windowFunction.size() - 1) / (m_pageBlocks * m_blockSamples))
		// same page - just return
		return m_wave.item(hop * m_hopSamples / m_blockSamples, 0, hop * m_hopSamples % m_blockSamples, m_windowFunction.size());
	else
	{
		// differing pages - need to create vector<float> and copy.
		shared_ptr<vector<float> > data = make_shared<vector<float> >(m_windowFunction.size());
		int length1 = m_pageBlocks * m_blockSamples - hop * m_hopSamples % (m_pageBlocks * m_blockSamples);
		auto i1 = m_wave.item(hop * m_hopSamples / m_blockSamples, 0, hop * m_hopSamples % m_blockSamples, length1);
		valcpy(data->data(), i1.data(), length1);
		auto i2 = m_wave.item((hop * m_hopSamples / (m_blockSamples * m_pageBlocks) + 1) * m_pageBlocks, 0, 0, m_windowFunction.size() - length1);
		if (i2)
			valcpy(data->data() + length1, i2.data(), m_windowFunction.size() - length1);
		return foreign_vector<float>(data->data(), m_windowFunction.size()).tied(data);
	}
}

bool NotedBase::resampleWave()
{
	SF_INFO info;
	m_sndfile = sf_open(m_sourceFileName.toLocal8Bit().data(), SFM_READ, &info);
	if (m_sndfile)
	{
		QMutexLocker l(&x_wave);
		m_wave.init(calculateWaveFingerprint(), m_pageBlocks, m_blockSamples);

		sf_seek(m_sndfile, 0, SEEK_SET);
		float buffer[m_blockSamples * info.channels];

		// COULDDO: could only do RMS for first level - for later levels, just do mean
		// ...or only to max for first level - for later levels, just do mean.
		assert(m_pageBlocks == m_blockSamples);
		auto accF = [&](float* current, float* acc, unsigned _n)
		{
			acc[_n] = packEvaluate(current, m_blockSamples, [](v4sf& acc, v4sf const& in){ acc = __builtin_ia32_maxps(acc, __builtin_ia32_maxps(in, -in)); }, [](float const* acc){return max(max(acc[0], acc[1]), max(acc[2], acc[3]));});
		};
		auto distillF = [&](float*)
		{
//			packTransform(acc, m_blockSamples, [&](v4sf& a){});
		};

		if (info.samplerate == (int)m_rate)
		{
			// Just copy across...
			m_samples = info.frames;
			unsigned outBlocks = (m_samples + m_blockSamples - 1) / m_blockSamples;
			auto baseF = [&](unsigned blockIndex, float* page)
			{
				(void)blockIndex;
				unsigned rc = sf_readf_float(m_sndfile, buffer, m_blockSamples);
				valcpy<float>(page, buffer, rc, 1, info.channels);	// just take the channel 0.
				memset(page + rc, 0, sizeof(float) * (m_blockSamples - rc));	// zeroify what's left.
			};
			m_wave.fill(baseF, accF, distillF, [](){}, outBlocks);
		}
		else
		{
			// Needs a resample
			double factor = double(m_rate) / info.samplerate;
			void* resampler = resample_open(1, factor, factor);
			m_samples = fromBase(toBase(info.frames, info.samplerate), m_rate);
			unsigned outBlocks = (m_samples + m_blockSamples - 1) / m_blockSamples;

			unsigned bufferPos = m_blockSamples;

			auto baseF = [&](unsigned blockIndex, float* page)
			{
				unsigned pagePos = 0;
				while (pagePos != m_blockSamples)
				{
					if (bufferPos == m_blockSamples)
					{
						// At end of current (input) buffer - refill and reset position.
						int rc = sf_readf_float(m_sndfile, buffer, m_blockSamples);
						if (rc < 0)
							rc = 0;
						valcpy<float>(buffer, buffer, rc, 1, info.channels);	// just take the channel 0.
						memset(buffer + rc, 0, sizeof(float) * (m_blockSamples - rc));	// zeroify what's left.
						bufferPos = 0;
					}
					int used = 0;
					pagePos += resample_process(resampler, factor, buffer + bufferPos, m_blockSamples - bufferPos, blockIndex == outBlocks - 1, &used, page + pagePos, m_blockSamples - pagePos);
					bufferPos += used;
				}
				for (unsigned i = 0; i < m_blockSamples; ++i)
					page[i] = clamp(page[i], -1.f, 1.f);
			};
			m_wave.fill(baseF, accF, distillF, [](){}, outBlocks);
			resample_close(resampler);
		}
		sf_close(m_sndfile);
	}
	else
		return false;
	return true;
}

void setVector(v4sf& _v, float _f)
{
	float* f = (float*)&_v;
	f[0] = _f;
	f[1] = _f;
	f[2] = _f;
	f[3] = _f;
}

void NotedBase::rejigSpectra()
{
	QMutexLocker l(&x_spectra);
	int ss = spectrumSize();
	if (samples())
	{
		if (!m_spectra.init(calculateSpectraFingerprint(m_wave.fingerprint()), "spectrum", ss * 3 * sizeof(float), hops()))
		{
			FFTW fftw(m_windowFunction.size());
			vector<float> lastPhase(spectrumSize(), 0);
			for (unsigned index = 0; index < hops(); ++index)
			{
				float* b = fftw.in();
				foreign_vector<float> win = waveWindow(index);
				assert(win.data());
				float* d = win.data();
				float* w = m_windowFunction.data();
				unsigned off = m_zeroPhase ? m_windowFunction.size() / 2 : 0;
				for (unsigned j = 0; j < m_windowFunction.size(); ++d, ++j, ++w)
					b[(j + off) % m_windowFunction.size()] = *d * *w;
				fftw.process();

				auto sd = m_spectra.item<float>(0, index);
				valcpy(sd.data(), fftw.mag().data(), ss);
				float const* phase = fftw.phase().data();
				float intpart;
				for (int i = 0; i < ss; ++i)
				{
					sd[i + ss] = phase[i] / TwoPi;
					sd[i + ss*2] = modf((phase[i] - lastPhase[i]) / TwoPi + 1.f, &intpart);
				}
				lastPhase = fftw.phase();
			}
			m_spectra.generate([=](Lightbox::foreign_vector<float> a, Lightbox::foreign_vector<float> b, Lightbox::foreign_vector<float> ret)
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
		m_spectra.init(calculateSpectraFingerprint(m_wave.fingerprint()), "spectrum", ss * 3, 0);
	}
}
