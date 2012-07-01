#include <QtOpenGL>
#include <QtGui>
#include <GL/glu.h>

#include <Common/Global.h>
#include <Common/Time.h>
using namespace Lightbox;

#include "OutputItem.h"
#include "LightshowPlugin.h"
#include "GLView.h"

GLView::GLView(QWidget* _parent): QGLWidget(QGLFormat(QGL::SampleBuffers), _parent), m_p(nullptr)
{
	m_position = Vector3(0.0, -0.5, -5.0);
	m_view = Vector3(0, 0.2, 1).normalized();
	m_up = Vector3(0, 1, 0);
	m_speed = 0.f;
	m_lastTime = wallTime();
}

void GLView::save(QSettings& _s) const
{
	auto saveVector = [&](QString _n, Vector3 _v) { _s.setValue("GLView." + _n + ".x", _v.x); _s.setValue("GLView." + _n + ".y", _v.y); _s.setValue("GLView." + _n + ".z", _v.z); };
	saveVector("position", m_position);
	saveVector("view", m_view);
	saveVector("up", m_up);
}

void GLView::load(QSettings const& _s)
{
	if (!_s.contains("GLView.position.x"))
		return;
	auto loadVector = [&](QString _n) { return Vector3(_s.value("GLView." + _n + ".x").toFloat(), _s.value("GLView." + _n + ".y").toFloat(), _s.value("GLView." + _n + ".z").toFloat()); };
	m_position = loadVector("position");
	m_view = loadVector("view");
	m_up = loadVector("up");
}

static const unsigned c_steps = 24;
static float s_sincos[c_steps][2];
static bool s_sinCosFilled = false;

void GLView::initializeGL()
{
	glClearColor(0.1, 0.1, 0.1, 1.0);
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_CULL_FACE);

	float const L = 5.f; // how far from the flight the cone extends
	float const r = 1.f;//0.2f; // radius of cone at 1 metre (i.e gradient)
	float const R = L * r; // the radius of the cone at the farthest extent
	if (!s_sinCosFilled)
	{
		s_sinCosFilled = true;
		for (unsigned i = 0; i <= c_steps; ++i)
		{
			float a = TwoPi * i / c_steps;
			s_sincos[i][0] = sin(a);
			s_sincos[i][1] = cos(a);
		}
	}

	m_lightCone = glGenLists(3);

	glNewList(m_lightCone, GL_COMPILE);
	const unsigned c_rsteps = 1;
	glBegin(GL_TRIANGLES);
	for (unsigned w = 0; w < c_rsteps; ++w)
	{
		float wR = R * (w + 1) / c_rsteps;
		for (unsigned i = 0; i < c_steps; ++i)
		{
			glVertex3f(0, 0, 0);
			glVertex3f(wR * s_sincos[i][0], wR * s_sincos[i][1], L);
			glVertex3f(wR * s_sincos[(i + 1) % c_steps][0], wR * s_sincos[(i + 1) % c_steps][1], L);
		}
	}
	glEnd();
	glEndList();

	const unsigned c_lsteps = 1;
	glNewList(m_lightCone + 1, GL_COMPILE);
	glBegin(GL_TRIANGLES);
	for (unsigned n = 0; n < c_lsteps; ++n)
	{
		float wL = L * (n + 1) /  c_lsteps;
		float wLr = wL * r;
		for (unsigned i = 1; i < c_steps - 1; ++i)
		{
			glVertex3f(wLr * s_sincos[0][0], wLr * s_sincos[0][1], wL);
			glVertex3f(wLr * s_sincos[i][0], wLr * s_sincos[i][1], wL);
			glVertex3f(wLr * s_sincos[i + 1][0], wLr * s_sincos[i + 1][1], wL);
		}
	}
	glEnd();
	glEndList();

	glNewList(m_lightCone + 2, GL_COMPILE);
	glPushMatrix();
	glScalef(0.021f, 0.021f, 0.021f);
	glBegin(GL_TRIANGLES);
	glVertex3f( 1,  1,  1); // A
	glVertex3f(-1,  1,  1); // B
	glVertex3f(-1, -1,  1); // C

	glVertex3f( 1,  1,  1); // A
	glVertex3f(-1, -1,  1); // C
	glVertex3f( 1, -1,  1); // D

	glVertex3f( 1,  1,  1); // A
	glVertex3f( 0,  0, -1); // E
	glVertex3f(-1,  1,  1); // B

	glVertex3f(-1,  1,  1); // B
	glVertex3f( 0,  0, -1); // E
	glVertex3f(-1, -1,  1); // C

	glVertex3f(-1, -1,  1); // C
	glVertex3f( 0,  0, -1); // E
	glVertex3f( 1, -1,  1); // D

	glVertex3f( 1, -1,  1); // D
	glVertex3f( 0,  0, -1); // E
	glVertex3f( 1,  1,  1); // A
	glEnd();
	glPopMatrix();
	glEndList();
}

void GLView::resizeGL(int _w, int _h)
{
	glViewport(0, 0, _w, _h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.f, float(_w) / _h, 0.5, 500); // near/far in metres
	glMatrixMode(GL_MODELVIEW);
}

void GLView::paintGL()
{
	Time t = wallTime();
	Time dt = t - m_lastTime;
	m_position += toSeconds(dt) * m_speed * m_view;
	m_lastTime = t;

	// OpenGL coords (right-handed):
	// x-axis right
	// y-axis up
	// z_axis towards viewer (camera looks out along -v.e. z axis)
	// Rotations: clockwise looking out along axis

	// Camera always at origin; all transforms affect camera coordinate system

	// Distances are metres

	glDepthMask(GL_TRUE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	// Global (Camera) transform
	Vector3 ppv = m_position + m_view;
	gluLookAt(m_position.x, m_position.y, m_position.z, ppv.x, ppv.y, ppv.z, m_up.x, m_up.y, m_up.z);

	glPushMatrix();
#if !defined(LIGHTBOX_DEBUGGING_GL)
	for (int x = -20; x < 20; ++x)
		for (int z = -20; z < 20; ++z)
		{
			glBegin(GL_QUADS);
			float c = (abs(x + z) % 2) * 0.125 + 0.125;
			glColor3f(c, c, c);
			glVertex3f(x, 0, z + 1);
			glVertex3f(x + 1, 0, z + 1);
			glVertex3f(x + 1, 0, z);
			glVertex3f(x,  0, z);
			glEnd();
		}
#else
	glBegin(GL_QUADS);
	glColor3f(0.7, 0.6, 0.2);
	// Add 'table'
	glVertex3f( 0,  0, 0);
	glVertex3f( 0,  0, 1.5);
	glVertex3f( 0.8,0, 1.5);
	glVertex3f( 0.8,0, 0);
	glEnd();
#endif
	glPopMatrix();

	foreach (OutputItem* o, m_p->outputItems())
	{
		glPushMatrix();

		OutputProfiles ops = o->cursorProfiles();
		Physical ph = o->output().physical();
		glTranslatef(ph.p.x, ph.p.y, ph.p.z);
		Vector3 axis = ph.d.axis();
		glRotatef(ph.d.angle() * 360.f / TwoPi, axis.x, axis.y, axis.z);

		for (unsigned li = 0; li < ops.size(); ++li)
		{
			OutputProfile const& op = ops[li];
			Physical pho = o->output().physicalOffset(li);

			glPushMatrix();

			glTranslatef(pho.p.x, pho.p.y, pho.p.z);
			Vector3 axis = pho.d.axis();
			glRotatef(pho.d.angle() * 360.f / TwoPi, axis.x, axis.y, axis.z);

	#if defined(LIGHTBOX_DEBUGGING_GL)
			// add axes
			glBegin(GL_LINES);
			glLineWidth(2.f);
			glColor3f(1.f, 0.f, 0.f); // x (red)
			glVertex3f(0, 0, 0);
			glVertex3f(1, 0, 0);
			glColor3f(0.f, 0.f, 1.f); // y (blue)
			glVertex3f(0, 0, 0);
			glVertex3f(0, 1, 0);
			glColor3f(1.f, 1.f, 0.f); // z (yellow)
			glVertex3f(0, 0, 0);
			glVertex3f(0, 0, 1);
			glEnd();
	#endif

			//cout << "angle=" << angl << "; ";
			//cout << "axis=(" << axis.x << "," << axis.y << "," << axis.z << ")" << endl;

			ThetaPhi thphi = op.direction;
			//cout<<"theta="<<thphi.theta * 360.f / TwoPi<<"; ";
			//cout<<"phi="<<thphi.phi * 360.f / TwoPi<<endl;

			glRotatef(thphi.theta * 360.f / TwoPi, 0, -1, 0);
			glRotatef(thphi.phi * 360.f / TwoPi, -1, 0, 0);
			// Our convention rotates in opposite direction to OpenGL

			glColor3ub(op.color.r(), op.color.g(), op.color.b());
			glDepthMask(GL_TRUE);
			glCallList(m_lightCone + 2);

			// Light cone
			glEnable(GL_BLEND);
			glDepthMask(GL_FALSE);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive
			glDisable(GL_CULL_FACE);
			glScalef(o->output().beamWidth(li), o->output().beamWidth(li), o->output().beamLength(li));
			glColor4f(op.color.r() / 255.f, op.color.g() / 255.f, op.color.b() / 255.f, o->output().beamStrength(li));
			glCallList(m_lightCone);
			glColor4f(op.color.r() / 255.f, op.color.g() / 255.f, op.color.b() / 255.f, o->output().beamStrength(li));
			glCallList(m_lightCone + 1);
			glDisable(GL_BLEND);
			glEnable(GL_CULL_FACE);

			glPopMatrix();
		}
		glPopMatrix();
	}
	//glFlush(); // Flush the OpenGL buffers to the window
}

void GLView::mousePressEvent(QMouseEvent* _e)
{
	m_original = _e->pos();
	grabMouse(Qt::BlankCursor);
}

void GLView::mouseReleaseEvent(QMouseEvent*)
{
	releaseMouse();
//	m_up = Vector3(0.0, 1.0, 0.0);
//	m_view.setY(0.f);
//	m_view.normalize();
}

void GLView::mouseMoveEvent(QMouseEvent* _e)
{
	if (_e->pos() == m_original)
		return;
	QPoint rel = _e->pos() - m_original;
	QCursor::setPos(mapToGlobal(m_original));
	if (abs(rel.x()) > 100 || abs(rel.y()) > 100 || !(_e->buttons() & (Qt::MiddleButton|Qt::LeftButton|Qt::RightButton)))
		return;
	if (_e->buttons() & (Qt::LeftButton|Qt::RightButton))
	{
		auto r = Quaternion::rotation(m_up, double(-rel.x()) * TwoPi / 360.0 / 10.0);
		m_view = Quaternion((r * Quaternion(m_view)) * r.conjugated()).axis();
	}
	if (_e->buttons() & (Qt::LeftButton))
		m_position = m_position + m_view * .05 * float(rel.y());
	if (_e->buttons() & (Qt::MiddleButton))
	{
		auto r = Quaternion::rotation(m_view, double(-rel.x()) * TwoPi / 360.0 / 10.0);
		m_up = Quaternion((r * Quaternion(m_up)) * r.conjugated()).axis();
	}
	if (_e->buttons() & (Qt::RightButton))
	{
		auto r = Quaternion::rotation(m_view.crossed(m_up), double(rel.y()) * TwoPi / 360.0 / 10.0);
		m_view = Quaternion((r * Quaternion(m_view)) * r.conjugated()).axis();
		m_up = Quaternion((r * Quaternion(m_up)) * r.conjugated()).axis();
	}
	m_view.normalize();
	m_up.normalize();
}

void GLView::wheelEvent(QWheelEvent* _e)
{
	if (m_speed == 0.f && _e->delta() < 0)
		m_speed = 0.1f;
	else
		m_speed *= pow(1.1, -_e->delta() / 20.f);
	if (m_speed < 0.1f)
		m_speed = 0.f;
}
