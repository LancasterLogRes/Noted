#include <Common/Global.h>
#include <Grapher/Grapher.h>
#include <NotedPlugin/NotedFace.h>
#include <EventCompiler/GraphSpec.h>
#include "GraphView.h"
using namespace std;
using namespace lb;

GraphView::GraphView(QWidget* _p, QString const& _name): CurrentView(_p)
{
	setObjectName(_name);
}

GraphView::~GraphView()
{
	quit();
}

void GraphView::addGraph(GraphSpec* _g)
{
	m_graphs.push_back({NotedFace::events()->getEventCompilerName(_g->ec()).toStdString(), _g->name(), NullColor});
}

QColor toQColor(Color _c)
{
	return QColor::fromHsvF(_c.h(), _c.s(), _c.v(), _c.a());
}

void GraphView::rejig()
{
}

void GraphView::renderGL(QSize _s)
{
	QOpenGLPaintDevice glpd(_s);
	QPainter p(&glpd);

	Grapher grapher;
	int i = 0;
	unsigned s = m_graphs.size();

	for (GraphViewPlot const& gr: m_graphs)
	{
		EventCompiler ec = NotedFace::events()->findEventCompiler(QString::fromStdString(gr.ec));
		if (!ec.isNull())
			if (GraphSpec* cg = ec.asA<EventCompilerImpl>().graph(gr.graph))
				if (GraphSparseDense* g = dynamic_cast<GraphSparseDense*>(cg))
				{
					if (!i)
						grapher.init(&p, g->xrangeReal(), g->yrangeReal(), id, id, idL, 30, 16);
					grapher.drawAxes();
					grapher.setDataTransform(g->xtx().scale(), g->xtx().offset());
					grapher.drawLineGraph(g->dataPoint(NotedFace::audio()->cursorIndex()), toQColor(defaultTo(gr.c, Color(float(i) / s, 0.7, 0.5), NullColor)), Qt::NoBrush, 0.f);
					++i;
				}
	}
}

