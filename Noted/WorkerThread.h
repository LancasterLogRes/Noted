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

#include <QDebug>
#include <QThread>
#include <QMutex>

class WorkerThread: public QThread
{
public:
	WorkerThread(): QThread(0), m_quitting(false) {}
	void quit() { m_quitting = true; }

protected:
	bool m_quitting;
};

template <class _F>
class TypedWorkerThread: public WorkerThread
{
public:
	TypedWorkerThread(_F const& _f): m_f(_f) {}
	virtual void run() { m_quitting = false; while (!m_quitting) { if (!m_f()) m_quitting = true; } }

private:
	_F m_f;
};

template <class _F> static TypedWorkerThread<_F>* createWorkerThread(_F const& _f) { return new TypedWorkerThread<_F>(_f); }
