#pragma once

#include <QQuickItem>
#include <QVector3D>
#include <QSGGeometryNode>
#include "Noted.h"
#include "GraphView.h"
#include "TimelineItem.h"

class CursorGraphItem: public QQuickItem
{
	Q_OBJECT

public:
	CursorGraphItem(QQuickItem* _p = nullptr);

	QVector3D yScaleHint() const;
	float yFromHint() const { return yScaleHint().x(); }
	float yDeltaHint() const { return yScaleHint().y(); }
	float yFrom() const { return m_yFrom; }
	float yDelta() const { return m_yDelta; }
	int yMode() const { return m_yMode; }
	void setYFrom(float _v) { m_yFrom = _v; yScaleChanged(); update(); }
	void setYDelta(float _v) { m_yDelta = _v; yScaleChanged(); update(); }
	void setYMode(int _m) { m_yMode = _m; yScaleChanged(); update(); }

	QVector3D xScaleHint() const;
	float xFromHint() const { return xScaleHint().x(); }
	float xDeltaHint() const { return xScaleHint().y(); }
	float xFrom() const { return m_xFrom; }
	float xDelta() const { return m_xDelta; }
	int xMode() const { return m_xMode; }
	void setXFrom(float _v) { m_xFrom = _v; yScaleChanged(); update(); }
	void setXDelta(float _v) { m_xDelta = _v; yScaleChanged(); update(); }
	void setXMode(int _m) { m_xMode = _m; yScaleChanged(); update(); }

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

	float m_xFrom = 0;
	float m_xDelta = 1;
	int m_xMode = 0;		///< 0 -> xFrom/xDelta, 1 -> hint

	float m_yFrom = 0;
	float m_yDelta = 1;
	int m_yMode = 0;		///< 0 -> yFrom/yDelta, 1 -> hint

	bool m_highlight = false;

	void setAvailability(bool _graph, bool _data);
	bool m_graphAvailable = false;
	bool m_dataAvailable = false;

	bool m_invalidated = true;

	mutable QSGGeometry* m_quad = nullptr;
	QSGGeometry* quad() const;
};
