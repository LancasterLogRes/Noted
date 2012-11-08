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

#include <array>
#include <vector>
#include <map>
#include <iostream>
#include <iomanip>
#include <cstdint>

#include "Global.h"
#include "RGBA.h"

namespace Lightbox
{

class Color;
typedef std::vector<Color> Colors;

class Color
{
public:
	LIGHTBOX_STRUCT_INTERNALS_4(Color, float, m_hue, float, m_sat, float, m_value, float, m_alpha);

	Color(): m_hue(0), m_sat(0), m_value(0), m_alpha(0) {}
	explicit Color(float _value, float _alpha = 1.f): m_hue(0), m_sat(0), m_value(_value), m_alpha(_alpha) {}
	Color(float _hue, float _sat, float _value, float _alpha = 1.f): m_hue(_hue), m_sat(_sat), m_value(_value), m_alpha(_alpha) {}
	explicit Color(RGBA const& _rgba): m_hue(_rgba.h() / 360.f), m_sat(_rgba.s() / 255.f), m_value(_rgba.v() / 255.f), m_alpha(_rgba.a() / 255.f) {}
	Color(Color const& _c): m_hue(_c.m_hue), m_sat(_c.m_sat), m_value(_c.m_value), m_alpha(_c.m_alpha) {}

	float hue() const { return m_hue; }
	float sat() const { return m_sat; }
	float value() const { return m_value; }
	float alpha() const { return m_alpha; }
	Color& setHue(float _hue) { m_hue = _hue; return *this; }
	Color& setSat(float _sat) { m_sat = _sat; return *this; }
	Color& setValue(float _value) { m_value = _value; return *this; }
	Color& setAlpha(float _alpha) { m_alpha = _alpha; return *this; }
	Color withHue(float _hue) const { return Color(_hue, m_sat, m_value, m_alpha); }
	Color withSat(float _sat) const { return Color(m_hue, _sat, m_value, m_alpha); }
	Color withValue(float _value) const { return Color(m_hue, m_sat, _value, m_alpha); }
	Color withAlpha(float _alpha) const { return Color(m_hue, m_sat, m_value, _alpha); }

	Color& attenuate(float _x) { m_value *= _x; return *this; }
	Color attenuated(float _x) const { return Color(m_hue, m_sat, m_value * _x, m_alpha); }

	RGBA toRGBA() const { RGBA ret; ret.setHsv(clamp<int>(m_hue * 360, 0, 359), clamp<int>(m_sat * 255, 0, 255), clamp<int>(m_value * 255, 0, 255), clamp<int>(m_alpha * 255, 0, 255)); return ret; }
	static Color fromRGB(std::array<float, 3> _rgb);
//	operator RGBA() const { return toRGBA(); }

	// shorthand forms.
	float h() const { return m_hue; }
	float s() const { return m_sat; }
	float v() const { return m_value; }
	float a() const { return m_alpha; }
	Color& h(float _hue) { m_hue = _hue; return *this; }
	Color& s(float _sat) { m_sat = _sat; return *this; }
	Color& v(float _value) { m_value = _value; return *this; }
	Color& a(float _alpha) { m_alpha = _alpha; return *this; }

	Color& operator*=(float _x) { return attenuate(_x); }
	Color operator*(float _x) const { return attenuated(_x); }
	Color& operator/=(float _x) { return attenuate(1.f / _x); }
	Color operator/(float _x) const { return attenuated(1.f / _x); }

	float distance(Color _hueSat, bool _useHue = true, bool _useSat = true) const
	{
		return (_useSat ? sqr(std::fabs(m_sat - _hueSat.sat()) / 3) : 0) + (_useHue ? sqr((0.5 - std::fabs(std::fabs(m_hue - _hueSat.hue()) - 0.5)) * m_sat) : 0);
	}

	/// Nearest hue/saturation to the given collection of h/s pairs.
	inline unsigned nearest(Colors const& _p, bool _useHue = true, bool _useSat = true) const { return std::get<2>(findBest(_p, [&](Color const& c){ return -distance(c, _useHue, _useSat); })); }

	static float hueCorrection(float _h);

private:
	float m_hue;
	float m_sat;
	float m_value;
	float m_alpha;
};

static const Color White = Color(1);
static const Color Red = Color(0, 1, 1);
static const Color Green = Color(0.333333, 1, 1);
static const Color Blue = Color(0.666666, 1, 1);
static const Color Yellow = Color(0.166666, 1, 1);
static const Color Magenta = Color(0.8333333, 1, 1);
static const Color Cyan = Color(0.5, 1, 1);
static const Color Black = Color(0);

static const Color NullColor;

static const Colors NullColors;
static const Colors RedOnly = { Red };
static const Colors GreenOnly = { Green };
static const Colors BlueOnly = { Blue };

}
