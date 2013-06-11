#include <QtQuick>
#include <Common/Global.h>
#include <EventCompiler/GraphSpec.h>
#include "Global.h"
#include "TimelinesItem.h"
using namespace std;
using namespace lb;

void XLabelsItem::paint(QPainter* _p)
{
	_p->fillRect(_p->window(), QBrush(Qt::white));
	GraphParameters<Time> nor(make_pair(m_offset, m_offset + m_pitch * width()), width() / 80, toBase(1, 1000000));
	for (Time t = nor.from; t < nor.to; t += nor.incr)
		if (nor.isMajor(t))
		{
			float x = (t - m_offset) / (double)m_pitch;
			QString s = QString::fromStdString(textualTime(t, nor.delta, nor.major));
			_p->setPen(QColor(128, 128, 128));
			QSize z = _p->fontMetrics().boundingRect(s).size();
			z = QSize(z.width() + z.height(), z.height() * 1.5);
			QRect r(x - z.width() / 2, 0, z.width(), height());
			_p->drawText(r, Qt::AlignCenter, s);
		}
}

void YLabelsItem::paint(QPainter* _p)
{
	int h = height() - m_overflow * 2;
	GraphParameters<double> nor(make_pair(m_yFrom, m_yFrom + m_yDelta), h / 24, 1);
	for (double v = nor.from; v <= nor.to + nor.incr / 2; v += nor.incr)
		if (nor.isMajor(v) && int((v - m_yFrom) / m_yDelta * h) <= h)
		{
			// Avoid negative pseudo-zeroes
			if (fabs(v) < nor.incr / 2)
				v = 0;
			double y = (1.0 - double(v - m_yFrom) / m_yDelta) * h + m_overflow;
			QString s = QString::number(v);
			_p->setPen(QColor(128, 128, 128));
			QSize z = _p->fontMetrics().boundingRect(s).size();
			QRect r(0, y - z.height() / 2, width(), z.height());
			_p->drawText(r, Qt::AlignRight | Qt::AlignVCenter, s);
		}
}

QSGNode* YScaleItem::updatePaintNode(QSGNode* _old, UpdatePaintNodeData*)
{
	QSGSimpleRectNode* base = static_cast<QSGSimpleRectNode*>(_old);
	if (!base)
	{
		base = new QSGSimpleRectNode;
		base->appendChildNode(new QSGTransformNode);
		base->setColor(QColor(255, 255, 255, 192));
	}
	QSGTransformNode* obase = dynamic_cast<QSGTransformNode*>(base->firstChild());

	base->setRect(boundingRect());

	obase->removeAllChildNodes();

	unsigned majorCount = 0;
	unsigned minorCount = 0;
	GraphParameters<double> nor(make_pair(m_yFrom, m_yFrom + m_yDelta), height() / 24, 1);
	for (double v = nor.from; v <= nor.to + nor.incr / 2; v += nor.incr)
		if (int((v - m_yFrom) / m_yDelta * height()) <= height())
			(nor.isMajor(v) ? majorCount : minorCount)++;

	auto majorGeo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), majorCount * 2);
	auto minorGeo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), minorCount * 2);

	float* majorData = static_cast<float*>(majorGeo->vertexData());
	float* minorData = static_cast<float*>(minorGeo->vertexData());
	for (double v = nor.from; v <= nor.to + nor.incr / 2; v += nor.incr)
		if (int((v - m_yFrom) / m_yDelta * height()) <= height())
		{
			float*& d = nor.isMajor(v) ? majorData : minorData;
			d[1] = d[3] = v;
			d[0] = 0;
			d[2] = 1;
			d += 4;
		}
	assert(majorData == static_cast<float*>(majorGeo->vertexData()) + majorCount * 4);
	assert(minorData == static_cast<float*>(minorGeo->vertexData()) + minorCount * 4);

	minorGeo->setDrawingMode(GL_LINES);
	majorGeo->setDrawingMode(GL_LINES);

	QSGFlatColorMaterial* majorMaterial = new QSGFlatColorMaterial;
	majorMaterial->setColor(QColor::fromHsv(0, 0, 208));
	QSGGeometryNode* majorNode = new QSGGeometryNode();
	majorNode->setGeometry(majorGeo);
	majorNode->setFlag(QSGNode::OwnsGeometry);
	majorNode->setMaterial(majorMaterial);
	majorNode->setFlag(QSGNode::OwnsMaterial);
	obase->appendChildNode(majorNode);

	QSGFlatColorMaterial* minorMaterial = new QSGFlatColorMaterial;
	minorMaterial->setColor(QColor::fromHsv(0, 0, 232));
	QSGGeometryNode* minorNode = new QSGGeometryNode();
	minorNode->setGeometry(minorGeo);
	minorNode->setFlag(QSGNode::OwnsGeometry);
	minorNode->setMaterial(minorMaterial);
	minorNode->setFlag(QSGNode::OwnsMaterial);
	obase->appendChildNode(minorNode);

	QMatrix4x4 gmx;
	gmx.translate(0, height());
	gmx.scale(width(), -height());
	gmx.scale(1, 1.f / m_yDelta);
	gmx.translate(0, -m_yFrom);
	obase->setMatrix(gmx);

	return base;
}

QSGNode* IntervalItem::updatePaintNode(QSGNode* _old, UpdatePaintNodeData*)
{
	QSGTransformNode* base = static_cast<QSGTransformNode*>(_old);
	if (!base)
	{
		base = new QSGTransformNode;

		{
			auto geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4);
			float* d = static_cast<float*>(geo->vertexData());
			d[0] = d[1] = d[3] = d[6] = 0;
			d[2] = d[7] = d[4] = d[5] = 1;
			geo->setDrawingMode(GL_QUADS);

			QSGGeometryNode* n = new QSGGeometryNode();
			n->setGeometry(geo);
			n->setFlag(QSGNode::OwnsGeometry);
			QSGFlatColorMaterial* majorMaterial = new QSGFlatColorMaterial;
			majorMaterial->setColor(QColor(128, 0, 0, 64));
			n->setMaterial(majorMaterial);
			n->setFlag(QSGNode::OwnsMaterial);
			base->appendChildNode(n);
		}
		{
			auto geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4);
			float* d = static_cast<float*>(geo->vertexData());
			d[1] = 0;
			d[0] = d[2] = d[3] = 1;
			d[4] = d[5] = d[6] = 0;
			d[7] = 1;
			geo->setDrawingMode(GL_LINES);

			QSGGeometryNode* n = new QSGGeometryNode();
			n->setGeometry(geo);
			n->setFlag(QSGNode::OwnsGeometry);
			QSGFlatColorMaterial* majorMaterial = new QSGFlatColorMaterial;
			majorMaterial->setColor(QColor(64, 0, 0, 128));
			n->setMaterial(majorMaterial);
			n->setFlag(QSGNode::OwnsMaterial);
			base->appendChildNode(n);
		}
	}

	QMatrix4x4 gmx;
	double stride = m_duration;
	gmx.scale(stride / m_pitch, height());
	gmx.translate((m_begin - m_offset) / stride, 0);
	base->setMatrix(gmx);

	return base;
}

QSGNode* CursorItem::updatePaintNode(QSGNode* _old, UpdatePaintNodeData*)
{
	QSGTransformNode* base = static_cast<QSGTransformNode*>(_old);
	if (!base)
	{
		base = new QSGTransformNode;

		{
			auto geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 2);
			float* d = static_cast<float*>(geo->vertexData());
			d[1] = 0;
			d[0] = d[2] = d[3] = 1;
			geo->setDrawingMode(GL_LINES);
			geo->setLineWidth(3);

			QSGGeometryNode* n = new QSGGeometryNode();
			n->setGeometry(geo);
			n->setFlag(QSGNode::OwnsGeometry);
			QSGFlatColorMaterial* majorMaterial = new QSGFlatColorMaterial;
			majorMaterial->setColor(QColor(255, 255, 255, 96));
			n->setMaterial(majorMaterial);
			n->setFlag(QSGNode::OwnsMaterial);
			base->appendChildNode(n);
		}
		{
			auto geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4);
			float* d = static_cast<float*>(geo->vertexData());
			d[0] = d[1] = d[3] = d[6] = 0;
			d[2] = d[7] = d[4] = d[5] = 1;
			geo->setDrawingMode(GL_QUADS);

			QSGGeometryNode* n = new QSGGeometryNode();
			n->setGeometry(geo);
			n->setFlag(QSGNode::OwnsGeometry);
			QSGFlatColorMaterial* majorMaterial = new QSGFlatColorMaterial;
			majorMaterial->setColor(QColor(0, 0, 255, 64));
			n->setMaterial(majorMaterial);
			n->setFlag(QSGNode::OwnsMaterial);
			base->appendChildNode(n);
		}
		{
			auto geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 2);
			float* d = static_cast<float*>(geo->vertexData());
			d[1] = 0;
			d[0] = d[2] = d[3] = 1;
			geo->setDrawingMode(GL_LINES);

			QSGGeometryNode* n = new QSGGeometryNode();
			n->setGeometry(geo);
			n->setFlag(QSGNode::OwnsGeometry);
			QSGFlatColorMaterial* majorMaterial = new QSGFlatColorMaterial;
			majorMaterial->setColor(QColor(0, 0, 0, 192));
			n->setMaterial(majorMaterial);
			n->setFlag(QSGNode::OwnsMaterial);
			base->appendChildNode(n);
		}
	}

	QMatrix4x4 gmx;
	double stride = m_cursorWidth;
	gmx.scale(stride / m_pitch, height());
	gmx.translate((m_cursor - m_offset) / stride - 1, 0);
	base->setMatrix(gmx);

	return base;
}

TimelinesItem::TimelinesItem(QQuickItem* _p):
	TimelineItem(_p)
{
	setClip(false);
	setFlag(ItemClipsChildrenToShape, false);
	setFlag(ItemHasContents, true);
	setFlag(ItemAcceptsDrops, true);
	connect(this, &QQuickItem::widthChanged, [=](){ widthChanged(width()); });
}

void TimelinesItem::dragEnterEvent(QDragEnterEvent* _event)
{
	_event->acceptProposedAction();
	setCursor(Qt::DragMoveCursor);
}

void TimelinesItem::dragLeaveEvent(QDragLeaveEvent*)
{
	unsetCursor();
}

void TimelinesItem::dropEvent(QDropEvent* _event)
{
	_event->acceptProposedAction();
	emit textDrop(_event->mimeData()->text());
	unsetCursor();
}

void TimelinesItem::setAcceptingDrops(bool _accepting)
{
	if (_accepting == m_accepting)
		return;
	m_accepting = _accepting;
	setFlag(ItemAcceptsDrops, _accepting);
	emit acceptingDropsChanged();
}

QSGNode* TimelinesItem::updatePaintNode(QSGNode* _old, UpdatePaintNodeData*)
{
	QSGTransformNode *base = static_cast<QSGTransformNode*>(_old);
	if (!base)
		base = new QSGTransformNode;

	base->removeAllChildNodes();

	unsigned majorCount = 0;
	unsigned minorCount = 0;
	GraphParameters<Time> nor(make_pair(m_offset, m_offset + m_pitch * width()), width() / 80, toBase(1, 1000000));
	for (Time t = nor.from; t < nor.to; t += nor.incr)
		if (t > m_offset)
			(nor.isMajor(t) ? majorCount : minorCount)++;

	auto majorGeo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), majorCount * 2);
	auto minorGeo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), minorCount * 2);

	float* majorData = static_cast<float*>(majorGeo->vertexData());
	float* minorData = static_cast<float*>(minorGeo->vertexData());
	for (Time t = nor.from; t < nor.to; t += nor.incr)
		if (t > m_offset)
		{
			float*& d = nor.isMajor(t) ? majorData : minorData;
			d[0] = d[2] = t - m_offset;
			d[1] = 0;
			d[3] = 1;
			d += 4;
		}
	assert(majorData == static_cast<float*>(majorGeo->vertexData()) + majorCount * 4);
	assert(minorData == static_cast<float*>(minorGeo->vertexData()) + minorCount * 4);

	minorGeo->setDrawingMode(GL_LINES);
	majorGeo->setDrawingMode(GL_LINES);

	QSGFlatColorMaterial* majorMaterial = new QSGFlatColorMaterial;
	majorMaterial->setColor(QColor(127, 127, 127));
	QSGGeometryNode* majorNode = new QSGGeometryNode();
	majorNode->setGeometry(majorGeo);
	majorNode->setFlag(QSGNode::OwnsGeometry);
	majorNode->setMaterial(majorMaterial);
	majorNode->setFlag(QSGNode::OwnsMaterial);
	base->appendChildNode(majorNode);

	QSGFlatColorMaterial* minorMaterial = new QSGFlatColorMaterial;
	minorMaterial->setColor(QColor(192, 192, 192));
	QSGGeometryNode* minorNode = new QSGGeometryNode();
	minorNode->setGeometry(minorGeo);
	minorNode->setFlag(QSGNode::OwnsGeometry);
	minorNode->setMaterial(minorMaterial);
	minorNode->setFlag(QSGNode::OwnsMaterial);
	base->appendChildNode(minorNode);

	QMatrix4x4 gmx;
	gmx.scale(1 / (double)m_pitch, height());
	base->setMatrix(gmx);

	return base;
}
