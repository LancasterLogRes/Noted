#include <QtQuick>
#include <Common/Global.h>
#include <EventCompiler/GraphSpec.h>
#include "Global.h"
#include "CursorGraphItem.h"
using namespace std;
using namespace lb;

inline pair<GraphMetadata, DataKey> findGraph(QString const& _url)
{
	if (GraphMetadata g = NotedFace::get()->graphs()->find(_url))
		return make_pair(g, DataKey(g.isRawSource() ? Noted::audio()->rawKey() : Noted::audio()->key(), g.operationKey()));
	return pair<GraphMetadata, DataKey>();
}

CursorGraphItem::CursorGraphItem(QQuickItem* _p): QQuickItem(_p)
{
	connect(this, &CursorGraphItem::urlChanged, this, &CursorGraphItem::update);

	connect(this, &CursorGraphItem::urlChanged, this, &CursorGraphItem::yScaleHintChanged);
	connect(this, &CursorGraphItem::yScaleHintChanged, this, &CursorGraphItem::yScaleChanged);
	connect(this, &CursorGraphItem::yScaleChanged, this, &CursorGraphItem::update);
	connect(this, &CursorGraphItem::highlightChanged, [=](){ m_invalidated = true; update(); });
	connect(Noted::data(), &DataMan::dataComplete, this, &CursorGraphItem::onDataComplete);
	connect(Noted::graphs(), &GraphMan::graphsChanged, this, &CursorGraphItem::update);
	connect(Noted::graphs(), &GraphManFace::addedGraph, this, &CursorGraphItem::onGraphAdded);
}

void CursorGraphItem::onGraphAdded(GraphMetadata const& _g)
{
	if (QString::fromStdString(_g.url()) == m_url)
		yScaleHintChanged();
}

void CursorGraphItem::onDataComplete(DataKey _k)
{
	cnote << "dataComplete(" << _k << ") [" << (void*)this << "] interested in" << m_url.toStdString();
	cnote << "=" << findGraph(m_url).second;
	if (_k == findGraph(m_url).second)
	{
		m_invalidated = true;
		update();
	}
}

QVector3D CursorGraphItem::xScaleHint() const
{
	if (GraphMetadata g = NotedFace::get()->graphs()->find(m_url))
		if (g.axes().size() > 1)
			return QVector3D(g.axis(0).range.first, g.axis(0).range.second - g.axis(0).range.first, 0);
	return QVector3D(0, 1, 0);
}

QVector3D CursorGraphItem::yScaleHint() const
{
	if (GraphMetadata g = NotedFace::get()->graphs()->find(m_url))
		return QVector3D(g.axis().range.first, g.axis().range.second - g.axis().range.first, 0);
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

	GraphMetadata g;
	DataKey dk;
	tie(g, dk) = findGraph(m_url);
	DataSetFloatPtr ds = Noted::data()->get(dk);
	setAvailability(!!g, !!ds);
	if (g && ds)
	{
		std::vector<float> vs = ds->getRecord(Noted::audio()->cursor());

		// Normalize height so within range [0,1] for the height xform.
		float yf = m_yFrom;
		float yd = m_yDelta;
		if (m_yMode)
		{
			yf = g.axis().range.first;
			yd = g.axis().range.second - g.axis().range.first;
		}
		QMatrix4x4 gmx;
		gmx.translate(0, height());
		gmx.scale(1, -height());
		gmx.scale(1, 1.f / yd);
		gmx.translate(0, -yf);
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
