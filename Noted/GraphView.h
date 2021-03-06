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
#include <utility>

#include <EventCompiler/EventCompiler.h>

#include "CurrentView.h"

struct GraphViewPlot
{
	std::string ec;
	std::string graph;
	lb::Color c;
};

namespace lb {
class GraphSpec;
}

class GraphView: public CurrentView
{
	Q_OBJECT

public:
	GraphView(QWidget* _parent, QString const& _name);
	virtual ~GraphView();

	void addGraph(lb::GraphSpec* _g);

public slots:
	void rejig();

private:
	virtual void renderGL(QSize);

	std::vector<GraphViewPlot> m_graphs;
};


