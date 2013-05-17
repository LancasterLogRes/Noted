#include <sndfile.h>
#include <libresample.h>
#include <Common/Global.h>
#include "Global.h"
#include "Noted.h"
#include "AudioMan.h"
using namespace std;
using namespace lb;

class ResampleWaveAc: public AcausalAnalysis
{
public:
	ResampleWaveAc(): AcausalAnalysis("Resampling wave") {}
	virtual void analyze(unsigned, unsigned, lb::Time) { Noted::audio()->resampleWave(); }
};

AudioMan::AudioMan():
	x_wave			(QMutex::Recursive),
	x_waveProfile	(QMutex::Recursive)
{
	m_resampleWaveAcAnalysis = AcausalAnalysisPtr(new ResampleWaveAc);
	connect(Noted::compute(), SIGNAL(analyzed(AcausalAnalysis*)), SLOT(onAnalyzed(AcausalAnalysis*)));
	m_audioThread = createWorkerThread([=](){return serviceAudio();});
}

AudioMan::~AudioMan()
{
	cnote << "Disabling playback...";
	stop();
	for (m_audioThread->quit(); !m_audioThread->wait(1000); m_audioThread->terminate()) {}
	delete m_audioThread;
	m_audioThread = nullptr;
	cnote << "Disabled permenantly.";
}

void AudioMan::onAnalyzed(AcausalAnalysis* _analysis)
{
	if (_analysis == &*m_resampleWaveAcAnalysis)
	{
		setCursor(0, true);
		emit dataLoaded();
	}
}

void AudioMan::setRate(unsigned _s)
{
	emit prepareForChange();
	emit prepareForHopChange();

	Noted::compute()->suspendWork();

	m_fineCursor = 0;
	stop();
	m_rate = _s;
	updateKeys();

	Noted::compute()->noteLastValidIs(nullptr);
	Noted::compute()->resumeWork();

	emit hopChanged();
	emit changed();
}

void AudioMan::setHopSamples(unsigned _s)
{
	emit prepareForChange();
	emit prepareForHopChange();

	Noted::compute()->suspendWork();

	m_fineCursor = 0;
	stop();
	m_hopSamples = _s;
	updateKeys();

	Noted::compute()->noteLastValidIs(m_resampleWaveAcAnalysis);
	Noted::compute()->resumeWork();

	emit hopChanged();
	emit changed();
}

void AudioMan::setFilename(QString const& _filename)
{
	emit prepareForChange();
	emit prepareForDataChange();

	Noted::compute()->suspendWork();

	m_fineCursor = 0;
	stop();
	m_filename = _filename;
	if (!QFile(m_filename).open(QFile::ReadOnly))
		m_filename.clear();
	updateKeys();

	Noted::compute()->noteLastValidIs(nullptr);
	Noted::compute()->resumeWork();

	emit dataChanged();
	emit changed();
}

void AudioMan::play(bool _causal)
{
	if (_causal)
	{
		Noted::compute()->suspendWork();
		m_isCausal = true;
		Noted::compute()->initializeCausal(nullptr);
		m_lastIndex = cursorIndex();
	}
	else
		m_isAcausal = true;
	if (!m_audioThread->isRunning())
		m_audioThread->start(QThread::TimeCriticalPriority);

	emit stateChanged(isAcausal(), isCausal(), isPassing());
}

void AudioMan::passthrough()
{
	Noted::compute()->suspendWork();
	m_isPassing = true;
	Noted::compute()->initializeCausal(nullptr);
	if (!m_audioThread->isRunning())
		m_audioThread->start(QThread::TimeCriticalPriority);
	m_lastIndex = cursorIndex();

	emit stateChanged(isAcausal(), isCausal(), isPassing());
}

void AudioMan::stop()
{
	if (m_isPassing)
	{
		Noted::compute()->finalizeCausal();
		m_isPassing = false;
		Noted::compute()->resumeWork();
		emit stateChanged(isAcausal(), isCausal(), isPassing());
	}
	if (m_isCausal)
	{
		while (m_audioThread->isRunning())
			usleep(100000);
		Noted::compute()->finalizeCausal();
		m_isCausal = false;
		Noted::compute()->resumeWork();
		emit stateChanged(isAcausal(), isCausal(), isPassing());
	}
	if (m_isAcausal)
	{
		m_isAcausal = false;
		emit stateChanged(isAcausal(), isCausal(), isPassing());
	}
}

void AudioMan::setCursor(lb::Time _c, bool _warp)
{
	if (m_fineCursor != _c)
	{
		m_fineCursor = _c;
		if (_warp && isCausal())
			m_lastIndex = cursorIndex();
		QMetaObject::invokeMethod(this, "processCursorChange");
	}
}

void AudioMan::processCursorChange()
{
	if (m_fineCursor != m_lastFineCursor)
	{
		if (m_fineCursor >= duration())
		{
			// Played to end of audio
			stop();
			setCursor(0);
		}
		m_lastFineCursor = m_fineCursor;

		if (m_lastCursorChangedSignal - wallTime() > FromMsecs<100>::value)
			emit cursorHasChanged(m_fineCursor);
		emit cursorChanged(m_fineCursor);
	}
}

// returns true if it's pairs of max/rms, false if it's samples.
bool AudioMan::waveBlock(Time _from, Time _duration, lb::foreign_vector<float> o_toFill, bool _forceSamples) const
{
	int samples = fromBase(_duration, rate());
	int items = _forceSamples ? o_toFill.size() : (o_toFill.size() / 2);
	int samplesPerItem = samples / items;
	if (samplesPerItem < (int)hopSamples() || _forceSamples)
	{
		QMutexLocker l(&x_wave);
		int imin = -_from * items / _duration;
		int imax = (duration() - _from) * items / _duration;
		float const* d = m_wave.data<float>().data() + fromBase(_from, rate());
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

lb::foreign_vector<float const> AudioMan::waveWindow(int _window) const
{
	QMutexLocker l(&x_wave);

	_window = clamp<int, int>(_window, 0, hops());

	// 0th window begins at (1 - hopsPerWindow) hops; all negative samples are 0 values.
	unsigned windowSize = Noted::get()->m_windowFunction.size();
	int hopsPerWindow = windowSize / hopSamples();
	int hop = _window + 1 - hopsPerWindow;
	if (hop < 0)
	{
		shared_ptr<vector<float> > data = make_shared<vector<float>>(windowSize, 0.f);
		if (windowSize + hop * hopSamples() > 0)
		{
			auto i = m_wave.data<float>().cropped(0, windowSize + hop * hopSamples());
			valcpy(data->data() + windowSize - i.count(), i.data(), i.count());
		}
		return foreign_vector<float const>(data->data(), windowSize).tied(data);
	}
	else
		// same page - just return
		return m_wave.data<float>().cropped(hop * hopSamples(), windowSize).tied(std::make_shared<QMutexLocker>(&x_wave));
}

bool AudioMan::resampleWave()
{
	SF_INFO info;
	auto sndfile = sf_open(m_filename.toLocal8Bit().data(), SFM_READ, &info);
	if (sndfile)
	{
		QMutexLocker l1(&x_waveProfile);
		QMutexLocker l2(&x_wave);
		unsigned outHops = (fromBase(toBase(info.frames, info.samplerate), rate()) + hopSamples() - 1) / hopSamples();
		m_samples = outHops * hopSamples();
		bool waveOk = m_wave.init(rawKey(), key(), 0, samples() * sizeof(float));
		bool waveProfileOk = m_waveProfile.init(rawKey(), key(), 1, 2 * sizeof(float), outHops);
		if (!waveOk || !waveProfileOk)
		{
			sf_seek(sndfile, 0, SEEK_SET);
			vector<float> buffer(hopSamples() * info.channels);

			float* cache = m_wave.data<float>().data();
			float* wave = m_waveProfile.data<float>().data();
			if (info.samplerate == (int)rate())
			{
				// Just copy across...
				for (unsigned i = 0; i < outHops; ++i, wave += 2, cache += hopSamples())
				{
					unsigned rc = sf_readf_float(sndfile, buffer.data(), hopSamples());
					valcpy<float>(cache, buffer.data(), rc, 1, info.channels);	// just take the channel 0.
					memset(cache + rc, 0, sizeof(float) * (hopSamples() - rc));	// zeroify what's left.
					wave[0] = sigma(buffer);
					auto r = range(buffer);
					wave[1] = max(fabs(r.first), r.second);
					WorkerThread::setCurrentProgress(i * 100 / outHops);
				}
			}
			else
			{
				// Needs a resample
				double factor = double(rate()) / info.samplerate;
				void* resampler = resample_open(1, factor, factor);
				unsigned bufferPos = hopSamples();

				for (unsigned i = 0; i < outHops; ++i, wave += 2, cache += hopSamples())
				{
					unsigned pagePos = 0;
					while (pagePos != hopSamples())
					{
						if (bufferPos == hopSamples())
						{
							// At end of current (input) buffer - refill and reset position.
							int rc = sf_readf_float(sndfile, buffer.data(), hopSamples());
							if (rc < 0)
								rc = 0;
							valcpy<float>(buffer.data(), buffer.data(), rc, 1, info.channels);	// just take the channel 0.
							memset(buffer.data() + rc, 0, sizeof(float) * (hopSamples() - rc));	// zeroify what's left.
							bufferPos = 0;
						}
						int used = 0;
						pagePos += resample_process(resampler, factor, buffer.data() + bufferPos, hopSamples() - bufferPos, i == outHops - 1, &used, cache + pagePos, hopSamples() - pagePos);
						bufferPos += used;
					}
					for (unsigned j = 0; j < hopSamples(); ++j)
						cache[j] = clamp(cache[j], -1.f, 1.f);
					wave[0] = sigma(cache, cache + hopSamples(), 0.f);
					auto r = range(cache, cache + hopSamples());
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
		m_wave.init(key(), "wave", 0);
		m_waveProfile.init(key(), "waveProfile", 2 * sizeof(float), 0);
		return false;
	}
	return true;
}

bool AudioMan::serviceAudio()
{
	bool doneWork = false;
	if (isAcausal() || isCausal())
	{
		if (!m_playback)
		{
			try {
				m_playback = shared_ptr<Audio::Playback>(new Audio::Playback(m_playDevice, 2, defaultTo<int>(m_playRate, rate()), m_playChunkSamples, m_playChunks, m_playForce16Bit));
			} catch (...) {}
		}
		if (m_playback)
		{
			unsigned f = m_playback->frames();
			unsigned r = m_playback->rate();
			vector<float> output(f * m_playback->channels());
			if (m_fineCursor >= 0 && m_fineCursor < duration())
			{
				if (rate() == m_playback->rate())
				{
					// no resampling necessary
					waveBlock(m_fineCursor, toBase(f, r), &output, true);
				}
				else
				{
					vector<float> source(f);
					double factor = double(m_playback->rate()) / rate();
					if (m_fineCursorWas != m_fineCursor || !m_resampler)
					{
						// restart resampling.
						if (m_resampler)
							resample_close(m_resampler);
						m_resampler = resample_open(1, factor, factor);
						m_nextResample = m_fineCursor;
					}
					m_fineCursorWas = m_fineCursor + toBase(m_playback->frames(), m_playback->rate());

					unsigned outPos = 0;
					int used = 0;
					// Try our luck without going to the expensive waveBlock call first.
					outPos += resample_process(m_resampler, factor, nullptr, 0, 0, &used, &(output[outPos]), f - outPos);
					while (outPos != f)
					{
						// At end of current (input) buffer - refill and reset position.
						waveBlock(m_nextResample, toBase(f, rate()), &source, true);
						outPos += resample_process(m_resampler, factor, &(source[0]), f, 0, &used, &(output[outPos]), f - outPos);
						m_nextResample += toBase(used, rate());
					}
					for (float& f: output)
						f = clamp(f, -1.f, 1.f);
				}
				if (m_playback->isInterleaved())
					valfan2(&(output[0]), &(output[0]), f);
				else
					valcpy(&(output[f]), &(output[0]), f);
			}
			m_playback->write(output);
			setCursor(m_fineCursor + toBase(f, r));	// might be different to m_fineCursorWas...
		}
		doneWork = true;
	}
	else if (m_playback)
	{
		if (m_playback)
			m_playback.reset();
		if (m_resampler)
		{
			resample_close(m_resampler);
			m_resampler = nullptr;
		}
	}

	if (m_isPassing)
	{
		if (!m_capture)
		{
			try {
				m_capture = shared_ptr<Audio::Capture>(new Audio::Capture(m_captureDevice, 1, rate(), hopSamples(), m_captureChunks));
			} catch (...) {}
			m_currentWave = vector<float>(Noted::get()->windowSizeSamples(), 0);
			/*
			m_fftw = shared_ptr<FFTW>(new FFTW(windowSizeSamples()));
			m_currentMagSpectrum = vector<float>(spectrumSize(), 0);
			m_currentPhaseSpectrum = vector<float>(spectrumSize(), 0);
			*/
		}
		if (m_capture)
		{
			// pull out another chunk, rotate m_currentWave hopSamples
			memmove(m_currentWave.data(), m_currentWave.data() + hopSamples(), (Noted::get()->windowSizeSamples() - hopSamples()) * sizeof(float));
			m_capture->read(foreign_vector<float>(m_currentWave.data() + Noted::get()->windowSizeSamples() - hopSamples(), hopSamples()));
			/*
			float* b = m_fftw->in();
			foreign_vector<float> win(&m_currentWave);
			assert(win.data());
			float* d = win.data();
			float* w = m_windowFunction.data();
			unsigned off = m_zeroPhase ? m_windowFunction.size() / 2 : 0;
			for (unsigned j = 0; j < m_windowFunction.size(); ++d, ++j, ++w)
				b[(j + off) % m_windowFunction.size()] = *d * *w;
			m_fftw->process();

			m_currentMagSpectrum = m_fftw->mag();
			m_currentPhaseSpectrum = m_fftw->phase();
			*/
			/*
			float const* phase = fftw.phase().data();
			float intpart;
			for (int i = 0; i < ss; ++i)
			{
				sd[i + ss] = phase[i] / twoPi<float>();
				sd[i + ss2] = modf((phase[i] - lp[i]) / twoPi<float>() + 1.f, &intpart);
			}
			*/

			// update
			Noted::compute()->updateCausal(m_lastIndex++, 1);
		}
		doneWork = true;
	}
	else if (m_capture)
	{
		if (m_capture)
			m_capture.reset();
		if (m_resampler)
		{
			resample_close(m_resampler);
			m_resampler = nullptr;
		}
	}

	if (m_isCausal && m_lastIndex != (int)cursorIndex())
	{
		// do events until cursor.
		if (!((int)cursorIndex() < m_lastIndex || (int)cursorIndex() - m_lastIndex > 100))	// probably skipped.
			Noted::compute()->updateCausal(m_lastIndex + 1, cursorIndex() - m_lastIndex);
		m_lastIndex = cursorIndex();
		doneWork = true;
	}

	return doneWork;
}
