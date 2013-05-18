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

#include <cstdlib>
#include <memory>
#include <unordered_map>
#include <QDebug>
#include <QtGui>
#include <Common/Common.h>
#include "NotedFace.h"
using namespace std;
using namespace lb;

NotedFace* NotedFace::s_this = nullptr;

NotedFace::NotedFace(QWidget* _p):
	QMainWindow					(_p)
{
	s_this = this;
	qRegisterMetaType<lb::Time>("lb::Time");
	qRegisterMetaType<DataKey>();
	qRegisterMetaType<AcausalAnalysis*>();
	qRegisterMetaType<CausalAnalysis*>();
}

NotedFace::~NotedFace()
{
}
