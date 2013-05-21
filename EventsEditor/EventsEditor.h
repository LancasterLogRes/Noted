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

#pragma once

#include <memory>

#include <QMutex>
#include <QString>
#include <QGraphicsView>
#include <EventCompiler/StreamEvent.h>
#include <NotedPlugin/Timeline.h>
#include <NotedPlugin/EventsStore.h>

class NotedFace;
class EventsEditScene;
class QPushButton;
class QSettings;

class EventsGraphicsView: public QGraphicsView, public EventsStore, public Timeline
{
	Q_OBJECT

public:
	EventsGraphicsView(QWidget* _parent, QString _filename = "");
	~EventsGraphicsView();

	virtual QString niceName() const { return m_filename; }

	bool isEnabled() const;
	bool isIndependent() const { return true; } // TODO: remove
	virtual bool isMutable() const { return true; }

	QString queryFilename();
	EventsEditScene* scene() const { return &*m_scene; }
	virtual lb::StreamEvents events(int _i) const;
	virtual lb::StreamEvents cursorEvents() const;
	virtual unsigned eventCount() const { return m_events.size(); }
	void setEvents(QList<lb::StreamEvents> const& _es, int _forceChannel = -1);

	void save(QSettings& _c) const;
	void load(QSettings const& _c);

public slots:
	void onViewParamsChanged();
	void onSave();
	void onSaveAs();
	void onEnableChanged(bool);

	void onCut();
	void onCopy();
	void onPaste();
	void onDelete();

	void onInsertAttack();
	void onInsertSustain();
	void onInsertDecay();
	void onInsertRelease();
	void onInsertPeriodSet();
	void onInsertPeriodTweak();
	void onInsertPeriodReset();
	void onInsertSyncPoint();

	void onRejigRhythm();
	void clearEvents();

	void onChanged(bool _requiresRecompile);
	void onChanged() { onChanged(true); }

protected:
	// TODO: Move to use DataSet.
	QList<lb::StreamEvents> m_events;
	mutable QMutex x_events;

private:
	virtual void resizeEvent(QResizeEvent*) { onViewParamsChanged(); }
	virtual void mousePressEvent(QMouseEvent* _e);
	virtual void mouseReleaseEvent(QMouseEvent* _e);
	virtual void mouseMoveEvent(QMouseEvent* _e);
	virtual void drawBackground(QPainter* _e, QRectF const& _r);
	virtual void timerEvent(QTimerEvent*);

	std::shared_ptr<EventsEditScene> m_scene;

	QString m_filename;

	lb::Time m_draggingTime;
	QPointF m_lastScenePosition;
	QVector<QPair<int, lb::StreamEvent> > m_clipboard;
	QPushButton* m_enabled;
	bool m_lastTimerDirty;
	bool m_eventsDirty;
};

class EventsEditor: public EventsGraphicsView
{
	Q_OBJECT

public:
	EventsEditor(QWidget* _parent, QString _filename = ""): EventsGraphicsView(_parent, _filename) {}
	virtual ~EventsEditor() {}
};
