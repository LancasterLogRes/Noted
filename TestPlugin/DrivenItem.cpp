#include <QtGui>

#include <Common/Common.h>
using namespace Lightbox;

#include "DriverItem.h"
#include "OutputItem.h"
#include "DrivenItem.h"

DrivenItem::DrivenItem(DriverItem* _dw, OutputItem* _ow):
	m_dw(_dw),
	m_ow(_ow)
{
	setPen(QColor::fromHsvF(0, .85, .9));
	reposition();
}

void DrivenItem::connect()
{
	if (!m_dw->driver().isNull() && !m_ow->output().isNull()) m_dw->driver().addOutput(m_ow->output()); else delete this;
}

void DrivenItem::reposition()
{
	setLine(QLineF(m_ow->port(), m_dw->port()));
}

