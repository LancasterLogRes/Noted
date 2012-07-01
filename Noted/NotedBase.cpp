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

#include "Page.h"
#include "NotedBase.h"

using namespace std;
using namespace Lightbox;

NotedBase::NotedBase(QWidget* _p):
	NotedFace					(_p),
	m_wave						("wave"),
	m_blockSamples				(128),
	m_pageBlocks				(128),
	m_spectra					("spectrum"),
	m_pageSpectra				(16)	// spectra are ~512 * 4 = 2K, so this would be 32K per file; 16 -> 256 -> 4096 -> 65536
{
}

NotedBase::~NotedBase()
{
}

uint32_t NotedBase::calculateWaveFingerprint() const
{
	return uint32_t(qHash(m_audioFile.fileName())) ^ (m_normalize ? 0 : 42) ^ m_blockSamples;
}

uint32_t NotedBase::calculateSpectraFingerprint(uint32_t _base) const
{
	uint32_t ret = _base;
	ret ^= (uint32_t&)m_windowFunction[m_windowFunction.size() * 7 / 13];
	if (!m_zeroPhase)
		ret *= 69;
	ret ^= m_hopSamples << 8;
	ret ^= m_windowSizeSamples << 16;
	return ret;
}

bool NotedBase::waveBlock(Time _from, Time _duration, Lightbox::foreign_vector<float> o_toFill, bool) const
{
	QMutexLocker l(&x_wave);
	IndexLevel il(-1, 0);
	unsigned samplesRequired = o_toFill.count();
	int pageBlocksRaisedLevel = 1;
	int samplesPerPage = m_pageBlocks * m_blockSamples;

	for (unsigned samples = fromBase(_duration, m_rate); samples / m_pageBlocks * 2 > samplesRequired; )
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

		if (si < 0 || si >= (int)m_samples)	// TODO: optimize through memset zero and loop cropping.
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

			o_toFill[i] = data[pageOffset];
			bool lastPositive = i ? o_toFill[i - 1] > 0 : true;

			if (pageOffset > lPageOffset + 1 && lPageOffset != -1)
				for (int j = lPageOffset + 1; j < pageOffset; ++j)
					o_toFill[i] = (fabs(data[j]) + (lastPositive == data[j] < 0 ? .15f : 0.f) > fabs(o_toFill[i])) ? data[j] : o_toFill[i];
			lPageOffset = pageOffset;
		}
	}
	return il.level > 0;
}

Lightbox::foreign_vector<float> NotedBase::waveWindow(int _window) const
{
	QMutexLocker l(&x_wave);

	// 0th window begins at (1 - hopsPerWindow) hops; all negative samples are 0 values.
	int hopsPerWindow = m_windowSizeSamples / m_hopSamples;
	int hop = _window + 1 - hopsPerWindow;
	if (hop < 0)
	{
		shared_ptr<vector<float> > data = make_shared<vector<float> >(m_windowSizeSamples, 0.f);
		auto i = m_wave.item(0, 0, true, 0, m_windowSizeSamples + hop * m_hopSamples);
		memcpy(data->data() + m_windowSizeSamples - i.count(), i.data(), i.count() * sizeof(float));
		return foreign_vector<float>(data->data(), m_windowSizeSamples).tied(data);
	}
	else if ((hop * m_hopSamples) / (m_pageBlocks * m_blockSamples) == (hop * m_hopSamples + m_windowSizeSamples - 1) / (m_pageBlocks * m_blockSamples))
		// same page - just return
		return m_wave.item(hop * m_hopSamples / m_blockSamples, 0, true, hop * m_hopSamples % m_blockSamples, m_windowSizeSamples);
	else
	{
		// differing pages - need to create vector<float> and copy.
		shared_ptr<vector<float> > data = make_shared<vector<float> >(m_windowSizeSamples);
		int length1 = m_pageBlocks * m_blockSamples - hop * m_hopSamples % (m_pageBlocks * m_blockSamples);
		auto i1 = m_wave.item(hop * m_hopSamples / m_blockSamples, 0, true, hop * m_hopSamples % m_blockSamples, length1);
		memcpy(data->data(), i1.data(), length1 * sizeof(float));
		auto i2 = m_wave.item((hop * m_hopSamples / (m_blockSamples * m_pageBlocks) + 1) * m_pageBlocks, 0, true, 0, m_windowSizeSamples - length1);
		if (i2)
			memcpy(data->data() + length1, i2.data(), (m_windowSizeSamples - length1) * sizeof(float));
		return foreign_vector<float>(data->data(), m_windowSizeSamples).tied(data);
	}
}

bool NotedBase::resampleWave(std::function<bool(int)> const& _carryOn)
{
	m_audioHeader = reinterpret_cast<WavHeader const*>(m_audioFile.map(0, sizeof(WavHeader)));
	if (m_audioHeader && m_audioHeader->verify() && m_audioHeader->bitsPerSample == 16)	// can't handle anything other than 16 bit wavs for now.
	{
		unsigned dataBytes = m_audioHeader->dataBytes;
		if (dataBytes > m_audioFile.size() - sizeof(WavHeader))
		{
			qDebug() << "BAD Wav file header. Recovering, though the data may be corrupted.";
			dataBytes = m_audioFile.size() - sizeof(WavHeader);
		}

		unsigned inFrames = dataBytes / m_audioHeader->bytesPerFrame;
		Time inPeriod = toBase(1, m_audioHeader->rate);
		m_samples = fromBase(toBase(inFrames, m_audioHeader->rate), m_rate);
		unsigned outBlocks = (m_samples + m_blockSamples - 1) / m_blockSamples;

		m_audioData = m_audioFile.map(sizeof(WavHeader), dataBytes);
		auto sampleAt = [=](unsigned _i) -> int16_t { return (_i < dataBytes) ? *(int16_t const*)(m_audioData + _i) : 0; };

//		float rpb4[4] = { 1.f / m_pageBlocks, 1.f / m_pageBlocks, 1.f / m_pageBlocks, 1.f / m_pageBlocks };

		//TODO: only do RMS for first level - for later levels, just do mean. OR....
		//TODO: only to max for first level - for later levels, just do mean.
		assert(m_pageBlocks == m_blockSamples);
		auto accF = [&](float* current, float* acc, unsigned _n)
		{
			acc[_n] = packEvaluate(current, m_blockSamples, [](v4sf& acc, v4sf const& in){ acc = __builtin_ia32_maxps(acc, __builtin_ia32_maxps(in, -in)); }, [](float const* acc){return max(max(acc[0], acc[1]), max(acc[2], acc[3]));});
		};
		auto distillF = [&](float*)
		{
//			packTransform(acc, m_blockSamples, [&](v4sf& a){});
		};

		pair<int16_t, int16_t> r = range((int16_t*)m_audioData, (int16_t*)(m_audioData + inFrames * m_audioHeader->bytesPerFrame));
		float factor = m_normalize ? 1 / max<float>(abs(r.first), abs(r.second)) : (1 / 32768.f);

		QMutexLocker l(&x_wave);
		m_wave.init(calculateWaveFingerprint(), m_pageBlocks, m_blockSamples);
		if (m_audioHeader->rate == m_rate)
		{
			// Just copy across...
			auto baseF = [&](unsigned blockIndex, float* page)
			{
				for (unsigned i = 0; i < m_blockSamples; ++i)
					page[i] = sampleAt((i + blockIndex * m_blockSamples) * m_audioHeader->bytesPerFrame) * factor;
			};
			m_wave.fill(baseF, accF, distillF, [](){}, _carryOn, outBlocks);
		}
		else
		{
			// Needs a resample
			auto baseF = [&](unsigned blockIndex, float* page)
			{
				for (unsigned i = 0; i < m_blockSamples; ++i)
				{
					unsigned frame = i + blockIndex * m_blockSamples;
					unsigned j = fromBase(toBase(frame, m_rate), m_audioHeader->rate);
					unsigned x = float(toBase(frame, m_rate) - toBase(j, m_audioHeader->rate)) / inPeriod;
					page[i] = lerp(x, *(int16_t*)(m_audioData + j * m_audioHeader->bytesPerFrame), sampleAt((j + 1) * m_audioHeader->bytesPerFrame)) * factor;
				}
			};
			m_wave.fill(baseF, accF, distillF, [](){}, _carryOn, outBlocks);
		}
	}
	else
		return false;
	return true;
}

void NotedBase::rejigSpectra(std::function<bool(int)> const& _carryOn)
{
	QMutexLocker l(&x_spectra);
	if (samples())
	{
		m_spectra.init(calculateSpectraFingerprint(m_wave.fingerprint()), m_pageSpectra, spectrumSize() * 3);

		FFTW fftw(m_windowSizeSamples);
		vector<float> lp(spectrumSize(), 0);

		float p4[4] = { (float)m_pageSpectra, (float)m_pageSpectra, (float)m_pageSpectra, (float)m_pageSpectra };
		int ss = spectrumSize();
		int ss2 = spectrumSize() * 2;
		int ss3 = spectrumSize() * 3;

		auto baseF = [&](unsigned index, float* sd)
		{
			float* b = fftw.in();
			foreign_vector<float> win = waveWindow(index);
			float* d = win.data();
			float* w = m_windowFunction.data();
			unsigned off = m_zeroPhase ? m_windowSizeSamples / 2 : 0;
			for (unsigned j = 0; j < m_windowSizeSamples; ++d, ++j, ++w)
				b[(j + off) % m_windowSizeSamples] = *d * *w;
			fftw.process();

			packTransform(&(lp[0]), &(fftw.phase()[0]), ss, [&](v4sf& a, v4sf const& b){ a = b - a; });

			memcpy(sd, &(fftw.mag()[0]), ss * sizeof(float));
			memcpy(sd + ss, &(fftw.phase()[0]), ss * sizeof(float));
			memcpy(sd + ss2, &(lp[0]), ss * sizeof(float));
		};
		auto sumF = [&](float* current, float* acc, unsigned)
		{
			packTransform(acc, current, ss3, [](v4sf& a, v4sf const& b){ a = a + b; });
		};
		auto divideF = [&](float* sd)
		{
			packTransform(sd, ss3, [&](v4sf& a){ a /= *(v4sf*)p4; });
		};
		auto doneF = [&]() { lp = fftw.phase(); };

		m_spectra.fill(baseF, sumF, divideF, doneF, _carryOn, hops());
	}
	else
	{
		m_spectra.fillFromExisting();
	}
}
