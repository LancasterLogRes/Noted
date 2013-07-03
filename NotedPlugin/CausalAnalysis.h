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

class CausalAnalysis: public AcausalAnalysis // TODO: kill AcA's public API
{
public:
	explicit CausalAnalysis(QString const& _processName): AcausalAnalysis(_processName) {}
	virtual ~CausalAnalysis() {}

	// public API (new)
	void initialize(bool _willRecord) { init(_willRecord); }

	// inherited
	virtual void init();
	virtual void fini(bool _completed);
	virtual unsigned prepare(unsigned _from, unsigned _count, lb::Time);
	virtual void analyze(unsigned _from, unsigned _count, lb::Time _hop);

	// new
protected:
	virtual bool init(bool _willRecord) { return _willRecord; }
public:// TODO: move to protected & introduce non-virtual public API.
	virtual void fini(bool _completed, bool _didRecord) { (void)_completed; (void)_didRecord; }
	virtual void noteBatch(unsigned, unsigned) {}
	virtual void process(unsigned, lb::Time) {}
	virtual void record(unsigned, lb::Time) {}

private:
	bool m_willCompute;
};

typedef std::shared_ptr<CausalAnalysis> CausalAnalysisPtr;
typedef std::vector<CausalAnalysisPtr> CausalAnalysisPtrs;

Q_DECLARE_METATYPE(CausalAnalysisPtr);

template <class _S> _S& operator<<(_S& _out, CausalAnalysis const& _ca) { return _out << "CA(" << _ca.name() << ")"; }
