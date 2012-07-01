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

#include <memory>
#include <cstdint>
#include <QFile>
#include <Common/Global.h>
#include <Common/Time.h>

LIGHTBOX_STRUCT(2, IndexLevel, unsigned, index, unsigned, level);

inline unsigned qHash(IndexLevel const& _il) { return _il.index ^ (_il.level << 16); }

class PagerBase;
template <class _T> class Pager;

class Page
{
	friend class PagerBase;
	template <class _T> friend class Pager;

public:
	~Page();

	explicit operator bool() const { return m_mapping && m_sizeof; }
	template <class _T> Lightbox::foreign_vector<_T> data(unsigned _offset = 0, unsigned _count = (unsigned)-1) { return Lightbox::foreign_vector<_T>((_T*)m_mapping + _offset, std::min<unsigned>(_count, (m_sizeof / sizeof(_T) - _offset))); }

	void load();
	void resetLtu() { m_ltu = Lightbox::wallTime(); }

	Lightbox::Time ltu() const { return m_ltu; }
	IndexLevel const& ii() const { return m_ii; }
	bool alreadyExisted() const { return m_alreadyExisted; }

private:
	Page(IndexLevel const& _ii, QString const& _filename, bool _allowCreate, unsigned _size);

	uint8_t* m_mapping;
	unsigned m_sizeof;

	Lightbox::Time m_ltu;			///< Time is last used wall clock.
	IndexLevel m_ii;
	bool m_alreadyExisted;

	QFile m_file;					///< filename is the pageHash given the index/level.
};

typedef std::shared_ptr<Page> PagePtr;
