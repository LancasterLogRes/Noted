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

#include <QPainter>
#ifdef Q_OS_MAC
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#include <Common/Common.h>

#include "NotedFace.h"
#include "Prerendered.h"

using namespace std;
using namespace Lightbox;

NotedFace* Prerendered::c() const
{
	if (!m_c)
		m_c = dynamic_cast<NotedFace*>(window());
	if (!m_c)
		m_c = dynamic_cast<NotedFace*>(window()->parentWidget()->window());
	return m_c;
}

void Prerendered::rerender()
{
	m_rendered = QImage();
	updateGL();
}

void Prerendered::initializeGL()
{
	glClearColor(1, 1, 1, 1);
	glClearDepth(-1.f);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, m_texture);
	glBindTexture (GL_TEXTURE_2D, m_texture[0]);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void Prerendered::resizeGL(int _w, int _h)
{
	glViewport(0, 0, _w, _h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, _w, 0, _h, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void Prerendered::paintGL()
{
	if (m_rendered.size() != size() && c()->samples())
	{
//		qDebug() << "Rendering " << (void*)this;
		m_rendered = QImage(size(), QImage::Format_RGB32);
		doRender(m_rendered);
		QImage glRendered = convertToGLFormat(m_rendered);
//		qDebug() << "Rendered " << m_rendered.size();
		glBindTexture(GL_TEXTURE_2D, m_texture[0]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glRendered.size().width(), glRendered.size().height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, glRendered.constBits());
	}
	glColor4f(1.f, 1.f, 1.f, 1.f);
	glBindTexture(GL_TEXTURE_2D, m_texture[0]);
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2i(0, 0);
	glVertex2i(0, 0);
	glTexCoord2i(1, 0);
	glVertex2i(width(), 0);
	glTexCoord2i(0, 1);
	glVertex2i(0, height());
	glTexCoord2i(1, 1);
	glVertex2i(width(), height());
	glEnd();
}

