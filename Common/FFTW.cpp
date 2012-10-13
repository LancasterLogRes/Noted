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

#include <cmath>
#include <unistd.h>
#include <mutex>
#include <fftw3.h>
#include "Maths.h"
#include "FFTW.h"
using namespace std;
using namespace Lightbox;

FFTW::FFTW(unsigned _arity): m_arity(_arity), m_plan(nullptr)
{
	static std::mutex m;
	m.lock();
	m_in = (float *)fftwf_malloc(sizeof(float) * m_arity);
	m_work = (float *)fftwf_malloc(sizeof(float) * m_arity);
	m_plan = fftwf_plan_r2r_1d(m_arity, m_in, m_work, FFTW_R2HC, FFTW_MEASURE);
	m.unlock();
	m_mag.resize(m_arity / 2 + 1);
	m_phase.resize(m_arity / 2 + 1);
}

FFTW::~FFTW()
{
	if (m_in) fftwf_free(m_in);
	if (m_work) fftwf_free(m_work);
	if (m_plan) fftwf_destroy_plan(m_plan);
}

void FFTW::process()
{
/*	for (uint i = 0; i < m_arity; i++)
		if (m_in[i] > 1.f || m_in[i] < -1.f)
		{
			cerr << "!!! Bad input data - " << m_in[i] << " outside unity. Clamping." << endl;
			m_in[i] = std::min(1.f, std::max(-1.f, m_in[i]));
		}*/
	fftwf_execute(m_plan);
	for (unsigned i = 0; i <= m_arity / 2; i++)
	{
		float re = (i == m_arity / 2) ? 0 : m_work[i];
		float im = i ? m_work[m_arity - i] : 0;
		float p = (re * re + im * im) / float(m_arity);
		float m = (isFinite(p) && p != 0.f) ? sqrt(p) : 0;
		m_mag[i] = m;
        m_phase[i] = atan2(re, im) + Pi;
        while (m_phase[i] >= Pi * 2)
            m_phase[i] -= Pi * 2;
/*		if (m_phase[i] < 0)
			cerr << "!!! Bad output phase " << m_phase[i] << endl;
		if (m_mag[i] < -1.f || m_mag[i] > 1.f)
			cerr << "!!! Bad output mag " << m_mag[i] << endl;*/
//		cerr << i << ": " << m_mag[i] << ", " << m_phase[i] << endl;
	}
}
