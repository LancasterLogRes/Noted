#include <QtQuick>
#include <Common/Global.h>
#include <EventCompiler/GraphSpec.h>
#include "Global.h"
#include "TimelineItem.h"
using namespace std;
using namespace lb;

TimelineItem::TimelineItem(QQuickItem* _p): QQuickItem(_p)
{
	setClip(true);
	setFlag(ItemHasContents, true);
	connect(this, &TimelineItem::offsetChanged, this, &TimelineItem::update);
	connect(this, &TimelineItem::pitchChanged, this, &TimelineItem::update);
	cnote << "Created TimelineItem" << (void*)this;
}

TimelineItem::~TimelineItem()
{
	cdebug << "Killing TimelineItem" << (void*)this;
}
