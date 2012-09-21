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

#include <vector>
#include "Maths.h"
using namespace std;
using namespace Lightbox;

std::vector<float> Lightbox::windowFunction(unsigned _size, WindowFunction _f, float _parameter)
{
	std::vector<float> ret(_size);
	switch (_f)
	{
	case RectangularWindow:
		for (unsigned i = 0; i < _size; ++i)
			ret[i] = 1.f;
		break;
	case HammingWindow:
		for (unsigned i = 0; i < _size; ++i)
            ret[i] = .54f - .46f * cos(2.f * Pi * float(i) / float(_size - 1));
		break;
	case HannWindow:
		for (unsigned i = 0; i < _size; ++i)
            ret[i] = .5f * (1.f - cos(2.f * Pi * float(i) / float(_size - 1)));
		break;
	case TukeyWindow:
	{
		float a = _parameter;//0.5f;
		float OmaNo2 = (1.f - a) * _size / 2.f;

		for (unsigned i = 0; i < _size / 2; ++i)
            ret[_size - 1 - i] = ret[i] = (i < OmaNo2) ? .5f * (1.f - cos(Pi * float(i) / OmaNo2)) : 1.f;
		break;
	}
	case GaussianWindow:
	{
		float o = _parameter;//0.4f;
		float const Nm1o2 = (_size - 1) / 2.f;
		float const oNm1o2 = o * Nm1o2;
		for (unsigned i = 0; i < _size; ++i)
			ret[i] = exp(-.5f * sqr(((float)i - Nm1o2) / (oNm1o2)));
		break;
	}
	case KaiserWindow:
	{
		float a = _parameter;//3.f;
        float const pa = Pi * a;
		float const ToNm1 = 2.f / float(_size - 1);
		float const jpa = io(pa);
		for (unsigned i = 0; i < _size / 2; ++i)
			ret[_size / 2 - 1 - i] = ret[_size / 2 + i] = io(pa * sqrt(1.f - sqr(ToNm1 * (float)i))) / jpa;
		 break;
	}
	case BlackmanWindow:
	{
		float a = _parameter;//0.16f;
		float a0 = (1.f - a) / 2.f;
		float a1 = .5f;
		float a2 = a / 2.f;
		for (unsigned i = 0; i < _size; ++i)
            ret[i] = a0 - a1 * cos(2.f * Pi * float(i) / (_size - 1)) + a2 * cos(4.f * Pi * float(i) / (_size - 1));
		break;
	}
	default:;
	}
	return ret;
}

std::vector<float> Lightbox::zeroPhaseWindow(std::vector<float> const& _w)
{
	std::vector<float> ret(_w.size());
	unsigned off = (_w.size() + 1) / 2;
	for (unsigned i = 0; i < _w.size(); ++i)
		ret[i] = _w[(i + off) % _w.size()];
	return ret;
}

std::vector<float> Lightbox::scaledWindow(std::vector<float> const& _w, float _f)
{
	std::vector<float> ret(_w.size());
	for (unsigned i = 0; i < _w.size(); ++i)
		ret[i] = _w[i] * _f;
	return ret;
}

vector<float> Lightbox::solveQuadratic(float a, float b, float c)
{
	float disc = b*b - 4*a*c;
	if (disc < 0)
		return vector<float>();
	else if (disc == 0)
		return vector<float>({{ -b / (2*a) }});
	return vector<float>({{(-b + sqrt(disc)) / (2*a), (-b - sqrt(disc)) / (2*a)}});
}

vector<float> Lightbox::solveCubic(float a, float b, float c, float d)
{
	// a is too small - go to quadratic solver
	if (fabs(a) < .0001)
		return solveQuadratic(b, c, d);

	if (fabs(d) < .0001)
		return vector<float>(1, 0.f);

	b /= a;
	c /= a;
	d /= a;

	float q = (3*c - b*b) / 9;
	float r = -27*d + b*(9*c - 2*b*b);
	r /= 54;
	float disc = q*q*q + r*r;

	float term1 = b / 3;

	if (disc > 0)
	{
		float s = r + sqrt(disc);
		s = (s < 0) ? -pow(-s, 1/3.f) : pow(s, 1/3.f);
		float t = r - sqrt(disc);
		t = (t < 0) ? -pow(-t, 1/3.f) : pow(t, 1/3.f);
		return vector<float>(1, -term1 + s + t);
	}

	// The remaining options are all real
	if (disc == 0)
	{
		// All roots real, at least two are equal.
		float r13 = (r < 0) ? -pow(-r, 1/3.f) : pow(r, 1/3.f);
		float ret1 = -term1 + 2*r13;
		float ret2 = -(r13 + term1);
		return vector<float>({{ ret1, ret2 }});
	}

	// Only option left is that all roots are real and unequal (to get here, q < 0)
	q = -q;
	float dum1 = q*q*q;
	dum1 = acos(r / sqrt(dum1));
	float r13 = 2 * sqrt(q);
	float ret1 = -term1 + r13 * cos(dum1 / 3);
	float ret2 = -term1 + r13 * cos((dum1 + 2.0*Pi) / 3);
	float ret3 = -term1 + r13 * cos((dum1 + 4.0*Pi) / 3);
	return vector<float>({{ ret1, ret2, ret3 }});
}

static float biasHelper(float _x, float _z)
{
	_x = clamp(_x, -1.f, 2.f);
	_z = clamp(_z, -1.f, 1.f);
	if (_x > 1.f)
		return clamp(2.f - biasHelper(2.f - _x, _z), -.25f, 1.25f);
	if (_x == 0.f)
		return 0.f;
	if (_x == -1.f)
		return -1.f;
	if (_x == 1.f)
		return 1.f;
	if (_z == 0.f)
		return _x;
	if (_x < 0.f)
		return clamp(-biasHelper(-_x, _z), -.25f, 1.25f);
	if (_z < 0.f)
		return 1.f - biasHelper(1.f - _x, -_z);
	auto ts = solveCubic(3 * _z - 2, 3 - 3 * _z, 0, -_x);
	for (auto t: ts)
		if (t >= 0.f && t <= 1.f)
		{
			float ret = cubicBezierT(t, _z);
			if (ret >= 0.f && ret <= 1.f)
				return ret;
		}
	for (auto t: ts)
	{
		float ret = cubicBezierT(t, _z);
		if (ret >= 0.f && ret <= 1.f)
			return ret;
	}
	return numeric_limits<float>::infinity();
}

/// Bias an x == y curve by some amount _z.
float Lightbox::bias(float _x, float _z)
{
	return biasHelper(_x + .25 * _z, _z);
}
