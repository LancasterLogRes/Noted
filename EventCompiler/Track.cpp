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

#include <string>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>
#include "Track.h"
using namespace std;
using namespace Lightbox;

LIGHTBOX_API void Track::readFile(string const& _filename)
{
	syncPoints.clear();
	syncPoints.push_back(Time(0));
	events.clear();
	ifstream in;
	in.open(_filename.c_str());
	if (in)
	{
		using boost::property_tree::ptree;
		ptree pt;
		read_xml(in, pt);

		foreach (ptree::value_type const& v, pt.get_child("events"))
			if (v.first == "time")
			{
				int64_t ms = v.second.get<int64_t>("<xmlattr>.ms", 0);
				Time t = v.second.get<Time>("<xmlattr>.value", fromMsecs(ms));
				foreach (ptree::value_type const& w, v.second)
					if (w.first != "<xmlattr>")
					{
						StreamEvent se;
						se.type = toEventType(w.first, false);
						se.strength = w.second.get<float>("<xmlattr>.strength", 1.f);
						se.temperature = w.second.get<float>("<xmlattr>.temperature", 0.f);
						se.jitter = w.second.get<float>("<xmlattr>.jitter", 0.f);
						se.constancy = w.second.get<float>("<xmlattr>.constancy", 0.f);
						se.position = w.second.get<int>("<xmlattr>.position", -1);
						se.surprise = w.second.get<float>("<xmlattr>.surprise", 1.f);
						se.character = toCharacter(w.second.get<string>("<xmlattr>.character", "Dull"));
						se.assign(w.second.get<int>("<xmlattr>.channel", CompatibilityChannel));
						events.insert(make_pair(t, se));
						if (se.type == SyncPoint)
							syncPoints.push_back(t);
					}
			}
	}
}

void Track::updateSyncPoints()
{
	syncPoints.clear();
	syncPoints.push_back(Time(0));
	for (auto i: events)
		if (i.second.type == SyncPoint)
			syncPoints.push_back(i.first);
}
