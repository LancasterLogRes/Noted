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

#include <Common/Common.h>
#include <QGLWidget>

class RenderThread;

class Prerendered: public QGLWidget
{
	Q_OBJECT

	friend class RenderThread;

public:
	Prerendered(QWidget* _p);
	~Prerendered();

	bool shouldRepaint() const { return !size().isEmpty() && isVisible() && (needsRepaint() || shouldRerender()); }
	bool shouldRerender() const { return needsRerender(); }

public slots:
	void repaint() { m_needsRepaint = true; }
	void rerender() { m_needsRerender = true; }

protected:
	virtual bool needsRepaint() const { return m_needsRepaint; }
	virtual bool needsRerender() const { return m_needsRerender; }

	virtual void initializeGL();
	virtual void resizeGL(int _w, int _h);
	virtual void paintGL() { paintGL(size()); }
	virtual void paintGL(QSize);
	virtual void renderGL(QSize) {}

	virtual void paintEvent(QPaintEvent*);
	virtual void hideEvent(QHideEvent*);
	virtual void closeEvent(QCloseEvent*);
	virtual void resizeEvent(QResizeEvent* _e);

	void quit();

private:
	bool serviceRender();

	RenderThread* m_renderThread = nullptr;

	bool m_needsRepaint = true;
	bool m_needsRerender = true;
	QSize m_resize;
	QSize m_size;
};
