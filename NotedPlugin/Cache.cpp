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

#include <iostream>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include "Cache.h"
using namespace std;

Cache::Cache() {}

void Cache::reset()
{
	if (m_mapping)
	{
//		cnote << "Unmapping" << m_file.fileName().toStdString() << "@" << (void*)m_mapping << "+" << m_file.size();
		m_file.unmap(m_mapping);
		m_mapping = nullptr;
	}
	m_file.close();
}

bool Cache::init(DataKey _sourceKey, DataKey _operationKey, DataKey _extraKey, size_t _bytes)
{
	reset();

	QString p = QDir::tempPath() + "/Noted.cache";
	QDir().mkpath(p);
	QString f = (p + "/%1-%2-%3").arg(_sourceKey, 8, 16, QChar('0')).arg(_operationKey, 8, 16, QChar('0')).arg(_extraKey, 8, 16, QChar('0'));
	m_file.setFileName(f);
	m_file.open(QIODevice::ReadWrite);

//	cdebug << "init" << _bytes << "on" << m_file.size() << "-" <<  sizeof(Header) << ", file" << m_file.fileName().toStdString();
	if (m_file.size() == (int)(_bytes + sizeof(Header)))
	{
		// File looks the right size - map it and check the header.
		m_mapping = m_file.map(0, m_file.size());
		assert(m_mapping);
//		cnote << "Mapped" << m_file.fileName().toStdString() << "@" << (void*)m_mapping << "+" << m_file.size();
//		cdebug << "init" << hex << _sourceKey << "/" << _operationKey << "on" << hex << header().sourceKey << "/" << header().operationKey << ", file" << m_file.fileName().toStdString();
		if (header().bytes == _bytes && header().operationKey == _operationKey && header().sourceKey == _sourceKey && isGood())
			// Header agrees with parameters - trust the flags on whether it's complete.
			return true;
		// Header wrong - something changed between now and then or it's corrupt. In any case, reinitialize.
	}
	else
	{
		// File wrong size - resize and (re)initialize.
		m_file.resize(_bytes + sizeof(Header));
		m_mapping = m_file.map(0, m_file.size());
		assert(m_mapping);
	}

	// Initialize header - we'll (re)compute payload.
	header().bytes = _bytes;
	header().flags = 0;
	header().operationKey = _operationKey;
	header().sourceKey = _sourceKey;
//	cdebug << "Set header of" << m_file.fileName().toStdString() << "as" << hex << header().sourceKey << "/" << header().operationKey;

	assert(isMapped());
	return false;
}

bool Cache::init(DataKey _sourceKey, DataKey _operationKey, DataKey _extraKey)
{
	reset();

	QString p = QDir::tempPath() + "/Noted.cache";
	QDir().mkpath(p);
	QString f = (p + "/%1-%2-%3").arg(_sourceKey, 8, 16, QChar('0')).arg(_operationKey, 8, 16, QChar('0')).arg(_extraKey, 8, 16, QChar('0'));
	m_file.setFileName(f);
	m_file.open(QIODevice::ReadWrite);

	if (m_file.size() > (int)(sizeof(Header)))
	{
		// File looks the right size - map it and check the header.
		m_mapping = m_file.map(0, m_file.size());
		assert(m_mapping);
//		cnote << "Mapped" << m_file.fileName().toStdString() << "@" << (void*)m_mapping << "+" << m_file.size();
//		cdebug << "init" << hex << _sourceKey << "/" << _operationKey << "on" << hex << header().sourceKey << "/" << header().operationKey << ", file" << m_file.fileName().toStdString();
		if (header().bytes == m_file.size() - sizeof(Header) && header().operationKey == _operationKey && header().sourceKey == _sourceKey && isGood())
			// Header agrees with parameters - trust the flags on whether it's complete.
			return true;
		// Header wrong or data incomplete. In any case, restart.
	}

	// Something invalid - clear and restart.
	reset();
	m_file.open(QIODevice::ReadWrite | QIODevice::Truncate);

	// Initialize header - we'll (re)compute payload.
	Header h;
	h.bytes = 0;
	h.flags = 0;
	h.operationKey = _operationKey;
	h.sourceKey = _sourceKey;
	append(h);

	return false;
}

void Cache::setGood()
{
	assert(isOpen());
	if (!isMapped())
	{
		m_file.flush();
		assert(m_file.size());
		m_mapping = m_file.map(0, m_file.size());
//		cnote << "Mapped" << m_file.fileName().toStdString() << "@" << (void*)m_mapping << "+" << m_file.size();
		assert(m_mapping);
		header().bytes = m_file.size() - sizeof(Header);
	}
//	cdebug << "setGood on" << hex << header().sourceKey << "/" << header().operationKey << ", file" << m_file.fileName().toStdString();
	header().flags = IsGood;
}

Cache::AvailableMap Cache::available()
{
	AvailableMap ret;
	QDir d(QDir::tempPath() + "/Noted.cache");
	QPair<DataKeySet, uint64_t> t(DataKeySet(), 0);
	QDateTime mru;
	for (QFileInfo const& i: d.entryInfoList(QDir::Files, QDir::Name))
	{
		DataKeySet dks(i.fileName().section('-', 0, 0).toULongLong(0, 16), i.fileName().section('-', 1, 1).toULongLong(0, 16));
		if (t.first != dks || !t.second)
		{
			if (t.second)
				ret.insertMulti(mru, t);
			t.first = dks;
			t.second = i.size();
			mru = i.lastRead();
		}
		else
		{
			if (mru < i.lastRead())
				mru = i.lastRead();
			t.second += i.size();
		}
	}
	if (t.second)
		ret.insertMulti(mru, t);
	return ret;
}

void Cache::kill(DataKeySet _k)
{
	QDir p(QDir::tempPath() + "/Noted.cache");
	QString f = ("%1-%2-*");
	f = f.arg(_k.source, 8, 16, QChar('0')).arg(_k.operation, 8, 16, QChar('0'));

	for (QFileInfo const& i: QDir(p).entryInfoList({f}, QDir::Files, QDir::Name))
		QFile::remove(i.filePath());
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
		m_mappingAtDepth[i] += (size_t)payload<uint8_t>();
		assert(m_mappingAtDepth[i] >= payload<uint8_t>());
		assert(m_mappingAtDepth[i] + m_itemsAtDepth[i] * m_sizeofItem <= payload<uint8_t>() + bytes());
	}

	return ret;
}
