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

QVector3D GraphItem::yScaleHint() const
{
	if (GraphMetadata g = NotedFace::get()->graphs()->find(m_url))
		return QVector3D(g.axis().range.first, g.axis().range.second - g.axis().range.first, 0);
	return QVector3D(0, 1, 0);
}

QSGNode* GraphItem::updatePaintNode(QSGNode* _old, UpdatePaintNodeData*)
{
	QSGTransformNode *base = static_cast<QSGTransformNode*>(_old);
	if (!base)
		base = new QSGTransformNode;

	// Update - TODO: optimise by chunking (according to how stored on disk) and only inserting new chunks - use boundingRect to work out what chunks are necessary.
	// transform each chunk separately.
	if (GraphMetadata g = NotedFace::get()->graphs()->find(m_url))
	{
		if (DataSetPtr ds = Noted::data()->readDataSet(DataKeys(g.isRawSource() ? Noted::audio()->rawKey() : Noted::audio()->key(), g.operationKey())))
		{
			if (ds->isMonotonic())
			{
				int lod;
				Time from;
				Time duration;
				unsigned records;
				cnote << m_url << textualTime(m_offset) << "+" << textualTime(m_pitch * width()) << "/" << width();
				std::tie(from, records, lod, duration) = ds->bestFit(m_offset, m_pitch * max<qreal>(1, width()), max<qreal>(1, width()));
				cnote << "Got" << textualTime(from) << "+" << textualTime(duration) << "/" << records << "@" << lod;

				if (ds->isScalar())	// Simple 1D line graph
				{
					base->removeAllChildNodes();
					if (lod < 0)
					{
						float intermed[records];
						if (records)
							ds->populateRaw(from, intermed, records);
						QSGGeometry* geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), records);
						geo->setDrawingMode(GL_LINE_STRIP);
						geo->setLineWidth(1);
						float* v = static_cast<float*>(geo->vertexData());
						for (unsigned i = 0; i < records; ++i, v += 2)
						{
							v[0] = i;
							v[1] = g.axis().transform.apply(intermed[i]);
						}

						QSGGeometryNode* n = new QSGGeometryNode();
						n->setGeometry(geo);
						n->setFlag(QSGNode::OwnsGeometry);

						QSGFlatColorMaterial* m = new QSGFlatColorMaterial;
						m->setColor(QColor::fromHsvF(0, 0, 0, (m_highlight ? 0.5 : 0.25)));
						n->setMaterial(m);
						n->setFlag(QSGNode::OwnsMaterial);

						base->appendChildNode(n);
					}
					else
					{
						if (ds->haveDigest(MinMaxInOutDigest))
						{
							unsigned digestZ = digestSize(MinMaxInOutDigest);
							float intermed[records * digestZ];
							if (records)
								ds->populateDigest(MinMaxInOutDigest, lod, from, intermed, records * digestZ);

							QSGGeometry* geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), records * 2);
							geo->setDrawingMode(GL_LINES);
							float* v = static_cast<float*>(geo->vertexData());
							for (unsigned i = 0; i < records; ++i, v += 4)
							{
								v[0] = i;
								v[1] = g.axis().transform.apply(intermed[i * 4]);
								v[2] = i;
								v[3] = g.axis().transform.apply(intermed[i * 4 + 1]);
							}

							QSGGeometryNode* n = new QSGGeometryNode();
							n->setGeometry(geo);
							n->setFlag(QSGNode::OwnsGeometry);

							QSGFlatColorMaterial* m = new QSGFlatColorMaterial;
							m->setColor(QColor::fromHsvF(0, 0, (m_highlight ? 0.5 : 0.75), 1));
							n->setMaterial(m);
							n->setFlag(QSGNode::OwnsMaterial);

							base->appendChildNode(n);
						}
						if (ds->haveDigest(MeanRmsDigest))
						{
							unsigned digestZ = digestSize(MeanRmsDigest);
							float intermed[records * digestZ];
							if (records)
								ds->populateDigest(MeanRmsDigest, lod, from, intermed, records * digestZ);
/*
							QSGGeometry* geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), records);
							geo->setDrawingMode(GL_LINE_STRIP);
							geo->setLineWidth(1);
							float* v = static_cast<float*>(geo->vertexData());
							for (unsigned i = 0; i < records; ++i, v += 2)
							{
								v[0] = i;
								v[1] = g.axis().transform.apply(intermed[i * 2])];
							}

							QSGGeometryNode* n = new QSGGeometryNode();
							n->setGeometry(geo);
							n->setFlag(QSGNode::OwnsGeometry);

							QSGFlatColorMaterial* m = new QSGFlatColorMaterial;
							m->setColor(QColor::fromHsvF(0, 0, 0, m_highlight ? 0.5 : 0.25));
							n->setMaterial(m);
							n->setFlag(QSGNode::OwnsMaterial);

							base->appendChildNode(n);*/


							QSGGeometry* ngeo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), records * 2);
							ngeo->setDrawingMode(GL_QUAD_STRIP);
							auto v = static_cast<float*>(ngeo->vertexData());
							for (unsigned i = 0; i < records; ++i, v += 4)
							{
								v[0] = i;
								v[1] = g.axis().transform.apply(intermed[i * 2 + 1]);
								v[2] = i;
								v[3] = -g.axis().transform.apply(intermed[i * 2 + 1]);
							}

							QSGGeometryNode* nn = new QSGGeometryNode();
							nn->setGeometry(ngeo);
							nn->setFlag(QSGNode::OwnsGeometry);

							QSGFlatColorMaterial* nm = new QSGFlatColorMaterial;
							nm->setColor(QColor::fromHsvF(0, 0, 0, (m_highlight ? 0.5 : 0.25) * 0.5));
							nn->setMaterial(nm);
							nn->setFlag(QSGNode::OwnsMaterial);

							base->appendChildNode(nn);
						}
					}
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
					double stride = duration / (double)records;
					gmx.scale(stride / m_pitch, 1.f / yd);
					gmx.translate((m_offset - from) / -stride, -yf);
					base->setMatrix(gmx);
				}
			}
		}
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
	GraphParameters<double> nor(make_pair(m_yFrom, m_yFrom + m_yDelta), height() / 24, 1);
//	auto fp = ceil(log10(fabs(nor.from)));
//	auto dp = floor(log10(fabs(nor.incr)));
	for (double v = nor.from; v < nor.to; v += nor.incr)
		if (nor.isMajor(v))
		{
			// Avoid negative pseudo-zeroes
			if (fabs(v) < nor.incr / 2)
				v = 0;
			double y = (1.0 - double(v - m_yFrom) / m_yDelta) * height();
			QString s = QString::number(v);//, fp < -2 ? 'e' : 'f', fp - dp);
			_p->setPen(QColor(128, 128, 128));
			QSize z = _p->fontMetrics().boundingRect(s).size();
			z = QSize(z.width() + z.height(), z.height() * 1.5);
			QRect r(0, y - z.height() / 2, z.width(), z.height());
			_p->drawText(r, Qt::AlignCenter, s);
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
	for (double v = nor.from; v < nor.to; v += nor.incr)
		(nor.isMajor(v) ? majorCount : minorCount)++;

	auto majorGeo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), majorCount * 2);
	auto minorGeo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), minorCount * 2);

	float* majorData = static_cast<float*>(majorGeo->vertexData());
	float* minorData = static_cast<float*>(minorGeo->vertexData());
	for (double v = nor.from; v < nor.to; v += nor.incr)
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

TimelinesItem::TimelinesItem(QQuickItem* _p):
	TimelineItem(_p)
{
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
