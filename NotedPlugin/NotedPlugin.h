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
#include <QObject>
#include <Common/Time.h>
#include <Common/MemberMap.h>

#include "NotedFace.h"
#include "CausalAnalysis.h"
#include "AcausalAnalysis.h"

class Noted;
class QSettings;
class AuxLibraryFace;

// NotedPlugin -> readSettings -> [ titleAmendment | ( newAuxLibrary -> AuxLibraryFace::load { == false -> ~AuxLibraryFace } ) | ( AuxLibraryFace::unload -> ~AuxLibraryFace ) | WORKER{rejigData} | ( rejigData {in WORKER thread} -> onAnalyzed ) ] * -> writeSettings -> ~NotedPlugin

class NotedPlugin: public QObject
{
	friend class Noted;

public:
	typedef NotedPlugin LIGHTBOX_PROPERTIES_BaseClass;

	NotedPlugin(NotedFace*);
	virtual ~NotedPlugin();

	virtual AuxLibraryFace* newAuxLibrary() { return nullptr; }
	virtual void readSettings(QSettings&) {}
	virtual void writeSettings(QSettings&) {}
	virtual AcausalAnalysisPtrs ripeAnalysis(AcausalAnalysisPtr const&) { return AcausalAnalysisPtrs(); }
	virtual QString titleAmendment(QString const& _title) const { return _title; }
	virtual Lightbox::MemberMap propertyMap() const { return Lightbox::NullMemberMap; }
	virtual void onPropertiesChanged() {}

	NotedFace* noted() const { return m_noted; }

protected:
	QList<std::weak_ptr<AuxLibraryFace> > auxLibraries() { return m_auxLibraries; }

	unsigned rate() const { return m_noted->rate(); }
	unsigned hopSamples() const { return m_noted->hopSamples(); }
	Lightbox::Time hop() const { return m_noted->hop(); }
	unsigned hops() const { return m_noted->hops(); }
	unsigned windowIndex(Lightbox::Time _t) const { return m_noted->windowIndex(_t); }
	void updateWindowTitle() { m_noted->updateWindowTitle(); }
	void notePluginDataChanged() { m_noted->notePluginDataChanged(); }

private:
	void removeDeadAuxes();

	NotedFace* m_noted;
	QList<std::weak_ptr<AuxLibraryFace> > m_auxLibraries;
};

#define NOTED_PLUGIN(O) \
	LIGHTBOX_FINALIZING_LIBRARY \
	extern "C" __attribute__ ((visibility ("default"))) NotedPlugin* newPlugin(NotedFace* n) { return new O(n); } \
	extern "C" __attribute__ ((visibility ("default"))) char const* pluginName() { return #O; }

