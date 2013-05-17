#include <QtQuick>
#include <Common/Global.h>
#include <EventCompiler/GraphSpec.h>
#include "Global.h"
#include "TimelinesItem.h"
using namespace std;
using namespace lb;

GraphItem::GraphItem()
{
	connect(Noted::data(), SIGNAL(dataComplete(quint32)), SLOT(noteDataComplete()));
}

void GraphItem::noteDataComplete(quint32 _key)
{
	qDebug() << "data is complete somewhere :-)" << hex << _key;

	update();
}

EventCompiler GraphItem::eventCompiler() const
{
	return NotedFace::get()->findEventCompiler(QString::fromStdString(spec().ec));
}

QSGNode* ChartItem::updatePaintNode(QSGNode* _old, UpdatePaintNodeData*)
{
	QSGTransformNode *base = static_cast<QSGTransformNode*>(_old);
	if (!base)
		base = new QSGTransformNode;

	// Update - TODO: optimise by chunking (according to how stored on disk) and only inserting new chunks - use boundingRect to work out what chunks are necessary.
	// transform each chunk separately.
	EventCompiler ec = eventCompiler();
	if (GraphChart* g = ec.isNull() ? nullptr : dynamic_cast<GraphChart*>(ec.asA<EventCompilerImpl>().graph(spec().graph)))
	{
		if (!m_geo)
		{
			// TODO: This is all wrong; doesn't account for m_first
			DataKey k = qHash(QString::fromStdString(g->name()));
			auto z = Noted::data()->rawRecordCount(k);
			qDebug() << QString::fromStdString(g->name()) << " key:" << hex << k << " records:" << z;
			if (!z)
				return base;
			float intermed[z];
			Noted::data()->populateRaw(k, 0, intermed, z);
			m_geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), z);
			m_geo->setDrawingMode(GL_LINE_STRIP);
			m_geo->setLineWidth(1);
			float* v = static_cast<float*>(m_geo->vertexData());
			for (unsigned i = 0; i < z; ++i, v += 2)
			{
				v[0] = i;
				v[1] = intermed[i];
			}
		}

		base->removeAllChildNodes();

		QSGFlatColorMaterial *m = new QSGFlatColorMaterial;
		m->setColor(Qt::black);
		QSGGeometryNode* n = new QSGGeometryNode();
		n->setGeometry(m_geo);
		n->setFlag(QSGNode::OwnsGeometry);
		n->setMaterial(m);
		n->setFlag(QSGNode::OwnsMaterial);
		base->appendChildNode(n);

		// Normalize height so within range [0,1] for the height xform.
		float yf = m_yFrom;
		float yd = m_yDelta;
		if (m_yMode == 1)
		{
			yf = g->yrangeReal().first;
			yd = g->yrangeReal().second - g->yrangeReal().first;
		}
		if (m_yMode == 2)
		{
			yf = g->yrangeHint().first;
			yd = g->yrangeHint().second - g->yrangeHint().first;
		}
		QMatrix4x4 gmx;
		gmx.translate(0, height());
		gmx.scale(1, -height());
		gmx.translate(0, yf);
		gmx.scale(Noted::audio()->hop() / (double)fromSeconds(m_pitch), 1.f / yd);
		gmx.translate(fromSeconds(m_offset) / -(double)Noted::audio()->hop(), 0);
		base->setMatrix(gmx);
	}

	return base;
}

QSGNode* TimelinesItem::updatePaintNode(QSGNode* _old, UpdatePaintNodeData*)
{
	QSGTransformNode *base = static_cast<QSGTransformNode*>(_old);
	if (!base)
		base = new QSGTransformNode;

	base->removeAllChildNodes();

	unsigned majorCount = 0;
	unsigned minorCount = 0;
	GraphParameters<Time> nor(make_pair(Noted::get()->earliestVisible(), Noted::get()->earliestVisible() + Noted::get()->pixelDuration() * width()), width() / 80, toBase(1, 1000000));
	for (Time t = nor.from; t < nor.to; t += nor.incr)
		(nor.isMajor(t) ? majorCount : minorCount)++;

	auto majorGeo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), majorCount * 2);
	auto minorGeo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), minorCount * 2);

	float* majorData = static_cast<float*>(majorGeo->vertexData());
	float* minorData = static_cast<float*>(minorGeo->vertexData());
	for (Time t = nor.from; t < nor.to; t += nor.incr)
	{
		float*& d = nor.isMajor(t) ? majorData : minorData;
		d[0] = d[2] = t - Noted::get()->earliestVisible();
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
	gmx.scale(1 / (double)Noted::get()->pixelDuration(), height());
	base->setMatrix(gmx);

	return base;
}

void XLabelsItem::paint(QPainter* _p)
{
	_p->fillRect(_p->window(), QBrush(Qt::white));
	GraphParameters<Time> nor(make_pair(Noted::get()->earliestVisible(), Noted::get()->earliestVisible() + Noted::get()->pixelDuration() * width()), width() / 80, toBase(1, 1000000));
	for (Time t = nor.from; t < nor.to; t += nor.incr)
		if (nor.isMajor(t))
		{
			float x = (t - Noted::get()->earliestVisible()) / (double)Noted::get()->pixelDuration();
			QString s = QString::fromStdString(textualTime(t, nor.delta, nor.major));
			_p->setPen(QColor(128, 128, 128));
			QSize z = _p->fontMetrics().boundingRect(s).size();
			z = QSize(z.width() + z.height(), z.height() * 1.5);
			QRect r(x - z.width() / 2 , 0, z.width(), z.height());
			_p->drawText(r, Qt::AlignCenter, s);
		}
}

void YLabelsItem::paint(QPainter* _p)
{
	_p->fillRect(_p->window(), QBrush(Qt::white));
}

QSGNode* YScaleItem::updatePaintNode(QSGNode* _old, UpdatePaintNodeData*)
{
	QSGSimpleRectNode *base = static_cast<QSGSimpleRectNode*>(_old);
	if (!base)
	{
		base = new QSGSimpleRectNode();
	}
	base->setRect(boundingRect());
	base->setColor(QColor(255, 255, 255, 192));
	return base;
}

