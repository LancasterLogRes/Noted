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

#include <map>
#include <vector>
#include <string>
#include <utility>
#include <functional>
#include <sstream>
#include <iomanip>
#include <QBrush>
#include <QRect>

class QPainter;

namespace lb
{

template <int _SF>
inline std::string IdentityLabel(float _f)
{
	std::stringstream ss;
	ss << std::setprecision(_SF) << _f;
	return ss.str();
}

template <int _SFx, int _SFy>
inline std::string IdentityLabel(float _x, float _y)
{
	std::stringstream ss;
	ss << "(" << std::setprecision(_SFx) << _x << ", " << std::setprecision(_SFy) << _y << ")";
	return ss.str();
}

class Grapher
{
public:
	Grapher();
	void init(QPainter* _p, std::pair<float, float> _xRange, std::pair<float, float> _yRange, std::function<std::string(float _f)> _xLabel = IdentityLabel<2>, std::function<std::string(float _f)> _yLabel = IdentityLabel<2>, std::function<std::string(float, float)> _pLabel = IdentityLabel<2, 2>, int _leftGutter = 30, int _bottomGutter = 16);
	void init(QPainter* _p, std::pair<float, float> _xRange, std::pair<float, float> _yRange, std::function<std::string(float _f)> _xLabel, std::function<std::string(float _f)> _yLabel, std::function<std::string(float, float)> _pLabel, QRect _active)
	{
		p = _p;
		active = _active;
		xRange = _xRange;
		yRange = _yRange;
		dx = xRange.second - xRange.first;
		dy = yRange.second - yRange.first;
		xLabel = _xLabel;
		yLabel = _yLabel;
		pLabel = _pLabel;
	}

	void setDataTransform(float _xM, float _xC, float _yM, float _yC)
	{
		xM = _xM;
		xC = _xC;
		yM = _yM;
		yC = _yC;
	}
	void setDataTransform(float _xM, float _xC)
	{
		xM = _xM;
		xC = _xC;
		yM = 1.f;
		yC = 0.f;
	}
	void resetDataTransform() { xM = yM = 1.f; xC = yC = 0.f; }

	bool drawAxes(bool _x = true, bool _y = true) const;
	void drawLineGraph(std::vector<float> const& _data, QColor _color = QColor(128, 128, 128), QBrush const& _fillToZero = Qt::NoBrush, float _width = 0.f) const;
	void drawLineGraph(std::function<float(float)> const& _f, std::function<QColor(float)> const& _color, QBrush const& _fillToZero = Qt::NoBrush, float _width = 0.f) const;
	void drawLineGraph(std::function<float(float)> const& _f, QColor _color = QColor(128, 128, 128), QBrush const& _fillToZero = Qt::NoBrush, float _width = 0.f) const { return drawLineGraph(_f, [=](float){return _color;}, _fillToZero, _width); }
	void ruleY(float _x, QColor _color = QColor(128, 128, 128), float _width = 0.f) const;
	void labelYOrderedPoints(std::map<float, float> const& _translatedData, int _maxCount = 20, float _minFactor = .01f) const;

protected:
	QPainter* p;
	QRect active;
	std::pair<float, float> xRange;
	std::pair<float, float> yRange;

	float xM;
	float xC;
	float yM;
	float yC;

	float dx;
	float dy;

	std::function<std::string(float _f)> xLabel;
	std::function<std::string(float _f)> yLabel;
	std::function<std::string(float _x, float _y)> pLabel;

	// Translate from raw indexed data into x/y graph units. Only relevant for indexed data.
	float xT(float _dataIndex) const { return _dataIndex * xM + xC; }
	float yT(float _dataValue) const { return _dataValue * yM + yC; }
	// Translate from x/y graph units to widget pixels.
	int xP(float _xUnits) const { return active.left() + (_xUnits - xRange.first) / dx * active.width(); }
	int yP(float _yUnits) const { return active.bottom() - (_yUnits - yRange.first) / dy * active.height(); }
	QPoint P(float _x, float _y) const { return QPoint(xP(_x), yP(_y)); }
	// Translate direcly from raw indexed data to widget pixels.
	int xTP(float _dataIndex) const { return active.left() + (xT(_dataIndex) - xRange.first) / dx * active.width(); }
	int yTP(float _dataValue) const { return active.bottom() - (yT(_dataValue) - yRange.first) / dy * active.height(); }
	// Translate back from pixels into graph units.
	float xU(int _xPixels) const { return (_xPixels - active.left() + .5f) / active.width() * dx + xRange.first; }
	// Translate back from graph units into raw data index.
	float xR(float _xUnits) const { return (_xUnits - xC) / xM; }
	// Translate directly from pixels into raw data index. xRU(xTP(X)) == X
	float xRU(int _xPixels) const { return xR(xU(_xPixels)); }
};

}
