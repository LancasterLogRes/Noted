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
using namespace std;

#include "Maths.h"
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
			ret[i] = .54f - .46f * cos(2.f * M_PI * float(i) / float(_size - 1));
		break;
	case HannWindow:
		for (unsigned i = 0; i < _size; ++i)
			ret[i] = .5f * (1.f - cos(2.f * M_PI * float(i) / float(_size - 1)));
		break;
	case TukeyWindow:
	{
		float a = _parameter;//0.5f;
		float OmaNo2 = (1.f - a) * _size / 2.f;

		for (unsigned i = 0; i < _size / 2; ++i)
			ret[_size - 1 - i] = ret[i] = (i < OmaNo2) ? .5f * (1.f - cos(M_PI * float(i) / OmaNo2)) : 1.f;
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
		float const pa = M_PI * a;
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
			ret[i] = a0 - a1 * cos(2.f * M_PI * float(i) / (_size - 1)) + a2 * cos(4.f * M_PI * float(i) / (_size - 1));
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
