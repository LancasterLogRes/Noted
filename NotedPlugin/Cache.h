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

#pragma once

#include <boost/type_traits/function_traits.hpp>
#include <memory>
#include <tuple>
#include <vector>
#include <cstdint>
#include <QString>
#include <QHash>
#include <QMap>
#include <QFile>
#include <Common/Global.h>
#include <Common/Algorithms.h>
#include <Common/Time.h>
#include "Common.h"

struct Void {};

class ProtoCache
{
	enum { IsGood = 1 };

	struct BaseHeader
	{
		uint32_t flags;
		uint32_t bytes;
	};

public:
	ProtoCache(size_t _headerSize): m_headerSize(_headerSize) {}
	~ProtoCache() { reset(); }

	typedef QMultiMap<QDateTime, QPair<DataKey, uint64_t>> AvailableMap;

	static AvailableMap available();
	static void kill(DataKey);

	bool open(lb::SimpleKey _sourceKey, lb::SimpleKey _operationKey, lb::SimpleKey _extraKey, size_t _bytes);
	bool open(lb::SimpleKey _sourceKey, lb::SimpleKey _operationKey, lb::SimpleKey _extraKey);

	void ensureMapped();
	void setGood();

	template <class _T> void append(_T const& _raw) { assert(isOpen()); assert(!isGood()); m_file.write((char const*)&_raw, sizeof(_T)); }
	template <class _T> void append(lb::foreign_vector<_T> const& _v) { assert(isOpen()); assert(!isGood()); m_file.write((char const*)_v.data(), _v.size() * sizeof(_T)); }
	template <class _T> lb::foreign_vector<_T> data() { assert(isMapped()); return lb::foreign_vector<_T>(payload<_T>(), bytes() / sizeof(_T)); }
	template <class _T> lb::foreign_vector<_T const> data() const { assert(isGood()); return lb::foreign_vector<_T const>((_T const*)payload<_T>(), bytes() / sizeof(_T)); }

	void reset();

	bool isOpen() const { return m_file.isOpen(); }
	bool isMapped() const { return !!m_mapping; }
	bool isGood() const { return isMapped() && header().flags & IsGood; }
	size_t bytes() const { assert(isMapped()); return header().bytes; }
	QFile& file() { return m_file; }

protected:
	BaseHeader& header() const { return *(BaseHeader*)m_mapping; }
	template <class _T> _T* payload() const { return (_T*)(m_mapping + m_headerSize); }

	uint8_t* m_mapping = nullptr;
	QFile m_file;
	size_t m_headerSize;
};

// Usage: call init(), then initialize data with call to data(), then mention it's good with setGood(). data() can then be used to read data.
template <class _Metadata = Void>
class Cache: public ProtoCache
{
	struct Header
	{
		uint32_t flags;
		uint32_t bytes;
		_Metadata metadata;
	};

public:
	Cache(): ProtoCache(sizeof(Header)) {}

	_Metadata const& metadata() const { assert(isMapped()); return header().metadata; }
	_Metadata& mutableMetadata() { assert(isMapped() && !isGood()); return header().metadata; }

protected:
	Header& header() const { return *(Header*)m_mapping; }
};

// Usage: call init(), then initialize base level with call to data(0), then the other levels with a call to generate().
template <class _Metadata = Void>
class MipmappedCache: protected Cache<_Metadata>
{
public:
	typedef Cache<_Metadata> Super;
	using Super::isGood;
	using Super::setGood;

	bool open(lb::SimpleKey _sourceKey, lb::SimpleKey _operationKey, lb::SimpleKey _extraKey, unsigned _sizeofItem, unsigned _items)
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

		bool ret = ProtoCache::open(_sourceKey, _operationKey, _extraKey, totalItems * m_sizeofItem);
		for (unsigned i = 0; i < levels(); ++i)
		{
			m_mappingAtDepth[i] += (size_t)ProtoCache::payload<uint8_t>();
			assert(m_mappingAtDepth[i] >= ProtoCache::payload<uint8_t>());
			assert(m_mappingAtDepth[i] + m_itemsAtDepth[i] * m_sizeofItem <= ProtoCache::payload<uint8_t>() + ProtoCache::bytes());
		}

		return ret;
	}

	template <class _T> size_t valuesAtDepth(unsigned _depth) const { return m_itemsAtDepth[_depth] * m_sizeofItem / sizeof(_T); }

	unsigned levels() const { return m_itemsAtDepth.size(); }

	template <class _T> lb::foreign_vector<_T> item(unsigned _depth, unsigned _i)
	{
		return (int)_depth < m_mappingAtDepth.size() ? lb::foreign_vector<_T>((_T*)(m_mappingAtDepth[_depth] + std::min<unsigned>(_i, m_itemsAtDepth[_depth] - 1) * m_sizeofItem), m_sizeofItem / sizeof(_T)) : lb::foreign_vector<_T>();
	}

	template <class _T> lb::foreign_vector<_T> data(unsigned _depth = 0)
	{
		return (int)_depth < m_mappingAtDepth.size() ? lb::foreign_vector<_T>((_T*)m_mappingAtDepth[_depth], valuesAtDepth<_T>(_depth)) : lb::foreign_vector<_T>();
	}

	template <class _T> lb::foreign_vector<_T const> item(unsigned _depth, unsigned _i) const
	{
		if (isGood() && (int)_depth < m_itemsAtDepth.size() && m_itemsAtDepth[_depth] > 0)
			return lb::foreign_vector<_T const>((_T*)(m_mappingAtDepth[_depth] + std::min<unsigned>(_i, m_itemsAtDepth[_depth] - 1) * m_sizeofItem), m_sizeofItem / sizeof(_T));
		return lb::foreign_vector<_T const>();
	}

	template <class _T> lb::foreign_vector<_T const> items(unsigned _i, unsigned _c) const
	{
		int depth = std::min(lb::log2(_c), levels());
		int index = _i >> depth;
		return item<_T>(depth, index);
	}

	template <class _T> lb::foreign_vector<_T const> data(unsigned _depth = 0) const
	{
		if (isGood() && (int)_depth < m_itemsAtDepth.size() && m_itemsAtDepth[_depth] > 0)
			return lb::foreign_vector<_T const>((_T*)m_mappingAtDepth[_depth], valuesAtDepth<_T>(_depth));
		return lb::foreign_vector<_T const>();
	}

	// _Mean must operate on three foreign_vector<_T>, of size (_sizeofItem / sizeof(_T)), such that mean(arg1, arg2) = arg3; arg1 and arg2 are guaranteed sequential.
	template <class _Mean, class _T> void generate(_Mean _mean, _T)
	{
		for (int d = 1; d < m_mappingAtDepth.size(); ++d)
			for (unsigned i = 0; i < m_itemsAtDepth[d]; ++i)
				_mean(item<_T>(d - 1, i * 2), item<_T>(d - 1, i * 2 + 1), item<_T>(d, i));
		setGood();
	}

	// Just uses the arithmetic mean. Fine in most cases.
	template <class _T> void generate()
	{
		/*
		generate([](lb::foreign_vector<_T> a, lb::foreign_vector<_T> b, lb::foreign_vector<_T> ret)
		{
			lb::valcpy(ret.data(), a.data(), a.size());
			lb::packTransform(ret.data(), b.data(), ret.size(), [](v4sf& rv, v4sf bv){ rv = (rv + bv) * v4sf({.5f, .5f, .5f, .5f}); });
		});
		*/
		generate([](lb::foreign_vector<_T> a, lb::foreign_vector<_T> b, lb::foreign_vector<_T> ret)
		{
			unsigned i = 0;
			for (; i + 3 < a.size(); i += 4)
			{
				ret[i] = (a[i] + b[i]) / 2;
				ret[i+1] = (a[i+1] + b[i+1]) / 2;
				ret[i+2] = (a[i+2] + b[i+2]) / 2;
				ret[i+3] = (a[i+3] + b[i+3]) / 2;
			}
			for (; i < a.size(); ++i)
				ret[i] = (a[i] + b[i]) / 2;
		}, _T());
	}

private:
	QList<uint8_t*> m_mappingAtDepth;
	QList<size_t> m_itemsAtDepth;

	size_t m_sizeofItem = 0;
	unsigned m_items = 0;
	unsigned m_levels = 0;
};
