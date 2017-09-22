/********************************************************************
 * Copyright © 2016 Computational Molecular Biology Group,          * 
 *                  Freie Universität Berlin (GER)                  *
 *                                                                  *
 * This file is part of ReaDDy.                                     *
 *                                                                  *
 * ReaDDy is free software: you can redistribute it and/or modify   *
 * it under the terms of the GNU Lesser General Public License as   *
 * published by the Free Software Foundation, either version 3 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU Lesser General Public License for more details.              *
 *                                                                  *
 * You should have received a copy of the GNU Lesser General        *
 * Public License along with this program. If not, see              *
 * <http://www.gnu.org/licenses/>.                                  *
 ********************************************************************/


/**
 * << detailed description >>
 *
 * @file CenterOfMass.cpp
 * @brief << brief description >>
 * @author clonker
 * @date 13.03.17
 * @copyright GNU Lesser General Public License v3.0
 */

#include <h5rd/h5rd.h>

#include <readdy/model/Kernel.h>
#include <readdy/model/observables/io/Types.h>
#include <readdy/model/observables/io/TimeSeriesWriter.h>

namespace readdy {
namespace model {
namespace observables {


struct CenterOfMass::Impl {
    using writer_t = h5rd::DataSet;
    std::unique_ptr<writer_t> ds;
    std::unique_ptr<util::TimeSeriesWriter> timeSeries;
    std::unique_ptr<util::CompoundH5Types> h5types;
};

CenterOfMass::CenterOfMass(readdy::model::Kernel *const kernel, unsigned int stride,
                           unsigned int particleType)
        : Observable(kernel, stride), particleTypes({particleType}), pimpl(std::make_unique<Impl>()) {
}

CenterOfMass::CenterOfMass(Kernel *const kernel, unsigned int stride,
                           const std::string &particleType)
        : CenterOfMass(kernel, stride, kernel->context().particle_types().id_of(particleType)) {}

void CenterOfMass::evaluate() {
    Vec3 com{0, 0, 0};
    unsigned long n_particles = 0;
    for (auto &&p : kernel->stateModel().getParticles()) {
        if (particleTypes.find(p.getType()) != particleTypes.end()) {
            ++n_particles;
            com += p.getPos();
        }
    }
    com /= n_particles;
    result = com;
}

CenterOfMass::CenterOfMass(Kernel *const kernel, unsigned int stride,
                           const std::vector<unsigned int> &particleTypes)
        : Observable(kernel, stride), particleTypes(particleTypes.begin(), particleTypes.end()),
          pimpl(std::make_unique<Impl>()) {
}

CenterOfMass::CenterOfMass(Kernel *const kernel, unsigned int stride, const std::vector<std::string> &particleType)
        : Observable(kernel, stride), pimpl(std::make_unique<Impl>()) {
    for (auto &&pt : particleType) {
        particleTypes.emplace(kernel->context().particle_types().id_of(pt));
    }

}

void CenterOfMass::initializeDataSet(File &file, const std::string &dataSetName, unsigned int flushStride) {
    if (!pimpl->ds) {
        pimpl->h5types = std::make_unique<util::CompoundH5Types>(util::getVec3Types(file.parentFile()));
        h5rd::dimensions fs = {flushStride};
        h5rd::dimensions dims = {h5rd::UNLIMITED_DIMS};
        auto group = file.createGroup(std::string(util::OBSERVABLES_GROUP_PATH) + "/" + dataSetName);
        pimpl->ds = group.createDataSet("data", fs, dims, std::get<0>(*pimpl->h5types), std::get<1>(*pimpl->h5types));
        log::debug("created data set with path {}", std::string(util::OBSERVABLES_GROUP_PATH) + "/" + dataSetName);
        pimpl->timeSeries = std::make_unique<util::TimeSeriesWriter>(group, flushStride);
    }
}

void CenterOfMass::append() {
    pimpl->ds->append({1}, &result);
    pimpl->timeSeries->append(t_current);
}

void CenterOfMass::flush() {
    if (pimpl->ds) pimpl->ds->flush();
    if (pimpl->timeSeries) pimpl->timeSeries->flush();
}

CenterOfMass::~CenterOfMass() = default;

}
}
}