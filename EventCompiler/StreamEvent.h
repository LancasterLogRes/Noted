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
#include <Common/RGBA8.h>

#include "EventType.h"
#include "Character.h"

namespace Lightbox
{

/**
 * All events use the strength, character and temperature unless otherwise noted.
 *
 * Attack represents a singularity in the time-series, generally it represent the onset
 * of an energetic phenomenon, but can also represent the lack of such an eventuality.
 * It is not classified, though the temperature and character field may help distinguish.
 * It uses the position, period and surprise members of StreamEvent; period describes
 * the length of activity that can be attributed to this onset. A value of zero will
 * do something sensible.
 *
 * Sustain is for voices that are truly sustained (i.e. not simply
 * decaying slowly). It can represent only one at once and must be ended by Release
 * Multiple Sustain events without Release tweak the temperature
 * of the voice (e.g. sweeping through the frequencies/loudnesses). They should be given
 * in addition to an onset (as in RT we'll have no idea whether the onset will be sustained
 * until it has been going for some time).
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

	StreamEvent(EventType _type, float _strength, float _temperature, Character _character, float _jitter = .5f, float _constancy = .5f, int8_t _position = -1, float _surprise = 1.f, Aux* _aux = nullptr): type(_type), position(_position), character(_character), channel(-1), temperature(_temperature), strength(_strength), surprise(_surprise), jitter(_jitter), constancy(_constancy), m_aux(std::shared_ptr<Aux>(_aux)) { }
	StreamEvent(float _strength, float _temperature): type(Graph), position(0), character(Dull), channel(-1), temperature(_temperature), strength(_strength), surprise(1.f), jitter(0.5f), constancy(0.5f) {}
	StreamEvent(EventType _type, float _strength, Time _period, Time _phase): type(_type), position(0), character(Dull), channel(-1), temperature(0.f), strength(_strength), surprise(1.f), jitter(toSeconds(_phase)), constancy(toSeconds(_period)), m_aux(nullptr) {}
	StreamEvent(EventType _type, float _temperature, Time _period, Aux* _aux): type(_type), position(0), character(Dull), channel(-1), temperature(_temperature), strength(0.f), surprise(1.f), jitter(0.5f), constancy(toSeconds(_period)), m_aux(_aux) {}
	StreamEvent(EventType _type = NoEvent): type(_type), position(0), character(Dull), channel(-1), temperature(-1.f), strength(1.f), surprise(1.f), jitter(0.5f), constancy(0.5f) {}

	void assign(int _channel)
	{
		if (isChannelSpecific(type))
			channel = _channel;
		else
			channel = -1;
	}
	StreamEvent assignedTo(int _ch) const { StreamEvent ret = *this; ret.assign(_ch); return ret; }

	bool operator==(StreamEvent const& _c) const { return type == _c.type && temperature == _c.temperature && strength == _c.strength; }
	bool operator!=(StreamEvent const& _c) const { return !operator==(_c); }
	bool operator<(StreamEvent const& _c) const { return type < _c.type; }

	void sanitize() { memset(&m_aux, 0, sizeof(m_aux)); }

	std::shared_ptr<Aux> const& aux() const { return m_aux; }

	EventType type;				///< Type of the event.
	int8_t position;			///< -1 unknown, 0-63 for first 16th note in super-bar, second 16th note, &c.
	Character character;		///< The character of this event.
	int8_t channel;				///< The channel this event is on. Typically < 4; -1 -> channel n/a or unknown.

	float temperature;			///< Abstract quantity in range [0, 1] to describe primary aspects of event.
	float strength;				///< Non-zero quantity in range [-1, 1], to describe loudness/confidence that phenomenon actually happened. If negative describes confidence that phenomenon didn't happen.
	float surprise;				///< Quantity [0, 1] to describe how easily predicted that this StreamEvent was. Negative strength makes this value describe surprise that the phenomenon didn't happen.
	float jitter;				///< Between [0, 1]; adds random stuff into decay (Attack/Decay) or easing error limit before snap change (Sustain).
	float constancy;			///< "Slowness" between [0, 1]. Decay period (Attack/Decay) or inverse of easing speed (Sustain).
	std::shared_ptr<Aux> m_aux;	///< Auxilliary data for the event. TODO: Deprecate in favour of an index/store comm. method.
};

inline float toHue(float _temperature)
{
	float f;
	return std::modf((1.666666 - clamp(_temperature, -.25f, 1.25f) * .66666), &f);
}

inline float fromHue(float _hue)
{
	float f = (1.666666 - clamp(_hue, 0.f, 1.f)) * 1.5f - 1.5f;
	if (f < -0.25f)
		return f + 1.5f;
	else
		return f;
}

/** e.g.
 *
 * Attack(strength=1; surprise=1) means onset definitely occurred that we were unable to predict (either
 * because it was unusual or because we haven't yet built a good enough model of the music).
 *
 * Attack(strength=0.5; surprise=1) means onset quite-possibly occurred which, in the case, would be surprising.
 *
 * Attack(strength=1; surprise=0) means onset occurred that we had abundant reason to suppose would indeed have occurred.
 *
 * Attack(strength=-1; surprise=1) means onset didn't occur that we had abundant reason to suppose would have occurred.
 *
 * Attack(strength=-1; surprise=0) [nonsensical]
 * Attack(strength=0; surprise=?) [nonsensical]
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
	AuxVector(_T const* _d, unsigned _s): data(std::vector<_T>(_d, _d + _s)) {}
	AuxVector(foreign_vector<_T> const& _v): data(std::vector<_T>(_v.begin(), _v.end())) {}
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
	GraphSpec(float _n, EventType _f, GraphType _t, RGBA8 _p, float _xC = 0.f, float _xM = 1.f, float _yC = 0.f, float _yM = 1.f, Range const& _xR = AutoRange, Range const& _yR = AutoRange):
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

	GraphSpec(std::function<float(float)> const& _f, GraphType _t, RGBA8 _p, float _xC = 0.f, float _xM = 1.f, float _yC = 0.f, float _yM = 1.f, Range const& _xR = AutoRange, Range const& _yR = AutoRange):
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
	RGBA8 primary;
	float xC;
	float xM;
	float yC;
	float yM;
	std::function<float(float)> f;
	std::pair<float, float> xRange;
	std::pair<float, float> yRange;
};

std::string id(float _y);
std::string ms(float _x);
std::string msL(float _x, float _y);
std::string bpm(float _x);
std::string bpmL(float _x, float _y);

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
	return _out << "{" << int(_e.channel) << ":" << _e.type << "@" << _e.strength << "$" << _e.character << "/" << _e.temperature << /*std::hex << int(_e.temperature[0]) << "," << int(_e.temperature[1]) << "," << int(_e.temperature[2]) <<*/ "}";
}

}
