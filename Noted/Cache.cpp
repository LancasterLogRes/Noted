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

Cache::Cache() {}

bool Cache::init(DataKey _sourceKey, DataKey _operationKey, DataKey _extraKey, size_t _bytes)
{
	if (m_mapping)
		m_file.unmap(m_mapping);
	m_file.close();

	QString p = (QDir::tempPath() + "/Noted-%1").arg(_sourceKey, 8, 16, QChar('0'));
	QDir().mkpath(p);
	QString f = (p + "/%1-%2").arg(_operationKey, 8, 16, QChar('0')).arg(_extraKey, 8, 16, QChar('0'));
	m_file.setFileName(f);
	m_file.open(QIODevice::ReadWrite);

	if (m_file.size() == (int)(_bytes + sizeof(Header)))
	{
		// File looks the right size - map it and check the header.
		m_mapping = m_file.map(0, m_file.size());
		if (header().bytes == _bytes && header().operationKey == _operationKey && header().sourceKey == _sourceKey)
			// Header agrees with parameters - trust the flags on whether it's complete.
			return isGood();
		// Header wrong - something changed between now and then or it's corrupt. In any case, reinitialize.
	}
	else
	{
		// File wrong size - resize and (re)initialize.
		m_file.resize(_bytes + sizeof(Header));
		m_mapping = m_file.map(0, m_file.size());
	}

	// Initialize header - we'll (re)compute payload.
	header().bytes = _bytes;
	header().flags = 0;
	header().operationKey = _operationKey;
	header().sourceKey = _sourceKey;

	assert(isMapped());
	return false;
}

MipmappedCache::MipmappedCache() {}

bool MipmappedCache::init(DataKey _sourceKey, DataKey _operationKey, DataKey _extraKey, unsigned _sizeofItem, unsigned _items)
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

	bool ret = Cache::init(_sourceKey, _operationKey, _extraKey, totalItems * m_sizeofItem);
	for (unsigned i = 0; i < levels(); ++i)
	{
		m_mappingAtDepth[i] += (size_t)m_mapping;
		assert(m_mappingAtDepth[i] >= m_mapping);
		assert(m_mappingAtDepth[i] + m_itemsAtDepth[i] * m_sizeofItem <= m_mapping + bytes());
	}

	return ret;
}
