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
 * @file CPUTestParallelizedGillespie.cpp
 * @brief << brief description >>
 * @author clonker
 * @date 01.09.16
 */

#include <gtest/gtest.h>
#include <readdy/kernel/cpu/CPUKernel.h>
#include <readdy/kernel/cpu/nl/AdaptiveNeighborList.h>

namespace {

TEST(TestParallelGillespie, Sanity) {
    readdy::kernel::cpu::CPUKernel kernel;
    kernel.context().boxSize() = {{10, 10, 11}};
    kernel.context().particle_types().add("A", 10.0, 1.);
    kernel.context().reactions().addFusion("Fusion", "A", "A", "A", 10, 1.0);
    kernel.addParticle("A", {-5, .2, -5.5});
    kernel.addParticle("A", {-5, .2, 5.5});
    kernel.addParticle("A", {-5, .2, 0});
    {
        readdy::conf::Configuration conf;
        conf.cpu.neighborList.type = "Adaptive";
        kernel.context().kernelConfiguration() = conf;
    }
    kernel.context().configure();
    kernel.initialize();
    kernel.getCPUKernelStateModel().initializeNeighborList(0.);

    auto prog = kernel.createAction<readdy::model::actions::reactions::Gillespie>(1);
    prog->perform();
}
}