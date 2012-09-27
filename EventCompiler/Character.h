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
#include <Common/Trivial.h>
#include <Common/UnitTesting.h>

namespace Lightbox
{

static const int c_negativeCharacters = 4;

/**
 * May be |ed together to finish up with a valid Character; if you use asCharacter, it'll
 * automatically canonicalise it for you removing the chance of an invalid bit-configuration.
 */
enum CharacterComponent: uint8_t
{
	NoCharacter = 0,
	Aggressive = 4,
	Structured = 2,
	Pointed = 1,
	Peaceful = Aggressive << c_negativeCharacters,
	Chaotic = Structured << c_negativeCharacters,
	Disparate = Pointed << c_negativeCharacters,
	SimpleComponents = Aggressive | Structured | Pointed,
	InvertedComponents = SimpleComponents << c_negativeCharacters,
	EveryCharacter = SimpleComponents | InvertedComponents
};

static const CharacterComponent Charming = NoCharacter;
static const CharacterComponent Tedious = NoCharacter;

LIGHTBOX_FLAGS(CharacterComponent, CharacterComponents, (Aggressive)(Structured)(Pointed)(Peaceful)(Chaotic)(Disparate));

inline CharacterComponent component(int _index, bool _on)
{
	return CharacterComponent(1u << (_index + (_on ? 0 : c_negativeCharacters)));
}

/**
 * All 8 valid configurations of CharacterComponents, named.
 *
 * This datatype uses only 4 bits; CharacterComponents uses 8 in order to allow
 * multiple configurations (i.e. via a mask) to be represented. They can be
 * exchanged with CharacterComponents using utility functions below, but think
 * twice before doing so.
 */
enum Character: uint8_t
{
	Dull = 0,	//=	(Charming|Peaceful|Chaotic|Disparate)&SimpleComponents,			// (space)
	Vibrant,	//=	(Charming|Peaceful|Chaotic|Pointed)&SimpleComponents,			// ~
	Harmonious,	//= (Charming|Peaceful|Structured|Disparate)&SimpleComponents,		// #
	Adroit,		//=	(Charming|Peaceful|Structured|Pointed)&SimpleComponents,		// |
	Explosive,	//=	(Charming|Aggressive|Chaotic|Disparate)&SimpleComponents,		// *
	Implosive,	//=	(Charming|Aggressive|Chaotic|Pointed)&SimpleComponents,			// /
	Visceral,	//=	(Charming|Aggressive|Structured|Disparate)&SimpleComponents,	// !
	Piquant,	//=	(Charming|Aggressive|Structured|Pointed)&SimpleComponents		// ^
	Ditzy = Dull,			//=	(Tedious|Peaceful|Chaotic|Disparate)&SimpleComponents,			// (space)
	Foolish = Vibrant,		//=	(Tedious|Peaceful|Chaotic|Pointed)&SimpleComponents,			// F
	Geeky = Harmonious,		//=	(Tedious|Peaceful|Structured|Disparate)&SimpleComponents,		// E
	Glib = Adroit,			//=	(Tedious|Peaceful|Structured|Pointed)&SimpleComponents,			// G
	Yobbish = Explosive,	//=	(Tedious|Aggressive|Chaotic|Disparate)&SimpleComponents,		// Y
	Fanatical = Implosive,	//=	(Tedious|Aggressive|Chaotic|Pointed)&SimpleComponents,			// F
	Violent = Visceral,		//=	(Tedious|Aggressive|Structured|Disparate)&SimpleComponents,		// V
	Psychotic = Piquant,	//=	(Tedious|Aggressive|Structured|Pointed)&SimpleComponents,		// O
};

LIGHTBOX_ENUM_TOSTRING(Character, Dull, Vibrant, Harmonious, Adroit, Explosive, Implosive, Visceral, Piquant);

/// Unneeded unless you suspect _c could be an invalid bit pattern (i.e. not a character).
/// The character returned is guaranteed to be valid.
inline CharacterComponents toComponents(Character _c) { return CharacterComponents(int(_c) | ((int(_c) << c_negativeCharacters) ^ InvertedComponents)); }
inline Character toCharacter(CharacterComponents _c) { return Character(_c & SimpleComponents); }
inline Character toNegativeCharacter(CharacterComponents _c) { return Character((_c >> c_negativeCharacters) & SimpleComponents); }

inline char toChar(Character _c)
{
	switch (_c)
	{
	case Dull: return ' ';
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

inline CharacterComponent operator|(CharacterComponent _a, CharacterComponent _b) { return CharacterComponent((int)_a | (int)_b); }

inline Character masked(Character _c, CharacterComponents _cc)
{
	return Character(_c & ~(toNegativeCharacter(_cc)) | toCharacter(_cc));
}

LIGHTBOX_UNITTEST(3, "masked(Character, CharacterComponents)")
{
	LIGHTBOX_REQUIRE_EQUAL(masked(Dull, NoCharacter), Dull);
	LIGHTBOX_REQUIRE_EQUAL(masked(Vibrant, NoCharacter), Vibrant);
	LIGHTBOX_REQUIRE_EQUAL(masked(Adroit, NoCharacter), Adroit);
	LIGHTBOX_REQUIRE_EQUAL(masked(Piquant, NoCharacter), Piquant);

	LIGHTBOX_REQUIRE_EQUAL(masked(Dull, Aggressive), Explosive);
	LIGHTBOX_REQUIRE_EQUAL(masked(Vibrant, Aggressive), Implosive);
	LIGHTBOX_REQUIRE_EQUAL(masked(Adroit, Aggressive), Piquant);
	LIGHTBOX_REQUIRE_EQUAL(masked(Piquant, Aggressive), Piquant);

	LIGHTBOX_REQUIRE_EQUAL(masked(Dull, Disparate), Dull);
	LIGHTBOX_REQUIRE_EQUAL(masked(Vibrant, Disparate), Dull);
	LIGHTBOX_REQUIRE_EQUAL(masked(Adroit, Disparate), Harmonious);
	LIGHTBOX_REQUIRE_EQUAL(masked(Piquant, Disparate), Visceral);
}

inline CharacterComponents maskOf(CharacterComponents _cc)
{
	return (_cc | (_cc << c_negativeCharacters) | (_cc >> c_negativeCharacters)) & EveryCharacter;
}

}
