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

#include <vector>
#include <map>
#include <iostream>
#include <iomanip>
#include <cstdint>

#include "Global.h"
#include "Maths.h"
#include "Algorithms.h"

namespace Lightbox
{

class Color
{
public:
	Color() { m_data.value = 0; m_data.rgb.a = 255; }
	explicit Color(uint8_t _val) { r() = _val; g() = _val; b() = _val; }
	Color(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 255) { m_data.value = 0; r() = _r; g() = _g; b() = _b; a() = _a; }

	uint32_t uint32() const { return m_data.rgb.value(); }
	uint8_t r() const { return m_data.rgb.r; }
	uint8_t g() const { return m_data.rgb.g; }
	uint8_t b() const { return m_data.rgb.b; }
	uint8_t a() const { return m_data.rgb.a; }
	uint8_t& r() { return m_data.rgb.r; }
	uint8_t& g() { return m_data.rgb.g; }
	uint8_t& b() { return m_data.rgb.b; }
	uint8_t& a() { return m_data.rgb.a; }
	void setR(uint8_t _r) { m_data.rgb.r = _r; }
	void setG(uint8_t _g) { m_data.rgb.g = _g; }
	void setB(uint8_t _b) { m_data.rgb.b = _b; }
	void setA(uint8_t _a) { m_data.rgb.a = _a; }
	uint8_t chroma() const { return maxRgb() - minRgb(); }
	uint8_t maxRgb() const { return std::max(r(), std::max(g(), b())); }
	uint8_t minRgb() const { return std::min(r(), std::min(g(), b())); }

	unsigned h() const
	{
		uint8_t const m = maxRgb();
		uint8_t const c = chroma();
		if (!c)
			return 0;
		else if (m == r())
			return ((int(g()) - b()) * 6000 / c + 36000) % 36000 / 100;
		else if (m == g())
			return (int(b()) - r()) * 60 / c + 120.f;
		else
			return (int(r()) - g()) * 60 / c + 240.f;
	}
	uint8_t s() const { int const c = chroma(); return c ? c * 255 / maxRgb() : 0; }
	uint8_t v() const { return maxRgb(); }
	void setH(unsigned _h) { setHsv(_h, s(), v()); }
	void setS(uint8_t _s) { setHsv(h(), _s, v()); }
	void setV(uint8_t _v) { setHsv(h(), s(), _v); }

	static float hueCorrection(unsigned _h);
	static Color fromHsv(unsigned _h, uint8_t _s, uint8_t _v) { Color c; c.setHsv(_h, _s, _v); return c; }
	static Color fromHsp(unsigned _h, uint8_t _s, uint8_t _v) { Color c; c.setHsp(_h, _s, _v); return c; }
	void setHsp(unsigned _h, uint8_t _s, uint8_t _p, uint8_t _a = 255) { setHsv(_h, _s, hueCorrection(_h) * _p, _a); }

	void setHsv(unsigned _h, uint8_t _s, uint8_t _v, uint8_t _a = 255)
	{
		a() = _a;
		if (_s == 0)
			r() = g() = b() = _v;
		else
		{
			int z = _v * (255 - _s) / 255;
			int h_ = _h * 1000 / 60;
			int f = h_ - _h / 60 * 1000;	// 0-999
			int q = _v * ( 255 - _s * f / 999 ) / 255;
			int t = _v * ( 255 - _s * ( 999 - f ) / 999 ) / 255;
			if (h_ < 1000)
			{	r() = _v; g() = t; b() = z; }
			else if (h_ < 2000)
			{	r() = q; g() = _v; b() = z; }
			else if (h_ < 3000)
			{	r() = z; g() = _v; b() = t; }
			else if (h_ < 4000)
			{	r() = z; g() = q; b() = _v; }
			else if (h_ < 5000)
			{	r() = t; g() = z; b() = _v; }
			else
			{	r() = _v; g() = z; b() = q; }
		}
	}

	friend inline std::ostream& operator<<(std::ostream& _o, Color const& _this) { return _o << "#" << std::hex << std::setw(2) << std::setfill('0') << (int)_this.r() << std::setw(2) << std::setfill('0') << (int)_this.g() << std::setw(2) << std::setfill('0') << (int)_this.b() << std::dec; }

private:
	union
	{
		uint32_t value;
		struct
		{
			uint8_t s;
			uint8_t v;
			uint16_t h;
		} hsv;
		struct
		{
			uint8_t r;
			uint8_t g;
			uint8_t b;
			uint8_t a;
			uint8_t value() const { return uint8_t((unsigned(r) + unsigned(g) + unsigned(b)) / 3); }
		} rgb;
	} m_data;
};

static const Color NullColor;

/// Nearest hue/saturation to the given collection of h/s pairs.
inline unsigned nearest(std::vector< std::pair<int, int> > const& _p, int _h = -1, int _s = -1)
{
	int bestd = 0;
	int besti = -1;
	for (unsigned i = 0; i < _p.size(); ++i)
	{
		int ds =  (std::abs(_s - int(_p[i].second))) / 3;
		int dh = (180 - std::abs(std::abs(_h - int(_p[i].first)) - 180)) * _s / 255;
		int d = ((_s > -1) ? ds * ds : 0) + ((_h > -1) ? dh * dh : 0);
		if (besti < 0 || bestd > d)
			besti = i, bestd = d;
	}
	return besti;
}

std::vector< std::pair<int, int> > extractHS(std::vector<Lightbox::Color> const& _c);

std::vector<uint8_t> gammaTable(float _g);

template <unsigned _GammaX10>
uint8_t gamma(uint8_t _x)
{
	static std::vector<uint8_t> s_ret;
	if (s_ret.empty())
		s_ret = gammaTable(float(_GammaX10) / 10.f);
	return s_ret[_x];
}

typedef std::vector<Color> Colors;

static const Colors NullColors;
static const Colors RedOnly = { Color(255, 0, 0) };
static const Colors GreenOnly = { Color(0, 255, 0) };
static const Colors BlueOnly = { Color(0, 0, 255) };

}
