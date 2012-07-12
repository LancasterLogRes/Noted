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

#include <QtGui>

#include "PropertiesEditor.h"

using namespace std;
using namespace Lightbox;

PropertiesEditor::PropertiesEditor(QWidget* _p):
	QWidget(_p)
{
	new QGridLayout(this);
}

PropertiesEditor::~PropertiesEditor()
{
}

void PropertiesEditor::setPropertyMap(Lightbox::Properties const& _properties)
{
	m_properties = _properties;
	updateWidgets();
}

template <class _Looker>
typename _Looker::Returns doTyped(Property const& _p, void* _o, _Looker& _l)
{
	string n(_p.type->name());
#define	DO(X) if (n == typeid(X).name()) { X x; X& rx = _p.get<X>(_o, x); return _l(rx); }
#include "DoTypes.h"
#undef DO
	return typename _Looker::Returns(0);
}

void PropertiesEditor::updateWidgets()
{
	foreach (auto i, findChildren<QWidget*>())
		delete i;

	struct Populator
	{
		typedef QWidget* Returns;
		QWidget* operator()(bool _b)
		{
			QPushButton* w = new QPushButton("Enable");
			w->setCheckable(true);
			w->setChecked(_b);
			return w;
		}
		QWidget* operator()(unsigned char _l) { return operator()((unsigned long long)_l); }
		QWidget* operator()(unsigned short _l) { return operator()((unsigned long long)_l); }
		QWidget* operator()(unsigned _l) { return operator()((unsigned long long)_l); }
		QWidget* operator()(unsigned long _l) { return operator()((unsigned long long)_l); }
		QWidget* operator()(signed char _l) { return operator()((signed long long)_l); }
		QWidget* operator()(signed short _l) { return operator()((signed long long)_l); }
		QWidget* operator()(signed _l) { return operator()((signed long long)_l); }
		QWidget* operator()(signed long _l) { return operator()((signed long long)_l); }
		QWidget* operator()(unsigned long long _l) { return operator()((signed long long)_l); }
		QWidget* operator()(signed long long _l)
		{
			QSpinBox* w = new QSpinBox;
			w->setValue(_l);
			return w;
		}
		QWidget* operator()(double _d)
		{
			QDoubleSpinBox* w = new QDoubleSpinBox;
			w->setValue(_d);
			return w;
		}
		QWidget* operator()(string const& _s)
		{
			QLineEdit* w = new QLineEdit;
			w->setText(_s.c_str());
			return w;
		}
	} pop;

	int r = 0;
	foreach (auto i, m_properties)
	{
		dynamic_cast<QGridLayout*>(layout())->addWidget(new QLabel(QString::fromStdString(i.first)), r, 0);
		// Big long type lookup...
		if (QWidget* w = doTyped(i.second, m_object, pop))
		{
			dynamic_cast<QGridLayout*>(layout())->addWidget(w, r, 1);
		}
		++r;
	}
}

