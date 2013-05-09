#include <Common/Global.h>
#include <Grapher/Grapher.h>
#include <NotedPlugin/NotedFace.h>
#include "GraphView.h"
using namespace std;
using namespace Lightbox;

GraphView::GraphView(QWidget* _p, QString const& _name): CurrentView(_p)
{
	setObjectName(_name);
}

GraphView::~GraphView()
{
	quit();
}

void GraphView::addGraph(CompilerGraph* _g)
{
	m_graphs.push_back({c()->getEventCompilerName(_g->ec()).toStdString(), _g->name()});
}

void GraphView::renderGL()
{
	for (auto gr: m_graphs)
	{
		EventCompiler ec = c()->findEventCompiler(QString::fromStdString(gr.ec));
		if (!ec.isNull())
			if (CompilerGraph* g = ec.asA<EventCompilerImpl>()->graph(gr.graph))
			{
				Grapher g;
/*				g.init(&p, xRange, yRange, spec->xLabel, spec->yLabel, spec->pLabel, 30, m_xRangeSpec.lock() ? 0 : 16);
				if (g.drawAxes())
				{
					for (unsigned i = 0; i < ses.size(); ++i)
					{
						GraphSpec const& s = spec->graphs[i];
						StreamEvent const& e = ses[i];
						g.setDataTransform(s.xM, s.xC, s.yM, s.yC);
					}
				}*/
				// TODO: work out axes.
				// TODO: draw graph.
			}
	}
}

