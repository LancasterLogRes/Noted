#pragma once

#include <QObject>
#include <QHash>
#include <Common/Time.h>

typedef uint32_t SimpleHash;

class AudioManFace: public QObject
{
	Q_OBJECT

public:
	SimpleHash key() const { return m_key; }

	QString const& filename() const { return m_filename; }
	inline unsigned rate() const { return m_rate; }
	inline unsigned hopSamples() const { return m_hopSamples; }
	inline unsigned samples() const { return m_samples; }

	void setRate(unsigned _s) { m_rate = _s; rejig(); emit changed(); }
	void setHopSamples(unsigned _s) { m_hopSamples = _s; rejig(); emit changed(); }
	void setSamples(unsigned _s) { m_samples = _s; rejig(); emit changed(); }
	void setFilename(QString const& _fn) { m_filename = _fn; rejig(); emit changed(); }

	inline unsigned hops() const { return samples() ? samples() / hopSamples() : 0; }
	inline lb::Time duration() const { return lb::toBase(samples(), rate()); }
	inline lb::Time hop() const { return lb::toBase(hopSamples(), rate()); }
	inline unsigned index(lb::Time _t) const { return (_t < 0) ? 0 : std::min<unsigned>(_t / hop(), samples() / hopSamples()); }

signals:
	void changed();

private:
	void rejig() { m_key = qHash(m_filename) ^ qHash(m_hopSamples) ^ qHash(m_rate); }

	QString m_filename;
	SimpleHash m_key = 0;
	unsigned m_rate = 1;
	unsigned m_hopSamples = 2;
	unsigned m_samples = 0;
};
