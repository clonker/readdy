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
 * @file SingleCPUEulerDBIntegrator.h
 * @brief << brief description >>
 * @author clonker
 * @date 19.04.16
 */
#pragma once
#include <readdy/model/actions/Actions.h>
#include <readdy/kernel/singlecpu/SCPUKernel.h>
#include <readdy/common/boundary_condition_operations.h>

namespace readdy {
namespace kernel {
namespace scpu {

namespace actions {
class SCPUEulerBDIntegrator : public readdy::model::actions::EulerBDIntegrator {

public:
    SCPUEulerBDIntegrator(SCPUKernel *kernel, scalar timeStep)
            : readdy::model::actions::EulerBDIntegrator(timeStep), kernel(kernel) {};

    void perform(const util::PerformanceNode &node) override {
        const auto &pbc = kernel->context().periodicBoundaryConditions();
        if(pbc[0]) {
            if(pbc[1]) {
                if(pbc[2]) {
                    performImpl<true, true, true>(node);
                } else {
                    performImpl<true, true, false>(node);
                }
            } else {
                if(pbc[2]) {
                    performImpl<true, false, true>(node);
                } else {
                    performImpl<true, false, false>(node);
                }
            }
        } else {
            if(pbc[1]) {
                if(pbc[2]) {
                    performImpl<false, true, true>(node);
                } else {
                    performImpl<false, true, false>(node);
                }
            } else {
                if(pbc[2]) {
                    performImpl<false, false, true>(node);
                } else {
                    performImpl<false, false, false>(node);
                }
            }
        }
    }

private:

    template<bool PX, bool PY, bool PZ>
    void performImpl(const util::PerformanceNode &node) {
        auto t = node.timeit();
        const auto &context = kernel->context();
        const auto &kbt = context.kBT();
        const auto &box = context.boxSize();
        auto& stateModel = kernel->getSCPUKernelStateModel();
        const auto pd = stateModel.getParticleData();
        for(auto& entry : *pd) {
            if(!entry.is_deactivated()) {
                const scalar D = context.particle_types().diffusionConstantOf(entry.type);
                const auto randomDisplacement = std::sqrt(2. * D * timeStep) * (readdy::model::rnd::normal3<readdy::scalar>());
                entry.pos += randomDisplacement;
                const auto deterministicDisplacement = entry.force * timeStep * D / kbt;
                entry.pos += deterministicDisplacement;
                bcs::fixPosition<PX, PY, PZ>(entry.pos, box);
            }
        }
    }

    SCPUKernel *kernel;
};
}
}
}
}
