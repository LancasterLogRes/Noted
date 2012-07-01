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

struct fftwf_plan_s;

namespace Lightbox
{

class FFTW
{
public:
	explicit FFTW(unsigned _arity);
	~FFTW();

	unsigned arity() const { return m_arity; }
	unsigned bands() const { return m_arity / 2 + 1; }
    float* in() const { return m_in; }
    std::vector<float> const& mag() const { return m_mag; }
    std::vector<float> const& phase() const { return m_phase; }

	void process();

private:
	unsigned m_arity;
	float* m_in;
    float* m_work;
    std::vector<float> m_mag;
    std::vector<float> m_phase;
    fftwf_plan_s* m_plan;
};

}
