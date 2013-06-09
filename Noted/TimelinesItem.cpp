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

inline pair<GraphMetadata, DataKeys> findGraph(QString const& _url)
{
	if (GraphMetadata g = NotedFace::get()->graphs()->find(_url))
		return make_pair(g, DataKeys(g.isRawSource() ? Noted::audio()->rawKey() : Noted::audio()->key(), g.operationKey()));
	return pair<GraphMetadata, DataKeys>();
}

GraphItem::GraphItem(QQuickItem* _p): TimelineItem(_p)
{
	connect(this, &GraphItem::urlChanged, this, &GraphItem::update);

	connect(this, &GraphItem::urlChanged, this, &GraphItem::yScaleHintChanged);
	connect(this, &GraphItem::yScaleHintChanged, this, &GraphItem::yScaleChanged);
	connect(this, &GraphItem::yScaleChanged, this, &GraphItem::update);
	connect(this, &GraphItem::highlightChanged, [=](){ m_invalidated = true; update(); });
	connect(Noted::data(), &DataMan::dataComplete, [=](DataKeys k){ if (k == findGraph(m_url).second) update(); });
	connect(Noted::graphs(), &GraphMan::graphsChanged, this, &GraphItem::update);
	connect(Noted::graphs(), &GraphManFace::addedGraph,
			[=](GraphMetadata g)
	{
		cnote << "Added:" << g.url() << "; interested in" << m_url.toStdString();
		if (QString::fromStdString(g.url()) == m_url)
			yScaleHintChanged();
	});
	connect(Noted::quickView(), &QQuickView::sceneGraphInitialized, [=](){});
}

QVector3D GraphItem::yScaleHint() const
{
	if (GraphMetadata g = NotedFace::get()->graphs()->find(m_url))
		return QVector3D(g.axis().range.first, g.axis().range.second - g.axis().range.first, 0);
	return QVector3D(0, 1, 0);
}

struct MyPoint2D {
	float x;
	float y;
	float tx;
	float ty;

	void set(float _x, float _y, float _tx, float _ty)
	{
		x = _x;
		y = _y;
		tx = _tx;
		ty = _ty;
	}
};

QSGGeometry::Attribute MyPoint2D_Attributes[] = {
	QSGGeometry::Attribute::create(0, 2, GL_FLOAT, true),
	QSGGeometry::Attribute::create(1, 2, GL_FLOAT, false)
};

QSGGeometry::AttributeSet MyPoint2D_AttributeSet = {
	2,
	sizeof(MyPoint2D),
	MyPoint2D_Attributes
};

struct SpectrumShaderState
{
	XOf transform;
	QSGTexture* t = nullptr;
};

class SpectrumShader: public QSGSimpleMaterialShader<SpectrumShaderState>
{
	static QSGMaterialShader* createShader()
	{
		return new SpectrumShader;
	}

public:
	class Material: public QSGSimpleMaterial<SpectrumShaderState>
	{
	public:
		template <class _T> Material(_T const& _t): QSGSimpleMaterial<SpectrumShaderState>(_t) {}
		virtual ~Material() { delete state()->t; }
	};

	static Material* createMaterial()
	{
		return new Material(createShader);
	}

public:
	const char *vertexShader() const {
		return
		"attribute highp vec2 pos;                  \n"
		"attribute highp vec2 tex;                  \n"
		"uniform highp mat4 qt_Matrix;              \n"
		"varying vec2 v_tex;\n"
		"void main() {                              \n"
		"    gl_Position = qt_Matrix * vec4(pos.x, pos.y, 0, 1);      \n"
		"    v_tex = tex;\n"
		"}";
	}

	const char *fragmentShader() const {
		return
		"uniform lowp float qt_Opacity;             \n"
		"uniform sampler2D qt_Texture;              \n"
		"uniform lowp float scale;                  \n"
		"uniform lowp float offset;                 \n"
		"varying vec2 v_tex;\n"
		"void main() {                              \n"
		"    float f = (texture2D(qt_Texture, v_tex).r * scale + offset) * 3;\n"
		"    gl_FragColor = vec4(f, f - 1, f - 2, qt_Opacity);\n"
		"}";
	}

	QList<QByteArray> attributes() const
	{
		return QList<QByteArray>() << "pos" << "tex";
	}

	void updateState(SpectrumShaderState const* _state, SpectrumShaderState const*)
	{
		program()->setUniformValue("scale", _state->transform.scale());
		program()->setUniformValue("offset", _state->transform.offset());
		_state->t->bind();
	}
};

inline void checkGL(char const* _e)
{
	for (GLint error = glGetError(); error; error = glGetError())
		cwarn << _e << ": glError (0x" << error << ")";
}

static const unsigned c_recordsPerPage = 4096;

static pair<unsigned, unsigned> pageIntervalsRequired(lb::Time _from, lb::Time _duration, DataSetPtr _ds, int _lod)
{
	unsigned pf = clamp<int, int>((_from - _ds->first()) / (_ds->stride() * _ds->lodFactor(_lod) * c_recordsPerPage), 0, _ds->rawRecords() / _ds->lodFactor(_lod) / c_recordsPerPage + 1);
	unsigned pt = clamp<int, int>((_from - _ds->first() + _duration) / (_ds->stride() * _ds->lodFactor(_lod) * c_recordsPerPage), 0, _ds->rawRecords() / _ds->lodFactor(_lod) / c_recordsPerPage + 1);
	return make_pair(pf, pt);
}

static lb::Time pageTime(unsigned _index, DataSetPtr _ds, int _lod)
{
	return _index * (_ds->stride() * _ds->lodFactor(_lod) * c_recordsPerPage) + _ds->first();
}

QSGGeometry* GraphItem::spectrumQuad() const
{
	if (!m_spectrumQuad)
	{
		m_spectrumQuad = new QSGGeometry(MyPoint2D_AttributeSet, 4);
		MyPoint2D* d = static_cast<MyPoint2D*>(m_spectrumQuad->vertexData());
		d[0] = {0, 0, 0, 0};
		d[1] = {1, 0, 0, 1};
		d[2] = {1, 1, 1, 1};
		d[3] = {0, 1, 1, 0};
		m_spectrumQuad->setDrawingMode(GL_QUADS);
	}
	return m_spectrumQuad;

}

QSGNode* GraphItem::geometryPage(unsigned _index, GraphMetadata _g, DataSetPtr _ds)
{
	if (!m_geometryCache.contains(_index))
	{
		if (_ds->isMonotonic())
		{
			m_geometryCache[_index] = new QSGTransformNode;

			if (_ds->isScalar())
			{
				if (m_lod < 0 || _ds->haveDigest(MeanDigest))
				{
					float intermed[c_recordsPerPage * _ds->recordLength()];
					if (m_lod < 0)
						_ds->populateRaw(pageTime(_index, _ds, m_lod), intermed, c_recordsPerPage * _ds->recordLength(), _g.axis(GraphMetadata::ValueAxis).transform);
					else
						_ds->populateDigest(MeanDigest, m_lod, pageTime(_index, _ds, m_lod), intermed, c_recordsPerPage * _ds->recordLength(), _g.axis(GraphMetadata::ValueAxis).transform);


					QSGGeometryNode* n = new QSGGeometryNode();
					n->setFlag(QSGNode::OwnedByParent, false);

					QSGGeometry* geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), c_recordsPerPage);
					geo->setDrawingMode(GL_LINE_STRIP);
					float* v = static_cast<float*>(geo->vertexData());
					for (unsigned i = 0; i < c_recordsPerPage; ++i, v += 2)
					{
						v[0] = i;
						v[1] = intermed[i];
					}
					n->setGeometry(geo);
					n->setFlag(QSGNode::OwnsGeometry);

					QSGFlatColorMaterial* m = new QSGFlatColorMaterial;
					m->setColor(QColor::fromHsvF(0, 0, (m_highlight ? 0.5 : 0.75), 1));
					n->setMaterial(m);
					n->setFlag(QSGNode::OwnsMaterial);
					m_geometryCache[_index]->appendChildNode(n);
				}
				if (m_lod >= 0 && _ds->haveDigest(MinMaxInOutDigest))
				{
					unsigned digestZ = digestSize(MinMaxInOutDigest);
					float intermed[c_recordsPerPage * digestZ];
					_ds->populateDigest(MinMaxInOutDigest, m_lod, pageTime(_index, _ds, m_lod), intermed, c_recordsPerPage * _ds->recordLength() * digestZ, _g.axis(GraphMetadata::ValueAxis).transform);

					QSGGeometryNode* n = new QSGGeometryNode();
					n->setFlag(QSGNode::OwnedByParent, false);

					QSGGeometry* geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), c_recordsPerPage * 2);
					geo->setDrawingMode(GL_LINES);
					float* v = static_cast<float*>(geo->vertexData());
					for (unsigned i = 0; i < c_recordsPerPage; ++i, v += 4)
					{
						v[0] = i;
						v[1] = intermed[i * 4];
						v[2] = i;
						v[3] = intermed[i * 4 + 1];
					}
					n->setGeometry(geo);
					n->setFlag(QSGNode::OwnsGeometry);

					QSGFlatColorMaterial* m = new QSGFlatColorMaterial;
					m->setColor(QColor::fromHsvF(0, 0, (m_highlight ? 0.5 : 0.75), 1));
					n->setMaterial(m);
					n->setFlag(QSGNode::OwnsMaterial);
					m_geometryCache[_index]->appendChildNode(n);
				}
				if (m_lod >= 0 && _ds->haveDigest(MeanRmsDigest))
				{
					unsigned digestZ = digestSize(MeanRmsDigest);
					float intermed[c_recordsPerPage * digestZ];
					_ds->populateDigest(MeanRmsDigest, m_lod, pageTime(_index, _ds, m_lod), intermed, c_recordsPerPage * _ds->recordLength() * digestZ, _g.axis(GraphMetadata::ValueAxis).transform);

#if 0
					QSGGeometry* geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), records);
					geo->setDrawingMode(GL_LINE_STRIP);
					geo->setLineWidth(1);
					float* v = static_cast<float*>(geo->vertexData());
					for (unsigned i = 0; i < records; ++i, v += 2)
					{
						v[0] = i;
						v[1] = g.axis().transform.apply(intermed[i * 2]);
					}

					QSGGeometryNode* n = new QSGGeometryNode();
					n->setGeometry(geo);
					n->setFlag(QSGNode::OwnsGeometry);

					QSGFlatColorMaterial* m = new QSGFlatColorMaterial;
					m->setColor(QColor::fromHsvF(0, 0, 0, m_highlight ? 0.5 : 0.25));
					n->setMaterial(m);
					n->setFlag(QSGNode::OwnsMaterial);

					base->appendChildNode(n);
#endif

					QSGGeometryNode* n = new QSGGeometryNode();
					n->setFlag(QSGNode::OwnedByParent, false);

					QSGGeometry* geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), c_recordsPerPage * 2);
					geo->setDrawingMode(GL_QUAD_STRIP);
					auto v = static_cast<float*>(geo->vertexData());
					for (unsigned i = 0; i < c_recordsPerPage; ++i, v += 4)
					{
						v[0] = i;
						v[1] = intermed[i * 2 + 1];
						v[2] = i;
						v[3] = -intermed[i * 2 + 1];
					}
					n->setGeometry(geo);
					n->setFlag(QSGNode::OwnsGeometry);

					QSGFlatColorMaterial* m = new QSGFlatColorMaterial;
					m->setColor(QColor::fromHsvF(0, 0, (m_highlight ? 0.75 : 0.875), 1));
					n->setMaterial(m);
					n->setFlag(QSGNode::OwnsMaterial);
					m_geometryCache[_index]->appendChildNode(n);
				}
			}
			else if (_ds->isFixed())
			{
				float intermed[c_recordsPerPage * _ds->recordLength()];
				if (m_lod < 0)
					_ds->populateRaw(pageTime(_index, _ds, m_lod), intermed, c_recordsPerPage * _ds->recordLength(), _g.axis(GraphMetadata::ValueAxis).transform.composed(XOf::toUnity(_g.axis(GraphMetadata::ValueAxis).range)));
				else
					_ds->populateDigest(MeanDigest, m_lod, pageTime(_index, _ds, m_lod), intermed, c_recordsPerPage * _ds->recordLength(), _g.axis(GraphMetadata::ValueAxis).transform.composed(XOf::toUnity(_g.axis(GraphMetadata::ValueAxis).range)));

				QSGGeometryNode* n = new QSGGeometryNode();
				n->setFlag(QSGNode::OwnedByParent, false);
				n->setGeometry(spectrumQuad());

				GLuint id;
				glEnable(GL_TEXTURE_2D);
				glGenTextures(1, &id);
				glBindTexture(GL_TEXTURE_2D, id);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, _ds->recordLength(), c_recordsPerPage, 0, GL_LUMINANCE, GL_FLOAT, intermed);
				glBindTexture(GL_TEXTURE_2D, 0);
				glDisable(GL_TEXTURE_2D);

				auto m = SpectrumShader::createMaterial();
				m->state()->transform = XOf(1, 0);
				m->state()->t = window()->createTextureFromId(id, QSize(_ds->recordLength(), c_recordsPerPage), QQuickWindow::TextureOwnsGLTexture);
				n->setMaterial(m);
				n->setFlag(QSGNode::OwnsMaterial);

				QMatrix4x4 mx;
				mx.translate(0, _g.axis(GraphMetadata::XAxis).transform.offset());
				mx.scale(c_recordsPerPage, _g.axis(GraphMetadata::XAxis).transform.scale() * _ds->recordLength());
				static_cast<QSGTransformNode*>(m_geometryCache[_index])->setMatrix(mx);
				m_geometryCache[_index]->appendChildNode(n);
			}
		}
	}
	return m_geometryCache[_index];
}

void GraphItem::killCache()
{
	for (auto i: m_geometryCache)
		delete i;
	m_geometryCache.clear();
}

QSGNode* GraphItem::updatePaintNode(QSGNode* _old, UpdatePaintNodeData*)
{
	QSGTransformNode *base = static_cast<QSGTransformNode*>(_old);
	if (!base)
		base = new QSGTransformNode;

	base->removeAllChildNodes();

	// Optimise by chunking (according to how stored on disk) and only inserting new chunks - use boundingRect to work out what chunks are necessary.
	// transform each chunk separately.

	GraphMetadata g;
	DataKeys dk;
	tie(g, dk) = findGraph(m_url);
	DataSetPtr ds = Noted::data()->readDataSet(dk);
	if (g && ds)
	{
		if (ds->isMonotonic())
		{
			int lod;
			Time from;
			Time duration;
			unsigned records;
//			cnote << m_url << textualTime(m_offset) << "+" << textualTime(m_pitch * width()) << "/" << width();
			std::tie(from, records, lod, duration) = ds->bestFit(m_offset, m_pitch * max<qreal>(1, width()), max<qreal>(1, width()));
//			cnote << "Got" << textualTime(from) << "+" << textualTime(duration) << "/" << records << "@" << lod;

			if (lod != m_lod || m_invalidated)
			{
				m_lod = lod;
				killCache();
				m_invalidated = false;
			}

			pair<unsigned, unsigned> req = pageIntervalsRequired(m_offset, m_pitch * max<qreal>(1, width()), ds, lod);
//			cnote << "Pages required:" << req;

			double stride = ds->stride() * ds->lodFactor(m_lod);
			for (unsigned i = req.first; i <= req.second; ++i)
			{
				QSGTransformNode* tr = new QSGTransformNode;
				tr->appendChildNode(geometryPage(i, g, ds));
				QMatrix4x4 lmx;
				lmx.scale(stride / m_pitch, 1);
				lmx.translate((m_offset - pageTime(i, ds, m_lod)) / -stride, 0);
				tr->setMatrix(lmx);
//				cnote << "Page" << i << "x" << (stride / m_pitch) << "+" << ((m_offset - pageTime(i, ds, m_lod)) / -stride);
				base->appendChildNode(tr);
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
			gmx.scale(1, 1.f / yd);
			gmx.translate(0, -yf);
			base->setMatrix(gmx);
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
			QRect r(x - z.width() / 2, 0, z.width(), height());
			_p->drawText(r, Qt::AlignCenter, s);
		}
}

void YLabelsItem::paint(QPainter* _p)
{
//	_p->fillRect(_p->window(), QBrush(Qt::white));
	GraphParameters<double> nor(make_pair(m_yFrom, m_yFrom + m_yDelta), height() / 24, 1);
//	auto fp = ceil(log10(fabs(nor.from)));
//	auto dp = floor(log10(fabs(nor.incr)));
	for (double v = nor.from; v < nor.to; v += nor.incr)
		if (nor.isMajor(v))
		{
			// Avoid negative pseudo-zeroes
			if (fabs(v) < nor.incr / 2)
				v = 0;
			double y = (1.0 - double(v - m_yFrom) / m_yDelta) * parentItem()->height();
			QString s = QString::number(v);//, fp < -2 ? 'e' : 'f', fp - dp);
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
