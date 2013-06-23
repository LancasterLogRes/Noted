#include <QtQuick>
#include <Common/Global.h>
#include <EventCompiler/GraphSpec.h>
#include "Global.h"
#include "CursorGraphItem.h"
using namespace std;
using namespace lb;

CursorGraphItem::CursorGraphItem(QQuickItem* _p): QQuickItem(_p)
{
	setClip(true);
	setFlag(ItemHasContents, true);

	connect(this, &CursorGraphItem::urlChanged, this, &CursorGraphItem::xScaleHintChanged);
	connect(this, &CursorGraphItem::urlChanged, this, &CursorGraphItem::yScaleHintChanged);
	connect(this, &CursorGraphItem::xScaleHintChanged, this, &CursorGraphItem::xScaleChanged);
	connect(this, &CursorGraphItem::yScaleHintChanged, this, &CursorGraphItem::yScaleChanged);
	connect(this, &CursorGraphItem::xScaleChanged, this, &CursorGraphItem::update);
	connect(this, &CursorGraphItem::yScaleChanged, this, &CursorGraphItem::update);
	connect(this, &CursorGraphItem::colorChanged, this, &CursorGraphItem::update);
	connect(this, &CursorGraphItem::highlightChanged, [=](){ m_invalidated = true; update(); });
	connect(Noted::audio(), &AudioMan::cursorChanged, this, &CursorGraphItem::update);
	connect(Noted::data(), &DataMan::dataComplete, this, &CursorGraphItem::onDataComplete);
	connect(Noted::graphs(), &GraphMan::graphsChanged, this, &CursorGraphItem::update);
	connect(Noted::graphs(), &GraphManFace::addedGraph, this, &CursorGraphItem::onGraphAdded);
}

void CursorGraphItem::onGraphAdded(GraphMetadata const& _g)
{
	if (QString::fromStdString(_g.url()) == m_url)
	{
		xScaleHintChanged();
		yScaleHintChanged();
	}
}

void CursorGraphItem::onDataComplete(DataKey _k)
{
	cnote << "dataComplete(" << _k << ") [" << (void*)this << "] interested in" << m_url.toStdString();
	cnote << "=" << Noted::graphs()->graphAndKey(m_url).second;
	if (_k == Noted::graphs()->graphAndKey(m_url).second)
	{
		m_invalidated = true;
		update();
	}
}

QVector3D CursorGraphItem::xScaleHint() const
{
	if (GraphMetadata g = NotedFace::get()->graphs()->find(m_url))
		if (g.axes().size() > 1)
			return QVector3D(g.axis(GraphMetadata::XAxis).range.first, g.axis(GraphMetadata::XAxis).range.second - g.axis(GraphMetadata::XAxis).range.first, 0);
	return QVector3D(0, 1, 0);
}

QVector3D CursorGraphItem::yScaleHint() const
{
	if (GraphMetadata g = NotedFace::get()->graphs()->find(m_url))
		return QVector3D(g.axis(GraphMetadata::YAxis).range.first, g.axis(GraphMetadata::YAxis).range.second - g.axis(GraphMetadata::YAxis).range.first, 0);
	return QVector3D(0, 1, 0);
}

void CursorGraphItem::setAvailability(bool _graph, bool _data)
{
	if ((m_graphAvailable != _graph) || (m_dataAvailable != _data))
	{
		m_graphAvailable = _graph;
		m_dataAvailable = _data;
		emit availabilityChanged();
	}
}

QSGGeometry* CursorGraphItem::quad() const
{
	if (!m_quad)
	{
		m_quad = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4);
		QSGGeometry::Point2D* d = static_cast<QSGGeometry::Point2D*>(m_quad->vertexData());
		d[0] = {0, 0};
		d[1] = {1, 0};
		d[2] = {1, 1};
		d[3] = {0, 1};
		m_quad->setDrawingMode(GL_QUADS);
	}
	return m_quad;
}

QSGNode* CursorGraphItem::updatePaintNode(QSGNode* _old, UpdatePaintNodeData*)
{
	QSGTransformNode *base = static_cast<QSGTransformNode*>(_old);
	if (!base)
		base = new QSGTransformNode;

	base->removeAllChildNodes();
	if (width() <= 0)
		return base;

	GraphMetadata g;
	DataKey dk;
	tie(g, dk) = Noted::graphs()->graphAndKey(m_url);
	DataSetFloatPtr ds = Noted::data()->get(dk);
	setAvailability(!!g, !!ds);
	if (g && ds)
	{
		vector<float> vs = ds->getRecord(Noted::audio()->cursor());
		g.axis(GraphMetadata::YAxis).transform.apply(vs);

		QSGGeometryNode* n = new QSGGeometryNode();
		n->setFlag(QSGNode::OwnedByParent, true);

		QSGGeometry* geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), vs.size());
		geo->setDrawingMode(GL_LINE_STRIP);
		float* v = static_cast<float*>(geo->vertexData());
		for (unsigned i = 0; i < vs.size(); ++i, v += 2)
		{
			v[0] = i;
			v[1] = vs[i];
		}
		n->setGeometry(geo);
		n->setFlag(QSGNode::OwnsGeometry);

		QSGFlatColorMaterial* m = new QSGFlatColorMaterial;
		m->setColor(m_color);
		n->setMaterial(m);
		n->setFlag(QSGNode::OwnsMaterial);
		base->appendChildNode(n);

		// Normalize height so within range [0,1] for the height xform.
		float yf = m_yScale.x();
		float yd = m_yScale.y();
		if (m_yMode)
		{
			yf = g.axis(GraphMetadata::YAxis).range.first;
			yd = g.axis(GraphMetadata::YAxis).range.second - g.axis(GraphMetadata::YAxis).range.first;
		}

		float xf = m_xScale.x();
		float xd = m_xScale.y();
		if (m_xMode)
		{
			xf = g.axis(GraphMetadata::XAxis).range.first;
			xd = g.axis(GraphMetadata::XAxis).range.second - g.axis(GraphMetadata::XAxis).range.first;
		}

		QMatrix4x4 gmx;
		gmx.translate(0, height());
		gmx.scale(width(), -height());
		gmx.scale(1.f / xd, 1.f / yd);
		gmx.translate(-xf, -yf);
		gmx.translate(g.axis(GraphMetadata::XAxis).transform.offset(), 0);
		gmx.scale(g.axis(GraphMetadata::XAxis).transform.scale(), 1);
		base->setMatrix(gmx);
	}
	else
	{
		QSGGeometryNode* n = new QSGGeometryNode();
		n->setFlag(QSGNode::OwnedByParent, false);
		n->setGeometry(quad());

		QSGFlatColorMaterial* m = new QSGFlatColorMaterial;
		m->setColor(QColor::fromHsvF(g ? 0.1 : 0, 1, 1, 0.25));
		n->setMaterial(m);
		n->setFlag(QSGNode::OwnsMaterial);

		base->appendChildNode(n);
		QMatrix4x4 gmx;
		gmx.scale(width(), height());
		base->setMatrix(gmx);
	}

	return base;
}
