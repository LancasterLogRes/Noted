#pragma once

#include <QObject>
#include <QSet>
#include <QMutex>
#include <EventCompiler/EventCompiler.h>
#include "EventsStore.h"
#include "CausalAnalysis.h"

class EventCompilerView;
class Timeline;

class EventsManFace: public QObject
{
	Q_OBJECT

public:
	EventsManFace(QObject* _p): QObject(_p) {}

	void registerStore(EventsStore* _es);
	void unregisterStore(EventsStore* _es);
	QSet<EventsStore*> eventsStores() const { return m_stores; }

	virtual lb::SimpleKey hash() const = 0;

	virtual void registerEventsView(EventCompilerView* _ev) = 0;
	virtual void unregisterEventsView(EventCompilerView* _ev) = 0;
	virtual QSet<EventCompilerView*> eventsViews() const = 0;
	virtual lb::EventCompiler findEventCompiler(QString const& _name) const = 0;
	virtual QString getEventCompilerName(lb::EventCompilerImpl* _ec) const = 0;

	virtual CausalAnalysisPtr compileEventsAnalysis() const { return m_compileEventsAnalysis; }
	virtual CausalAnalysisPtr collateEventsAnalysis() const { return m_collateEventsAnalysis; }

	virtual lb::StreamEvents inWindow(unsigned _i, bool _usePredetermined) const = 0;

public slots:
	void noteEventCompilersChanged();
	void notePluginDataChanged();

signals:
	void eventsChanged();
	void storesChanged();

protected:
	CausalAnalysisPtr m_compileEventsAnalysis;
	CausalAnalysisPtr m_collateEventsAnalysis;
	QSet<EventsStore*> m_stores;
};

