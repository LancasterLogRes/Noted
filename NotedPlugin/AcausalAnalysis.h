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
#include <vector>
#include <QMetaType>
#include <QString>
#include <Common/Time.h>

class NotedFace;

class AcausalAnalysis
{
public:
	explicit AcausalAnalysis(QString const& _processName): m_name(_processName) {}
	virtual ~AcausalAnalysis() {}

	void initialize() { init(); }

	QString const& name() const { return m_name; }
	void go(unsigned _from, unsigned _count = (unsigned)-1);

protected:
	virtual void init() {}
	virtual void fini(bool _completed) { (void)_completed; }
	virtual unsigned prepare(unsigned, unsigned, lb::Time) { return 100; }
	virtual void analyze(unsigned, unsigned, lb::Time) {}

	bool done(unsigned _i);

private:
	QString m_name;
	unsigned m_steps;
};

typedef std::shared_ptr<AcausalAnalysis> AcausalAnalysisPtr;
typedef std::vector<AcausalAnalysisPtr> AcausalAnalysisPtrs;

Q_DECLARE_METATYPE(AcausalAnalysis*);

template <class _S> _S& operator<<(_S& _out, AcausalAnalysis const& _ca) { return _out << "AcA(" << _ca.name() << ")"; }

