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

	QVector3D xScaleHint() const;
	QVector3D xScale() const { return m_xScale; }
	int xMode() const { return m_xMode; }
	void setXScale(QVector3D _v) { m_xScale = _v; xScaleChanged(); }
	void setXMode(int _m) { m_xMode = _m; xScaleChanged(); }

	QVector3D yScaleHint() const;
	QVector3D yScale() const { return m_yScale; }
	int yMode() const { return m_yMode; }
	void setYScale(QVector3D _v) { m_yScale = _v; yScaleChanged(); }
	void setYMode(int _m) { m_yMode = _m; yScaleChanged(); }

	bool graphAvailable() const { return m_graphAvailable; }
	bool dataAvailable() const { return m_dataAvailable; }

signals:
	void urlChanged(QString _url);
	void xScaleChanged();
	void yScaleChanged();
	void highlightChanged();
	void xScaleHintChanged();
	void yScaleHintChanged();
	void availabilityChanged();
	void colorChanged();

private slots:
	void onDataComplete(DataKey);
	void onGraphAdded(lb::GraphMetadata const&);

protected:
	Q_PROPERTY(QString url MEMBER m_url NOTIFY urlChanged)
	Q_PROPERTY(QVector3D xScaleHint READ xScaleHint NOTIFY xScaleHintChanged)
	Q_PROPERTY(QVector3D xScale READ xScale WRITE setXScale NOTIFY xScaleChanged)
	Q_PROPERTY(int xMode READ xMode WRITE setXMode NOTIFY xScaleChanged)
	Q_PROPERTY(QVector3D yScaleHint READ yScaleHint NOTIFY yScaleHintChanged)
	Q_PROPERTY(QVector3D yScale READ yScale WRITE setYScale NOTIFY yScaleChanged)
	Q_PROPERTY(int yMode READ yMode WRITE setYMode NOTIFY yScaleChanged)
	Q_PROPERTY(bool highlight MEMBER m_highlight NOTIFY highlightChanged)
	Q_PROPERTY(bool graphAvailable READ graphAvailable NOTIFY availabilityChanged)
	Q_PROPERTY(bool dataAvailable READ dataAvailable NOTIFY availabilityChanged)
	Q_PROPERTY(QColor color MEMBER m_color NOTIFY colorChanged)

	virtual QSGNode* updatePaintNode(QSGNode* _old, UpdatePaintNodeData*);

	QString m_url;

	QVector3D m_xScale = QVector3D(0, 1, 0);
	int m_xMode = 0;		///< 0 -> xScale, 1 -> hint

	QVector3D m_yScale = QVector3D(0, 1, 0);
	int m_yMode = 0;		///< 0 -> yScale, 1 -> hint

	QColor m_color = Qt::black;
	bool m_highlight = false;

	void setAvailability(bool _graph, bool _data);
	bool m_graphAvailable = false;
	bool m_dataAvailable = false;

	bool m_invalidated = true;

	mutable QSGGeometry* m_quad = nullptr;
	QSGGeometry* quad() const;
};
