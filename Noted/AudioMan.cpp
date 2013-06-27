#include <sndfile.h>
#include <libresample.h>
#include <Common/Global.h>
#include "Global.h"
#include "FileAudioStream.h"
#include "Noted.h"
#include "AudioMan.h"
using namespace std;
using namespace lb;

class ResampleWaveAc: public AcausalAnalysis
{
public:
	ResampleWaveAc(): AcausalAnalysis("Resampling wave") {}
	virtual unsigned prepare(unsigned, unsigned, Time)
	{
		DataSetFloatPtr wave;
		{
			QMutexLocker l(&((AudioMan*)NotedFace::audio())->x_wave);
			((AudioMan*)NotedFace::audio())->m_wave = wave = NotedFace::data()->create(DataKey(NotedFace::audio()->rawKey(), 0));
		}

		if (wave->haveRaw())
		{
			((AudioMan*)NotedFace::audio())->m_samples = wave->rawRecords();
			return 0;
		}
		else
		{
			FileAudioStream as(256, ((AudioMan*)NotedFace::audio())->m_filename.toStdString(), ((AudioMan*)NotedFace::audio())->m_rate);
			as.init();
			((AudioMan*)NotedFace::audio())->m_samples = fromBase(as.duration(), ((AudioMan*)NotedFace::audio())->m_rate);
			return ((AudioMan*)NotedFace::audio())->m_samples;
		}
	}

	virtual void analyze(unsigned, unsigned, lb::Time)
	{
		const unsigned c_chunkSamples = 65536;

		unsigned r = ((AudioMan*)NotedFace::audio())->m_rate;
		DataSetFloatPtr wave = ((AudioMan*)NotedFace::audio())->m_wave;

		FileAudioStream as(c_chunkSamples, ((AudioMan*)NotedFace::audio())->m_filename.toStdString(), r);
		as.init();
		unsigned s = fromBase(as.duration(), r);
		wave->setDense(1, toBase(1, r));

		if (!wave->haveRaw())
		{
			float chunk[c_chunkSamples];
			unsigned done = 0;
			for (; done < s; done += c_chunkSamples)
			{
				as.copyTo(0, chunk);
				foreign_vector<float> rs(chunk, min(s - done, c_chunkSamples));
				wave->appendDenseRecords(rs);
				AcausalAnalysis::done(done);
			}
		}

		wave->ensureHaveDigest(MeanRmsDigest);
		wave->ensureHaveDigest(MinMaxInOutDigest);
		wave->done();
	}
};

AudioMan::AudioMan():
	x_wave			(QMutex::Recursive)
{
	m_resampleWaveAcAnalysis = AcausalAnalysisPtr(new ResampleWaveAc);
	connect(Noted::compute(), SIGNAL(analyzed(AcausalAnalysis*)), SLOT(onAnalyzed(AcausalAnalysis*)));
	m_audioThread = createWorkerThread([=](){return serviceAudio();});
	NotedFace::compute()->registerJobSource(this);
	NotedFace::graphs()->registerGraph("wave", m_waveGraph);
}

AudioMan::~AudioMan()
{
	cnote << "Disabling playback...";
	stop();
	for (m_audioThread->quit(); !m_audioThread->wait(1000); m_audioThread->terminate()) {}
	delete m_audioThread;
	m_audioThread = nullptr;
	cnote << "Disabled permenantly.";
	NotedFace::graphs()->unregisterGraph("wave");
	NotedFace::compute()->unregisterJobSource(this);
}

AcausalAnalysisPtrs AudioMan::ripeAcausalAnalysis(AcausalAnalysisPtr const& _finished)
{
	if (_finished == nullptr)
		return { resampleWaveAcAnalysis() };
	return {};
}

void AudioMan::onAnalyzed(AcausalAnalysis* _analysis)
{
	if (_analysis == &*m_resampleWaveAcAnalysis)
	{
		setCursor(0, true);
		emit dataLoaded();
	}
}

void AudioMan::ensureWaveHeld()
{
	m_wave = NotedFace::data()->get(DataKey(rawKey(), 0));
}

void AudioMan::setRate(unsigned _s)
{
	emit prepareForChange();
	emit prepareForHopChange();

	Noted::compute()->suspendWork();

	m_fineCursor = 0;
	stop();
	m_rate = _s;

	sourceChanged();

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

	sourceChanged();

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

	sourceChanged();

	Noted::compute()->resumeWork();

	emit dataChanged();
	emit changed();
}

void AudioMan::sourceChanged()
{
	auto oldKey = m_key;
	auto oldRawKey = m_rawKey;
	updateKeys();

	if (oldRawKey != m_rawKey || oldKey != m_key)
		Noted::compute()->invalidate();
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
		m_isCausal = false;
		while (m_audioThread->isRunning())
			usleep(100000);
		Noted::compute()->finalizeCausal();
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
		if (quantized(m_fineCursor) != quantized(m_lastFineCursor))
			emit hopCursorChanged(hopCursor());

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

void AudioMan::populateHop(unsigned _index, std::vector<float>& _h) const
{
	QMutexLocker l(&x_wave);
	if (m_wave)
		m_wave->populateSeries(_index * hop(), &_h);
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
					QMutexLocker l(&x_wave);
					if (m_wave)
						m_wave->populateSeries(m_fineCursor, foreign_vector<float>(output.data(), f));
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
						{
							QMutexLocker l(&x_wave);
							if (m_wave)
								m_wave->populateSeries(m_nextResample, foreign_vector<float>(source.data(), f));
						}
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
			m_currentWave = vector<float>(hopSamples(), 0);
		}
		if (m_capture)
		{
			// pull out another chunk, rotate m_currentWave hopSamples
			m_capture->read(foreign(m_currentWave.data(), hopSamples()));

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
