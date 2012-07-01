#pragma once

#include <QString>

#include <Faceman/Driver.h>

#include "CoreItem.h"

class DriverItem: public CoreItem
{
public:
	DriverItem(Lightbox::Driver const& _d);

	Lightbox::Driver const& driver() const { return m_d; }

	QString name() const;
	void restore();
	void save();
	virtual void paint(QPainter* _p, QStyleOptionGraphicsItem const*, QWidget*);
	virtual QPointF port() const;

	virtual QPointF sep();
	static void reorder(QGraphicsItem* _g);
	virtual QRectF boundingRect() const;

private:
	QString m_savedName;

	Lightbox::Driver m_d;
};
