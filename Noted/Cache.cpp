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
#include "Cache.h"

Cache::Cache(): m_fingerprint(0), m_bytes(0), m_mapping(nullptr)
{
}

bool Cache::init(uint32_t _fingerprint, QString const& _type, size_t _bytes)
{
	m_fingerprint = _fingerprint;
	m_type = _type;
	m_bytes = _bytes;
	m_file.close();
	QString p = (QDir::tempPath() + "/Noted-%1").arg(m_fingerprint, 8, 16, QChar('0'));
	QDir().mkpath(p);
	QString f = (p + "/%1").arg(m_type);
	m_file.setFileName(f);
	m_file.open(QIODevice::ReadWrite);
	m_isGood = false;
	if (m_file.size() == (int)(_bytes + 4))
	{
		uint32_t good;
		if (m_file.read((char*)&good, 4) == 4 && good == _bytes)
			m_isGood = true;
	}
	else
		m_file.resize(_bytes + 4);
	m_mapping = m_file.map(4, m_bytes);
	assert(!m_bytes || m_mapping);
	return m_isGood;
}

void Cache::setGood()
{
	m_file.seek(0);
	m_file.write((char*)&m_bytes, 4);
	m_isGood = true;
}

MipmappedCache::MipmappedCache(): m_sizeofItem(0), m_items(0)
{
}

bool MipmappedCache::init(uint32_t _fingerprint, QString const& _type, unsigned _sizeofItem, unsigned _items)
{
	m_sizeofItem = _sizeofItem;
	m_items = _items;
	m_mappingAtDepth.clear();
	m_itemsAtDepth.clear();

	unsigned totalItems = 0;
	for (unsigned level = 0, levelItems = _items; levelItems > 2; levelItems = (levelItems + 1) / 2, ++level)
	{
		if (level)
			m_mappingAtDepth.push_back(m_mappingAtDepth[level - 1] + m_itemsAtDepth[level - 1] * m_sizeofItem);
		else
			m_mappingAtDepth.push_back(nullptr);
		m_itemsAtDepth.push_back(levelItems);
		totalItems += levelItems;
	}

	bool ret = Cache::init(_fingerprint, _type, totalItems * m_sizeofItem);
	for (unsigned i = 0; i < levels(); ++i)
	{
		m_mappingAtDepth[i] += (size_t)m_mapping;
		assert(m_mappingAtDepth[i] >= m_mapping);
		assert(m_mappingAtDepth[i] + m_itemsAtDepth[i] * m_sizeofItem <= m_mapping + m_bytes);
	}

	return ret;
}
