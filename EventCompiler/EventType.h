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

#include <set>
#include <Common/Trivial.h>

namespace Lightbox
{

LIGHTBOX_TEXTUAL_ENUM_INHERITS(EventType, uint8_t,
				NoEvent,
				Attack, Sustain, Decay, Release,
				SyncPoint, PeriodSet, PeriodTweak, PeriodReset, Tick, Beat, Bar, Cycle,
				Comment, GraphSpecComment, AuxComment, RhythmCandidatesComment, RhythmVectorComment, HistoryComment, PhaseVectorComment, PhaseCandidatesComment, LastBarDistanceComment,
				WorkingComment, PDFComment,
				Graph, GraphUnder, GraphBar)

typedef std::set<EventType> EventTypes;

inline EventTypes operator|(EventType _a, EventType _b) { return EventTypes({_a, _b}); }
inline EventTypes operator|(EventTypes _a, EventType _b) { _a.insert(_b); return _a; }
inline EventTypes operator|(EventType _a, EventTypes _b) { _b.insert(_a); return _b; }

static const EventType BeginStandard = Attack;
static const EventType EndStandard = Comment;
static const EventTypes AllEventTypes = { Attack, PeriodSet };
static const EventTypes JustAttack = { Attack };

inline bool isGraph(EventType _e)
{
	return _e >= Graph;
}

inline bool isComment(EventType _e)
{
	return _e >= Comment && _e < SyncPoint;
}

inline bool isStandard(EventType _e)
{
	return _e >= BeginStandard && _e < EndStandard;
}

inline bool isChannelSpecific(EventType _e)
{
	return _e > NoEvent && _e < SyncPoint;
}

inline bool impliesActive(EventType _e)
{
	return _e > NoEvent && _e < Release;
}

}
