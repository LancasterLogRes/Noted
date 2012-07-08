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

#include <QDir>

#include "Pager.h"

using namespace std;
using namespace Lightbox;

PagerBase::PagerBase(QString const& _type):
	m_fingerprint				(0),
	m_maxMapped					(512),
	m_topLevel					(1),
	m_fileSize					(0),
	m_type						(_type)
{
}

void PagerBase::init(uint32_t _fp, unsigned _itemsPerPage, unsigned _itemLength, unsigned _typeSize)
{
	m_mapped.clear();
	m_mappedI.clear();
	m_fingerprint = _fp;
	m_itemsPerPage = _itemsPerPage;
	m_itemLength = _itemLength;
	m_fileSize = _typeSize * m_itemsPerPage * m_itemLength;
}

std::pair<PagePtr, unsigned> PagerBase::item(int _index, int _number, bool _force) const
{
	// TODO: make appropriate mean
	if (!m_fileSize)
		return make_pair(PagePtr(), 0u);

	IndexLevel il(0, 0);
	unsigned levelFactor = 1;
	for (unsigned n = _number; n > m_itemsPerPage / 2; ++il.level, n /= m_itemsPerPage, levelFactor *= m_itemsPerPage) {}
	il.index = (_index / levelFactor) / m_itemsPerPage;
	unsigned elementIndex = (_index / levelFactor) % m_itemsPerPage;

	if (_force || true)// always force for now.
	{
		if (!ensureMapped(il))
			return make_pair(PagePtr(), 0u);
	}

	{
		QMutexLocker l(&x_mapped);
		if (m_mappedI.contains(il) && m_mappedI[il])
		{
			refreshLtuHAVELOCK(m_mappedI[il]);
			return make_pair(m_mappedI[il], elementIndex);
		}
	}

	// TODO: put it on the queue.
	return item(_index, _number * m_itemsPerPage, il.level >= m_topLevel - 1);
}

void PagerBase::refreshLtuHAVELOCK(PagePtr const& _p, bool _force) const
{
	if (_force || wallTime() - _p->ltu() < 10 * msecs)
	{
		m_mapped.remove(_p->ltu());
		_p->resetLtu();
		m_mapped.insert(m_mappedI[_p->ii()]->ltu(), _p);
	}
}

QString PagerBase::filename(IndexLevel _il) const
{
	QString f = (QDir::tempPath() + "/Noted-%1").arg(m_fingerprint, 8, 16);
	if (!QFile::exists(f))
		QDir().mkpath(f);
    return (f + "/%2@%3x%4^%5").arg(m_type).arg(_il.index).arg(m_itemsPerPage).arg(_il.level);
}

bool PagerBase::ensureMapped(IndexLevel _il) const
{
	PagePtr p;
	{
		QMutexLocker l(&x_mapped);
		if (!m_mappedI.contains(_il))
		{
			p = page(_il, false);				// remap
			if (!p->alreadyExisted())
				return false;
			m_mappedI[_il] = p;
			if (m_mappedI.size() > (int)m_maxMapped)			// abandon oldest mapping if too many around (it may still be alive if something is holding onto a Page)
			{
				m_mappedI.remove((*m_mapped.begin())->ii());
				m_mapped.erase(m_mapped.begin());
			}
		}
		refreshLtuHAVELOCK(m_mappedI[_il], true);
	}
	return true;
}
