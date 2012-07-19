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

#include <QTableWidget>
#include <Common/Members.h>

class PropertiesEditor: public QTableWidget
{
	Q_OBJECT

public:
	explicit PropertiesEditor(QWidget* _p = nullptr);
	~PropertiesEditor();
	
	void setProperties(Lightbox::VoidMembers const& _properties);

signals:
	void changed();

public slots:
	void updateWidgets();

private slots:
	void onChanged();

private:
	Lightbox::VoidMembers m_properties;
	void* m_object;
};
