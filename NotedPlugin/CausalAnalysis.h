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
#include <Common/Time.h>

#include "AcausalAnalysis.h"

class CausalAnalysis: public AcausalAnalysis
{
public:
	CausalAnalysis(QString const& _processName): AcausalAnalysis(_processName) {}
	virtual ~CausalAnalysis() {}

	// inherited
	virtual void init() { init(true); }
	virtual void fini() { fini(true); }
	virtual unsigned prepare(unsigned _from, unsigned _count, Lightbox::Time) { noteBatch(_from, _count); return _count; }
	virtual void analyze(unsigned _from, unsigned _count, Lightbox::Time _hop);

	// new
	virtual void init(bool _willRecord) { (void)_willRecord; }
	virtual void fini(bool _didRecord) { (void)_didRecord; }
	virtual void noteBatch(unsigned, unsigned) {}
	virtual void process(unsigned, Lightbox::Time) {}
	virtual void record() {}
};

typedef std::shared_ptr<CausalAnalysis> CausalAnalysisPtr;
typedef std::vector<CausalAnalysisPtr> CausalAnalysisPtrs;
