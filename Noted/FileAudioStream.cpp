#include <iostream>
#include <sndfile.h>
#include <libresample.h>
#include <Common/Time.h>
#include "FileAudioStream.h"
using namespace std;
using namespace lb;

void FileAudioStream::init()
{
	SF_INFO info;
	if (m_filename.size())
		m_sndfile = sf_open(m_filename.c_str(), SFM_READ, &info);
	else
		m_sndfile = sf_open_fd(0, SFM_READ, &info, false);

	if (m_sndfile)
	{
		m_incomingFrames = info.frames;
		m_incomingRate = info.samplerate;
		m_incomingChannels = info.channels;
		m_last.resize(m_hop);
		m_fresh = true;

		m_outHops = (fromBase(toBase(m_incomingFrames, m_incomingRate), m_rate) + m_hop - 1) / m_hop;
		m_outSamples = m_outHops * m_hop;

		m_buffer = vector<float>(m_hop * m_incomingChannels, 0);

		m_partialRead = m_atEnd = false;


		m_factor = double(m_rate) / m_incomingRate;
		m_bufferPos = m_hop;

		if (m_incomingRate != m_rate)
			m_resampler = resample_open(1, m_factor, m_factor);

//		cerr << "Opened " << m_filename << ": " << m_incomingChannels << " channels at " << m_incomingRate << " Hz, " << m_incomingFrames << " frames (" << textualTime(toBase(m_incomingFrames, m_incomingRate)) << ")." << endl;
//		cerr << "Require mono " << m_rate << " Hz (factor = " << m_factor << "). Will output " << m_outSamples << " frames (" << textualTime(toBase(m_outSamples, m_rate)) << ") making " << m_outHops << " hops." << endl;
	}
}

void FileAudioStream::fini()
{
	if (m_resampler)
		resample_close(m_resampler);
	m_resampler = nullptr;

	if (m_sndfile)
		sf_close(m_sndfile);
	m_sndfile = nullptr;
}

bool FileAudioStream::isGood() const
{
	return m_sndfile && !m_atEnd;
}

void FileAudioStream::copyTo(unsigned /*_channel*/, float* _p)
{
	if (!m_sndfile || m_atEnd)
	{
		memset(_p, 0, sizeof(float) * m_hop);
		return;
	}

	if (m_fresh)
	{
		if (m_incomingRate == m_rate)
		{
			// Just copy across...
			int rc;
			if (m_incomingChannels == 1)
				rc = max<int>(0, sf_readf_float(m_sndfile, m_last.data(), m_hop));
			else
			{
				rc = max<int>(0, sf_readf_float(m_sndfile, m_buffer.data(), m_hop));
				valcpy<float>(m_last.data(), m_buffer.data(), rc, 1, m_incomingChannels);	// just take the channel 0.
			}
			m_partialRead = m_atEnd = (rc != (int)m_hop);
			memset(m_last.data() + rc, 0, sizeof(float) * (m_hop - rc));	// zeroify what's left.
		}
		else
		{
			// Needs a resample
			unsigned pagePos = 0;
			for (; pagePos != m_hop && !m_atEnd;)
			{
				if (m_bufferPos == m_hop && !m_partialRead)
				{
					// At end of current (input) buffer - refill and reset position.
					int rc = max<int>(0, sf_readf_float(m_sndfile, m_buffer.data(), m_hop));
					if (m_incomingChannels != 1)
						valcpy<float>(m_buffer.data(), m_buffer.data(), rc, 1, m_incomingChannels);	// just take the channel 0.
					if (rc != (int)m_hop)
						memset(m_buffer.data() + rc, 0, sizeof(float) * (m_hop - rc));	// zeroify what's left.
					m_bufferPos = 0;
					m_partialRead = (rc != (int)m_hop);
				}
				int used = 0;
				pagePos += resample_process(m_resampler, m_factor, m_buffer.data() + m_bufferPos, m_hop - m_bufferPos, m_partialRead, &used, m_last.data() + pagePos, m_hop - pagePos);
				m_bufferPos += used;
				m_atEnd = (!used && m_partialRead);
			}
			if (m_hop != pagePos)
				memset(m_last.data() + pagePos, 0, sizeof(float) * (m_hop - pagePos));
		}
	}
	valcpy(_p, m_last.data(), m_hop);
}
