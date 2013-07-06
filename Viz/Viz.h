#pragma once

#include <vector>
#include <memory>
#include <Common/Common.h>
#include "VizImpl.h"

namespace lb
{

class Viz
{
public:
	static Viz create(VizImpl* _i) { Viz ret; ret.m_impl = std::shared_ptr<VizImpl>(_i); return ret; }

	Members<VizImpl> properties() const { return m_impl ? Members<VizImpl>(m_impl->propertyMap(), m_impl) : Members<VizImpl>(); }
	std::string name() const { if (m_impl) return demangled(typeid(*m_impl).name()); else return ""; }
	std::vector<lb::GenericCompute> tasks() { return m_impl ? m_impl->tasks() : std::vector<lb::GenericCompute>(); }

	void initializeGL() const { if (m_impl) m_impl->initializeGL(); }
	void resizeGL(int _w, int _h) { if (m_impl) m_impl->resizeGL(_w, _h); }
	void prepGL() { if (m_impl) m_impl->prepGL(); }
	void paintGL(Time _sinceLast) { if (m_impl) m_impl->paintGL(_sinceLast); }

	bool operator<(Viz const& _c) const { return m_impl < _c.m_impl; }
	bool operator==(Viz const& _c) const { return m_impl == _c.m_impl; }
	bool operator!=(Viz const& _c) const { return !operator==(_c); }

	bool isNull() const { return !m_impl; }

	template <class T> bool isA() const { return !!dynamic_cast<T*>(m_impl.get()); }
	template <class T> T& asA() const { return *dynamic_cast<T*>(m_impl.get()); }

private:
	std::shared_ptr<VizImpl> m_impl;
};

typedef std::vector<Viz> Vizs;

static const Viz NullViz;

template <class _S>
_S& operator<<(_S& _out, Viz const& _d)
{
	return _out << "{" << _d.name() << "}";
}

}
