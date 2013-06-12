#include "DataMan.h"
#include "NotedFace.h"
#include "AudioManFace.h"
using namespace std;
using namespace lb;

void AudioManFace::updateKeys()
{
	m_rawKey = qHash(m_filename) ^ qHash(m_rate);
	m_key = qHash(m_hopSamples) ^ m_rawKey;
}
