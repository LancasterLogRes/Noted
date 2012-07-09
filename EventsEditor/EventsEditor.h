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

class NotedFace;
class EventsEditScene;
class QPushButton;
class QSettings;

class EventsEditor: public QGraphicsView, public EventsStore, public Timeline
{
	Q_OBJECT

public:
	EventsEditor(QWidget* _parent, QString _filename = "");

	virtual QString niceName() const { return m_filename; }
	virtual QWidget* widget() { return this; }

	QString queryFilename();
	EventsEditScene* scene() const { return &*m_scene; }
	NotedFace* c() const;
	virtual Lightbox::StreamEvents events(int _i) const;

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

	void onInsertSpike();
	void onInsertChain();
	void onInsertPeriodSet();
	void onInsertPeriodTweak();
	void onInsertPeriodReset();
	void onInsertSustain();
	void onInsertEndSustain();
	void onInsertBackSustain();
	void onInsertEndBackSustain();

	void onRejigRhythm();

	void onChanged();

private:
	virtual void resizeEvent(QResizeEvent*) { onViewParamsChanged(); }
	virtual void mousePressEvent(QMouseEvent* _e);
	virtual void mouseReleaseEvent(QMouseEvent* _e);
	virtual void mouseMoveEvent(QMouseEvent* _e);
	virtual void drawBackground(QPainter* _e, QRectF const& _r);
	virtual void timerEvent(QTimerEvent*);

	mutable NotedFace* m_c;

	std::shared_ptr<EventsEditScene> m_scene;

	QString m_filename;

	Lightbox::Time m_draggingTime;
	QPointF m_lastScenePosition;
	QVector<QPair<int, Lightbox::StreamEvent> > m_clipboard;
	QPushButton* m_enabled;
	bool m_lastTimerDirty;
	bool m_eventsDirty;

	QList<Lightbox::StreamEvents> m_events;
	mutable QMutex x_events;
};
