#pragma once

#include <QQuickItem>
#include <QVector3D>
#include <QQuickPaintedItem>
#include <QSGGeometryNode>
#include "Noted.h"
#include "GraphView.h"
#include "TimelineItem.h"

class GraphItem: public TimelineItem
{
	Q_OBJECT

public:
	GraphItem(QQuickItem* _p = nullptr);

	QVector3D yScaleHint() const;
	float yFromHint() const { return yScaleHint().x(); }
	float yDeltaHint() const { return yScaleHint().y(); }
	float yFrom() const { return m_yFrom; }
	float yDelta() const { return m_yDelta; }
	int yMode() const { return m_yMode; }
	void setYFrom(float _v) { m_yFrom = _v; yScaleChanged(); update(); }
	void setYDelta(float _v) { m_yDelta = _v; yScaleChanged(); update(); }
	void setYMode(int _m) { m_yMode = _m; yScaleChanged(); update(); }

	bool graphAvailable() const { return m_graphAvailable; }
	bool dataAvailable() const { return m_dataAvailable; }

signals:
	void urlChanged(QString _url);
	void yScaleChanged();
	void highlightChanged();
	void yScaleHintChanged();
	void availabilityChanged();

private slots:
	void onDataComplete(DataKey);
	void onGraphAdded(GraphMetadata const&);

protected:
	Q_PROPERTY(QString url MEMBER m_url NOTIFY urlChanged)
	Q_PROPERTY(QVector3D yScaleHint READ yScaleHint NOTIFY yScaleHintChanged)
	Q_PROPERTY(float yFrom READ yFrom WRITE setYFrom NOTIFY yScaleChanged)
	Q_PROPERTY(float yDelta READ yDelta WRITE setYDelta NOTIFY yScaleChanged)
	Q_PROPERTY(int yMode READ yMode WRITE setYMode NOTIFY yScaleChanged)
	Q_PROPERTY(bool highlight MEMBER m_highlight NOTIFY highlightChanged)
	Q_PROPERTY(bool graphAvailable READ graphAvailable NOTIFY availabilityChanged)
	Q_PROPERTY(bool dataAvailable READ dataAvailable NOTIFY availabilityChanged)

	virtual QSGNode* updatePaintNode(QSGNode* _old, UpdatePaintNodeData*);

	QString m_url;
	float m_yFrom = 0;
	float m_yDelta = 1;
	int m_yMode = 0;		///< 0 -> yFrom/yDelta, 1 /*-> auto (global)*/, 2 -> hint
	bool m_highlight = false;

	void setAvailability(bool _graph, bool _data);
	bool m_graphAvailable = false;
	bool m_dataAvailable = false;

	mutable QSGGeometry* m_spectrumQuad = nullptr;

	// At current LOD (m_lod)
	QSGNode* geometryPage(unsigned _index, GraphMetadata _g, DataSetFloatPtr _ds);
	QSGGeometry* spectrumQuad() const;
	void killCache();
	bool m_invalidated = true;	// Cache is invalid.
	int m_lod = -1;
	mutable QMap<unsigned, QSGNode*> m_geometryCache;
};
