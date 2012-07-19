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

#include <iomanip>
#include "StreamIO.h"

using namespace std;
using namespace Lightbox;

std::string Lightbox::toString(double _d)
{
	std::stringstream ss;
	ss << setprecision(std::numeric_limits<double>::digits10) << _d;
	return ss.str();
}

std::string Lightbox::toString(float _f)
{
	std::stringstream ss;
	ss << setprecision(std::numeric_limits<float>::digits10) << _f;
	return ss.str();
}

void Lightbox::fromString(std::string& _t, std::string const& _s)
{
	if (_s.size() > 1 && _s[0] == '"' && _s[_s.size() - 1] == '"')
		_t = _s.substr(1, _s.size() - 2);
}
