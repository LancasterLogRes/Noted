#include <GL/glu.h>
#include <QtQuick>
#include <Common/Global.h>
#include <Compute/Common.h>
#include <EventCompiler/GraphSpec.h>
#include "Global.h"
#include "GraphItem.h"
using namespace std;
using namespace lb;

GraphItem::GraphItem(QQuickItem* _p): TimelineItem(_p)
{
	connect(this, &GraphItem::urlChanged, this, &GraphItem::update);

	connect(this, &GraphItem::urlChanged, this, &GraphItem::yScaleHintChanged);
	connect(this, &GraphItem::yScaleHintChanged, this, &GraphItem::yScaleChanged);
	connect(this, &GraphItem::yScaleChanged, this, &GraphItem::update);
	connect(this, &GraphItem::highlightChanged, this, &GraphItem::invalidate);
	connect(Noted::data(), &DataMan::dataComplete, this, &GraphItem::onDataComplete);
	connect(Noted::graphs(), &GraphManFace::addedGraph, this, &GraphItem::onGraphAdded);
	connect(Noted::graphs(), &GraphManFace::removedGraph, this, &GraphItem::onGraphRemoved);
	connect(Noted::quickView(), &QQuickView::sceneGraphInitialized, [=](){});
}

void GraphItem::onGraphRemoved(GraphMetadata const& _g)
{
	cnote << "graphRemoved(" << _g.url() << ") [" << (void*)this << "] interested in" << m_url.toStdString();
	if (QString::fromStdString(_g.url()) == m_url)
		invalidate();
}

void GraphItem::onGraphAdded(GraphMetadata const& _g)
{
	cnote << "graphAdded(" << _g.url() << "/" << std::hex << _g.operationKey() << ") [" << (void*)this << "] interested in" << m_url.toStdString();
	if (QString::fromStdString(_g.url()) == m_url)
		invalidate();
}

void GraphItem::onDataComplete(DataKey _k)
{
	cnote << "dataComplete(" << _k << ") [" << (void*)this << "] interested in" << m_url.toStdString();
	cnote << "=" << Noted::graphs()->graphAndKey(m_url).second;
	if (_k == Noted::graphs()->graphAndKey(m_url).second)
		invalidate();
}

QVector3D GraphItem::yScaleHint() const
{
	if (GraphMetadata g = NotedFace::get()->graphs()->find(m_url))
		return QVector3D(g.axis(0).range.first, g.axis(0).range.second - g.axis(0).range.first, 0);
	return QVector3D(0, 1, 0);
}

struct PosSizePhase
{
	float x;
	float y;
	float s;
	float ph;
};

QSGGeometry::Attribute PosSizePhase_Attributes[] = {
	QSGGeometry::Attribute::create(0, 2, GL_FLOAT, true),
	QSGGeometry::Attribute::create(1, 1, GL_FLOAT, false),
	QSGGeometry::Attribute::create(2, 1, GL_FLOAT, false)
};

QSGGeometry::AttributeSet PosSizePhase_AttributeSet = {
	3,
	sizeof(PosSizePhase),
	PosSizePhase_Attributes
};

struct SizedPhasePointState
{
};

class SizedPhasePointShader: public QSGSimpleMaterialShader<SizedPhasePointState>
{
public:
	const char *vertexShader() const {
		return
		"attribute highp vec2 pos;                  \n"
		"attribute highp float size;                  \n"
		"attribute highp float phase;                  \n"
		"uniform highp mat4 qt_Matrix;              \n"
		"varying highp float v_phase; \n"
		"void main() {                              \n"
		"    gl_Position = qt_Matrix * vec4(pos.x, pos.y, 0, 1);      \n"
		"    v_phase = phase;\n"
		"    gl_PointSize = size;\n"
		"}";
	}

	const char *fragmentShader() const {
		return
		"uniform lowp float qt_Opacity;             \n"
		"varying float v_phase;\n"
		"void main() {                              \n"
		"    gl_FragColor = vec4(v_phase, 0, 0, qt_Opacity);\n"
		"}";
	}

	QList<QByteArray> attributes() const
	{
		return QList<QByteArray>() << "pos" << "size" << "phase";
	}

	void updateState(SizedPhasePointState const* _state, SizedPhasePointState const*)
	{
		(void)_state;
//		program()->setUniformValue("scale", _state->transform.scale());
	}

	QSG_DECLARE_SIMPLE_SHADER(SizedPhasePointShader, SizedPhasePointState)
};


struct PosSize
{
	float x;
	float y;
	float s;
};

QSGGeometry::Attribute PosSize_Attributes[] = {
	QSGGeometry::Attribute::create(0, 2, GL_FLOAT, true),
	QSGGeometry::Attribute::create(1, 1, GL_FLOAT, false),
};

QSGGeometry::AttributeSet PosSize_AttributeSet = {
	2,
	sizeof(PosSize),
	PosSize_Attributes
};

struct SizedPointState
{
	QColor color;
	float mag;
};

class SizedPointShader: public QSGSimpleMaterialShader<SizedPointState>
{
public:
	const char *vertexShader() const {
		return
		"uniform float mag;\n"
		"attribute highp vec2 pos;                  \n"
		"attribute highp float size;                  \n"
		"uniform highp mat4 qt_Matrix;              \n"
		"void main() {                              \n"
		"    gl_Position = qt_Matrix * vec4(pos.x, pos.y, 0, 1);      \n"
		"    gl_PointSize = size * mag;\n"
		"}";
	}

	const char *fragmentShader() const {
		return
		"uniform lowp float qt_Opacity;             \n"
		"uniform vec4 color;\n"
		"void main() {                              \n"
		"    gl_FragColor = vec4(color.r, color.g, color.b, color.a * qt_Opacity);\n"
		"}";
	}

	QList<QByteArray> attributes() const
	{
		return QList<QByteArray>() << "pos" << "size";
	}

	void updateState(SizedPointState const* _state, SizedPointState const*)
	{
		glEnable(GL_PROGRAM_POINT_SIZE);
		program()->setUniformValue("color", _state->color);
		program()->setUniformValue("mag", _state->mag);
	}

	QSG_DECLARE_SIMPLE_SHADER(SizedPointShader, SizedPointState)
};

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
		"    float f = (texture2D(qt_Texture, v_tex).r * scale + offset) * 3.0;\n"
		"    gl_FragColor = vec4(f, f - 1.0, f - 2.0, qt_Opacity);\n"
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
static const Time c_timePerPage = FromSeconds<10>::value;

static pair<unsigned, unsigned> pageIntervalsRequired(lb::Time _from, lb::Time _duration, GenericDataSetPtr _ds, int _lod)
{
	if (_ds->isMonotonic())
	{
		unsigned pf = clamp<int, int>((_from - _ds->first()) / (_ds->stride() * _ds->lodFactor(_lod) * c_recordsPerPage), 0, _ds->rawRecords() / _ds->lodFactor(_lod) / c_recordsPerPage + 1);
		unsigned pt = clamp<int, int>((_from - _ds->first() + _duration) / (_ds->stride() * _ds->lodFactor(_lod) * c_recordsPerPage), 0, _ds->rawRecords() / _ds->lodFactor(_lod) / c_recordsPerPage + 1);
		return make_pair(pf, pt);
	}
	else
	{
		unsigned pf = (_from - _ds->first()) / c_timePerPage;
		return make_pair(pf, max<int>(pf + 1, (_from + _duration - _ds->first()) / c_timePerPage) - 1);
	}
}

static lb::Time pageTime(unsigned _index, GenericDataSetPtr _ds, int _lod)
{
	if (_ds->isMonotonic())
		return _index * (_ds->stride() * _ds->lodFactor(_lod) * c_recordsPerPage) + _ds->first();
	else
	{
		assert(_lod == -1);
		return _index  * c_timePerPage + _ds->first();
	}
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

QSGNode* GraphItem::geometryPage(unsigned _index, GraphMetadata _g, GenericDataSetPtr _gds)
{
	if (!m_geometryCache.contains(qMakePair(m_lod, _index)))
	{
		m_geometryCache[qMakePair(m_lod, _index)].first = new QSGTransformNode;
		m_geometryCache[qMakePair(m_lod, _index)].second = nullptr;

		if (_gds->isMonotonic())
		{
			float stride = _gds->stride() * _gds->lodFactor(m_lod);
			if (auto ds = dataset_cast<float>(_gds))
			{
				if (ds->isScalar())
				{
					if (m_lod < 0 || ds->haveDigest(MeanDigest))
					{
						vector<float> intermed(c_recordsPerPage * ds->recordLength());
						if (m_lod < 0)
							ds->populateSeries(pageTime(_index, ds, m_lod), &intermed);
						else
							ds->populateDigest(MeanDigest, m_lod, pageTime(_index, ds, m_lod), &intermed);
						_g.axis(GraphMetadata::ValueAxis).transform.apply(intermed);

						QSGGeometryNode* n = new QSGGeometryNode();
						n->setFlag(QSGNode::OwnedByParent, false);

						QSGGeometry* geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), c_recordsPerPage);
						geo->setDrawingMode(GL_LINE_STRIP);
						float* v = static_cast<float*>(geo->vertexData());
						for (unsigned i = 0; i < c_recordsPerPage; ++i, v += 2)
						{
							v[0] = i * stride;
							v[1] = intermed[i];
						}
						n->setGeometry(geo);
						n->setFlag(QSGNode::OwnsGeometry);

						QSGFlatColorMaterial* m = new QSGFlatColorMaterial;
						m->setColor(QColor::fromHsvF(0, 0, (m_highlight ? 0.5 : 0.75), 1));
						n->setMaterial(m);
						n->setFlag(QSGNode::OwnsMaterial);
						m_geometryCache[qMakePair(m_lod, _index)].first->appendChildNode(n);
					}
					if (m_lod >= 0)
					{
						vector<float> nxio;
						if (ds->haveDigest(MinMaxInOutDigest))
						{
							unsigned digestZ = digestSize(MinMaxInOutDigest);
							nxio = vector<float>(c_recordsPerPage * ds->recordLength() * digestZ);
							ds->populateDigest(MinMaxInOutDigest, m_lod, pageTime(_index, ds, m_lod), &nxio);
							_g.axis(GraphMetadata::ValueAxis).transform.apply(nxio);

							QSGGeometryNode* n = new QSGGeometryNode();
							n->setFlag(QSGNode::OwnedByParent, false);

							QSGGeometry* geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), c_recordsPerPage * 2);
							geo->setDrawingMode(GL_LINES);
							float* v = static_cast<float*>(geo->vertexData());
							for (unsigned i = 0; i < c_recordsPerPage; ++i, v += 4)
							{
								v[0] = i * stride;
								v[1] = nxio[i * 4];
								v[2] = i * stride;
								v[3] = nxio[i * 4 + 1];
							}
							n->setGeometry(geo);
							n->setFlag(QSGNode::OwnsGeometry);

							QSGFlatColorMaterial* m = new QSGFlatColorMaterial;
							m->setColor(QColor::fromHsvF(0, 0, (m_highlight ? 0.5 : 0.75), 1));
							n->setMaterial(m);
							n->setFlag(QSGNode::OwnsMaterial);
							m_geometryCache[qMakePair(m_lod, _index)].first->appendChildNode(n);
						}
						if (m_lod >= 0 && ds->haveDigest(MeanRmsDigest))
						{
							unsigned digestZ = digestSize(MeanRmsDigest);
							vector<float> intermed(c_recordsPerPage * ds->recordLength() * digestZ);
							ds->populateDigest(MeanRmsDigest, m_lod, pageTime(_index, ds, m_lod), &intermed);
							_g.axis(GraphMetadata::ValueAxis).transform.apply(intermed);

	#if 0
							QSGGeometry* geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), records);
							geo->setDrawingMode(GL_LINE_STRIP);
							geo->setLineWidth(1);
							float* v = static_cast<float*>(geo->vertexData());
							for (unsigned i = 0; i < records; ++i, v += 2)
							{
								v[0] = i * stride;
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
								v[0] = i * stride;
								v[1] = min(max(0.f, nxio[i * 4 + 1]), intermed[i * 2 + 1]);
								v[2] = i * stride;
								v[3] = max(min(0.f, nxio[i * 4]), -intermed[i * 2 + 1]);
							}
							n->setGeometry(geo);
							n->setFlag(QSGNode::OwnsGeometry);

							QSGFlatColorMaterial* m = new QSGFlatColorMaterial;
							m->setColor(QColor::fromHsvF(0, 0, (m_highlight ? 0.75 : 0.875), 1));
							n->setMaterial(m);
							n->setFlag(QSGNode::OwnsMaterial);
							m_geometryCache[qMakePair(m_lod, _index)].first->appendChildNode(n);
						}
					}
				}
				else if (ds->isFixed())
				{
					Timer t;
					vector<float> intermed(c_recordsPerPage * ds->recordLength());
	//				cnote << "intermed" << (void*)&intermed[0] << "+" << (c_recordsPerPage * _ds->recordLength()) << "*4";
					if (m_lod < 0)
						ds->populateSeries(pageTime(_index, ds, m_lod), &intermed);
					else
						ds->populateDigest(MeanDigest, m_lod, pageTime(_index, ds, m_lod), &intermed);
					t.reset();
					_g.axis(GraphMetadata::YAxis).transform.composed(XOf::toUnity(_g.axis(GraphMetadata::YAxis).range)).apply(intermed);
					t.reset();

					QSGGeometryNode* n = new QSGGeometryNode();
					n->setFlag(QSGNode::OwnedByParent, false);
					n->setGeometry(spectrumQuad());

					GLuint id;
					glEnable(GL_TEXTURE_2D);
					glGenTextures(1, &id);
					glBindTexture(GL_TEXTURE_2D, id);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, ds->recordLength(), c_recordsPerPage, 0, GL_LUMINANCE, GL_FLOAT, intermed.data());
	//				QOpenGLContext::currentContext()->functions()->glGenerateMipmap(GL_TEXTURE_2D);
					glBindTexture(GL_TEXTURE_2D, 0);
					glDisable(GL_TEXTURE_2D);
					t.reset();

					auto m = SpectrumShader::createMaterial();
					m->state()->transform = XOf(1, 0);
					QSGTexture* tex = window()->createTextureFromId(id, QSize(ds->recordLength(), c_recordsPerPage), QQuickWindow::CreateTextureOptions(QQuickWindow::TextureOwnsGLTexture/* | QQuickWindow::TextureHasMipmaps*/));
					m->state()->t = tex;
					m_geometryCache[qMakePair(m_lod, _index)].second = tex;
					n->setMaterial(m);
					n->setFlag(QSGNode::OwnsMaterial);
					t.reset();

					QMatrix4x4 mx;
					mx.translate(0, _g.axis(GraphMetadata::XAxis).transform.offset());
					mx.scale(c_recordsPerPage * stride, _g.axis(GraphMetadata::XAxis).transform.scale() * ds->recordLength());
					static_cast<QSGTransformNode*>(m_geometryCache[qMakePair(m_lod, _index)].first)->setMatrix(mx);
					m_geometryCache[qMakePair(m_lod, _index)].first->appendChildNode(n);
				}
			}
			else if (DataSetPtr<FreqPeak> ds = dataset_cast<FreqPeak>(_gds))
			{
				if (ds->isFixed())
				{
					m_geometryCache[qMakePair(m_lod, _index)].first = new QSGTransformNode;
					m_geometryCache[qMakePair(m_lod, _index)].second = nullptr;

					unsigned rl = ds->recordLength();
					vector<FreqPeak> intermed(c_recordsPerPage * rl);
					ds->populateSeries(pageTime(_index, ds, m_lod), &intermed);

					QSGGeometry* backgeo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), c_recordsPerPage * ds->recordLength() * 4);
					backgeo->setDrawingMode(GL_QUADS);
					auto w = static_cast<float*>(backgeo->vertexData());

					QSGGeometry* geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), c_recordsPerPage * ds->recordLength() * 4);
					geo->setDrawingMode(GL_QUADS);
					auto v = static_cast<float*>(geo->vertexData());

					for (unsigned i = 0; i < c_recordsPerPage; ++i)
						for (unsigned j = 0; j < rl; ++j, v += 8, w += 8)
						{
							w[0] = v[0] = i;
							v[1] = intermed[i * rl + j].band - intermed[i * rl + j].mag;
							w[1] = v[1] - 5;
							w[2] = v[2] = i;
							v[3] = intermed[i * rl + j].band + 1 + intermed[i * rl + j].mag;
							w[3] = v[3] + 5;
							w[4] = v[4] = (i + 1);
							v[5] = intermed[min(c_recordsPerPage - 1, i + 1) * rl + j].band + 1 + intermed[min(c_recordsPerPage - 1, i + 1) * rl + j].mag;
							w[5] = v[5] + 5;
							w[6] = v[6] = (i + 1);
							v[7] = intermed[min(c_recordsPerPage - 1, i + 1) * rl + j].band - intermed[min(c_recordsPerPage - 1, i + 1) * rl + j].mag;
							w[7] = v[7] - 5;
						}

					QSGGeometryNode* n = new QSGGeometryNode();
					n->setFlag(QSGNode::OwnedByParent, false);
					n->setGeometry(backgeo);
					n->setFlag(QSGNode::OwnsGeometry);
					QSGFlatColorMaterial* m = new QSGFlatColorMaterial;
					m->setColor(QColor::fromHsvF(0, 0, 1, 0.25));
					n->setMaterial(m);
					n->setFlag(QSGNode::OwnsMaterial);
					m_geometryCache[qMakePair(m_lod, _index)].first->appendChildNode(n);

					n = new QSGGeometryNode();
					n->setFlag(QSGNode::OwnedByParent, false);
					n->setGeometry(geo);
					n->setFlag(QSGNode::OwnsGeometry);
					m = new QSGFlatColorMaterial;
					m->setColor(QColor::fromHsvF(0, 0, (m_highlight ? 0.f : 0.25), 1));
					n->setMaterial(m);
					n->setFlag(QSGNode::OwnsMaterial);
					m_geometryCache[qMakePair(m_lod, _index)].first->appendChildNode(n);

					QMatrix4x4 mx;
					mx.scale(stride, _g.axis(GraphMetadata::XAxis).transform.scale());
					mx.translate(0, _g.axis(GraphMetadata::XAxis).transform.offset());
					dynamic_cast<QSGTransformNode*>(m_geometryCache[qMakePair(m_lod, _index)].first)->setMatrix(mx);
				}
			}
		}
		if (DataSetPtr<Peak<> > ds = dataset_cast<Peak<> >(_gds))
		{
			// work out how many elements on the page _index at m_lod (this will only work when lod is -1).
			assert(m_lod == -1);
			Time beginTime = pageTime(_index, ds, m_lod);
			Time endTime = pageTime(_index + 1, ds, m_lod);
			TocRef elements = ds->elementIndex(endTime);
			if (elements == InvalidTocRef)
				elements = ds->elements();
			elements -= ds->elementIndex(beginTime);

			QSGGeometry* geo = new QSGGeometry(PosSizePhase_AttributeSet, elements);
			geo->setDrawingMode(GL_POINTS);
			auto v = static_cast<PosSizePhase*>(geo->vertexData());
			for (unsigned ri = ds->recordIndexLater(beginTime); ri < ds->rawRecords() && ds->timeOfRecord(ri) < endTime; ++ri)
				ds->forEachElement(ri, [&](Peak<> const& f)
				{
					v->x = ds->timeOfRecord(ri) - beginTime;
					v->y = f.band;
					v->s = f.mag;
					v++;
				});
			assert(v == static_cast<PosSizePhase*>(geo->vertexData()) + elements);

			QSGGeometryNode* n = new QSGGeometryNode();
			n->setFlag(QSGNode::OwnedByParent, false);
			n->setGeometry(geo);
			n->setFlag(QSGNode::OwnsGeometry);
			auto m = SizedPointShader::createMaterial();
			m->state()->color = QColor::fromHsvF(0, 0, 1, 0.5);
			m->state()->mag = 10;
			n->setMaterial(m);
			n->setFlag(QSGNode::OwnsMaterial);
			m_geometryCache[qMakePair(m_lod, _index)].first->appendChildNode(n);

			n = new QSGGeometryNode();
			n->setFlag(QSGNode::OwnedByParent, false);
			n->setGeometry(geo);
			m = SizedPointShader::createMaterial();
			m->state()->color = QColor::fromHsvF(0, 0, 0);
			m->state()->mag = 5;
			n->setMaterial(m);
			n->setFlag(QSGNode::OwnsMaterial);
			m_geometryCache[qMakePair(m_lod, _index)].first->appendChildNode(n);

			QMatrix4x4 mx;
			mx.scale(1, _g.axis(GraphMetadata::XAxis).transform.scale());
			mx.translate(0, _g.axis(GraphMetadata::XAxis).transform.offset());
			dynamic_cast<QSGTransformNode*>(m_geometryCache[qMakePair(m_lod, _index)].first)->setMatrix(mx);
		}
	}
	return m_geometryCache[qMakePair(m_lod, _index)].first;
}

void GraphItem::killCache()
{
	for (auto i: m_geometryCache)
	{
		delete i.first;
		delete i.second;
	}
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
	DataKey dk;
	tie(g, dk) = Noted::graphs()->graphAndKey(m_url);
	GenericDataSetPtr ds = Noted::data()->getGeneric(dk);
	setAvailability(!!g, !!ds);
	if (g && ds)
	{
		int lod = -1;
		if (ds->isMonotonic())
		{
			Time from;
			Time duration;
			unsigned records;
//			cnote << m_url << textualTime(m_offset) << "+" << textualTime(m_pitch * width()) << "/" << width();
			std::tie(from, records, lod, duration) = ds->bestFit(m_offset, m_pitch * max<qreal>(1, width()), max<qreal>(1, width()));
//			cnote << "Got" << textualTime(from) << "+" << textualTime(duration) << "/" << records << "@" << lod;
		}

		if (lod != m_lod)
			m_lod = lod;
		if (m_invalidated)
		{
			killCache();
			m_invalidated = false;
		}

		pair<unsigned, unsigned> req = pageIntervalsRequired(m_offset, m_pitch * max<qreal>(1, width()), ds, lod);

//		cnote << "Pages required:" << req;

//		double stride = ds->stride() * ds->lodFactor(m_lod);
		for (unsigned i = req.first; i <= req.second; ++i)
		{
			QSGTransformNode* tr = new QSGTransformNode;
			tr->appendChildNode(geometryPage(i, g, ds));
			QMatrix4x4 lmx;
			lmx.scale(1.0 / m_pitch, 1);
			lmx.translate(pageTime(i, ds, m_lod) - m_offset, 0);
			tr->setMatrix(lmx);
//			cnote << "Page" << i << "x" << (stride / m_pitch) << "+" << ((m_offset - pageTime(i, ds, m_lod)) / -stride);
			base->appendChildNode(tr);
		}

		// Normalize height so within range [0,1] for the height xform.
		float yf = m_yScale.x();
		float yd = m_yScale.y();
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
