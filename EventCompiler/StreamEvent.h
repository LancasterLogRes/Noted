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

namespace Lightbox
{

LIGHTBOX_TEXTUAL_ENUM(EventType,
				NoEvent,
				Spike, Chain, Jet, EndJet,
				Sustain, EndSustain, BackSustain, EndBackSustain,
				PeriodSet, PeriodTweak, PeriodReset, Tick, Beat, Bar, Cycle,
				SpikeA, SpikeB, SpikeC, SpikeD, SpikeE, SpikeF,
				ChainA, ChainB, ChainC, ChainD, ChainE, ChainF,
				Comment, GraphSpecComment, AuxComment, RhythmCandidatesComment, RhythmVectorComment, HistoryComment, PhaseVectorComment, PhaseCandidatesComment, LastBarDistanceComment,
				WorkingComment, PDFComment,
				Graph, GraphUnder, GraphBar)

typedef std::vector<EventType> EventTypes;

inline EventTypes operator|(EventType _a, EventType _b) { return EventTypes({_a, _b}); }
inline EventTypes operator|(EventTypes _a, EventType _b) { _a.push_back(_b); return _a; }
inline EventTypes operator|(EventType _a, EventTypes _b) { _b.push_back(_a); return _b; }

static const EventType BeginStandard = Spike;
static const EventType EndStandard = Comment;
static const EventTypes AllEventTypes = { Spike, Jet, Sustain, BackSustain, PeriodSet };
static const EventTypes SustainTypes = { Sustain, BackSustain };
static const EventTypes JustSpike = { Spike };
static const EventTypes JustJet = { Jet };

inline EventType endToBegin(EventType _e)
{
	switch (_e)
	{
	case EndJet: return Jet;
	case EndSustain: return Sustain;
	case EndBackSustain: return BackSustain;
	case PeriodReset: return PeriodSet;
	default: return NoEvent;
	}
}

inline EventType toMain(EventType _e)
{
	switch (_e)
	{
	case EndJet: case Jet: return Jet;
	case EndSustain: case Sustain: return Sustain;
	case EndBackSustain: case BackSustain: return BackSustain;
	case PeriodReset: case PeriodTweak: case PeriodSet: return PeriodSet;
	case Chain: case Spike: return Spike;
	default: return _e;
	}
}

inline EventType asSustain(EventType _e)
{
	switch (_e)
	{
	case Sustain: case EndSustain: return Sustain;
	case BackSustain: case EndBackSustain: return BackSustain;
	default: return NoEvent;
	}
}

inline EventType asBegin(EventType _e)
{
	switch (_e)
	{
	case BackSustain: case Sustain: case Jet: case PeriodSet: return _e;
	default: return NoEvent;
	}
}

/**
 * May be |ed together to finish up with a valid Character; if you use asCharacter, it'll
 * automatically canonicalise it for you removing the chance of an invalid bit-configuration.
 */
enum CharacterComponent
{
	NoCharacter = 0,
	Charming = 8,
	Aggressive = 4,
	Structured = 2,
	Pointed = 1,
	Tedious = Charming << 16,
	Peaceful = Aggressive << 16,
	Chaotic = Structured << 16,
	Disparate = Pointed << 16,
	SimpleComponents = Charming | Aggressive | Structured | Pointed,
	InvertedComponents = SimpleComponents << 16,
	EveryCharacter = SimpleComponents | InvertedComponents
};

LIGHTBOX_FLAGS(CharacterComponent, CharacterComponents, (Charming)(Aggressive)(Structured)(Pointed)(Tedious)(Peaceful)(Chaotic)(Disparate));

/**
 * All 16 valid configurations of CharacterComponents, named.
 *
 * This datatype uses only 4 bits; CharacterComponents uses 8 in order to allow
 * multiple configurations (i.e. via a mask) to be represented. They can be
 * exchanged with CharacterComponents using utility functions below, but think
 * twice before doing so.
 */
enum Character
{
	Dull,		//=	(Tedious|Peaceful|Chaotic|Disparate)&SimpleComponents,			// (space)
	Foolish,	//=	(Tedious|Peaceful|Chaotic|Pointed)&SimpleComponents,			// F
	Geeky,		//=	(Tedious|Peaceful|Structured|Disparate)&SimpleComponents,		// E
	Glib,		//=	(Tedious|Peaceful|Structured|Pointed)&SimpleComponents,			// G
	Yobbish,	//=	(Tedious|Aggressive|Chaotic|Disparate)&SimpleComponents,		// Y
	Fanatical,	//=	(Tedious|Aggressive|Chaotic|Pointed)&SimpleComponents,			// F
	Violent,	//=	(Tedious|Aggressive|Structured|Disparate)&SimpleComponents,		// V
	Psychotic,	//=	(Tedious|Aggressive|Structured|Pointed)&SimpleComponents,		// O
	Ditzy,		//=	(Charming|Peaceful|Chaotic|Disparate)&SimpleComponents,			// ?
	Vibrant,	//=	(Charming|Peaceful|Chaotic|Pointed)&SimpleComponents,			// ~
	Harmonious,	//= 	(Charming|Peaceful|Structured|Disparate)&SimpleComponents,		// #
	Adroit,		//=	(Charming|Peaceful|Structured|Pointed)&SimpleComponents,		// |
	Explosive,	//=	(Charming|Aggressive|Chaotic|Disparate)&SimpleComponents,		// *
	Implosive,	//=	(Charming|Aggressive|Chaotic|Pointed)&SimpleComponents,			// /
	Visceral,	//=	(Charming|Aggressive|Structured|Disparate)&SimpleComponents,		// !
	Piquant		//=	(Charming|Aggressive|Structured|Pointed)&SimpleComponents		// ^
};

LIGHTBOX_ENUM_TOSTRING(Character, Dull, Foolish, Geeky, Glib, Yobbish, Fanatical, Violent, Psychotic, Ditzy, Vibrant, Harmonious, Adroit, Explosive, Implosive, Visceral, Piquant);

/// Unneeded unless you suspect _c could be an invalid bit pattern (i.e. not a character).
/// The character returned is guaranteed to be valid.
inline CharacterComponents toComponents(Character _c) { return CharacterComponents(int(_c) | ((int(_c) << 16) ^ InvertedComponents)); }
inline Character toCharacter(CharacterComponents _c) { return Character(_c & SimpleComponents); }

inline char toChar(Character _c)
{
	switch (_c)
	{
	case Dull: return ' ';
	case Foolish: return 'F';
	case Geeky: return 'E';
	case Glib: return 'G';
	case Yobbish: return 'Y';
	case Fanatical: return 'A';
	case Violent: return 'V';
	case Psychotic: return 'P';
	case Ditzy: return '?';
	case Vibrant: return '~';
	case Harmonious: return '#';
	case Adroit: return '|';
	case Explosive: return '*';
	case Implosive: return '/';
	case Visceral: return '!';
	case Piquant: return '^';
	default: return 0;
	}
}

inline Character toCharacter(char _c)
{
	switch (_c)
	{
	case ' ': return Dull;
	case 'F': return Foolish;
	case 'E': return Geeky;
	case 'G': return Glib;
	case 'Y': return Yobbish;
	case 'A': return Fanatical;
	case 'V': return Violent;
	case 'P': return Psychotic;
	case '?': return Ditzy;
	case '~': return Vibrant;
	case '#': return Harmonious;
	case '|': return Adroit;
	case '*': return Explosive;
	case '/': return Implosive;
	case '!': return Visceral;
	case '^': return Piquant;
	default: return Dull;
	}
}

inline CharacterComponent operator|(CharacterComponent _a, CharacterComponent _b) { return CharacterComponent(_a | _b); }

/**
 * All events use the strength, character and nature unless otherwise noted.
 *
 * Spike represents a singularity in the time-series, generally it represent the onset
 * of an energetic phenomenon, but can also represent the lack of such an eventuality.
 * It is not classified, though the nature and character field may help distinguish.
 * It uses the position, period and surprise members of StreamEvent; period describes
 * the length of activity that can be attributed to this onset. A value of zero will
 * do something sensible.
 *
 * For non-fuzzily classified spikes, use one of the SpikeA/SpikeB/... events. In this
 * case and nature represent the prototypical parameters.
 * They use the position and surprise members of StreamEvent.
 *
 * Chain is a Spike-like event subsequent to some other Chain or Spike, such that its
 * contextual meaning is heavily dependent on said preceeding event(s) to the point
 * where the two events may be considered a perceptually gestalt entity.
 * Chains are typically close together and similar in nature. Echos and drum rolls would
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
 * Multiple Sustain events without EndSustain tweak the nature
 * of the voice (e.g. sweeping through the frequencies/loudnesses). They should be given
 * in addition to an onset (as in RT we'll have no idea whether the onset will be sustained
 * until it has been going for some time).
 * BackSustain and EndBackSustain are equivalent but allow a secondary background
 * sustained voice to be represented also.
 *
 * Cycle should happen on a 4-bar boundary and determines the upcoming nature and dynamics
 * through the nature, strength and surprise properties.
 *
 * PeriodSet, PeriodTweak and PeriodReset give the beat period (i.e. inverse prop. to bpm).
 * They are for informational purposes only. They use only the period member of StreamEvent.
 *
 * Beat and Bar are used to demarkate the start of beats and bars.
 * They are for informational purposes only. They use only the position member of StreamEvent.
 */
struct StreamEvent
{
	struct Aux
	{
	public:
		virtual ~Aux() {}
	};

	StreamEvent(EventType _t, Aux* _aux): type(_t), nature(-1.f), period(0), strength(1.f), m_aux(std::shared_ptr<Aux>(_aux)) { }
	StreamEvent(EventType _t, float _s, float _n, Aux* _aux): type(_t), nature(_n), period(0), strength(_s), m_aux(std::shared_ptr<Aux>(_aux)) { }
	StreamEvent(EventType _t = NoEvent, float _s = 1.f, float _n = 0.f, Time _period = 0, Aux* _aux = nullptr, int8_t _position = -1, Character _character = Dull, float _surprise = 1.f): type(_t), position(_position), character(_character), nature(_n), period(_period), strength(_s), surprise(_surprise), m_aux(std::shared_ptr<Aux>(_aux)) { }

	bool operator==(StreamEvent const& _c) const { return type == _c.type && nature == _c.nature && strength == _c.strength; }
	bool operator!=(StreamEvent const& _c) const { return !operator==(_c); }
	bool operator<(StreamEvent const& _c) const { return type < _c.type; }

	std::shared_ptr<Aux> const& aux() const { return m_aux; }

	EventType type;
	int8_t position;	///< -1 unknown, 0-63 for first 16th note in super-bar, second 16th note, &c.
	Character character;		///< The character of this event.
	float nature;		///< Abstract quantity in range [0, 1] to describe primary aspects of event.
	Time period;		///< Value to describe EventType-dependent period.
	float strength;		///< Non-zero quantity in range [-1, 1], to describe loudness/confidence that phenomenon actually happened. If negative describes confidence that phenomenon didn't happen.
	float surprise;		///< Quantity [0, 1] to describe how easily predicted that this StreamEvent was. Negative strength makes this value describe surprise that the phenomenon didn't happen.
	std::shared_ptr<Aux> m_aux;///< Auxilliary data for the event. Will be removed for final build.
};

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
		nature(_n),
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

	float nature;
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
	return _out << "{" << _e.type << ":" << _e.strength << "@" << _e.character << "/" << _e.nature << /*std::hex << int(_e.nature[0]) << "," << int(_e.nature[1]) << "," << int(_e.nature[2]) <<*/ "}";
}

}
