#pragma once

#include <map>
#include <boost/foreach.hpp>
#include <QPainter>
#include <QMetaType>
#include <Common/Time.h>
#include <Common/Trivial.h>

typedef uint32_t SimpleKey;

LIGHTBOX_STRUCT(2, DataKey, SimpleKey, source, SimpleKey, operation);
inline uint qHash(DataKey _k) { return _k.source ^ SimpleKey(_k.operation << 16) ^ SimpleKey(_k.operation >> 16); }

Q_DECLARE_METATYPE(lb::Time)
Q_DECLARE_METATYPE(SimpleKey)
Q_DECLARE_METATYPE(DataKey)

template <class _FX, class _FY, class _FL>
void drawPeaks(QPainter& _p, std::map<float, float> const& _ps, int _yoffset, _FX _x, _FY _y, _FL _l, int _maxCount = 5)
{
	int ly = _yoffset;
	int pc = 0;
	BOOST_REVERSE_FOREACH (auto peak, _ps)
		if (peak.first > (--_ps.end())->first * .25 && pc++ < _maxCount)
		{
			int x = _x(peak.second);
			int y = _y(peak.first);
			_p.setPen(QColor::fromHsvF(float(pc) / _maxCount, 1.f, 0.5f, 0.5f));
			_p.drawEllipse(QPoint(x, y), 4, 4);
			_p.drawLine(x, y - 4, x, ly + 6);
			QString f = _l(peak.second);
			int fw = _p.fontMetrics().width(f);
			_p.drawLine(x + 16 + fw + 2, ly + 6, x, ly + 6);
			_p.setPen(QColor::fromHsvF(0, 0.f, .35f));
			_p.fillRect(QRect(x+12, ly-6, fw + 8, 12), QBrush(QColor(255, 255, 255, 160)));
			_p.drawText(QRect(x+16, ly-6, 160, 12), Qt::AlignVCenter, f);
			ly += 14;
		}
		else
			break;
}

