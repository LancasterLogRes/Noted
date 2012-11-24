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

#include <cxxabi.h>
#include <cstring>
#include <string>
#include <algorithm>
#if LIGHTBOX_CROSSCOMPILATION_ANDROID
#include <android/log.h>
#endif
#include "Global.h"
using namespace std;
using namespace Lightbox;

namespace Lightbox
{

bool g_debugEnabled[256] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, true, true, true};

void simpleDebugOut(std::string const& _s, unsigned char _id)
{
	if (g_debugEnabled[_id])
#if LIGHTBOX_CROSSCOMPILATION_ANDROID
		(void)__android_log_print(_id == 255 ? ANDROID_LOG_ERROR : _id == 254 ? ANDROID_LOG_WARN : _id == 253 ? ANDROID_LOG_INFO : ANDROID_LOG_INFO, LIGHTBOX_BITS_STRINGIFY("Lightbox"), "%s\n", _s.c_str())
#else
        std::cout << ((_id == 255) ? "!!! " : (_id == 254) ? "*** " : (_id == 253) ? "--- " : "    ") << _s << std::endl << std::flush;
#endif
}

std::function<void(std::string const&, unsigned char)> g_debugPost = simpleDebugOut;

}

string Lightbox::afterComma(char const* _s, unsigned _i)
{
	while (_i && *_s)
		if (*(_s++) == ',')
			_i--;
	while (isspace(*_s))
		_s++;
	unsigned l = 0;
	while (_s[l] && _s[l] != ',' && !isspace(_s[l]) && _s[l] != '=')
		++l;
	return string(_s, l);
}

string Lightbox::demangled(char const* _name)
{
	int status;
	char* r = abi::__cxa_demangle(_name, 0, 0, &status);
	string ret = r;
	free(r);
	return ret;
}

string Lightbox::shortened(string const& _s)
{
	string ret;
	ret.reserve(_s.size());
	for (unsigned i = 0; i < _s.size(); ++i)
		if (isupper(_s[i]) || (i && isupper(_s[i - 1])))
			ret += _s[i];
	return ret;
}
