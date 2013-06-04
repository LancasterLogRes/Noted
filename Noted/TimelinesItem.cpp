#include <QtQuick>
#include <Common/Global.h>
#include <EventCompiler/GraphSpec.h>
#include "Global.h"
#include "TimelinesItem.h"
using namespace std;
using namespace lb;

lb::Time TimelineItem::localTime(lb::Time _t, lb::Time _p)
{
	return _t + mapToScene(QPointF(0, 0)).x() * _p;
}

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

GraphItem::GraphItem()
{
	connect(this, &GraphItem::urlChanged, this, &GraphItem::update);
	connect(this, &GraphItem::yScaleChanged, this, &GraphItem::update);
	connect(this, &GraphItem::highlightChanged, this, &GraphItem::update);
	connect(Noted::data(), &DataMan::dataComplete, this, &GraphItem::update);
	connect(Noted::graphs(), &GraphMan::graphsChanged, this, &GraphItem::update);
}

QSGNode* GraphItem::updatePaintNode(QSGNode* _old, UpdatePaintNodeData*)
{
	QSGTransformNode *base = static_cast<QSGTransformNode*>(_old);
	if (!base)
		base = new QSGTransformNode;

	// Update - TODO: optimise by chunking (according to how stored on disk) and only inserting new chunks - use boundingRect to work out what chunks are necessary.
	// transform each chunk separately.
	if (GraphChart const* g = dynamic_cast<GraphChart const*>(NotedFace::get()->graphs()->lockGraph(m_url)))
	{
		if (!m_geo)
		{
			// TODO: This is all wrong; doesn't account for m_first
			DataKeys k = DataKeys(Noted::audio()->key(), qHash(QString::fromStdString(g->name())));
			auto z = Noted::data()->rawRecordCount(k);
			qDebug() << QString::fromStdString(g->name()) << " key:" << hex << k << " records:" << z;
			float intermed[z];
			if (z)
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
		m->setColor(m_highlight ? Qt::black : Qt::gray);
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
		gmx.scale(Noted::audio()->hop() / (double)m_pitch, 1.f / yd);
		gmx.translate(m_offset / -(double)Noted::audio()->hop(), 0);
		base->setMatrix(gmx);
		NotedFace::get()->graphs()->unlockGraph();
	}

	if (GraphMetadata const* g = NotedFace::get()->graphs()->lock(m_url))
	{
		if (DataSetPtr ds = Noted::data()->readDataSet(DataKeys(g->isRawSource() ? Noted::audio()->rawKey() : Noted::audio()->key(), g->operationKey())))
		{
			if (ds->isMonotonic())
			{
				int lodReq;
				Time from;
				Time duration;
				unsigned records;
				cnote << m_url << textualTime(m_offset) << "+" << textualTime(m_pitch * width()) << "/" << width();
				std::tie(from, records, lodReq, duration) = ds->bestFit(m_offset, m_pitch * width(), width());
				cnote << "Got" << textualTime(from) << "+" << textualTime(duration) << "/" << records << "@" << lodReq;
	//			if (lodReq != m_lod)
				{
					delete m_geo;
					m_geo = nullptr;
					m_lod = lodReq;
				}

				if (ds->isScalar())	// Simple 1D line graph
				{
					if (!m_geo)
					{
						if (m_lod < 0)
						{
							float intermed[records];
							if (records)
								ds->populateRaw(from, intermed, records);
							m_geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), records);
							m_geo->setDrawingMode(GL_LINE_STRIP);
							m_geo->setLineWidth(1);
							float* v = static_cast<float*>(m_geo->vertexData());
							for (unsigned i = 0; i < records; ++i, v += 2)
							{
								v[0] = i;
								v[1] = intermed[i];
							}
						}
						else if (ds->haveDigest(MeanRmsDigest))
						{
							unsigned digestZ = digestSize(MeanRmsDigest);
							float intermed[records * digestZ];
							if (records)
								ds->populateDigest(MeanRmsDigest, m_lod, from, intermed, records * digestZ);
							m_geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), records);
							m_geo->setDrawingMode(GL_LINE_STRIP);
							m_geo->setLineWidth(1);
							float* v = static_cast<float*>(m_geo->vertexData());
							for (unsigned i = 0; i < records; ++i, v += 2)
							{
								v[0] = i;
								v[1] = intermed[i * 2];
							}
						}
					}

					base->removeAllChildNodes();

					QSGFlatColorMaterial* m = new QSGFlatColorMaterial;
					m->setColor(m_highlight ? Qt::black : Qt::gray);
					QSGGeometryNode* n = new QSGGeometryNode();
					n->setGeometry(m_geo);
					n->setFlag(QSGNode::OwnsGeometry);
					n->setMaterial(m);
					n->setFlag(QSGNode::OwnsMaterial);
					base->appendChildNode(n);

					// Normalize height so within range [0,1] for the height xform.
					float yf = m_yFrom;
					float yd = m_yDelta;
					if (m_yMode)
					{
						yf = g->axis().range.first;
						yd = g->axis().range.second - g->axis().range.first;
					}
					QMatrix4x4 gmx;
					gmx.translate(0, height());
					gmx.scale(1, -height());
					double stride = duration / (double)records;
					gmx.scale(stride / m_pitch, 1.f / yd);
					gmx.translate((m_offset - from) / -stride, -yf);
					base->setMatrix(gmx);
				}
			}
		}
		NotedFace::get()->graphs()->unlock();
	}

	return base;
}

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

TimelinesItem::TimelinesItem(QQuickItem* _p):
	QQuickItem(_p)
{
	setFlag(ItemHasContents, true);
	setFlag(ItemAcceptsDrops, true);
	connect(this, &QQuickItem::widthChanged, [=](){ widthChanged(width()); });
	connect(this, SIGNAL(offsetChanged()), SLOT(update()));
	connect(this, SIGNAL(pitchChanged()), SLOT(update()));
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
		(nor.isMajor(t) ? majorCount : minorCount)++;

	auto majorGeo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), majorCount * 2);
	auto minorGeo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), minorCount * 2);

	float* majorData = static_cast<float*>(majorGeo->vertexData());
	float* minorData = static_cast<float*>(minorGeo->vertexData());
	for (Time t = nor.from; t < nor.to; t += nor.incr)
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
