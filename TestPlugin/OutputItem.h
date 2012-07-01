#pragma once

#include <QMutex>

#include <Faceman/Output.h>

#include "CoreItem.h"

class QGraphicsScene;

class OutputItem: public CoreItem
{
public:
	OutputItem(QGraphicsScene* _s, Lightbox::Output const& _o);

	bool haveColor() const;
	bool haveDirection() const;
	bool haveGobo() const;

	Lightbox::Output const& output() const { return m_o; }

	QString name() const;
	void restore();
	void save();
	virtual void paint(QPainter* _p, QStyleOptionGraphicsItem const*, QWidget*);

	void initProfiles(unsigned _s);
	void appendProfile();
	void shiftProfiles(unsigned _n);
	Lightbox::OutputProfiles cursorProfiles() const;

	virtual QPointF port() const;

	virtual QPointF sep();
	static void reorder(QGraphicsItem* _g);
	virtual QRectF boundingRect() const;

	Lightbox::OutputProfile outputProfile(unsigned _i, unsigned _li = 0) const;
	Lightbox::OutputProfiles outputProfiles(unsigned _i) const;

private:
	QString m_savedName;
	unsigned m_savedCh;
	Lightbox::Physical m_savedPh;

	Lightbox::Output m_o;
	QList<Lightbox::OutputProfiles> m_op;
	mutable QMutex x_op;
};
