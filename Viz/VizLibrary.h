#pragma once

#include <unordered_map>
#include <string>
#include <functional>
#include <Common/Global.h>

#include "VizImpl.h"

namespace lb
{
typedef std::unordered_map<std::string, std::function<VizImpl*()> > VizFactories;
}

#define LIGHTBOX_VIZ_LIBRARY_HEADER(TARGET) \
	extern "C" __attribute__ ((visibility ("default"))) lb::VizFactories const& vizFactories##TARGET();

#define LIGHTBOX_VIZ_LIBRARY(TARGET) \
	LIGHTBOX_FINALIZING_LIBRARY \
	extern "C" __attribute__ ((visibility ("default"))) lb::VizFactories const& vizFactories##TARGET() { static lb::VizFactories s_ret; return s_ret; } \
	static_assert(lb::static_strcmp(LIGHTBOX_BITS_STRINGIFY(LIGHTBOX_TARGET_NAME), #TARGET), "Value passed to LIGHTBOX_VIZ_LIBRARY(_HEADER) (" #TARGET ") must match QMake's TARGET (" LIGHTBOX_BITS_STRINGIFY(LIGHTBOX_TARGET_NAME) ").")

#define LIGHTBOX_VIZ(D) \
	__attribute__ ((used)) auto g_reg ## D = (const_cast<lb::VizFactories&>(LIGHTBOX_BITS_TARGETIFY(vizFactories)())[#D] = [](){return new D;})

#define LIGHTBOX_VIZ_TYPEDEF(D, T) \
	__attribute__ ((used)) auto g_reg ## D = (const_cast<lb::VizFactories&>(LIGHTBOX_BITS_TARGETIFY(vizFactories)())[#D] = [](){return new T;})
