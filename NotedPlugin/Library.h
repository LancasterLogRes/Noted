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
#include <QLibrary>
#include <EventCompiler/EventCompilerLibrary.h>

class AuxLibraryFace;
class NotedPlugin;

struct Library
{
	Library(QString const& _filename): filename(_filename) {}

	QString filename;
	QString nick;
	QLibrary l;

	// One of (p, cf, auxFace) is valid.
	std::shared_ptr<NotedPlugin> p;
	Lightbox::EventCompilerFactories cf;
	std::shared_ptr<AuxLibraryFace> auxFace;
	std::weak_ptr<NotedPlugin> aux;
};

typedef std::shared_ptr<Library> LibraryPtr;
