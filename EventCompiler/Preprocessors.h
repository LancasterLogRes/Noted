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

#include <algorithm>
#include <functional>
#include <tuple>
#include <deque>
#include <vector>
#include <iostream>
#include <array>
#include <boost/circular_buffer.hpp>
#include <Common/Common.h>
#include <EventCompiler/EventCompilerImpl.h>
using namespace std;
using boost::circular_buffer;
using namespace Lightbox;

namespace Lightbox
{
/*

	vector<float> lastPhase;
	vector<float> deltaPhase;

: phaseEntropyAvg(12),

deltaPhase.resize(bands(), -1);
lastPhase.resize(bands(), -1);
phaseEntropyAvg.init();
phaseEntropyC.setHistory(c_historySize, downsampleST);
*/
/*
		float phaseEntropy = 0.f;
		if (lastPhase[0] == -1)
			lastPhase = _phase;
		else if (deltaPhase[0] == -1)
			for (unsigned i = 0; i < bands(); ++i)
				deltaPhase[i] = withReflection(abs(_phase[i] - lastPhase[i]));
		else
		{
			for (unsigned i = 0; i < bands(); ++i)
			{
				float dp = withReflection(abs(_phase[i] - lastPhase[i]));
				lastPhase[i] = _phase[i];
				phaseEntropy += max<float>(0, withReflection(abs(dp - deltaPhase[i])) - HalfPi);
				deltaPhase[i] = dp;
			}
			ret.push_back(StreamEvent(Graph, phaseEntropy, 0.08f));
		}
		phaseEntropyAvg.push(phaseEntropy);
*/

class PhaseUnity
{
public:
	void init(EventCompilerImpl*);
	void execute(EventCompilerImpl* _eci, Time _t, vector<float> const&, vector<float> const& _phase, std::vector<float> const&);

	float get() const { return m_last; }
	bool changed() const { return true; }

private:
	deque<float> m_buffer;
	float m_last;
};

class HighEnergy
{
public:
	void init(EventCompilerImpl*) {}
	void execute(EventCompilerImpl* _eci, Time, vector<float> const& _mag, vector<float> const&, std::vector<float> const&)
	{
		unsigned b = _eci->bands();
		m_last = 0.f;
		for (unsigned j = 0; j < b; ++j)
			m_last += (j + 1) * _mag[j];
		m_last = sqr(m_last / (b * b / 2)) * 100;
	}

	float get() const { return m_last; }
	bool changed() const { return true; }

private:
	float m_last;
};

class Harmony
{
public:
	void init(EventCompilerImpl* _eci)
	{
		m_fftw = shared_ptr<FFTW>(new FFTW(_eci->bands() - 1));
		m_wF = windowFunction(m_fftw->arity(), HannWindow);

	}
	void execute(EventCompilerImpl*, Time, vector<float> const& _mag, vector<float> const&, std::vector<float> const&)
	{
		unsigned arity = m_fftw->arity();
		for (unsigned i = 0; i < arity; ++i)
			m_fftw->in()[i] = _mag[(i + arity / 2) % arity] * m_wF[(i + arity / 2) % arity];
		m_fftw->process();

		vector<float> const& mag = m_fftw->mag();
		if (m_last.size() == mag.size())
			m_d = packCombine(mag.begin(), m_last.begin(), mag.size(), [](v4sf& ret, v4sf const& a, v4sf const& b) { ret = ret + (a - b); }, [&](float const* c) { return (c[0] + c[1] + c[2] + c[3]) / mag.size() / 1000; });
		else
			m_d = 0;
		m_last = mag;
	}

	float get() const { return m_d; }
	bool changed() const { return true; }

private:
	std::shared_ptr<FFTW> m_fftw;
	std::vector<float> m_wF;
	std::vector<float> m_last;
	float m_d;
};

class LowEnergy
{
public:
	void init(EventCompilerImpl*) {}
	void execute(EventCompilerImpl* _eci, Time, vector<float> const& _mag, vector<float> const&, std::vector<float> const&)
	{
		unsigned b = _eci->bands();
		m_last = 0.f;
		for (unsigned j = 0; j < b; ++j)
			m_last += _mag[j] / max(1, int(j) - 5);
		m_last = sqr(m_last) * (b * b / 2) / 10000000;
	}

	float get() const { return m_last; }
	bool changed() const { return true; }

private:
	float m_last;
};

class Energy
{
public:
	void init(EventCompilerImpl*) {}
	void execute(EventCompilerImpl* _eci, Time, vector<float> const& _mag, vector<float> const&, std::vector<float> const&)
	{
		unsigned b = _eci->bands();
		m_last = 0.f;
		for (unsigned j = 0; j < b; ++j)
			m_last += _mag[j];
		m_last /= b;
	}

	float get() const { return m_last; }
	bool changed() const { return true; }

private:
	float m_last;
};

class DeltaEnergy
{
public:
	void init(EventCompilerImpl*)
	{
		m_lastMag.clear();
	}

	void execute(EventCompilerImpl* _eci, Time, vector<float> const& _mag, vector<float> const&, std::vector<float> const&)
	{
		unsigned b = _eci->bands();
		m_last = 0.f;
		if (m_lastMag.size())
		{
			for (unsigned j = 0; j < b; ++j)
				m_last += abs(_mag[j] - m_lastMag[j]);
			m_last *= 10.f / b;
		}
		m_lastMag = _mag;
	}

	bool changed() const { return true; }

	float get() const { return m_last; }

private:
	float m_last;
	vector<float> m_lastMag;
};

template <class _PP, unsigned _factorX1000000 = 950000>
class Decayed: public _PP
{
public:
	Decayed(float _factor = _factorX1000000 / 1000000.f): m_factor(_factor), m_acc(0.f) {}
	Decayed& setFactor(float _factor) { m_factor = _factor; m_acc = 0.f; return *this; }
	void init(EventCompilerImpl* _eci)
	{
		_PP::init(_eci);
		setFactor(m_factor);
	}
	void execute(EventCompilerImpl* _eci, Time _t, vector<float> const& _mag, vector<float> const& _phase, std::vector<float> const& _wave)
	{
		_PP::execute(_eci, _t, _mag, _phase, _wave);
		m_acc = max<float>(m_acc * m_factor, _PP::get());
	}

    using _PP::changed;

	float get() const { return m_acc; }

private:
	float m_factor;
	float m_acc;
};

template <class _PP, unsigned _df = 1u>
class Downsampled: public _PP
{
public:
	Downsampled(unsigned _f = _df): m_factor(_f), m_acc(0.f), m_count(_f) {}

	Downsampled& setDownsample(unsigned _f) { m_factor = _f; m_acc = 0.f; m_count = _f; return *this; }

	void init(EventCompilerImpl* _eci)
	{
		_PP::init(_eci);
		setDownsample(m_factor);
	}

	void execute(EventCompilerImpl* _eci, Time _t, vector<float> const& _mag, vector<float> const& _phase, std::vector<float> const& _wave)
	{
		_PP::execute(_eci, _t, _mag, _phase, _wave);
		if (m_count == m_factor)
		{
			m_acc = _PP::get();
			m_count = 1;
		}
		else
		{
			m_acc += _PP::get();
			++m_count;
		}
	}

	float get() const { return m_acc / m_count; }
	bool changed() const { return m_count == m_factor; }

private:
	unsigned m_factor;
	float m_acc;
	unsigned m_count;
};

// TODO: make 4 longer than necessary and put acc there.
// TODO: look into foreign-owned container, so we can pass back the vector with 4 fewer at end.

template <class _T>
class foreign
{
public:
	foreign(): m_data(nullptr), m_size(0) {}
	foreign(_T const* _data, unsigned _size): m_data(_data), m_size(_size) {}

	LIGHTBOX_STRUCT_INTERNALS_2(foreign, _T const*, m_data, unsigned, m_size)

	_T const* data() const { return m_data; }
	unsigned size() const { return m_size; }

private:
	_T const* m_data;
	unsigned m_size;
};

template <class _PP, unsigned _ds = 8>
class Historied: public _PP
{
public:
	Historied(unsigned _s = _ds): m_data(_s) {}

	Historied& setHistory(unsigned _s) { m_data.clear(); m_data.resize(_s + 4, 0); m_count = 0; return *this; }

	void init(EventCompilerImpl* _eci)
	{
		_PP::init(_eci);
		setHistory(m_data.size());
	}

	void resetBefore(unsigned _bins)
	{

	}

	void execute(EventCompilerImpl* _eci, Time _t, vector<float> const& _mag, vector<float> const& _phase, std::vector<float> const& _wave)
	{
		_PP::execute(_eci, _t, _mag, _phase, _wave);
		if (_PP::changed())
		{
			if (m_count == 4)
			{
				memmove(m_data.data(), m_data.data() + 4, (m_data.size() - 4) * sizeof(float));
				m_count = 0;
			}
			m_data[m_data.size() - 4 + m_count] = _PP::get();
			++m_count;
		}
	}

	bool changed() const { return _PP::changed() && m_count == 4; }
	foreign<float> get() const { return foreign<float>(m_data.data() + 4, m_data.size() - 4); }
	vector<float> const& getVector() const { return m_data; }	// note - offset by 4.

private:
	vector<float> m_data;
	unsigned m_count;
};

template <class _PP, class _X>
class Crossed: public _PP
{
public:
	void init(EventCompilerImpl* _eci)
	{
		_PP::init(_eci);
		m_lastAc.clear();
	}
	void execute(EventCompilerImpl* _eci, Time _t, vector<float> const& _mag, vector<float> const& _phase, std::vector<float> const& _wave)
	{
		_PP::execute(_eci, _t, _mag, _phase, _wave);
		if (_PP::changed())
		{
			vector<float> const& h = _PP::getVector();
			assert(isFinite(h[0]));
			unsigned s = h.size() - 4;
			if (m_lastAc.empty())
				m_lastAc.resize(s / 4, 0.0001);
			autocross(h.begin(), s, _X::call, s / 4, 4, m_lastAc);
			assert(isFinite(m_lastAc[0]));
		}
	}

	vector<float> const& get() const { return m_lastAc; }
    using _PP::changed;

	unsigned best() const
	{
		unsigned ret = 2;
		for (unsigned i = 3; i < m_lastAc.size(); ++i)
			if (m_lastAc[ret] < m_lastAc[i])
				ret = i;
		return ret;
	}

private:
	vector<float> m_lastAc;
};

struct CallSimilarity { inline static float call(vector<float>::const_iterator _a, vector<float>::const_iterator _b, unsigned _s) { return similarity(_a, _b, _s); } };
struct CallCorrelation { inline static float call(vector<float>::const_iterator _a, vector<float>::const_iterator _b, unsigned _s) { return correlation(_a, _b, _s); } };
struct CallDissimilarity { inline static float call(vector<float>::const_iterator _a, vector<float>::const_iterator _b, unsigned _s) { return dissimilarity(_a, _b, _s); } };

/*
template <class _PP> using Correlated = Crossed<_PP, CallCorrelation>;
template <class _PP> using Similarity = Crossed<_PP, CallSimilarity>;
template <class _PP> using Dissimilarity = Crossed<_PP, CallDissimilarity>;
*/
template <class _PP> class Correlated: public Crossed<_PP, CallCorrelation> {};
template <class _PP> class Similarity: public Crossed<_PP, CallSimilarity> {};
template <class _PP> class Dissimilarity: public Crossed<_PP, CallDissimilarity> {};

template <class _PP>
class AreaNormalized: public _PP
{
public:
	AreaNormalized() {}

	void init(EventCompilerImpl* _eci)
	{
		_PP::init(_eci);
		m_data.clear();
	}
	void execute(EventCompilerImpl* _eci, Time _t, vector<float> const& _mag, vector<float> const& _phase, std::vector<float> const& _wave)
	{
		_PP::execute(_eci, _t, _mag, _phase, _wave);
		if (_PP::changed())
		{
			m_data = _PP::get();
			assert(isFinite(m_data[0]));
			normalize(m_data);
			assert(isFinite(m_data[0]));
			makeTotalUnit(m_data);
			assert(isFinite(m_data[0]));
		}
	}
    using _PP::changed;

	vector<float> const& get() const { return m_data; }

private:
	vector<float> m_data;
};

template <class _PP1, class _PP2>
class Subbed
{
	void init(EventCompilerImpl* _eci)
	{
		_PP1::init(_eci);
		_PP2::init(_eci);
		m_last = 0;
	}
	void execute(EventCompilerImpl* _eci, Time _t, vector<float> const& _mag, vector<float> const& _phase, std::vector<float> const& _wave)
	{
		_PP1::execute(_eci, _t, _mag, _phase, _wave);
		_PP2::execute(_eci, _t, _mag, _phase, _wave);
		if (_PP1::changed() || _PP2::changed())
			m_last = _PP1::get() - _PP2::get();
	}
	bool changed() const { return _PP1::changed() || _PP2::changed(); }

	typedef typename std::remove_const<typename std::remove_reference<decltype(_PP1().get())>::type>::type TT;

	TT const& get() const { return m_last; }

private:
	TT m_last;
};

template <class _PP>
class Teed
{
public:
	Teed(_PP* _tee = 0): m_tee(_tee) {}

	Teed& setTeed(_PP& _tee) { m_tee = &_tee; return *this; }

	void init(EventCompilerImpl*) {}
	void execute(EventCompilerImpl*, Time, vector<float> const&, vector<float> const&, std::vector<float> const&) {}
	decltype(_PP().get()) get() const { return m_tee->get(); }
	bool changed() const { return m_tee->changed(); }

private:
	_PP* m_tee;
};

template <class _PP>
class Deltad: public _PP
{
public:
	void init(EventCompilerImpl* _eci)
	{
		_PP::init(_eci);
		m_last = 0;
	}
	void execute(EventCompilerImpl* _eci, Time _t, vector<float> const& _mag, vector<float> const& _phase, std::vector<float> const& _wave)
	{
		_PP::execute(_eci, _t, _mag, _phase, _wave);
		if (_PP::changed())
		{
			m_delta = fabs(_PP::get() - m_last);
			m_last = _PP::get();
		}
	}
    using _PP::changed;

	float get() const { return m_delta; }

private:
	float m_last;
	float m_delta;
};

}
