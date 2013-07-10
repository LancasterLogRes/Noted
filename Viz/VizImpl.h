#pragma once

#include <vector>
#include <Common/MemberCollection.h>
#include <Compute/Compute.h>

namespace lb
{

class VizImpl
{
public:
	typedef VizImpl LIGHTBOX_PROPERTIES_BaseClass;

	virtual lb::MemberMap propertyMap() const;
	virtual void onPropertiesChanged() {}

	virtual std::vector<lb::GenericCompute> tasks() { return {}; }

	virtual void initializeGL() {}
	virtual void prepGL() {}
	virtual void paintGL(Time _sinceLast) { (void)_sinceLast; }
};

}
