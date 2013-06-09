#pragma once

#include <EventCompiler/Compiler.h>

struct SNDFILE_tag;

namespace lb
{

/** Reads audio files and resamples to mono audio data.
 */
class FileAudioStream: public AudioStream
{
public:
	FileAudioStream(unsigned _hop, std::string const& _fn, unsigned _rate): m_hop(_hop), m_filename(_fn), m_rate(_rate) {}
	~FileAudioStream() { fini(); }

	Time duration() { return toBase(m_incomingFrames, m_incomingRate); }
	void setHop(unsigned _samples) { m_hop = _samples; }
	void setFile(std::string const& _fn, unsigned _rate) { m_filename = _fn; m_rate = _rate; }

	void init();
	void fini();
	bool isGood() const;

	virtual unsigned rate() const { return m_rate; }
	virtual unsigned hop() const { return m_hop; }
	virtual void iterate() { m_fresh = true; }
	virtual void copyTo(unsigned _channel, Fixed<1, 15>* _p) { translateTo<float>(_channel, _p); }
	virtual void copyTo(unsigned _channel, float* _p);

private:
	unsigned m_hop = 64;
	std::string m_filename;
	unsigned m_rate = 22050;
	bool m_fresh = true;
	std::vector<float> m_last;

	std::ifstream m_file;

	struct SNDFILE_tag* m_sndfile = nullptr;

	unsigned m_bufferPos;
	void* m_resampler = nullptr;
	double m_factor;

	unsigned m_incomingFrames;
	unsigned m_incomingRate;
	unsigned m_incomingChannels;
//	unsigned m_incomingFormat;

	unsigned m_outHops;
	unsigned m_outSamples;

	std::vector<float> m_buffer;

	bool m_partialRead;
	bool m_atEnd;
};

}
