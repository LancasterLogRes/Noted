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

#include <Common/MemberCollection.h>
#include <NotedPlugin/NotedPlugin.h>

class QDockWidget;
class QWidget;
class GLView;
class AnalyzeViz;

class ExamplePlugin: public NotedPlugin, public JobSource
{
	Q_OBJECT

	friend class AnalyzeViz;

public:
	ExamplePlugin();
	virtual ~ExamplePlugin();

	virtual CausalAnalysisPtrs ripeAnalysis(AcausalAnalysisPtr const& _finished);

	float scale = 2.f;
	float bias = .5f;
	LIGHTBOX_PROPERTIES(scale, bias);
	virtual void onPropertiesChanged();

private:
	QWidget* m_vizDock;
	GLView* m_glView;
	CausalAnalysisPtr m_analysis;
};
