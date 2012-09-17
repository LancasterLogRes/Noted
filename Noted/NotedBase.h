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
#include "Cache.h"

class Timeline;
class QEvent;
class QPainter;
class OutputItem;
class EventsView;
class Noted;
class NotedBase;

struct SNDFILE_tag;
typedef struct SNDFILE_tag SNDFILE;

class NotedBase: public NotedFace
{
	Q_OBJECT

public:
	explicit NotedBase(QWidget* _p);
	~NotedBase();

	virtual Lightbox::foreign_vector<float const> waveWindow(int _window) const;
	virtual bool waveBlock(Lightbox::Time _from, Lightbox::Time _duration, Lightbox::foreign_vector<float> o_toFill, bool _forceSamples = false) const;
	virtual Lightbox::foreign_vector<float const> multiSpectrum(int _i, int _n) const { QMutexLocker l(&x_spectra); return m_spectra.items<float>(_i, _n); }
	virtual Lightbox::foreign_vector<float const> magSpectrum(int _i, int _n) const { QMutexLocker l(&x_spectra); return m_spectra.items<float>(_i, _n).cropped(0, spectrumSize()); }
	virtual Lightbox::foreign_vector<float const> phaseSpectrum(int _i, int _n) const { QMutexLocker l(&x_spectra); return m_spectra.items<float>(_i, _n).cropped(spectrumSize(), spectrumSize()); }
	virtual Lightbox::foreign_vector<float const> deltaPhaseSpectrum(int _i, int _n) const { QMutexLocker l(&x_spectra); return m_spectra.items<float>(_i, _n).cropped(spectrumSize() * 2, spectrumSize()); }

protected:
	bool resampleWave();
	void rejigSpectra();

	uint32_t calculateWaveFingerprint() const;
	uint32_t calculateSpectraFingerprint(uint32_t _base) const;

	QString m_sourceFileName;

	mutable QMutex x_wave;
	Cache m_wave;
	mutable QMutex x_waveProfile;
	MipmappedCache m_waveProfile;
	mutable QMutex x_spectra;
	MipmappedCache m_spectra;
};
