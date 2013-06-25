#pragma once

#include <QQuickItem>
#include <QVector3D>
#include <QSGGeometryNode>
#include "Noted.h"
#include "GraphView.h"
#include "TimelineItem.h"

class QSGTexture;

class GraphItem: public TimelineItem
{
	Q_OBJECT

public:
	GraphItem(QQuickItem* _p = nullptr);

	QVector3D yScaleHint() const;
	QVector3D yScale() const { return m_yScale; }
	int yMode() const { return m_yMode; }
	void setYScale(QVector3D _s) { m_yScale = _s; yScaleChanged(); }
	void setYMode(int _m) { m_yMode = _m; yScaleChanged(); }

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
	void onGraphAdded(lb::GraphMetadata const&);
	void onGraphRemoved(lb::GraphMetadata const&);
	void invalidate() { m_invalidated = true; update(); }

protected:
	Q_PROPERTY(QString url MEMBER m_url NOTIFY urlChanged)
	Q_PROPERTY(QVector3D yScale READ yScale WRITE setYScale NOTIFY yScaleChanged)
	Q_PROPERTY(QVector3D yScaleHint READ yScaleHint NOTIFY yScaleHintChanged)
	Q_PROPERTY(int yMode READ yMode WRITE setYMode NOTIFY yScaleChanged)
	Q_PROPERTY(bool highlight MEMBER m_highlight NOTIFY highlightChanged)
	Q_PROPERTY(bool graphAvailable READ graphAvailable NOTIFY availabilityChanged)
	Q_PROPERTY(bool dataAvailable READ dataAvailable NOTIFY availabilityChanged)

	virtual QSGNode* updatePaintNode(QSGNode* _old, UpdatePaintNodeData*);

	QString m_url;
	QVector3D m_yScale = QVector3D(0, 1, 0);
	int m_yMode = 0;		///< 0 -> yScale, 1 -> hint
	bool m_highlight = false;

	void setAvailability(bool _graph, bool _data);
	bool m_graphAvailable = false;
	bool m_dataAvailable = false;

	mutable QSGGeometry* m_spectrumQuad = nullptr;
	QSGGeometry* spectrumQuad() const;

	// At current LOD (m_lod)
	QSGNode* geometryPage(unsigned _index, lb::GraphMetadata _g, DataSetFloatPtr _ds);
	void killCache();
	bool m_invalidated = true;	// Cache is invalid.
	int m_lod = -1;
	mutable QHash<QPair<int, unsigned>, QPair<QSGNode*, QSGTexture*>> m_geometryCache;
};
