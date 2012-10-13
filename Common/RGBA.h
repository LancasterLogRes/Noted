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

class RGBA
{
public:
	RGBA() { m_a = 255; }
	explicit RGBA(uint8_t _val) { m_r = _val; m_g = _val; m_b = _val; }
	RGBA(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 255) { m_r = _r; m_g = _g; m_b = _b; m_a = _a; }
	RGBA(RGBA const& _s) { m_r = _s.m_r; m_g = _s.m_g; m_b = _s.m_b; m_a = _s.m_a; }
	RGBA& operator=(RGBA const& _s) { m_r = _s.m_r; m_g = _s.m_g; m_b = _s.m_b; m_a = _s.m_a; return *this; }

	uint8_t* data() { return &m_r; }
	uint8_t const* data() const { return &m_r; }
	uint8_t r() const { return m_r; }
	uint8_t g() const { return m_g; }
	uint8_t b() const { return m_b; }
	uint8_t a() const { return m_a; }
	uint8_t& r() { return m_r; }
	uint8_t& g() { return m_g; }
	uint8_t& b() { return m_b; }
	uint8_t& a() { return m_a; }
	void setR(uint8_t _r) { m_r = _r; }
	void setG(uint8_t _g) { m_g = _g; }
	void setB(uint8_t _b) { m_b = _b; }
	void setA(uint8_t _a) { m_a = _a; }
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
	static RGBA fromHsv(unsigned _h, uint8_t _s, uint8_t _v) { RGBA c; c.setHsv(_h, _s, _v); return c; }
	static RGBA fromHsp(unsigned _h, uint8_t _s, uint8_t _v) { RGBA c; c.setHsp(_h, _s, _v); return c; }
	void setHsp(unsigned _h, uint8_t _s, uint8_t _p, uint8_t _a = 255) { setHsv(_h, _s, hueCorrection(_h) * _p, _a); }

	void setHsv(unsigned _h, uint8_t _s, uint8_t _v, uint8_t _a = 255)
	{
		m_a = _a;
		if (_s == 0)
			m_r = m_g = m_b = _v;
		else
		{
			int z = _v * (255 - _s) / 255;
			int h_ = _h * 1000 / 60;
			int f = h_ - _h / 60 * 1000;	// 0-999
			int q = _v * ( 255 - _s * f / 999 ) / 255;
			int t = _v * ( 255 - _s * ( 999 - f ) / 999 ) / 255;
			if (h_ < 1000)
			{	m_r = _v; m_g = t; m_b = z; }
			else if (h_ < 2000)
			{	m_r = q; m_g = _v; m_b = z; }
			else if (h_ < 3000)
			{	m_r = z; m_g = _v; m_b = t; }
			else if (h_ < 4000)
			{	m_r = z; m_g = q; m_b = _v; }
			else if (h_ < 5000)
			{	m_r = t; m_g = z; m_b = _v; }
			else
			{	m_r = _v; m_g = z; m_b = q; }
		}
	}

	friend inline std::ostream& operator<<(std::ostream& _o, RGBA const& _this) { return _o << "#" << std::hex << std::setw(2) << std::setfill('0') << (int)_this.r() << std::setw(2) << std::setfill('0') << (int)_this.g() << std::setw(2) << std::setfill('0') << (int)_this.b() << std::dec; }

private:
	uint8_t m_r;
	uint8_t m_g;
	uint8_t m_b;
	uint8_t m_a;
};

LIGHTBOX_API std::vector<uint8_t> gammaTable(float _g);

template <unsigned _GammaX10>
uint8_t gamma(uint8_t _x)
{
	static std::vector<uint8_t> s_ret;
	if (s_ret.empty())
		s_ret = gammaTable(float(_GammaX10) / 10.f);
	return s_ret[_x];
}

typedef std::vector<RGBA> RGBAs;

}
