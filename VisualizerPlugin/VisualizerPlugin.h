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

#include <QStringListModel>
#include <NotedPlugin/AuxLibraryFace.h>
#include <NotedPlugin/NotedPlugin.h>
#include <Viz/All.h>
#include <Viz/VizLibrary.h>

class VizGLWidgetProxy;
class VisualizerPlugin;
class QDockWidget;

struct AuxLibrary: public AuxLibraryFace
{
	AuxLibrary(VisualizerPlugin* _p): m_p(_p) {}
	virtual bool load(LibraryPtr const& _l);
	virtual void unload(LibraryPtr const& _l);

	lb::VizFactories m_vf;
	VisualizerPlugin* m_p;
};

class VisualizerPlugin: public NotedPlugin, public JobSource
{
	Q_OBJECT

	friend struct AuxLibrary;

public:
	VisualizerPlugin();
	virtual ~VisualizerPlugin();

	virtual AuxLibraryFace* newAuxLibrary() { return new AuxLibrary(this); }
	virtual CausalAnalysisPtrs ripeAnalysis(AcausalAnalysisPtr const& _finished);

//	virtual lb::MemberMap propertyMap() const;
//	virtual void onPropertiesChanged();

	lb::Viz createViz(std::string const& _name);

private:
	void rerealize();

	QList<VizGLWidgetProxy*> m_active;
	QStringListModel m_availableModel;
	QDockWidget* m_availableDock;
};
