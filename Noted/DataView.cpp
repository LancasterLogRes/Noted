/* BEGIN COPYRIGHT
 *
 * This file is part of Noted.
 *
 * Copyright ©2011, 2012, Lancaster Logic Response Limited.
 *
 * Noted is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Noted is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Noted.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <utility>
#include <cmath>
#include <QGraphicsOpacityEffect>
#include <QVariant>
#include <QComboBox>
#include <QDebug>
#include <QFrame>
#include <QPaintEvent>
#include <QPainter>
#include <Common/Common.h>
#include <NotedPlugin/NotedFace.h>

#include "Grapher.h"
#include "DataView.h"

using namespace std;
using namespace Lightbox;

DataView::DataView(QWidget* _parent, QString const& _name):
	CurrentView(_parent)
{
	setAcceptDrops(true);
	setObjectName(_name);
	rejig();
}

void DataView::checkSpec()
{
	if (!m_spec.lock())
		m_spec.reset();
	if (!m_xRangeSpec.lock())
		m_xRangeSpec.reset();
}

QColor toQColor(Color _c, uint8_t _a = 255)
{
	return QColor(_c.r(), _c.g(), _c.b(), _a);
}

shared_ptr<AuxGraphsSpec> DataView::findSpec(QString const& _n) const
{
	foreach (StreamEvent const& se, c()->initEventsOf(GraphSpecComment))
		if (shared_ptr<AuxGraphsSpec> ags = dynamic_pointer_cast<AuxGraphsSpec>(se.aux()))
			if (_n == QString::fromStdString(ags->name))
				return ags;
	return shared_ptr<AuxGraphsSpec>();
}

void DataView::rejig()
{
	m_spec = findSpec(objectName());
}

void DataView::mousePressEvent(QMouseEvent* _e)
{
	if (_e->button() == Qt::LeftButton)
	{
		QDrag* drag = new QDrag(this);
		QMimeData* mimeData = new QMimeData;
		mimeData->setText(objectName());
		drag->setMimeData(mimeData);
		/*Qt::DropAction dropAction =*/drag->exec();
	}
}

void DataView::dragEnterEvent(QDragEnterEvent* _e)
{
	if (_e->mimeData()->hasFormat("text/plain"))
		_e->acceptProposedAction();
}

void DataView::dropEvent(QDropEvent* _e)
{
	if (_e->mimeData()->text() == objectName())
		m_xRangeSpec.reset();
	else
		m_xRangeSpec = findSpec(_e->mimeData()->text());
	_e->acceptProposedAction();
	rerender();
}

class OurGrapher: public Grapher
{
public:
	void drawMeterExplanation(map<float, Meter> const& _candidates)
	{
		int zy = yTP(0.f);
		foreach (auto c, _candidates)
		{
			float t = xT(c.second.period);
			int x = xP(t);
			int y;
			y = yTP(c.second.Tstrength);
			p->setPen(Qt::red);
			p->drawLine(x - 6, zy, x - 6, y);

			y = yTP(c.second.Qstrength);
			p->setPen(Qt::darkBlue);
			p->drawLine(x - 4, zy, x - 4, y);

			y = yTP(c.second.Hstrength);
			p->setPen(Qt::darkBlue);
			p->drawLine(x - 2, zy, x - 2, y);

			y = yTP(c.second.Wstrength);
			p->setPen(Qt::darkCyan);
			p->drawLine(x, zy, x, y);

			y = yTP(c.second.Dstrength);
			p->setPen(Qt::darkGreen);
			p->drawLine(x + 2, zy, x + 2, y);

			y = yTP(-c.first);
			p->setPen(Qt::gray);
			p->drawLine(x + 4, zy, x + 4, y);
		}
	}

	void drawPhaseExplanation(map<float, Phase> const& _candidates)
	{
		int zy = yTP(0.f);
		foreach (auto c, _candidates)
		{
			float t = xT(c.second.phase);
			int x = xP(t);
			int y;
			y = yTP(c.second.trailGap);
			p->setPen(Qt::red);
			p->drawLine(x - 6, zy, x - 6, y);

			y = yTP(c.second.eBackbeat);
			p->setPen(Qt::darkBlue);
			p->drawLine(x - 4, zy, x - 4, y);

			y = yTP(c.second.qBackbeat);
			p->setPen(Qt::darkBlue);
			p->drawLine(x - 2, zy, x - 2, y);

			y = yTP(c.second.forebeat);
			p->setPen(Qt::darkCyan);
			p->drawLine(x, zy, x, y);

			y = yTP(c.second.mltAnchor);
			p->setPen(Qt::darkGreen);
			p->drawLine(x + 2, zy, x + 2, y);

			y = yTP(-c.first);
			p->setPen(Qt::gray);
			p->drawLine(x + 4, zy, x + 4, y);
		}
	}

	using Grapher::drawLineGraph;
	template <class _T>
	void drawLineGraph(vector<_T> const& _data, size_t _off, QColor _color, QBrush const& _fillToZero = Qt::NoBrush, float _width = 0.f) const
	{
		int s = _data.size();
		QPoint l;
		for (int i = 0; i < s; ++i)
		{
			float const& f = *(float*)(intptr_t(&_data[i]) + _off);
			int zy = yP(0.f);
			QPoint h(xTP(i), yTP(f));
			if (i)
			{
				if (_fillToZero != Qt::NoBrush)
				{
					p->setPen(Qt::NoPen);
					p->setBrush(_fillToZero);
					p->drawPolygon(QPolygon(QVector<QPoint>() << QPoint(h.x(), zy) << h << l << QPoint(l.x(), zy)));
				}
				p->setPen(QPen(_color, _width));
				p->drawLine(QLine(l, h));
			}
			l = h;
		}
	}

	void drawLinesGraph(vector<Tempo> const& _data)
	{
		drawLineGraph(_data, offsetof(Tempo, prior), QColor(128, 128, 128));
		drawLineGraph(_data, offsetof(Tempo, Wstrength), QColor(0, 0, 0));
		drawLineGraph(_data, offsetof(Tempo, Hstrength), QColor(100, 200, 0));
		drawLineGraph(_data, offsetof(Tempo, Qstrength), QColor(200, 100, 0));
		drawLineGraph(_data, offsetof(Tempo, Dstrength), QColor(0, 0, 255));
		drawLineGraph(_data, offsetof(Tempo, Tstrength), QColor(255, 0, 0));
	}

	void drawLinesGraph(vector<Start> const& _data)
	{
		drawLineGraph(_data, offsetof(Start, forebeat), QColor(0, 0, 0));
		drawLineGraph(_data, offsetof(Start, qBackbeat), QColor(100, 200, 0));
		drawLineGraph(_data, offsetof(Start, eBackbeat), QColor(200, 100, 0));
		drawLineGraph(_data, offsetof(Start, sBackbeat), QColor(255, 0, 0));
		drawLineGraph(_data, offsetof(Start, trailGap), QColor(0, 0, 255));
		drawLineGraph(_data, offsetof(Start, mltAnchor), QColor(128, 128, 128));
	}
};

pair<pair<float, float>, pair<float, float> > DataView::ranges(bool _needX, bool _needY, shared_ptr<AuxGraphsSpec> _spec, vector<StreamEvent> const& _ses)
{
	vector<StreamEvent> tses;
	if (_ses.empty())
	{
		foreach (GraphSpec s, _spec->graphs)
			tses.push_back(c()->eventOf(s.filter, s.nature));
	}
	vector<StreamEvent> const& ses = _ses.empty() ? tses : _ses;

	pair<float, float> xRange = _spec->xRange;
	bool widenX = _needX && (!isFinite(xRange.second) || !isFinite(xRange.first));

	pair<float, float> yRange = _spec->yRange;
	bool widenY = _needY && (!isFinite(yRange.second) || !isFinite(yRange.first));

	if (widenX || widenY)
		for (unsigned i = 0; i < ses.size(); ++i)
			if (std::shared_ptr<AuxFloatVector> av = std::dynamic_pointer_cast<AuxFloatVector>(ses[i].aux()))
			{
				if (widenX)
				{
					widenToFit(xRange, _spec->graphs[i].xC);
					widenToFit(xRange, _spec->graphs[i].xC + _spec->graphs[i].xM * (av->data.size() - 1));
				}
				if (widenY)
				{
					pair<float, float> r = range(av->data);
					assert(isFinite(r.first));
					assert(isFinite(r.second));
					widenToFit(yRange, _spec->graphs[i].yC + _spec->graphs[i].yM * r.first);
					widenToFit(yRange, _spec->graphs[i].yC + _spec->graphs[i].yM * r.second);
				}
			}
			else if (std::shared_ptr<AuxVector<Start> > av = std::dynamic_pointer_cast<AuxVector<Start> >(ses[i].aux()))
			{
				if (widenX)
				{
					widenToFit(xRange, _spec->graphs[i].xC);
					widenToFit(xRange, _spec->graphs[i].xC + _spec->graphs[i].xM * (av->data.size() - 1));
				}
			}
			else if (std::shared_ptr<AuxVector<Tempo> > av = std::dynamic_pointer_cast<AuxVector<Tempo> >(ses[i].aux()))
			{
				if (widenX)
				{
					widenToFit(xRange, _spec->graphs[i].xC);
					widenToFit(xRange, _spec->graphs[i].xC + _spec->graphs[i].xM * (av->data.size() - 1));
				}
/*				if (widenY)
				{
					pair<float, float> r = range(av->data);
					assert(isFinite(r.first));
					assert(isFinite(r.second));
					widenToFit(yRange, _spec->graphs[i].yC + _spec->graphs[i].yM * r.first);
					widenToFit(yRange, _spec->graphs[i].yC + _spec->graphs[i].yM * r.second);
				}*/
			}
			else if (std::shared_ptr<AuxFloatMap> av = std::dynamic_pointer_cast<AuxFloatMap>(ses[i].aux()))
			{
				if (widenY && av->data.size())
				{
					widenToFit(yRange, _spec->graphs[i].yC + _spec->graphs[i].yM * av->data.begin()->first);
					widenToFit(yRange, _spec->graphs[i].yC + _spec->graphs[i].yM * prev(av->data.end())->first);
				}
				if (widenX)
				{
					foreach (auto c, av->data)
						widenToFit(xRange, _spec->graphs[i].xC + _spec->graphs[i].xM * c.second);
				}
			}
			else if (std::shared_ptr<AuxMeterMap> av = std::dynamic_pointer_cast<AuxMeterMap>(ses[i].aux()))
			{
				if (widenX)
				{
					foreach (auto c, av->data)
						widenToFit(xRange, _spec->graphs[i].xC + _spec->graphs[i].xM * c.second.period);
				}
				if (widenY)
				{
					foreach (auto c, av->data)
					{
						widenToFit(yRange, -c.first);
						widenToFit(yRange, c.second.Wstrength);
						widenToFit(yRange, c.second.Hstrength);
						widenToFit(yRange, c.second.Qstrength);
						widenToFit(yRange, c.second.Dstrength);
					}
				}
			}
			else if (std::shared_ptr<AuxPhaseMap> av = std::dynamic_pointer_cast<AuxPhaseMap>(ses[i].aux()))
			{
				if (widenX)
				{
					foreach (auto c, av->data)
						widenToFit(xRange, _spec->graphs[i].xC + _spec->graphs[i].xM * c.second.phase);
				}
				if (widenY)
				{
					foreach (auto c, av->data)
					{
						widenToFit(yRange, -c.first);
						widenToFit(yRange, c.second.forebeat);
						widenToFit(yRange, c.second.qBackbeat);
						widenToFit(yRange, c.second.eBackbeat);
						widenToFit(yRange, c.second.sBackbeat);
						widenToFit(yRange, c.second.mltAnchor);
						widenToFit(yRange, c.second.trailGap);
					}
				}
			}
	return make_pair(xRange, yRange);
}

void DataView::doRender(QGLFramebufferObject* _fbo)
{
	shared_ptr<AuxGraphsSpec> spec = m_spec.lock();
	if (!spec)
		return;
	QPainter p(_fbo);

	vector<StreamEvent> ses;
	foreach (GraphSpec s, spec->graphs)
		ses.push_back(c()->eventOf(s.filter, s.nature));

	pair<float, float> xRange;
	pair<float, float> yRange;

	if (m_xRangeSpec.lock())
	{
		auto rxy = ranges(true, false, m_xRangeSpec.lock());
		xRange = rxy.first;
	}

	auto rxy = ranges(!m_xRangeSpec.lock(), true, spec, ses);

	if (!m_xRangeSpec.lock())
		xRange = rxy.first;
	yRange = rxy.second;

	OurGrapher g;
	g.init(&p, xRange, yRange, spec->xLabel, spec->yLabel, spec->pLabel, 30, m_xRangeSpec.lock() ? 0 : 16);
	if (g.drawAxes())
	{
		for (unsigned i = 0; i < ses.size(); ++i)
		{
			GraphSpec const& s = spec->graphs[i];
			StreamEvent const& e = ses[i];
			g.setDataTransform(s.xM, s.xC, s.yM, s.yC);
			if (s.filter == NoEvent)
			{
				switch (s.type)
				{
				case LineChart:
					g.drawLineGraph(s.f, toQColor(s.primary));
					break;
				case FilledLineChart:
					g.drawLineGraph(s.f, toQColor(s.primary), toQColor(s.primary, 47));
					break;
				default:;
				}
			}
			else
			{
				if (!e.aux())
					switch (s.type)
					{
					case RuleY:
						g.ruleY(e.strength, toQColor(s.primary));
						break;
					default:;
					}
				else if (auto av = std::dynamic_pointer_cast<AuxFloatVector>(e.aux()))
					switch (s.type)
					{
					case LineChart:
						g.drawLineGraph(av->data, toQColor(s.primary));
						break;
					case FilledLineChart:
						g.drawLineGraph(av->data, toQColor(s.primary), toQColor(s.primary, 47));
						break;
					case LabelPeaks:
						g.labelYOrderedPoints(parabolicPeaks(av->data));
						break;
					default:;
					}
				else if (auto av = std::dynamic_pointer_cast<AuxMeterMap>(e.aux()))
				{
					switch(s.type)
					{
					case MeterExplanation:
						g.drawMeterExplanation(av->data);
						break;
					default:;
					}
				}
				else if (auto av = std::dynamic_pointer_cast<AuxPhaseMap>(e.aux()))
				{
					switch(s.type)
					{
					case PhaseExplanation:
						g.drawPhaseExplanation(av->data);
						break;
					default:;
					}
				}
				else if (auto av = std::dynamic_pointer_cast<AuxFloatMap>(e.aux()))
					switch (s.type)
					{
					case LabeledPoints:
						g.labelYOrderedPoints(av->data, 100, 0);
						break;
					default:;
					}
				else if (auto av = std::dynamic_pointer_cast<AuxVector<Tempo> >(e.aux()))
					switch (s.type)
					{
					case LinesChart:
						g.drawLinesGraph(av->data);
						break;
					default:;
					}
				else if (auto av = std::dynamic_pointer_cast<AuxVector<Start> >(e.aux()))
					switch (s.type)
					{
					case LinesChart:
						g.drawLinesGraph(av->data);
						break;
					default:;
					}
			}
		}
	}
}
