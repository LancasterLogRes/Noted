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

#include "Pager.h"
#include "Page.h"

using namespace std;
using namespace Lightbox;

Page::Page(IndexLevel const& _ii, QString const& _filename, bool _allowCreate, unsigned _size):
	m_mapping			(nullptr),
	m_sizeof			(0),
	m_ltu				(UndefinedTime),
	m_ii				(_ii),
	m_alreadyExisted	(false),
	m_file				(_filename)
{
	if (m_file.exists() || _allowCreate)
	{
		m_file.open(QIODevice::ReadWrite);
//		qDebug() << m_file.fileName() << m_file.isOpen();
		if (m_file.size() == 0)
			m_file.resize(_size);
		else
		{
			m_alreadyExisted = true;
			assert((unsigned)m_file.size() == _size);
		}
		m_mapping = m_file.map(0, m_file.size());
		m_sizeof = m_file.size();
		assert(m_mapping);
	}
}

Page::~Page()
{
	if (m_file.isOpen())
	{
		m_file.unmap(m_mapping);
		m_file.close();
		m_mapping = nullptr;
	}
}

