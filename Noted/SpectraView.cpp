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

#include <cmath>
#include <contrib/glsl.h>
#include <QDebug>
#include <QFrame>
#include <QPaintEvent>
#include <QPainter>
#include <QGLFramebufferObject>
#include <Common/Maths.h>
#include <NotedPlugin/NotedFace.h>

#include "SpectraView.h"

using namespace std;
using namespace Lightbox;
using namespace cwc;

SpectraView::SpectraView(QWidget* _parent):
	PrerenderedTimeline	(_parent),
	m_sm				(nullptr),
	m_shader			(nullptr)
{
	m_texture[0] = 0;
}

SpectraView::~SpectraView()
{
}

Lightbox::Time SpectraView::period() const
{
	return c()->windowSize();
}

QByteArray fileDump(QString const& _name)
{
	QFile f(_name);
	f.open(QIODevice::ReadOnly);
	auto ret = f.readAll();
	ret.append("\0");
	return ret;
}

void SpectraView::doRender(QGLFramebufferObject* _fbo, int _dx, int _dw)
{
	unsigned bc = c()->spectrumSize();
	unsigned s = c()->hops();
	NotedFace* br = dynamic_cast<NotedFace*>(c());

	_fbo->bind();
	glEnable(GL_TEXTURE_1D);
	glPushMatrix();
	glLoadIdentity();
	glScalef(1, _fbo->height(), 1);
	glColor3f(1, 1, 1);

	if (!m_sm)
	{
		cnote << "Making shader man";
		m_sm = new glShaderManager;
	}
	if (!m_shader)
	{
		cnote << "Making shader";
		m_shader = m_sm->loadfromMemory(fileDump(":/SpectraView.vert").data(), 0, fileDump(":/SpectraView.frag").data());
		if (!m_shader)
		{
			cwarn << "Couldn't create shader :-(";
			m_shader = new glShader;
		}
	}

	if (m_shader->GetProgramObject())
		m_shader->begin();
	if (s && bc > 2)
	{
		for (int x = _dx; x < _dx + _dw; ++x)
		{
			int fi = renderingTimeOf(x) > 0 ? renderingTimeOf(x) / c()->hop() : -1;
			int ti = qMax<int>(fi + 1, renderingTimeOf(x + 1) / c()->hop());
			if (fi >= 0 && ti < (int)s)
			{
				auto mus = br->multiSpectrum(fi, ti - fi);
				if (!m_texture[0])
				{
					glGenTextures(1, m_texture);
					glBindTexture(GL_TEXTURE_1D, m_texture[0]);
					glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					if (!m_shader->GetProgramObject())
					{
						float bias = 1.f;
						float scale = -1.f;
						glPixelTransferf(GL_RED_SCALE, scale);
						glPixelTransferf(GL_GREEN_SCALE, scale);
						glPixelTransferf(GL_BLUE_SCALE, scale);
						glPixelTransferf(GL_RED_BIAS, bias);
						glPixelTransferf(GL_GREEN_BIAS, bias);
						glPixelTransferf(GL_BLUE_BIAS, bias);
					}
				}
				else
					glBindTexture(GL_TEXTURE_1D, m_texture[0]);
				glTexImage1D(GL_TEXTURE_1D, 0, 1, bc * 3, 0, GL_LUMINANCE, GL_FLOAT, mus.data());
				glBegin(GL_TRIANGLE_STRIP);
				glTexCoord1f(1/3.f);
				glVertex3i(x, 0, 0);
				glVertex3i(x + 1, 0, 0);
				glTexCoord1f(0.f);
				glVertex3i(x, 1, 0);
				glVertex3i(x + 1, 1, 0);
				glEnd();
			}
		}
	}
	if (m_shader->GetProgramObject())
		m_shader->end();
	glPopMatrix();
	_fbo->release();
}
