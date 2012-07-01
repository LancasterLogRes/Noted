#pragma once

#include <QGLWidget>

#include <Common/Time.h>
#include <Common/Vector3.h>

class LightshowPlugin;
class QSettings;

class GLView: public QGLWidget
{
	Q_OBJECT

public:
	explicit GLView(QWidget* _parent = 0);
	~GLView() {}

	void setPlugin(LightshowPlugin* _p) { m_p = _p; }
	void save(QSettings& _c) const;
	void load(QSettings const& _c);

public slots:

private:
	virtual void initializeGL();
	virtual void resizeGL(int _w, int _h);
	virtual void paintGL();

	virtual void mousePressEvent(QMouseEvent* _e);
	virtual void mouseReleaseEvent(QMouseEvent* _e);
	virtual void mouseMoveEvent(QMouseEvent* _e);
	virtual void wheelEvent(QWheelEvent* _e);

	int m_lightCone;

	QPoint m_original;

	Lightbox::Vector3 m_up;
	Lightbox::Vector3 m_view;
	Lightbox::Vector3 m_position;

	float m_speed;
	Lightbox::Time m_lastTime;

	LightshowPlugin* m_p;
};
