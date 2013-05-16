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
#include <QtWidgets>

#include "PropertiesEditor.h"

using namespace std;
using namespace lb;

PropertiesEditor::PropertiesEditor(QWidget* _p):
	QTableWidget(_p)
{
	horizontalHeader()->setStretchLastSection(true);
	horizontalHeader()->hide();
	setColumnCount(1);
	setSortingEnabled(false);
	setShowGrid(false);
}

PropertiesEditor::~PropertiesEditor()
{
}

void PropertiesEditor::setProperties(lb::VoidMembers const& _properties)
{
	m_properties = _properties;
	updateWidgets();
}

struct Populator
{
	typedef QWidget* Returns;
	QObject* changeCollater;

	// Numeric types...
	template <class _T> QWidget* operator()(_T _l)
	{
		if (std::numeric_limits<_T>::is_integer)
		{
			QSpinBox* w = new QSpinBox;
			w->setMinimum(std::numeric_limits<_T>::min());
			w->setMaximum(std::numeric_limits<_T>::max());
			w->setValue(_l);
			if (changeCollater)
				changeCollater->connect(w, SIGNAL(valueChanged(int)), SLOT(onChanged()));
			return w;
		}
		else
		{
			QDoubleSpinBox* w = new QDoubleSpinBox;
			w->setDecimals(std::numeric_limits<_T>::digits10 - 1);
			w->setRange(-100.f, 100.f);
			w->setValue(_l);
			if (changeCollater)
				changeCollater->connect(w, SIGNAL(valueChanged(double)), SLOT(onChanged()));
			return w;
		}
	}
	QWidget* operator()(bool _b)
	{
		QPushButton* w = new QPushButton("Enable");
		w->setCheckable(true);
		w->setChecked(_b);
		if (changeCollater)
			changeCollater->connect(w, SIGNAL(toggled(bool)), SLOT(onChanged()));
		return w;
	}
	QWidget* operator()(string const& _s)
	{
		QLineEdit* w = new QLineEdit;
		w->setText(_s.c_str());
		if (changeCollater)
			changeCollater->connect(w, SIGNAL(editingFinished()), SLOT(onChanged()));
		return w;
	}
};

struct Changer
{
	QObject* sender;

	// Numeric types...
	template <class _T> _T operator()(_T) const
	{
		QSpinBox* w = dynamic_cast<QSpinBox*>(sender);
		return w->value();
	}
	float operator()(float) const { return (float)operator()(double(0)); }
	double operator()(double) const
	{
		QDoubleSpinBox* w = dynamic_cast<QDoubleSpinBox*>(sender);
		assert(w);
		return w->value();
	}
	bool operator()(bool) const
	{
		QPushButton* w = dynamic_cast<QPushButton*>(sender);
		assert(w);
		return w->isChecked();
	}
	string operator()(string const&) const
	{
		QLineEdit* w = dynamic_cast<QLineEdit*>(sender);
		assert(w);
		return w->text().toStdString();
	}
};

void PropertiesEditor::onChanged()
{
	QString n = sender()->objectName();
	Changer c = { sender() };
	if (m_properties.typedSet(n.toStdString(), c))
		emit changed();
}

void PropertiesEditor::updateWidgets()
{
	setRowCount(0);
	int r = 0;
	QStringList headerLabels;
	auto names = m_properties.names();
	setRowCount(names.size());
	for (auto i: names)
	{
		headerLabels.append(i.c_str());
		Populator pop = { this };
		if (QWidget* w = m_properties.typedGet(i, pop))
		{
			setCellWidget(r, 0, w);
			setRowHeight(r, verticalHeader()->minimumSectionSize());
			w->setObjectName(i.c_str());
		}
		else
		{
			auto it = new QTableWidgetItem(QString("<%1>").arg(m_properties.typeinfo(i).name()));
			it->setFlags(Qt::NoItemFlags);
			setItem(r, 0, it);
		}
		++r;
	}
	setVerticalHeaderLabels(headerLabels);
}

