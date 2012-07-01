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

#include <sstream>
#include <string>
#include <iostream>

namespace Lightbox
{

template <int _I>
struct UnitTest
{
	constexpr static const char* name = 0;
	static void test() {}
};

static unsigned g_unitTestPassCount = 0;

template <int _I>
struct UnitTesting
{
	static bool go()
	{
		bool ok = true;
		if (UnitTest<_I>::name)
		{
			std::cout << "Testing " << UnitTest<_I>::name << "... " << std::flush;
			try
			{
				g_unitTestPassCount = 0;
				UnitTest<_I>::test();
				std::cout << g_unitTestPassCount << " PASSED OK" << std::endl;
			}
			catch (std::string _cond)
			{
				std::cout << "FAIL: " << _cond << std::endl;
				ok = false;
			}
			catch (...) {}
		}
		return UnitTesting<_I - 1>::go() && ok;
	}
};

template <>
struct UnitTesting<-1>
{
	static bool go() { return true; }
};

struct UnitTestBase
{
	template <class _U, class _V> static void requireEqual(_U _u, _V _v, char const* _desc)
	{
		++g_unitTestPassCount;
		if (!(_u == _v))
		{
			std::stringstream ss;
			ss << _desc << " !(" << _u << " == " << _v << ")";
			throw ss.str();
		}
	}
	template <class _U, class _V> static void requireLess(_U _u, _V _v, char const* _desc)
	{
		++g_unitTestPassCount;
		if (!(_u < _v))
		{
			std::stringstream ss;
			ss << _desc << " !(" << _u << " < " << _v << ")";
			throw ss.str();
		}
	}
	template <class _U, class _V> static void requireApproximates(_U _u, _V _v, char const* _desc)
	{
		++g_unitTestPassCount;
		if (!_u.approximates(_v))
		{
			std::stringstream ss;
			ss << _desc << " !(" << _u << " ~= " << _v << ")";
			throw ss.str();
		}
	}
	static void require(bool _cond, char const* _desc)
	{
		++g_unitTestPassCount;
		if (!_cond)
			throw std::string(_desc);
	}
	static void fail(std::string const& _problem)
	{
		throw _problem;
	}
};

}

#define LIGHTBOX_UNITTEST(_ID, _Name) \
	template <> struct UnitTest<_ID>: public UnitTestBase \
	{ \
		constexpr static const char* name = _Name; \
		inline static void test(); \
	}; \
	void UnitTest<_ID>::test()

#define LIGHTBOX_UNITTEST_DECLARE(_ID, _Name) \
	template <> struct UnitTest<_ID>: public UnitTestBase \
	{ \
		constexpr static const char* name = _Name; \
		static void test(); \
	};

#define LIGHTBOX_UNITTEST_DEFINE(_ID) \
	void Lightbox::UnitTest<_ID>::test()

#define LIGHTBOX_REQUIRE(_cond) require(_cond, #_cond)
#define LIGHTBOX_REQUIRE_EQUAL(_u, _v) requireEqual(_u, _v, #_u " == " #_v)
#define LIGHTBOX_REQUIRE_LESS(_u, _v) require(_u, _v, #_u " < " #_v)
#define LIGHTBOX_REQUIRE_APPROXIMATES(_u, _v) requireApproximates(_u, _v, #_u " ~= " #_v)
