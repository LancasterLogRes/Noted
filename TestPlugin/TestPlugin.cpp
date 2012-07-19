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

#include <QtGui>

#ifdef Q_OS_MAC
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include "TestPlugin.h"

using namespace std;
using namespace Lightbox;

NOTED_PLUGIN(TestPlugin);

static QWidget* glw;

class TestGLView: public QGLWidgetProxy
{
public:
	virtual void initializeGL()
	{
		glShadeModel(GL_SMOOTH);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClearDepth(1.0f);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	}
	virtual void resizeGL(int _w, int _h)
	{
		glViewport(0, 0, _w, _h);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45.0f,(GLfloat)_w/(GLfloat)_h,0.1f,100.0f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}
	virtual void paintGL()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glLoadIdentity();
		glTranslatef(-1.5f,0.0f,-6.0f);
		glBegin(GL_TRIANGLES);
		glVertex3f( 0.0f, 1.0f, 0.0f);
		glVertex3f(-1.0f,-1.0f, 0.0f);
		glVertex3f( 1.0f,-1.0f, 0.0f);
		glEnd();
	}
};

TestPlugin::TestPlugin(NotedFace* _c): NotedPlugin(_c)
{
	glw = noted()->addGLWidget(new TestGLView);
	glw->show();
}

TestPlugin::~TestPlugin()
{
	delete glw;
}
