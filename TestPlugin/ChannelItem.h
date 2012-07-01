#pragma once

#include <QGraphicsRectItem>

namespace Lightbox { class Output; }

class ChannelItem: public QGraphicsRectItem
{
public:
	ChannelItem(Lightbox::Output& _o);

	Lightbox::Output const& o() { return m_o; }

	virtual void paint(QPainter* _p, QStyleOptionGraphicsItem const*, QWidget*);
	virtual bool sceneEvent(QEvent* _e);

private:
	Lightbox::Output& m_o;
};
