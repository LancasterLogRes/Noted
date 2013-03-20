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
#include <string>
#include <QtCore>
#include <Common/Common.h>
#include <EventCompiler/EventCompilerImpl.h>

class ProcessEventCompiler: public Lightbox::EventCompilerNativeImpl<float>
{
public:
	ProcessEventCompiler(QString const& _program): m_s(&m_p), m_program(_program) {}
	std::string arguments;
	LIGHTBOX_PROPERTIES(arguments);

	virtual Lightbox::StreamEvents init()
	{
		m_p.kill();
		m_p.waitForFinished();
		QStringList args;
		args << QString::number(bands()) << QString::number(hop()) << QString::number(nyquist()) << QString::fromStdString(arguments).split(" ");
		m_p.start(m_program, args);
		return Lightbox::StreamEvents();
	}

	virtual Lightbox::StreamEvents compile(Lightbox::Time _t, std::vector<float> const& _mag, std::vector<float> const& _phase, std::vector<float> const& _wave)
	{
		Lightbox::StreamEvents ret;
		m_s << _t << endl;
		foreach (float f, _mag)
			m_s << f;
		m_s << endl;
		foreach (float f, _phase)
			m_s << f;
		m_s << endl;
		foreach (float f, _wave)
			m_s << f;
		m_s << endl;

		while (true)
		{
			m_p.waitForReadyRead();
			QString s = m_p.readLine();
			if (s.length() == 0)
				break;

			QStringList l = s.split(" ");
			Lightbox::StreamEvent e;
			e.type = Lightbox::toEventType(l[0].toStdString());
			e.strength = l[1].toFloat();
			e.temperature = l[2].toFloat();
			ret.push_back(e);
		}
		return ret;
	}

	QProcess m_p;
	QTextStream m_s;
	QString m_program;
};
