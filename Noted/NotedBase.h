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

#pragma once

#include <memory>
#include <functional>

#include <QMultiMap>
#include <QHash>
#include <QFile>
#include <QRect>
#include <QMutex>

#include <Common/Common.h>
#include <NotedPlugin/NotedFace.h>
#include "Pager.h"

class Timeline;
class QEvent;
class QPainter;
class OutputItem;
class EventsView;
class Noted;
class NotedBase;

struct SNDFILE_tag;
typedef struct SNDFILE_tag SNDFILE;

static const std::vector<float> NullVectorFloat;

class NotedBase: public NotedFace
{
	Q_OBJECT

public:
	explicit NotedBase(QWidget* _p);
	~NotedBase();

	virtual Lightbox::foreign_vector<float> waveWindow(int _window) const;
	virtual bool waveBlock(Lightbox::Time _from, Lightbox::Time _duration, Lightbox::foreign_vector<float> o_toFill) const;
	virtual Lightbox::foreign_vector<float> magSpectrum(int _i, int _n) const { QMutexLocker l(&x_spectra); return m_spectra.item(_i, _n, 0, spectrumSize()); }
	virtual Lightbox::foreign_vector<float> phaseSpectrum(int _i, int _n) const { QMutexLocker l(&x_spectra); return m_spectra.item(_i, _n, spectrumSize(), spectrumSize()); }
	virtual Lightbox::foreign_vector<float> deltaPhaseSpectrum(int _i, int _n) const { QMutexLocker l(&x_spectra); return m_spectra.item(_i, _n, spectrumSize() * 2, spectrumSize()); }

protected:
	bool resampleWave(std::function<bool(int)> const& _carryOn);
	void rejigSpectra(std::function<bool(int)> const& _carryOn);

	uint32_t calculateWaveFingerprint() const;
	uint32_t calculateSpectraFingerprint(uint32_t _base) const;

	QFile m_audioFile;
	SNDFILE* m_sndfile;
	uint8_t const* m_audioData;
	Lightbox::WavHeader const* m_audioHeader;

	mutable QMutex x_wave;
	Pager<float> m_wave;
	unsigned m_blockSamples;
	unsigned m_pageBlocks;

	mutable QMutex x_spectra;
	Pager<float> m_spectra;
	unsigned m_pageSpectra;
};
