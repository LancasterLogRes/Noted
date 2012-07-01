#include <iostream>
#include <cmath>
#include <vector>
#include <tuple>

#include <QSplitter>
#include <QGraphicsOpacityEffect>
#include <QPushButton>
#include <QDebug>
#include <QFrame>
#include <QPaintEvent>
#include <QPainter>

#include <Faceman/Output.h>

using namespace std;
using namespace Lightbox;

#include "LightshowPlugin.h"
#include "OutputItem.h"
#include "OutputsView.h"

OutputsView::OutputsView(QWidget* _parent, LightshowPlugin* _p):
	Timeline(_parent),
	m_p(_p)
{
	connect(c(), SIGNAL(eventsChanged()), this, SLOT(sourceChanged()));
}

OutputsView::~OutputsView()
{
}

static const int c_radius = 8;
static const int c_thetaR = 4;

static tuple<vector<int>, int, int> outputReq(Output const& _o)
{
	vector<int> lih;
	int h = 0;
	for (unsigned li = 0; li < _o.lightCount(); ++li)
	{
		int lh = 1;		// seperator above
		if (_o.directionLimits(li) == DirectionSegment())
			lh += 1;	// one-pixel for the light
		else
			lh += c_radius * 2 + c_thetaR * 2;	// for the light to make a circle/go up and down
		// TODO: shaking.
		// TODO: gobo & gobo rotation.
		h += lh;
		lih.push_back(lh);
	}
	int reqH = h + 1;	// plus 1 for the last light's bottom separator.
	int actH = max(12, reqH);	// minimum to draw label.
	return make_tuple(lih, reqH, actH);
}

QImage OutputsView::renderOverlay()
{
	vector<int> ys;
	int ly = 0;
	foreach (OutputItem* oi, m_p->outputItems())
	{
		ys.push_back(ly);
		auto r = outputReq(oi->output());
		ly += get<2>(r) + 1;
	}
	ys.push_back(ly);

	QImage ret(160, ly, QImage::Format_ARGB32_Premultiplied);
	ret.fill(qRgba(0, 0, 0, 0));
	QPainter p(&ret);

	int i = 0;
	foreach (OutputItem* oi, m_p->outputItems())
	{
		Output const& o = oi->output();
		QString s;
		if (ys[i + 1] - ys[i] > 20)
			s = QString("%1\n%2-%3").arg(QString::fromStdString(o.nickName())).arg(o.baseChannel()).arg(o.baseChannel()+o.channels());
		else
			s = QString("%1").arg(QString::fromStdString(o.nickName()));
		p.setPen(QPen(Qt::black, 3));
		p.drawText(0, ys[i] - 1, 160, 26, Qt::AlignLeft|Qt::AlignTop, s);
		p.drawText(2, ys[i] - 1, 160, 26, Qt::AlignLeft|Qt::AlignTop, s);
		p.drawText(1, ys[i] - 2, 160, 26, Qt::AlignLeft|Qt::AlignTop, s);
		p.drawText(1, ys[i], 160, 26, Qt::AlignLeft|Qt::AlignTop, s);
		p.setPen(Qt::white);
		p.drawText(1, ys[i] - 1, 160, 26, Qt::AlignLeft|Qt::AlignTop, s);
		++i;
	}
	return ret;
}

void OutputsView::doRender(QImage& _img, int _dx, int _dw)
{
	if (_img.isNull())
		return;
	QPainter p(&_img);
	if (!p.isActive())
		return;

	int h = height();
	QRect r(_dx, 0, _dw, h);

	p.setClipRect(r);

	int ly = 0;
	foreach (OutputItem* oi, m_p->outputItems())
	{
		Output const& o = oi->output();
		int reqH;
		int actH;
		vector<int> lih;
		tie(lih, reqH, actH) = outputReq(o);
		p.fillRect(QRect(_dx, ly, _dw, actH), QColor(16, 16, 16));

		Time h = c()->hop();
		int oly = ly;
		ly += (actH - reqH) / 2 + 1;	// plus 1 for the first light's top separator.
		for (unsigned li = 0; li < o.lightCount(); ++li)
		{
			if (o.directionLimits(li) == DirectionSegment())
			{
				for (int x = max(xOf(0), _dx - 16); x < _dx + _dw + 16 && timeOf(x) / h < c()->numSpectra(); ++x)
				{
					OutputProfile const& op = oi->outputProfile(max<int>(0, timeOf(x) / h), li);
					p.fillRect(x, ly, 1, 1, QColor(op.color.r(), op.color.g(), op.color.b()));
				}
			}
			else
			{
				// TODO: draw staggered direction segment.
				int zy = ly + c_radius;
				int lastY = -1;
				auto centerPhi = o.directionLimits(li).center().phi;
				auto radiusPhi = o.directionLimits(li).phiRange() / 2;
				for (int x = max(xOf(0), _dx - 16); x < _dx + _dw + 16 && timeOf(x) / h < c()->numSpectra(); x += 2)
				{
					OutputProfile const& op = oi->outputProfile(max<int>(0, timeOf(x) / h), li);
					int y = zy + (op.direction.phi - centerPhi) * c_radius / radiusPhi;
					if (lastY != -1)
					{
						p.setPen(QColor(op.color.r(), op.color.g(), op.color.b()));
						p.drawLine(x - 2, lastY, x, y);
					}
					if (x / 2 % (c_thetaR + 2) == 0)
					{
						p.setPen(QColor(255, 255, 255, 96));
						p.drawEllipse(QPoint(x, ly + c_radius * 2 + c_thetaR), c_thetaR, c_thetaR);
						p.setPen(QColor(255, 255, 255, 255));
						p.drawArc(QRectF(x, ly + c_radius * 2 + c_thetaR, 0, 0).adjusted(-c_thetaR, -c_thetaR, c_thetaR, c_thetaR), (op.direction.theta - Pi / 12) * 5760 / Pi, Pi / 6 * 5760 / Pi);
					}
					lastY = y;
				}
			}
			ly += lih[li];
		}
		ly = oly + actH + 1;	// to allow the background through for a seperator.
	}
	setMinimumHeight(ly - 1);
	setMaximumHeight(max(1, ly - 1));
}
