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
#include <NotedPlugin/NotedFace.h>
#include "EventsEditScene.h"
#include "SpikeItem.h"
#include "EventsEditor.h"

using namespace std;
using namespace Lightbox;

EventsEditor::EventsEditor(QWidget* _parent, QString _filename):
	QGraphicsView		(_parent),
	m_c					(nullptr),
	m_filename			(_filename),
	m_lastTimerDirty	(true),
	m_eventsDirty		(true)
{
	setFrameShape(NoFrame);

	connect(c(), SIGNAL(eventsChanged()), this, SLOT(sourceChanged()));
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

	setDragMode(RubberBandDrag);


	m_scene = make_shared<EventsEditScene>();
	setScene(&*m_scene);
	connect(&*m_scene, SIGNAL(newScale()), SLOT(onViewParamsChanged()));
	connect(c(), SIGNAL(offsetChanged()), SLOT(onViewParamsChanged()));
	connect(c(), SIGNAL(durationChanged()), SLOT(onViewParamsChanged()));
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setMinimumHeight(78);
	setMaximumHeight(78);
	onViewParamsChanged();

	setContextMenuPolicy(Qt::ActionsContextMenu);
	QAction* a;
	insertAction(0, (a = new QAction("&Save", this)));
	connect(a, SIGNAL(triggered()), this, SLOT(onSave()));
	insertAction(0, (a = new QAction("&Save As...", this)));
	connect(a, SIGNAL(triggered()), this, SLOT(onSaveAs()));
#define DO(X) \
	insertAction(0, (a = new QAction("Insert " #X, this))); \
	connect(a, SIGNAL(triggered()), this, SLOT(onInsert ## X()));
#include "DoEventTypes.h"
#undef DO
	insertAction(0, (a = new QAction("Cut", this)));
	a->setShortcut(QKeySequence::Cut);
	connect(a, SIGNAL(triggered()), this, SLOT(onCut()));
	insertAction(0, (a = new QAction("Copy", this)));
	a->setShortcut(QKeySequence::Copy);
	connect(a, SIGNAL(triggered()), this, SLOT(onCopy()));
	insertAction(0, (a = new QAction("Paste", this)));
	a->setShortcut(QKeySequence::Paste);
	connect(a, SIGNAL(triggered()), this, SLOT(onPaste()));

	insertAction(0, (a = new QAction("Delete", this)));
	a->setShortcut(QKeySequence::Delete);
	connect(a, SIGNAL(triggered()), this, SLOT(onDelete()));

	if (!_filename.isNull())
		m_scene->loadFrom(_filename);

	startTimer(500);

	initTimeline(c());
}

EventsEditor::~EventsEditor()
{
	QMutexLocker l(&x_events);
	m_events.clear();
	c()->noteEventCompilersChanged();
}

void EventsEditor::save(QSettings& _s) const
{
	_s.setValue(m_filename + ".enabled", m_enabled->isChecked());
}

void EventsEditor::load(QSettings const& _s)
{
	m_enabled->setChecked(_s.value(m_filename + ".enabled", false).toBool());
}

void EventsEditor::mousePressEvent(QMouseEvent* _e)
{
	m_lastScenePosition = mapToScene(_e->pos());
	if (itemAt(_e->pos()) || !(_e->buttons() & Qt::MiddleButton))
	{
		QGraphicsView::mousePressEvent(_e);
		m_draggingTime = Lightbox::UndefinedTime;
	}
	else
	{
		m_draggingTime = c()->timeOf(_e->x());
	}
}

void EventsEditor::mouseReleaseEvent(QMouseEvent* _e)
{
	QGraphicsView::mouseReleaseEvent(_e);
	if (scene()->selectedItems().isEmpty() && _e->button() == Qt::LeftButton)
		c()->setCursor(fromSeconds(mapToScene(_e->pos()).x() / 1000));
}

void EventsEditor::mouseMoveEvent(QMouseEvent* _e)
{
	if (m_draggingTime != Lightbox::UndefinedTime && _e->buttons() & Qt::MiddleButton)
		c()->setTimelineOffset(m_draggingTime - _e->x() * c()->pixelDuration());
	else if (_e->buttons() & Qt::MiddleButton)
	{
		setDragMode(QGraphicsView::NoDrag);
		QGraphicsView::mouseMoveEvent(_e);
		setDragMode(QGraphicsView::RubberBandDrag);
	}
	else
		QGraphicsView::mouseMoveEvent(_e);
}

StreamEvents EventsEditor::events(int _i) const
{
	if (m_enabled->isChecked())
	{
		QMutexLocker l(&x_events);
		if (_i >= 0 && _i < m_events.size())
			return m_events[_i];
	}
	return StreamEvents();
}

void EventsEditor::onEnableChanged(bool)
{
	c()->noteEventCompilersChanged();
}

void EventsEditor::onChanged()
{
	if (m_enabled->isChecked())
		m_lastTimerDirty = false;
	m_eventsDirty = true;
}

void EventsEditor::timerEvent(QTimerEvent*)
{
	if (m_enabled->isChecked())
	{
		if (m_lastTimerDirty)
		{
			m_lastTimerDirty = m_eventsDirty = false;
			{
				QMutexLocker l(&x_events);
				m_events = scene()->events(c()->hop());
			}
			c()->noteEventCompilersChanged();
		}
		else if (m_eventsDirty)
			m_lastTimerDirty = true;
	}
}

void EventsEditor::onCut()
{
	onCopy();
	onDelete();
}

void EventsEditor::onCopy()
{
	QVector<QPair<int, StreamEvent> > d;
	int t = scene()->selectedItems().first()->x();
	foreach (QGraphicsItem* it, scene()->selectedItems())
		if (StreamEventItem* sei = dynamic_cast<StreamEventItem*>(it))
			d.append(QPair<int, StreamEvent>(it->x() - t, sei->streamEvent()));
	m_clipboard = d;
}

void EventsEditor::onPaste()
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

void EventsEditor::onDelete()
{
	foreach (QGraphicsItem* it, scene()->selectedItems())
		delete it;
	scene()->rejigEvents();
}

#define DO(X) \
void EventsEditor::onInsert ## X() \
{ \
	StreamEventItem* it = StreamEventItem::newItem(StreamEvent(X, .125f, 0.f, FromBpm<130>::value, nullptr, -1, Dull, .125f)); \
	scene()->addItem(it); \
	it->setTime(m_lastScenePosition.x()); \
}
#include "DoEventTypes.h"
#undef DO

void EventsEditor::drawBackground(QPainter* _p, QRectF const& _r)
{
	_p->fillRect(_r, Qt::white);
}

void EventsEditor::onRejigRhythm()
{

}

QString EventsEditor::queryFilename()
{
	if (m_filename.isNull() || scene()->isDirty())
		onSave();
	return m_filename;
}

NotedFace* EventsEditor::c() const
{
	if (!m_c)
		m_c = dynamic_cast<NotedFace*>(window());
	if (!m_c)
		m_c = dynamic_cast<NotedFace*>(window()->parentWidget()->window());
	return m_c;
}

void EventsEditor::onViewParamsChanged()
{
	resetTransform();
	double hopsInWidth = toSeconds(c()->visibleDuration()) * 1000;
	double hopsFromBeginning = toSeconds(c()->earliestVisible()) * 1000;
	setSceneRect(hopsFromBeginning, 0, hopsInWidth, height());
	scale(width() / hopsInWidth, 1);
}

void EventsEditor::onSave()
{
	if (!m_filename.isNull())
		m_scene->saveTo(m_filename);
	else
		onSaveAs();
}

void EventsEditor::onSaveAs()
{
	m_filename = QFileDialog::getSaveFileName(this, "Save the Event Stream", QDir::homePath(), "*.xml");
	if (!m_filename.isNull())
		onSave();
}
