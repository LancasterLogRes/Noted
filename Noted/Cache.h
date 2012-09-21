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
#include <QFile>
#include <Common/Global.h>
#include <Common/Algorithms.h>
#include <Common/Time.h>

// Usage: call init(), then initialize data with call to data(), then mention it's good with setGood(). data() can then be used to read data.
class Cache
{
public:
	Cache();

	bool init(uint32_t _fingerprint, QString const& _type, size_t _bytes);
	template <class _T> Lightbox::foreign_vector<_T> data() { return Lightbox::foreign_vector<_T>((_T*)m_mapping, m_bytes / sizeof(_T)); }
	template <class _T> Lightbox::foreign_vector<_T const> data() const { assert(m_isGood); return Lightbox::foreign_vector<_T const>((_T const*)m_mapping, m_bytes / sizeof(_T)); }
	void setGood();

protected:
	uint32_t m_fingerprint;
	QString m_type;
	size_t m_bytes;
	bool m_isGood;

	uint8_t* m_mapping;
	QFile m_file;
};

// Usage: call init(), then initialize base level with call to data(0), then the other levels with a call to generate().
class MipmappedCache: protected Cache
{
public:
	MipmappedCache();

	bool init(uint32_t _fingerprint, QString const& _type, unsigned _sizeofItem, unsigned _items);

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
		if (m_isGood && (int)_depth < m_itemsAtDepth.size() && m_itemsAtDepth[_depth] > 0)
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
		if (m_isGood && (int)_depth < m_itemsAtDepth.size() && m_itemsAtDepth[_depth] > 0)
			return Lightbox::foreign_vector<_T const>((_T*)m_mappingAtDepth[_depth], m_itemsAtDepth[_depth]);
		return Lightbox::foreign_vector<_T const>();
	}

	// _Mean must operate on three foreign_vector<_T>, of size (_sizeofItem / sizeof(_T)), such that mean(arg1, arg2) = arg3.
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

	size_t m_sizeofItem;
	unsigned m_items;
	unsigned m_levels;
};
