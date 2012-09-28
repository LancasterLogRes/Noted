#pragma once

#include "Global.h"

namespace Lightbox
{

/// Cyclic reflection.
inline float withReflection(float _x, float _r = Pi)
{
	return (_x > _r) ? 2 * _r - _x : (_x < -_r) ? 2 * -_r + _x : _x;
}

inline float canonAngle(float _x)
{
	return _x < 0 ? _x + TwoPi : _x >= TwoPi ? _x - TwoPi : _x;
}

inline float angularSubtraction(float _tha, float _thb)
{
	float dth = _tha - _thb;
	return (dth >= Pi) ? dth - TwoPi : (dth <= -Pi) ? dth + TwoPi : dth;
}

}
