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

#include <memory>
#include <QtGui>
#include <QtWidgets>
#include <QGraphicsOpacityEffect>
#include <NotedPlugin/NotedFace.h>
#include "EventsEditScene.h"
#include "AttackItem.h"
#include "EventsEditor.h"
using namespace std;
using namespace lb;

EventsGraphicsView::EventsGraphicsView(QWidget* _parent, QString _filename):
	QGraphicsView		(_parent),
	m_filename			(_filename),
	m_lastTimerDirty	(true),
	m_eventsDirty		(true)
{
	setFrameShape(NoFrame);

	{
		connect(NotedFace::events(), SIGNAL(eventsChanged()), this, SLOT(rerender()));
		auto oe = []() -> QGraphicsEffect* { auto ret = new QGraphicsOpacityEffect; ret->setOpacity(0.7); return ret; };

		QPushButton* b = new QPushButton(this);
		b->setGeometry(0, 0, 23, 23);
		b->setText("X");
		b->setGraphicsEffect(oe());
		connect(b, SIGNAL(clicked()), SLOT(deleteLater()));

		m_enabled = new QPushButton(this);
		m_enabled->setGeometry(25, 0, 23, 23);
		m_enabled->setText("U");
		m_enabled->setCheckable(true);
		m_enabled->setChecked(true);
		m_enabled->setGraphicsEffect(oe());
		connect(m_enabled, SIGNAL(toggled(bool)), SLOT(onEnableChanged(bool)));
	}
	setDragMode(RubberBandDrag);

	m_scene = make_shared<EventsEditScene>();
	setScene(&*m_scene);
	connect(NotedFace::view(), SIGNAL(gutterWidthChanged(int)), SLOT(onViewParamsChanged()));
	connect(&*m_scene, SIGNAL(newScale()), SLOT(onViewParamsChanged()));
	connect(NotedFace::view(), SIGNAL(parametersChanged(lb::Time, lb::Time)), SLOT(onViewParamsChanged()));
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setMinimumHeight(16);
	setMaximumHeight(144);
	onViewParamsChanged();

	setContextMenuPolicy(Qt::ActionsContextMenu);
	QAction* a;
	insertAction(0, (a = new QAction("&Save", this)));
	connect(a, SIGNAL(triggered()), this, SLOT(onSave()));
	insertAction(0, (a = new QAction("&Save As...", this)));
	connect(a, SIGNAL(triggered()), this, SLOT(onSaveAs()));
	if (isMutable())
	{
#define DO(X) \
	insertAction(0, (a = new QAction("Insert " #X, this))); \
	connect(a, SIGNAL(triggered()), this, SLOT(onInsert ## X()));
#include "DoEventTypes.h"
#undef DO
	insertAction(0, (a = new QAction("Cut", this)));
	a->setShortcut(QKeySequence::Cut);
	connect(a, SIGNAL(triggered()), this, SLOT(onCut()));
	}
	insertAction(0, (a = new QAction("Copy", this)));
	a->setShortcut(QKeySequence::Copy);
	connect(a, SIGNAL(triggered()), this, SLOT(onCopy()));
	if (isMutable())
	{
	insertAction(0, (a = new QAction("Paste", this)));
	a->setShortcut(QKeySequence::Paste);
	connect(a, SIGNAL(triggered()), this, SLOT(onPaste()));

	insertAction(0, (a = new QAction("Delete", this)));
	a->setShortcut(QKeySequence::Delete);
	connect(a, SIGNAL(triggered()), this, SLOT(onDelete()));
	}
	if (!_filename.isNull())
		m_scene->loadFrom(_filename);

	startTimer(500);
}

EventsGraphicsView::~EventsGraphicsView()
{
	QMutexLocker l(&x_eventsCache);
	m_eventsCache.clear();
	NotedFace::events()->noteEventCompilersChanged();
}

void EventsGraphicsView::clearEvents()
{
	QMutexLocker l(&x_eventsCache);
	m_eventsCache.clear();
	scene()->clear();
}

bool EventsGraphicsView::isEnabled() const
{
	return m_enabled && m_enabled->isChecked();
}

void EventsGraphicsView::save(QSettings& _s) const
{
	_s.setValue(m_filename + ".enabled", m_enabled->isChecked());
}

void EventsGraphicsView::load(QSettings const& _s)
{
	m_enabled->setChecked(_s.value(m_filename + ".enabled", false).toBool());
}

void EventsGraphicsView::mousePressEvent(QMouseEvent* _e)
{
	m_lastScenePosition = mapToScene(_e->pos());
	if ((itemAt(_e->pos()) || !(_e->buttons() & Qt::MiddleButton)))// && isMutable())
	{
		QGraphicsView::mousePressEvent(_e);
		m_draggingTime = lb::UndefinedTime;
	}
	else
	{
		m_draggingTime = NotedFace::view()->timeOf(_e->x());
	}
}

void EventsGraphicsView::mouseReleaseEvent(QMouseEvent* _e)
{
	QGraphicsView::mouseReleaseEvent(_e);
	if (scene()->selectedItems().isEmpty() && _e->button() == Qt::LeftButton)
		NotedFace::audio()->setCursor(fromSeconds(mapToScene(_e->pos()).x() / 1000), true);
}

void EventsGraphicsView::mouseMoveEvent(QMouseEvent* _e)
{
	if (m_draggingTime != lb::UndefinedTime && _e->buttons() & Qt::MiddleButton)
		NotedFace::view()->setOffset(m_draggingTime - _e->x() * NotedFace::view()->pitch());
	else if (_e->buttons() & Qt::MiddleButton)
	{
		setDragMode(QGraphicsView::NoDrag);
		QGraphicsView::mouseMoveEvent(_e);
		setDragMode(QGraphicsView::RubberBandDrag);
	}
	else
		QGraphicsView::mouseMoveEvent(_e);
}

StreamEvents EventsGraphicsView::cursorEvents() const
{
	return events(NotedFace::audio()->isCausal() ? NotedFace::compute()->causalCursorIndex() : -1);
}

StreamEvents EventsGraphicsView::events(int _i) const
{
	if (m_enabled->isChecked())
	{
		QMutexLocker l(&x_eventsCache);
		if (_i >= 0 && _i < m_eventsCache.size())
			return m_eventsCache[_i];
	}
	return StreamEvents();
}

void EventsGraphicsView::onEnableChanged(bool)
{
	NotedFace::events()->noteEventCompilersChanged();
}

void EventsGraphicsView::onChanged(bool _requiresRecompile)
{
	if (!m_cleanPopulate)
	{
		if (m_enabled->isChecked())
			m_lastTimerDirty = false;
		m_eventsDirty = _requiresRecompile;
	}
}

void EventsGraphicsView::timerEvent(QTimerEvent*)
{
	if (m_enabled->isChecked())
	{
		if (m_lastTimerDirty)
		{
			m_lastTimerDirty = m_eventsDirty = false;
			{
				QMutexLocker l(&x_eventsCache);
				m_eventsCache = scene()->events(NotedFace::audio()->hop());
			}
			NotedFace::events()->noteEventCompilersChanged();
		}
		else if (m_eventsDirty)
			m_lastTimerDirty = true;
	}
}

void EventsGraphicsView::onCut()
{
	onCopy();
	onDelete();
}

void EventsGraphicsView::onCopy()
{
	QVector<QPair<int, StreamEvent> > d;
	if (scene()->selectedItems().size())
	{
		int t = scene()->selectedItems().first()->x();
		foreach (QGraphicsItem* it, scene()->selectedItems())
			if (StreamEventItem* sei = dynamic_cast<StreamEventItem*>(it))
				d.append(QPair<int, StreamEvent>(it->x() - t, sei->streamEvent()));
		m_clipboard = d;
	}
}

void EventsGraphicsView::onPaste()
{
	foreach (QGraphicsItem* it, scene()->selectedItems())
		it->setSelected(false);
	foreach (auto p, m_clipboard)
	{
		StreamEventItem* it = StreamEventItem::newItem(p.second);
		scene()->addItem(it);
		it->setTime(m_lastScenePosition.x() + p.first);
		it->setSelected(true);
	}
	scene()->rejigEvents();
}

void EventsGraphicsView::onDelete()
{
	if (isMutable())
	{
		for (QGraphicsItem* it: scene()->selectedItems())
		{
			if (auto sei = dynamic_cast<StreamEventItem*>(it))
				m_scene->setDirty(sei->isCausal());
			delete it;
		}
		scene()->rejigEvents();
	}
}

#define DO(X) \
void EventsGraphicsView::onInsert ## X() \
{ \
	StreamEventItem* it = StreamEventItem::newItem(StreamEvent(X, .125f, 0.f, Dull, .5f, .5f)); \
	scene()->addItem(it); \
	it->setChannel(max<int>((m_lastScenePosition.y() - 16) / 32, 0)); \
	it->setTime(m_lastScenePosition.x()); \
}
#include "DoEventTypes.h"
#undef DO

void EventsGraphicsView::drawBackground(QPainter* _p, QRectF const& _r)
{
	for (int x = -1; x < 4; ++x)
	{
		QLinearGradient g(QPointF(0, x == -1 ? 0 : (16 + x * 32)), QPointF(0, 16 + (x+1) * 32));
		g.setColorAt(0, QColor::fromHsv(0, 0, x == -1 ? 86 : 255));
		g.setColorAt(1, QColor::fromHsv(0, 0, x == -1 ? 48 : 232));
		_p->fillRect(QRectF(_r.left(), 16 + x * 32, _r.width(), 32), g);
	}
}

void EventsGraphicsView::onRejigRhythm()
{

}

QString EventsGraphicsView::queryFilename()
{
	cnote << isMutable();
	if ((m_filename.isNull() || scene()->isDirty()) && isMutable())
		onSave();
	return m_filename;
}

void EventsGraphicsView::onViewParamsChanged()
{
	resetTransform();
	int w = width();
	int vw = NotedFace::view()->width();
	int gutter = w - vw;
	Time dur = NotedFace::view()->visibleDuration();
	Time ear = NotedFace::view()->earliestVisible();

	// But there's a gutter...
	Time gutterTime = gutter * dur / (w - gutter);
	ear -= gutterTime;
	dur += gutterTime;

	double msInWidth = toSeconds(dur) * 1000;
	double msFromBeginning = toSeconds(ear) * 1000;
	setSceneRect(msFromBeginning, 0, msInWidth, height());
	scale(w / msInWidth, 1);
}

void EventsGraphicsView::onSave()
{
	if (!m_filename.isNull())
		m_scene->saveTo(m_filename);
	else
		onSaveAs();
}

void EventsGraphicsView::onSaveAs()
{
	m_filename = QFileDialog::getSaveFileName(this, "Save the Event Stream", QDir::homePath(), "*.xml");
	if (!m_filename.isNull())
		onSave();
}
