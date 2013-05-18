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

