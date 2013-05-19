#pragma once

#include <QObject>
#include <QSet>
#include <QMutex>
#include <EventCompiler/EventCompiler.h>
#include "EventsStore.h"
#include "CausalAnalysis.h"

class EventsView;
class Timeline;

class EventsManFace: public QObject
{
	Q_OBJECT

public:
	EventsManFace(QObject* _p): QObject(_p) {}

	void registerStore(EventsStore* _es);
	void unregisterStore(EventsStore* _es);
	QSet<EventsStore*> eventsStores() const { return m_stores; }

	virtual void registerEventsView(EventsView* _ev) = 0;
	virtual void unregisterEventsView(EventsView* _ev) = 0;
	virtual QSet<EventsView*> eventsViews() const = 0;
	virtual lb::EventCompiler findEventCompiler(QString const& _name) const = 0;
	virtual QString getEventCompilerName(lb::EventCompilerImpl* _ec) const = 0;

	virtual CausalAnalysisPtr compileEventsAnalysis() const { return m_compileEventsAnalysis; }
	virtual CausalAnalysisPtr collateEventsAnalysis() const { return m_collateEventsAnalysis; }

public slots:
	void noteEventCompilersChanged();
	void notePluginDataChanged();

signals:
	void storesChanged();

protected:
	CausalAnalysisPtr m_compileEventsAnalysis;
	CausalAnalysisPtr m_collateEventsAnalysis;
	QSet<EventsStore*> m_stores;
};

