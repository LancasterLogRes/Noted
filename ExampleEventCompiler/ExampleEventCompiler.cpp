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
	LIGHTBOX_EVENTCOMPILER_PREPROCESSORS(highEnergy);

private:
	Historied<HighEnergy> highEnergy;

	virtual StreamEvents init()
	{
		StreamEvents ret;
		unsigned const c_historySize = FromMsecs<100>::value / hop();
		highEnergy.setHistory(c_historySize);

		AuxGraphsSpec* ags = new AuxGraphsSpec("History", ms, id, msL);
		ags->addGraph(GraphSpec(0.1f, HistoryComment, LineChart, Color::fromHsp(0.1f * 360, 255, 255), 0, toSeconds(hop())));
		ret.push_back(StreamEvent(GraphSpecComment, ags));

		m_maxBeatLikelihood = 0.0f;
		m_lastBeatLikelihood = 0.f;
		return ret;
	}

	virtual StreamEvents compile(Time, vector<float> const&, vector<float> const&, std::vector<float> const&)
	{
		StreamEvents ret;

		// Graph the latest values directly (on the timeline).
		ret.push_back(StreamEvent(Graph, highEnergy.HighEnergy::get(), 0.1f));	// orange

		float beatLikelihood = highEnergy.HighEnergy::get();
		if (beatLikelihood < m_lastBeatLikelihood)
		{
			// Just past a peak
			m_maxBeatLikelihood = max(m_maxBeatLikelihood, m_lastBeatLikelihood);
			float recentLowest = range(highEnergy.getVector()).first;
			if (m_lastBeatLikelihood > m_maxBeatLikelihood / 4 && recentLowest < m_lastBeatLikelihood / 10)
				ret.push_back(StreamEvent(Spike, beatLikelihood / m_maxBeatLikelihood, 0.1));
		}
		m_lastBeatLikelihood = beatLikelihood;

		if (highEnergy.changed())
		{
			// If the historical graph has changed, record the latest graph.
			ret.push_back(StreamEvent(HistoryComment, 1.0, 0.1f, hop(), new AuxFloatVector(highEnergy.get().data(), highEnergy.get().size())));
		}

		return ret;
	}

	float m_lastBeatLikelihood;
	float m_maxBeatLikelihood;
};

LIGHTBOX_EVENTCOMPILER(BeatDetector);
