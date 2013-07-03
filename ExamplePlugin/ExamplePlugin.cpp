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

#include <Compute/Compute.h>
#include <NotedPlugin/NotedFace.h>
#include <NotedPlugin/ComputeAnalysis.h>
#include <Compute/All.h>
#include "ExamplePlugin.h"
using namespace std;
using namespace lb;

NOTED_PLUGIN(ExamplePlugin);

ExamplePlugin::ExamplePlugin()
{
	auto mag = ExtractMagnitude(WindowedFourier(AccumulateWave(ComputeRegistrar::feeder())));
	auto bark = BarkPhon(mag);
	auto zc = ZeroCrossings(ComputeRegistrar::feeder());
	std::vector<ComputeTask> tasks = {
		{ zc, [=](){return zc.info().axes();}, { MinMaxInOutDigest } },
		{ mag, [=](){return mag.info().axes();}, { MeanDigest } },
		{ bark, [=](){return bark.info().axes();}, { MeanDigest } }
	};
	m_analysis = CausalAnalysisPtr(new ComputeAnalysis(tasks));
	NotedFace::compute()->registerJobSource(this);
}

ExamplePlugin::~ExamplePlugin()
{
	NotedFace::compute()->unregisterJobSource(this);
}

CausalAnalysisPtrs ExamplePlugin::ripeAnalysis(AcausalAnalysisPtr const& _finished)
{
	if (_finished == NotedFace::audio()->resampleWaveAcAnalysis())
		return { m_analysis };
	return CausalAnalysisPtrs();
}

