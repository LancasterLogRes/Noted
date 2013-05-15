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
#include <cstdint>
#include <QString>
#include <QHash>
#include <QFile>
#include <Common/Global.h>
#include <Common/Algorithms.h>
#include <Common/Time.h>

typedef uint32_t DataKey;

// Usage: call init(), then initialize data with call to data(), then mention it's good with setGood(). data() can then be used to read data.
class Cache
{
	enum { IsGood = 1 };
	struct Header
	{
		uint32_t flags;
		uint32_t sourceKey;
		uint32_t operationKey;
		uint32_t bytes;
	};

public:
	Cache();
	~Cache() { reset(); }

	void reset();

	bool init(uint32_t _fingerprint, QString const& _type, size_t _bytes) { return init(_fingerprint, qHash(_type), 0, _bytes); }
	bool init(DataKey _sourceKey, DataKey _operationKey, DataKey _extraKey, size_t _bytes);
	bool init(DataKey _sourceKey, DataKey _operationKey, DataKey _extraKey);

	bool isOpen() const { return m_file.isOpen(); }
	bool isMapped() const { return !!m_mapping; }
	bool isGood() const { return isMapped() && header().flags & IsGood; }
	size_t bytes() const { assert(isMapped()); return header().bytes; }

	void setGood();

	template <class _T> void append(_T const& _raw) { assert(isOpen()); assert(!isGood()); m_file.write((char const*)&_raw, sizeof(_T)); }
	template <class _T> void append(Lightbox::foreign_vector<_T> const& _v) { assert(isOpen()); assert(!isGood()); m_file.write((char const*)_v.data(), _v.size() * sizeof(_T)); }
	template <class _T> Lightbox::foreign_vector<_T> data() { assert(isMapped()); return Lightbox::foreign_vector<_T>(payload<_T>(), bytes() / sizeof(_T)); }
	template <class _T> Lightbox::foreign_vector<_T const> data() const { assert(isGood()); return Lightbox::foreign_vector<_T const>((_T const*)payload<_T>(), bytes() / sizeof(_T)); }

protected:
	template <class _T> _T* payload() const { return (_T*)(m_mapping + sizeof(Header)); }
	Header& header() const { return *(Header*)m_mapping; }

	uint8_t* m_mapping = nullptr;
	QFile m_file;
};

// Usage: call init(), then initialize base level with call to data(0), then the other levels with a call to generate().
class MipmappedCache: protected Cache
{
public:
	MipmappedCache();

	bool init(DataKey _sourceKey, DataKey _operationKey, DataKey _extraKey, unsigned _sizeofItem, unsigned _items);
	bool init(uint32_t _fingerprint, QString const& _type, unsigned _sizeofItem, unsigned _items) { return init(_fingerprint, qHash(_type), (uint32_t)-1, _sizeofItem, _items); }

	unsigned levels() const { return m_itemsAtDepth.size(); }

	template <class _T> Lightbox::foreign_vector<_T> item(unsigned _depth, unsigned _i)
	{
		return Lightbox::foreign_vector<_T>((_T*)(m_mappingAtDepth[_depth] + std::min<unsigned>(_i, m_itemsAtDepth[_depth] - 1) * m_sizeofItem), m_sizeofItem / sizeof(_T));
	}

	template <class _T> Lightbox::foreign_vector<_T> data(unsigned _depth = 0)
	{
		return Lightbox::foreign_vector<_T>((_T*)m_mappingAtDepth[_depth], m_itemsAtDepth[_depth]);
	}

	template <class _T> Lightbox::foreign_vector<_T const> item(unsigned _depth, unsigned _i) const
	{
		if (isGood() && (int)_depth < m_itemsAtDepth.size() && m_itemsAtDepth[_depth] > 0)
			return Lightbox::foreign_vector<_T const>((_T*)(m_mappingAtDepth[_depth] + std::min<unsigned>(_i, m_itemsAtDepth[_depth] - 1) * m_sizeofItem), m_sizeofItem / sizeof(_T));
		return Lightbox::foreign_vector<_T const>();
	}

	template <class _T> Lightbox::foreign_vector<_T const> items(unsigned _i, unsigned _c) const
	{
		int depth = std::min(Lightbox::log2(_c), levels());
		int index = _i >> depth;
		return item<_T>(depth, index);
	}

	template <class _T> Lightbox::foreign_vector<_T const> data(unsigned _depth = 0) const
	{
		if (isGood() && (int)_depth < m_itemsAtDepth.size() && m_itemsAtDepth[_depth] > 0)
			return Lightbox::foreign_vector<_T const>((_T*)m_mappingAtDepth[_depth], m_itemsAtDepth[_depth]);
		return Lightbox::foreign_vector<_T const>();
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
		generate([](Lightbox::foreign_vector<_T> a, Lightbox::foreign_vector<_T> b, Lightbox::foreign_vector<_T> ret)
		{
			Lightbox::valcpy(ret.data(), a.data(), a.size());
			Lightbox::packTransform(ret.data(), b.data(), ret.size(), [](v4sf& rv, v4sf bv){ rv = (rv + bv) * v4sf({.5f, .5f, .5f, .5f}); });
		});
		*/
		generate([](Lightbox::foreign_vector<_T> a, Lightbox::foreign_vector<_T> b, Lightbox::foreign_vector<_T> ret)
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
