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
#include <QString>
#include <QMutex>
#include <QMultiMap>
#include <QHash>
#include <QDebug>
#include <Common/Time.h>
#include "WorkerThread.h"
#include "Page.h"

class PagerBase
{
public:
	explicit PagerBase(QString const& _type);

	uint32_t fingerprint() const { return m_fingerprint; }

	PagePtr page(IndexLevel _il, bool _allowCreate = true) const { return PagePtr(new Page(_il, filename(_il), _allowCreate, m_fileSize)); }

protected:
	void init(uint32_t _fp, unsigned _itemsPerPage, unsigned _itemLength, unsigned _typeSize);
	std::pair<PagePtr, unsigned> item(int _index, int _number) const;

	QString filename(IndexLevel _il) const;
	void refreshLtuHAVELOCK(PagePtr const& _p, bool _force = false) const;
	bool ensureMapped(IndexLevel _il) const;

	mutable QMutex x_mapped;
	mutable QMultiMap<Lightbox::Time, PagePtr> m_mapped;
	mutable QHash<IndexLevel, PagePtr> m_mappedI;

	uint32_t m_fingerprint;
	unsigned m_maxMapped;
	unsigned m_topLevel;
	unsigned m_itemsPerPage;
	unsigned m_itemLength;
	unsigned m_fileSize;
	QString m_type;
};

template <class _T>
class Pager: public PagerBase
{
public:
	explicit Pager(QString const& _type): PagerBase(_type) {}

	void init(uint32_t _fp, unsigned _itemsPerPage, unsigned _itemLength)
	{
		PagerBase::init(_fp, _itemsPerPage, _itemLength, sizeof(_T));
	}

	Lightbox::foreign_vector<_T> item(int _index, int _number, int _off = 0, int _trim = -1) const
	{
		auto p = PagerBase::item(_index, _number);
		if (_trim == -1)
			_trim = m_itemLength;
		PagePtr const& pp = p.first;
		if (pp)
			return pp->data<_T>(m_itemLength * p.second + _off, _trim).tied(pp);
		else
			return Lightbox::foreign_vector<_T>();
	}

	template <class _Base, class _Acc, class _Distill, class _Done>
	void fill(_Base _base, _Acc _accumulate, _Distill _distill, _Done _done, unsigned _items)
	{
		if (_items)
		{
			QList<PagePtr> levels;
			levels.push_back(page(IndexLevel(0, 0)));
			if (!levels[0]->alreadyExisted())
			{
				for (unsigned item = 0; item < _items && !WorkerThread::quitting(); ++item)
				{
					WorkerThread::setCurrentProgress(item * 100 / _items);
					nbug(4) << "Item:" << item << " (" << levels.size() << " levels)";
					unsigned leveledItem = item;	// the same at the base level, but itemsPerPage times smaller at each higher level.
					for (int l = 0; ; ++l)
					{
						// ASSERTION: We have data ready for a finished entry at level 'l'.
						// i is the index, in terms of elements at the current level (i.e. p ^ l real observations per 1 i).

						assert(levels[l]->data<_T>().data());
						_T* sd = levels[l]->data<_T>().data() + leveledItem % m_itemsPerPage * m_itemLength;

						// Record the data that's ready...
						if (l)
						{
							// If it's not the bottom level, then the data that's ready has been accumulated in-place; we need to divide it.
							_distill(sd);
						}
						else
						{
							_base(leveledItem, sd);
						}

						// Early out if we were only here to tie up loose pages and we're on the last page.
						if (leveledItem == _items - 1 && levels.size() == l + 1)
						{
							nbug(4) << "   " << l << (l ? "/" : "B");
							break;
						}

						// Accumulate into the next-higher level:

						// If it's first time we've ever raised to this level, then push the new level and create the Page.
						if (levels.size() <= l + 1)
						{
							assert(levels.size() == l + 1);
							levels.push_back(page(IndexLevel(0, levels.size())));
							nbug(4) << "   " << l << (l ? "/" : "B") << " => " << (l+1);
						}
						else
							nbug(4) << "   " << l << (l ? "/" : "B") << " +> " << (l+1);

						// Accumulate the (now-recorded) data from this level to the next-higher level.
						assert(levels[l + 1]->data<_T>().data());
						_T* sdh = levels[l + 1]->data<_T>().data() + leveledItem / m_itemsPerPage % m_itemsPerPage * m_itemLength;
						_accumulate(sd, sdh, leveledItem % m_itemsPerPage);

						// Move onto the next higher level when this level's Page is complete.
						// (Meaning that the accumulation at the next-higher level is finished).
						// Move onto the next level also if we're on the last sample to tie up (i.e. divide) loose pages.
						if ((leveledItem + 1) % m_itemsPerPage == 0 || (leveledItem == _items - 1 && levels.size() > l + 1))
						{
							levels[l] = page(IndexLevel(levels[l]->ii().index + 1, l));
							leveledItem /= m_itemsPerPage;
						}
						else
							break;
					}
					_done();
				}
				m_topLevel = levels.size() - 1;
			}
			if (WorkerThread::quitting())
			{
				QFile::remove(filename(IndexLevel(0, 0)));
			}
		}
		else
			fillFromExisting();
	}

	void fillFromExisting()
	{
		m_topLevel = 1;
		for (; page(IndexLevel(0, m_topLevel), false)->alreadyExisted(); ++m_topLevel)
		--m_topLevel;
	}
};

