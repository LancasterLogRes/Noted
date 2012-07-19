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

#include <EventCompiler/StreamEvent.h>

#include "CurrentView.h"

class QComboBox;

class DataView: public CurrentView
{
	Q_OBJECT

public:
	explicit DataView(QWidget* _parent, QString const& _name);
	~DataView() {}

	QComboBox* selection() { return m_selection; }
    void checkSpec();

public slots:
	void rejig();

private:
	virtual void mousePressEvent(QMouseEvent* _e);
	virtual void dropEvent(QDropEvent* _e);
	virtual void dragEnterEvent(QDragEnterEvent* _e);

	virtual void doRender(QImage& _img);

	std::pair<std::pair<float, float>, std::pair<float, float> > ranges(bool _needX, bool _needY, std::shared_ptr<Lightbox::AuxGraphsSpec> _spec, std::vector<Lightbox::StreamEvent> const& _ses = std::vector<Lightbox::StreamEvent>());
	std::shared_ptr<Lightbox::AuxGraphsSpec> findSpec(QString const& _n) const;

	QComboBox* m_selection;
	std::weak_ptr<Lightbox::AuxGraphsSpec> m_spec;
	std::weak_ptr<Lightbox::AuxGraphsSpec> m_xRangeSpec;
};
