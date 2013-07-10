#include <Common/Global.h>
#include "Global.h"
#include "Noted.h"
#include "ViewMan.h"
using namespace std;
using namespace lb;

void ViewMan::normalize()
{
	if (Noted::audio()->duration() && width())
		setParameters(Noted::audio()->duration() * -0.025, Noted::audio()->duration() / .95 / width());
	else
		setParameters(0, FromMsecs<1>::value);
}

Time ViewMan::globalOffset() const
{
	if (Noted::audio()->duration() && width())
		return Noted::audio()->duration() * -0.025;
	else
		return 0;
}

Time ViewMan::globalPitch() const
{
	if (Noted::audio()->duration() && width())
		return Noted::audio()->duration() / .95 / width();
	else
		return FromMsecs<1>::value;
}

void ViewMan::onGutterWidthChanged(int _gw)
{
	m_gutterWidth = _gw;
	emit gutterWidthChanged(_gw);
}
