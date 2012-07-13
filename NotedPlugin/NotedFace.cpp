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
using namespace std;

#include <QDebug>
#include <QtGui>

#include <Common/Common.h>
using namespace Lightbox;

#include "NotedFace.h"

NotedFace::NotedFace(QWidget* _p):
	QMainWindow					(_p),
	m_rate						(1),
	m_hopSamples				(2),
	m_samples					(0),
	m_zeroPhase					(false),
	m_normalize					(false)
{
}

NotedFace::~NotedFace()
{
}

// TODO: Change timelineDuration (or whatever it is) to pixelDuration and offset should be done from left border of Noted window.
