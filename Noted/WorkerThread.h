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
	void start(Priority _p = InheritPriority) { m_quitting = false; QThread::start(_p); }
	int progress() const { QMutexLocker l(&m_lock); return m_progress; }
	QString description() const { QMutexLocker l(&m_lock); return m_description; }

	static bool quitting() { if (auto wt = dynamic_cast<WorkerThread*>(currentThread())) return wt->m_quitting; return false; }
	static void setCurrentProgress(int _percent) { if (auto wt = dynamic_cast<WorkerThread*>(currentThread())) wt->setProgress(_percent); }
	static void setCurrentDescription(QString const& _s) { if (auto wt = dynamic_cast<WorkerThread*>(currentThread())) wt->setDescription(_s); }

	void setProgress(int _percent) { QMutexLocker l(&m_lock); m_progress = _percent; }
	void setDescription(QString const& _s) { QMutexLocker l(&m_lock); m_description = _s; }

protected:
	bool m_quitting;
	mutable QMutex m_lock;
	QString m_description;
	int m_progress;
};

template <class _F>
class TypedWorkerThread: public WorkerThread
{
public:
	TypedWorkerThread(_F const& _f): m_f(_f) {}
	virtual void run() { while (!m_quitting) { if (!m_f()) m_quitting = true; } }

private:
	_F m_f;
};

template <class _F> static TypedWorkerThread<_F>* createWorkerThread(_F const& _f) { return new TypedWorkerThread<_F>(_f); }

template <class _F, class _I, class _E>
class TypedInitFiniWorkerThread: public WorkerThread
{
public:
	TypedInitFiniWorkerThread(_F const& _f, _I const& _i, _E const& _e): m_f(_f), m_i(_i), m_e(_e) {}
	virtual void run() { m_i(); while (!m_quitting) { if (!m_f()) m_quitting = true; } m_e(); }

private:
	_F m_f;
	_I m_i;
	_E m_e;
};

template <class _F, class _I, class _E> static TypedInitFiniWorkerThread<_F, _I, _E>* createWorkerThread(_F const& _f, _I const& _i, _E const& _e) { return new TypedInitFiniWorkerThread<_F, _I, _E>(_f, _i, _e); }
