#include <QtQuick>
#include <Common/Global.h>
#include <EventCompiler/GraphSpec.h>
#include "Global.h"
#include "GraphItem.h"
using namespace std;
using namespace lb;

inline pair<GraphMetadata, DataKey> findGraph(QString const& _url)
{
//	cnote << "findGraph" << _url;
	if (GraphMetadata g = NotedFace::get()->graphs()->find(_url))
		return make_pair(g, DataKey(g.isRawSource() ? Noted::audio()->rawKey() : Noted::audio()->key(), g.operationKey()));
	return pair<GraphMetadata, DataKey>();
}

GraphItem::GraphItem(QQuickItem* _p): TimelineItem(_p)
{
	connect(this, &GraphItem::urlChanged, this, &GraphItem::update);

	connect(this, &GraphItem::urlChanged, this, &GraphItem::yScaleHintChanged);
	connect(this, &GraphItem::yScaleHintChanged, this, &GraphItem::yScaleChanged);
	connect(this, &GraphItem::yScaleChanged, this, &GraphItem::update);
	connect(this, &GraphItem::highlightChanged, [=](){ m_invalidated = true; update(); });
	connect(Noted::data(), &DataMan::dataComplete, this, &GraphItem::onDataComplete);
	connect(Noted::graphs(), &GraphMan::graphsChanged, this, &GraphItem::update);
	connect(Noted::graphs(), &GraphManFace::addedGraph, this, &GraphItem::onGraphAdded);
	connect(Noted::quickView(), &QQuickView::sceneGraphInitialized, [=](){});
}

void GraphItem::onGraphAdded(GraphMetadata const& _g)
{
	if (QString::fromStdString(_g.url()) == m_url)
		yScaleHintChanged();
}

void GraphItem::onDataComplete(DataKey _k)
{
	cnote << "dataComplete(" << _k << ") [" << (void*)this << "] interested in" << m_url.toStdString();
	cnote << "=" << findGraph(m_url).second;
	if (_k == findGraph(m_url).second)
	{
		m_invalidated = true;
		yScaleHintChanged();
		update();
	}
}

QVector3D GraphItem::yScaleHint() const
{
	if (GraphMetadata g = NotedFace::get()->graphs()->find(m_url))
		return QVector3D(g.axis(0).range.first, g.axis(0).range.second - g.axis(0).range.first, 0);
	return QVector3D(0, 1, 0);
}

struct MyPoint2D
{
	float x;
	float y;
	float tx;
	float ty;
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

	QSG_DECLARE_SIMPLE_SHADER(SpectrumShader, SpectrumShaderState)
};

inline void checkGL(char const* _e)
{
	for (GLint error = glGetError(); error; error = glGetError())
		cwarn << _e << ": glError (0x" << error << ")";
}

static const unsigned c_recordsPerPage = 4096;

static pair<unsigned, unsigned> pageIntervalsRequired(lb::Time _from, lb::Time _duration, DataSetFloatPtr _ds, int _lod)
{
	unsigned pf = clamp<int, int>((_from - _ds->first()) / (_ds->stride() * _ds->lodFactor(_lod) * c_recordsPerPage), 0, _ds->rawRecords() / _ds->lodFactor(_lod) / c_recordsPerPage + 1);
	unsigned pt = clamp<int, int>((_from - _ds->first() + _duration) / (_ds->stride() * _ds->lodFactor(_lod) * c_recordsPerPage), 0, _ds->rawRecords() / _ds->lodFactor(_lod) / c_recordsPerPage + 1);
	return make_pair(pf, pt);
}

static lb::Time pageTime(unsigned _index, DataSetFloatPtr _ds, int _lod)
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

void GraphItem::setAvailability(bool _graph, bool _data)
{
	if ((m_graphAvailable != _graph) || (m_dataAvailable != _data))
	{
		m_graphAvailable = _graph;
		m_dataAvailable = _data;
		emit availabilityChanged();
	}
}

QSGNode* GraphItem::geometryPage(unsigned _index, GraphMetadata _g, DataSetFloatPtr _ds)
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
					vector<float> intermed(c_recordsPerPage * _ds->recordLength());
					if (m_lod < 0)
						_ds->populateSeries(pageTime(_index, _ds, m_lod), &intermed);
					else
						_ds->populateDigest(MeanDigest, m_lod, pageTime(_index, _ds, m_lod), &intermed);
					_g.axis(GraphMetadata::ValueAxis).transform.apply(intermed);

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
					vector<float> intermed(c_recordsPerPage * _ds->recordLength() * digestZ);
					_ds->populateDigest(MinMaxInOutDigest, m_lod, pageTime(_index, _ds, m_lod), &intermed);
					_g.axis(GraphMetadata::ValueAxis).transform.apply(intermed);

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
					vector<float> intermed(c_recordsPerPage * _ds->recordLength() * digestZ);
					_ds->populateDigest(MeanRmsDigest, m_lod, pageTime(_index, _ds, m_lod), &intermed);
					_g.axis(GraphMetadata::ValueAxis).transform.apply(intermed);

#if 0
					QSGGeometry* geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), records);
					geo->setDrawingMode(GL_LINE_STRIP);
					geo->setLineWidth(1);
					float* v = static_cast<float*>(geo->vertexData());
					for (unsigned i = 0; i < records; ++i, v += 2)
					{
						v[0] = i;
						v[1] = g.axis(0).transform.apply(intermed[i * 2]);
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
				vector<float> intermed(c_recordsPerPage * _ds->recordLength());
//				cnote << "intermed" << (void*)&intermed[0] << "+" << (c_recordsPerPage * _ds->recordLength()) << "*4";
				for (unsigned i = 0; i < c_recordsPerPage * _ds->recordLength(); ++i)
					intermed[i] = 0;
				if (m_lod < 0)
					_ds->populateSeries(pageTime(_index, _ds, m_lod), &intermed);
				else
					_ds->populateDigest(MeanDigest, m_lod, pageTime(_index, _ds, m_lod), &intermed);
				_g.axis(GraphMetadata::YAxis).transform.composed(XOf::toUnity(_g.axis(GraphMetadata::YAxis).range)).apply(intermed);

				QSGGeometryNode* n = new QSGGeometryNode();
				n->setFlag(QSGNode::OwnedByParent, false);
				n->setGeometry(spectrumQuad());

				GLuint id;
				glEnable(GL_TEXTURE_2D);
				glGenTextures(1, &id);
				glBindTexture(GL_TEXTURE_2D, id);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, _ds->recordLength(), c_recordsPerPage, 0, GL_LUMINANCE, GL_FLOAT, intermed.data());
				glBindTexture(GL_TEXTURE_2D, 0);
				glDisable(GL_TEXTURE_2D);

				auto m = SpectrumShader::createMaterial();
				m->state()->transform = XOf(1, 0);
				m->state()->t = window()->createTextureFromId(id, QSize(_ds->recordLength(), c_recordsPerPage), QQuickWindow::TextureOwnsGLTexture);
				m_textures.append(m->state()->t);
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
	for (auto i: m_textures)
		delete i;
	m_geometryCache.clear();
	m_textures.clear();
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
	DataKey dk;
	tie(g, dk) = findGraph(m_url);
	DataSetFloatPtr ds = Noted::data()->get(dk);
	setAvailability(!!g, !!ds);
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
				yf = g.axis(0).range.first;
				yd = g.axis(0).range.second - g.axis(0).range.first;
			}
			QMatrix4x4 gmx;
			gmx.translate(0, height());
			gmx.scale(1, -height());
			gmx.scale(1, 1.f / yd);
			gmx.translate(0, -yf);
			base->setMatrix(gmx);
		}
	}
	else
	{
		QSGGeometryNode* n = new QSGGeometryNode();
		n->setFlag(QSGNode::OwnedByParent, false);
		n->setGeometry(spectrumQuad());

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
