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

#pragma once

#include <iostream>
#include <ios>
#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include <limits>

#include <functional>

#include <Common/Time.h>
#include <Common/Trivial.h>
#include <Common/Maths.h>
#include <Common/Color.h>

#include "EventType.h"
#include "Character.h"

namespace Lightbox
{

/**
 * All events use the strength, character and temperature unless otherwise noted.
 *
 * Spike represents a singularity in the time-series, generally it represent the onset
 * of an energetic phenomenon, but can also represent the lack of such an eventuality.
 * It is not classified, though the temperature and character field may help distinguish.
 * It uses the position, period and surprise members of StreamEvent; period describes
 * the length of activity that can be attributed to this onset. A value of zero will
 * do something sensible.
 *
 * For non-fuzzily classified spikes, use one of the SpikeA/SpikeB/... events. In this
 * case and temperature represent the prototypical parameters.
 * They use the position and surprise members of StreamEvent.
 *
 * Chain is a Spike-like event subsequent to some other Chain or Spike, such that its
 * contextual meaning is heavily dependent on said preceeding event(s) to the point
 * where the two events may be considered a perceptually gestalt entity.
 * Chains are typically close together and similar in temperature. Echos and drum rolls would
 * be examplar.
 * There's ChainA &c. for classified onsets.
 * They all use the position, surprise and period members of StreamEvents, with
 * similar semantics as for Spike.
 *
 * Jet is for a recurring voice without a clear phase (alignment). It'll probably be a
 * multiple of the beat period, if not the bar period itself. It can represent only one
 * at once and must be ended by EndJet. It uses the period member of StreamEvent.
 *
 * Sustain is for voices that are truly sustained (i.e. not simply
 * decaying slowly). It can represent only one at once and must be ended by EndSustain
 * Multiple Sustain events without EndSustain tweak the temperature
 * of the voice (e.g. sweeping through the frequencies/loudnesses). They should be given
 * in addition to an onset (as in RT we'll have no idea whether the onset will be sustained
 * until it has been going for some time).
 * BackSustain and EndBackSustain are equivalent but allow a secondary background
 * sustained voice to be represented also.
 *
 * Cycle should happen on a 4-bar boundary and determines the upcoming temperature and dynamics
 * through the temperature, strength and surprise properties.
 *
 * PeriodSet, PeriodTweak and PeriodReset give the beat period (i.e. inverse prop. to bpm).
 * They are for informational purposes only. They use only the period member of StreamEvent.
 *
 * Beat and Bar are used to demarkate the start of beats and bars.
 * They are for informational purposes only. They use only the position member of StreamEvent.
 */

static const int CompatibilityChannel = std::numeric_limits<int>::max();

struct StreamEvent
{
	struct Aux
	{
	public:
		virtual ~Aux() {}
	};

	StreamEvent(EventType _t, Aux* _aux): type(_t), temperature(-1.f), strength(1.f), period(0), m_aux(std::shared_ptr<Aux>(_aux)) { }
	StreamEvent(EventType _t, float _s, float _n, Aux* _aux): type(_t), temperature(_n), strength(_s), period(0), m_aux(std::shared_ptr<Aux>(_aux)) { }
	StreamEvent(EventType _t = NoEvent, float _s = 1.f, float _n = 0.f, Time _period = 0, Aux* _aux = nullptr, int8_t _position = -1, Character _character = Dull, float _surprise = 1.f): type(_t), position(_position), character(_character), channel(-1), temperature(_n), strength(_s), surprise(_surprise), period(_period), m_aux(std::shared_ptr<Aux>(_aux)) { }

	void assign(int _channel = CompatibilityChannel)
	{
		if (isChannelSpecific(type))
			if (_channel == CompatibilityChannel)
			{
				if (toMain(type) == Spike)
					channel = 0;
				else if (toMain(type) == Sustain)
					channel = 1;
				else if (toMain(type) == BackSustain)
				{
					channel = 2;
					type = Sustain;
				}
				else if (toMain(type) == Jet)
					channel = 3;
				else
					channel = -1;
			}
			else
				channel = _channel;
		else
			channel = -1;
	}
	StreamEvent assignedTo(int _ch) const { if (isChannelSpecific(type)) { StreamEvent ret = *this; ret.channel = _ch; return ret; } return *this; }

	bool operator==(StreamEvent const& _c) const { return type == _c.type && temperature == _c.temperature && strength == _c.strength; }
	bool operator!=(StreamEvent const& _c) const { return !operator==(_c); }
	bool operator<(StreamEvent const& _c) const { return type < _c.type; }

	std::shared_ptr<Aux> const& aux() const { return m_aux; }

	EventType type;				///< Type of the event.
	int8_t position;			///< -1 unknown, 0-63 for first 16th note in super-bar, second 16th note, &c.
	Character character;		///< The character of this event.
	int8_t channel;				///< The channel this event is on. Typically < 4; -1 -> channel n/a or unknown.

	float temperature;			///< Abstract quantity in range [0, 1] to describe primary aspects of event.
	float strength;				///< Non-zero quantity in range [-1, 1], to describe loudness/confidence that phenomenon actually happened. If negative describes confidence that phenomenon didn't happen.
	float surprise;				///< Quantity [0, 1] to describe how easily predicted that this StreamEvent was. Negative strength makes this value describe surprise that the phenomenon didn't happen.
	Time period;				///< Value to describe EventType-dependent period.
	std::shared_ptr<Aux> m_aux;	///< Auxilliary data for the event. TODO: Deprecate in favour of an index/store comm. method.
};

inline float toHue(float _temperature)
{
	return (600 - (int)(clamp(_temperature, -.25f, 1.25f) * 240)) % 360;
}

/** e.g.
 *
 * Spike(strength=1; surprise=1) means onset definitely occurred that we were unable to predict (either
 * because it was unusual or because we haven't yet built a good enough model of the music).
 *
 * Spike(strength=0.5; surprise=1) means onset quite-possibly occurred which, in the case, would be surprising.
 *
 * Spike(strength=1; surprise=0) means onset occurred that we had abundant reason to suppose would indeed have occurred.
 *
 * Spike(strength=-1; surprise=1) means onset didn't occur that we had abundant reason to suppose would have occurred.
 *
 * Spike(strength=-1; surprise=0) [nonsensical]
 * Spike(strength=0; surprise=?) [nonsensical]
 */

struct AuxLabel: public StreamEvent::Aux
{
	AuxLabel(std::string const& _label): label(_label) {}
	virtual ~AuxLabel() {}
	std::string label;
};

template <class _T>
struct AuxVector: public StreamEvent::Aux
{
	AuxVector() {}
	AuxVector(std::vector<_T> const& _d): data(_d) {}
	AuxVector(_T const* _d, unsigned _s): data(std::vector<_T>(_s)) { memcpy(data.data(), _d, _s * sizeof(_T)); }
	virtual ~AuxVector() {}
	std::vector<_T> data;
};
typedef AuxVector<float> AuxFloatVector;

struct AuxMeterMap: public StreamEvent::Aux
{
	AuxMeterMap(std::map<float, Meter> const& _d): data(_d) {}
	virtual ~AuxMeterMap() {}
	std::map<float, Meter> data;
};

struct AuxFloatMap: public StreamEvent::Aux
{
	AuxFloatMap(std::map<float, float> const& _d): data(_d) {}
	virtual ~AuxFloatMap() {}
	std::map<float, float> data;
};

struct AuxPhaseMap: public StreamEvent::Aux
{
	AuxPhaseMap(std::map<float, Phase> const& _d): data(_d) {}
	virtual ~AuxPhaseMap() {}
	std::map<float, Phase> data;
};

enum GraphType
{
	LineChart,
	FilledLineChart,
	BarChart,
	LabelPeaks,
	MeterExplanation,
	PhaseExplanation,
	LabeledPoints,
	LinesChart,
	RuleY
};

typedef std::pair<float, float> Range;
Range static const AutoRange = {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};

struct GraphSpec
{
	GraphSpec(float _n, EventType _f, GraphType _t, Color _p, float _xC = 0.f, float _xM = 1.f, float _yC = 0.f, float _yM = 1.f, Range const& _xR = AutoRange, Range const& _yR = AutoRange):
		temperature(_n),
		filter(_f),
		type(_t),
		primary(_p),
		xC(_xC),
		xM(_xM),
		yC(_yC),
		yM(_yM),
		xRange(_xR),
		yRange(_yR)
	{}

	GraphSpec(std::function<float(float)> const& _f, GraphType _t, Color _p, float _xC = 0.f, float _xM = 1.f, float _yC = 0.f, float _yM = 1.f, Range const& _xR = AutoRange, Range const& _yR = AutoRange):
		filter(NoEvent),
		type(_t),
		primary(_p),
		xC(_xC),
		xM(_xM),
		yC(_yC),
		yM(_yM),
		f(_f),
		xRange(_xR),
		yRange(_yR)
	{}

	float temperature;
	EventType filter;
	GraphType type;
	Color primary;
	float xC;
	float xM;
	float yC;
	float yM;
	std::function<float(float)> f;
	std::pair<float, float> xRange;
	std::pair<float, float> yRange;
};

struct AuxGraphsSpec: public StreamEvent::Aux
{
	AuxGraphsSpec(std::string const& _name, std::function<std::string(float)> const& _xLabel, std::function<std::string(float)> const& _yLabel, std::function<std::string(float, float)> const& _pLabel, std::pair<float, float> const& _xRange = AutoRange, std::pair<float, float> const& _yRange = AutoRange):
		name (_name),
		xLabel(_xLabel),
		yLabel(_yLabel),
		pLabel(_pLabel),
		xRange(_xRange),
		yRange(_yRange)
	{
	}

	AuxGraphsSpec(std::string const& _name, std::function<std::string(float, bool)> const& _pxLabel, std::function<std::string(float)> const& _yLabel, std::pair<float, float> const& _xRange = AutoRange, std::pair<float, float> const& _yRange = AutoRange):
		name (_name),
		xLabel([=](float _f){return _pxLabel(_f, false);}),
		yLabel(_yLabel),
		pLabel([=](float _x, float){ return _pxLabel(_x, true);}),
		xRange(_xRange),
		yRange(_yRange)
	{
	}

	void addGraph(GraphSpec const& _s) { graphs.push_back(_s); }

	std::string name;
	std::function<std::string(float)> xLabel;
	std::function<std::string(float)> yLabel;
	std::function<std::string(float, float)> pLabel;
	std::pair<float, float> xRange;
	std::pair<float, float> yRange;
	std::vector<GraphSpec> graphs;
};

typedef std::vector<StreamEvent> StreamEvents;

static const StreamEvents NullStreamEvents;

inline StreamEvents& merge(StreamEvents& _dest, StreamEvents const& _src)
{
	if (_dest.empty())
		return (_dest = _src);
	foreach (StreamEvent const& e, _src)
		_dest.push_back(e);
	return _dest;
}

inline StreamEvents noComment(StreamEvents const& _se)
{
	StreamEvents ret;
	for (auto i = _se.begin(); i != _se.end(); ++i)
		if (i->type != Comment && i->type < Graph)
			ret.push_back(*i);
	return ret;
}

inline std::ostream& operator<<(std::ostream& _out, StreamEvent const& _e)
{
	return _out << "{" << _e.type << ":" << _e.strength << "@" << _e.character << "/" << _e.temperature << /*std::hex << int(_e.temperature[0]) << "," << int(_e.temperature[1]) << "," << int(_e.temperature[2]) <<*/ "}";
}

}
