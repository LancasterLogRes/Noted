#pragma once

#include <QGraphicsLineItem>

class DriverItem;
class OutputItem;

class DrivenItem: public QGraphicsLineItem
{
public:
	DrivenItem(DriverItem* _dw, OutputItem* _ow);

	void connect();
	void reposition();

	DriverItem* dw() const { return m_dw; }
	OutputItem* ow() const { return m_ow; }

	// TODO: track src/dest

private:
	DriverItem* m_dw;
	OutputItem* m_ow;
};

