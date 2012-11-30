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

#include <Common/Common.h>
#include <EventCompiler/EventCompiler.h>
#include <EventCompiler/Preprocessors.h>
using namespace std;
using namespace Lightbox;

LIGHTBOX_EVENTCOMPILER_LIBRARY;

string id(float _y) { return toString(_y); }
string ms(float _x){ return toString(round(_x * 1000)) + (!_x ? "ms" : ""); }
string msL(float _x, float _y) { return toString(round(_x * 1000)) + "ms (" + toString(round(_y * 100)) + "%)"; }

class BeatDetector: public EventCompilerImpl
{
public:
	BeatDetector(): halfLife(0.01) {}

	LIGHTBOX_PREPROCESSORS(highEnergy);
	LIGHTBOX_PROPERTIES(halfLife);

	double halfLife;

private:
	Historied<HighEnergy> highEnergy;

	virtual StreamEvents init()
	{
		StreamEvents ret;
		unsigned const c_historySize = FromMsecs<100>::value / hop();
		highEnergy.setHistory(c_historySize);

/*		AuxGraphsSpec* ags = new AuxGraphsSpec("History", ms, id, msL);
		ags->addGraph(GraphSpec(0.1f, HistoryComment, LineChart, RGBA8::fromHsp(0.1f * 360, 255, 255), 0, toSeconds(hop())));
		ret.push_back(StreamEvent(GraphSpecComment, ags));*/

		m_maxBeatLikelihood = 0.0f;
		m_lastBL = 0.f;
		m_lastLastBL = 0.f;
		m_decayedBL = 0.f;
		return ret;
	}

	virtual StreamEvents compile(Time, vector<float> const&, vector<float> const&, std::vector<float> const&)
	{
		StreamEvents ret;

		Gaussian distro;
		distro.mean = mean(highEnergy.getVector());
		distro.sigma = max(.05f, sigma(highEnergy.getVector(), distro.mean));

		float prob = (distro.mean > 0.01) ? normal(highEnergy.HighEnergy::get(), distro) : 1.f;
		float beatLikelihood = -log(prob);

		// Graph the latest values directly (on the timeline).
		ret.push_back(StreamEvent(highEnergy.HighEnergy::get(), 0.0f));
		ret.push_back(StreamEvent(beatLikelihood, 0.9f));
		ret.push_back(StreamEvent(m_decayedBL, 0.5f));

		if (beatLikelihood < m_lastBL && m_lastBL > m_lastLastBL)
		{
			if (beatLikelihood < m_lastBL * .95)
			{
				// Just past a peak
				m_maxBeatLikelihood = max(m_maxBeatLikelihood, m_lastBL);
				if (m_lastBL > m_decayedBL && m_lastBL / m_maxBeatLikelihood > 0.0625)
					ret.push_back(StreamEvent(Attack, m_lastBL / m_maxBeatLikelihood, .1f, Dull));
				m_decayedBL = max(m_decayedBL, m_lastBL * 5);
				m_lastLastBL = m_lastBL;
				m_lastBL = beatLikelihood;
			}
			else
			{
				// Ignore this sample for purposes of peak-finding - it's too shallow a peak.
			}
		}
		else
		{
			m_lastLastBL = m_lastBL;
			m_lastBL = beatLikelihood;
		}
		m_decayedBL = decayed(m_decayedBL, hop(), fromSeconds(halfLife));

		return ret;
	}

	float m_maxBeatLikelihood;
	float m_lastBL;
	float m_lastLastBL;
	float m_decayedBL;
};

LIGHTBOX_EVENTCOMPILER(BeatDetector);
