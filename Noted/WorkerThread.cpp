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

#include <map>
#include "WorkerThread.h"
using namespace std;

void WorkerThread::setProgress(int _percent)
{
	if (m_progress != _percent)
	{
		QMutexLocker l(&m_lock);
		m_progress = _percent;
		if (lb::wallTime() - m_lastProgressSignal > lb::FromMsecs<100>::value)
		{
			emit progressed(m_description, _percent);
			m_lastProgressSignal = lb::wallTime();
		}
	}
}

void WorkerThread::setDescription(QString const& _s)
{
	QMutexLocker l(&m_lock);
	if (m_description != _s)
	{
		m_description = _s;
		emit progressed(_s, m_progress);
		m_lastProgressSignal = lb::wallTime();
	}
}
