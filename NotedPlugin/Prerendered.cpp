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

#include <Common/Common.h>
using namespace Lightbox;

#include "NotedFace.h"
#include "Prerendered.h"

NotedFace* Prerendered::c() const
{
	if (!m_c)
		m_c = dynamic_cast<NotedFace*>(window());
	if (!m_c)
		m_c = dynamic_cast<NotedFace*>(window()->parentWidget()->window());
	return m_c;
}

void Prerendered::rerender() { QMutexLocker l(&x_rendered); m_rendered = QImage(); update(); }

void Prerendered::paintEvent(QPaintEvent*)
{
	QMutexLocker l(&x_rendered);
	if (m_rendered.size() != size() && c()->samples())
	{
//		qDebug() << "Rendering " << (void*)this;
		m_rendered = QImage(size(), QImage::Format_ARGB32_Premultiplied);
		doRender(m_rendered);
//		qDebug() << "Rendered " << m_rendered.size();
	}
	QPainter p(this);
	p.drawImage(0, 0, m_rendered);
}
