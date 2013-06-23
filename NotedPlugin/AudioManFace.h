#pragma once

#include <QObject>
#include <QHash>
#include <Common/Global.h>
#include <Common/Time.h>
#include "Common.h"
#include "AcausalAnalysis.h"

class GenericDataSet;
template <class _T> class DataSet;
typedef std::shared_ptr<DataSet<float>> DataSetFloatPtr;

class AudioManFace: public QObject
{
	Q_OBJECT

public:
	AudioManFace() {}
	virtual ~AudioManFace() {}

	/// Data
	inline lb::SimpleKey key() const { return m_key; }
	inline lb::SimpleKey rawKey() const { return m_rawKey; }
	inline DataKey keySet(QString const& _operationId) const { return DataKey(key(), qHash(_operationId)); }
	inline DataKey rawKeySet(QString const& _operationId) const { return DataKey(rawKey(), qHash(_operationId)); }

	virtual DataSetFloatPtr wave() const = 0;
	virtual void populateHop(unsigned _index, std::vector<float>& _h) const = 0;

	QString const& filename() const { return m_filename; }
	inline unsigned rate() const { return m_rate; }
	inline unsigned hopSamples() const { return m_hopSamples; }
	inline unsigned samples() const { return m_samples; }

	inline unsigned hops() const { return samples() ? samples() / hopSamples() : 0; }
	inline lb::Time duration() const { return lb::toBase(samples(), rate()); }
	inline lb::Time hop() const { return lb::toBase(hopSamples(), rate()); }
	inline lb::Time quantized(lb::Time _t) const { return (_t < 0 ? _t - (hop() - 1) : _t) / hop() * hop(); }
	inline unsigned index(lb::Time _t) const { return (_t < 0) ? 0 : std::min<unsigned>(_t / hop(), samples() / hopSamples()); }

	AcausalAnalysisPtr resampleWaveAcAnalysis() const { return m_resampleWaveAcAnalysis; }

	/// Playback [consider splitting off]
	virtual bool isPlaying() const { return false; }
	inline bool isAcausal() const { return m_isAcausal; }
	inline bool isCausal() const { return m_isCausal; }
	inline bool isPassing() const { return m_isPassing; }
	inline bool isImmediate() const { return isCausal() || isPassing(); }
	inline bool isQuiet() const { return !isAcausal() && !isCausal() && !isPassing(); }

	inline lb::Time cursor() const { return m_fineCursor; }
	inline lb::Time hopCursor() const { return quantized(m_fineCursor); }
	inline unsigned cursorIndex() const { return index(cursor()); }

public slots:
	/// Data
	virtual void setFilename(QString const& _filename) = 0;
	virtual void setRate(unsigned _s) = 0;
	virtual void setHopSamples(unsigned _s) = 0;

	/// Playback
	inline void setPlayDevice(int _index) { m_playDevice = _index; }
	inline void setPlayRate(int _hz) { m_playRate = _hz; }
	inline void setPlayChunkSamples(int _s) { m_playChunkSamples = _s; }
	inline void setPlayChunks(int _chunks) { m_playChunks = _chunks; }
	inline void setPlayForce16Bit(bool _v) { m_playForce16Bit = _v; }

	inline void setCaptureDevice(int _index) { m_captureDevice = _index; }
	inline void setCaptureChunks(int _chunks) { m_captureChunks = _chunks; }

	virtual void play(bool _causal = false) = 0;
	virtual void passthrough() = 0;
	virtual void stop() = 0;
	virtual void setCursor(lb::Time _t, bool _warp = false) = 0;
	inline void setHopCursor(lb::Time _t) { setCursor(quantized(_t + hop() - 1), true); }	// Sets it to cursor that contains _t.

signals:
	/// Data
	void prepareForChange();
	void changed();
	void prepareForHopChange();
	void hopChanged();
	void prepareForDataChange();
	void dataChanged();
	void dataLoaded();

	/// Playback
	void stateChanged(bool _isAcausal, bool _isCausal, bool _isPassing);
	void hopCursorChanged(lb::Time);
	void cursorChanged(lb::Time);
	void cursorHasChanged(lb::Time);	// Called sometime after one or more cursor changes - meant for GUI updates that don't need per-millisecond updates.

protected:
	void updateKeys();

	Q_PROPERTY(lb::Time cursor READ hopCursor WRITE setHopCursor NOTIFY hopCursorChanged)
	Q_PROPERTY(lb::Time hop READ hop NOTIFY hopChanged)
	Q_PROPERTY(lb::Time duration READ duration NOTIFY dataLoaded)
	Q_PROPERTY(unsigned cursorIndex READ cursorIndex NOTIFY cursorChanged)

	/// Data
	AcausalAnalysisPtr m_resampleWaveAcAnalysis;
	QString m_filename;
	lb::SimpleKey m_rawKey = 0;				///< filename ^ rate
	lb::SimpleKey m_key = 0;					///< rawKey ^ hop
	unsigned m_rate = 1;
	unsigned m_hopSamples = 2;
	unsigned m_samples = 0;

	/// Playback
	int m_playDevice = -1;				///< device index. -1 -> default.
	int m_playRate = -1;				///< rate. -1 -> default; 0 -> rate().
	int m_playChunkSamples = 512;		///< samples per chunk.
	int m_playChunks = -1;				///< chunks. -1 -> default.
	bool m_playForce16Bit = true;
	int m_captureDevice = -1;				///< device index. -1 -> default.
	int m_captureChunks = -1;				///< chunks. -1 -> default.

	bool m_isAcausal = false;
	bool m_isCausal = false;
	bool m_isPassing = false;
	lb::Time m_fineCursor = 0;
};
