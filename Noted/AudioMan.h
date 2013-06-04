#pragma once

#include <memory>
#include <Common/FFT.h>
#include <Audio/Capture.h>
#include <Audio/Playback.h>
#include <NotedPlugin/AudioManFace.h>
#include <NotedPlugin/GraphManFace.h>
#include <NotedPlugin/DataSet.h>
#include <NotedPlugin/JobSource.h>
#include <EventCompiler/GraphSpec.h>
#include "WorkerThread.h"

class AudioMan: public AudioManFace, public JobSource
{
	Q_OBJECT

	friend class ResampleWaveAc;

public:
	AudioMan();
	virtual ~AudioMan();

	/// Data
	// TODO! Kill both in favour of DataMan/DataSet.
	virtual lb::foreign_vector<float const> waveWindow(int _window) const;
	virtual bool waveBlock(lb::Time _from, lb::Time _duration, lb::foreign_vector<float> o_toFill, bool _forceSamples = false) const;

	/// Playback
	virtual bool isPlaying() const { return !!m_playback; }

	// Playback device info - could move off into a struct or something?
	virtual QString deviceName() const { return QString::fromStdString(m_playback->deviceName()); }
	virtual unsigned deviceChannels() const { return m_playback->channels(); }
	virtual unsigned devicePeriods() const { return m_playback->periods(); }
	virtual unsigned deviceRate() const { return m_playback->rate(); }
	virtual unsigned deviceFrames() const { return m_playback->frames(); }

	virtual AcausalAnalysisPtrs ripeAcausalAnalysis(AcausalAnalysisPtr const&);

public slots:
	/// Data
	virtual void setFilename(QString const& _fn);
	virtual void setRate(unsigned _s);
	virtual void setHopSamples(unsigned _s);

	/// Playback
	virtual void play(bool _causal = false);
	virtual void passthrough();
	virtual void stop();

	/// Safe to be called from any thread.
	virtual void setCursor(lb::Time _t, bool _warp = false);

private slots:
	void onAnalyzed(AcausalAnalysis*);
	void processCursorChange();

private:
	bool resampleWave();
	bool serviceAudio();
	virtual void updateKeys();

	/// Data
	DataSetPtr m_newWave;
	// Actual data TODO: REMOVE
	mutable QMutex x_wave;
	Cache m_wave;
	mutable QMutex x_waveProfile;
	MipmappedCache m_waveProfile;

	/// Playback
	// Audio hardware i/o
	WorkerThread* m_audioThread;
	std::shared_ptr<Audio::Playback> m_playback;
	std::shared_ptr<Audio::Capture> m_capture;

	// Playback...
	lb::Time m_fineCursorWas = lb::UndefinedTime;
	lb::Time m_nextResample = lb::UndefinedTime;
	void* m_resampler = nullptr;

	// Passthrough...
	std::vector<float> m_currentWave;

	// Causal & passthrough...
	int m_lastIndex;

	// Cursor management...
	lb::Time m_lastFineCursor;
	lb::Time m_lastCursorChangedSignal;

	/// Our graph
	GraphMetadata m_waveGraph = GraphMetadata(0, {{ "Amplitude", lb::XOf(), lb::Range(-1, 1) }}, "PCM", true);
};
