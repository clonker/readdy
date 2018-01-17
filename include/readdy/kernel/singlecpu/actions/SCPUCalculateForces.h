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
 * @file SCPUCalculateForces.h
 * @brief << brief description >>
 * @author clonker
 * @date 20.06.16
 */
#pragma once
#include <readdy/model/actions/Actions.h>
#include <readdy/kernel/singlecpu/SCPUKernel.h>
#include <readdy/common/boundary_condition_operations.h>

namespace readdy {
namespace kernel {
namespace scpu {
namespace actions {
class SCPUCalculateForces : public readdy::model::actions::CalculateForces {
public:
    explicit SCPUCalculateForces(SCPUKernel *kernel) : kernel(kernel) {};

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
    };

private:

    template<bool PBC1, bool PBC2, bool PBC3>
    void performImpl(const util::PerformanceNode &node) {
        auto t = node.timeit();

        const auto &context = kernel->context();

        auto &stateModel = kernel->getSCPUKernelStateModel();
        auto &data = *stateModel.getParticleData();
        auto &neighborList = *stateModel.getNeighborList();

        stateModel.energy() = 0;

        const auto &potentials = context.potentials();
        auto &topologies = stateModel.topologies();
        if (!potentials.potentialsOrder1().empty() || !potentials.potentialsOrder2().empty() || !topologies.empty()) {
            auto tClear = node.subnode("clear").timeit();
            std::for_each(data.begin(), data.end(), [](auto &entry) {
                entry.force = {0, 0, 0};
            });
        }

        // update forces and energy order 1 potentials
        if (!potentials.potentialsOrder1().empty()) {
            {
                auto tFirstOrder = node.subnode("first order").timeit();
                std::transform(data.begin(), data.end(), data.begin(), [&potentials, &stateModel](auto &entry) {
                    if (!entry.deactivated) {
                        for (const auto &po1 : potentials.potentialsOf(entry.type)) {
                            po1->calculateForceAndEnergy(entry.force, stateModel.energy(), entry.position());
                        }
                    }
                    return entry;
                });
            }
        }

        // update forces and energy order 2 potentials
        if (!potentials.potentialsOrder2().empty()) {
            auto tSecondOrder = node.subnode("second order").timeit();

            const auto &box = context.boxSize();
            //const auto &difference = context.shortestDifferenceFun();
            for (auto cell = 0_z; cell < neighborList.nCells(); ++cell) {
                for (auto it = neighborList.particlesBegin(cell); it != neighborList.particlesEnd(cell); ++it) {
                    auto pidx = *it;
                    auto &entry = data.entry_at(pidx);
                    const auto &pots = potentials.potentialsOrder2(entry.type);
                    neighborList.forEachNeighbor(it, cell, [&](const std::size_t neighbor) {
                        auto &neighborEntry = data.entry_at(neighbor);
                        auto itPot = pots.find(neighborEntry.type);
                        if (itPot != pots.end()) {
                            Vec3 forceVec{0, 0, 0};
                            auto x_ij = bcs::shortestDifference<PBC1, PBC2, PBC3>(entry.position(), neighborEntry.position(), box);
                            for (const auto &potential : itPot->second) {
                                potential->calculateForceAndEnergy(forceVec, stateModel.energy(), x_ij);
                            }
                            entry.force += forceVec;
                            neighborEntry.force += -1 * forceVec;
                        }
                    });
                }
            }
        }
        // update forces and energy for topologies
        {
            auto tTopologies = node.subnode("topologies").timeit();
            auto taf = kernel->getTopologyActionFactory();
            for (auto &topology : topologies) {
                if (!topology->isDeactivated()) {
                    // calculate bonded potentials
                    for (const auto &bondedPot : topology->getBondedPotentials()) {
                        auto energy = bondedPot->createForceAndEnergyAction(taf)->perform(topology.get());
                        stateModel.energy() += energy;
                    }
                    for (const auto &anglePot : topology->getAnglePotentials()) {
                        auto energy = anglePot->createForceAndEnergyAction(taf)->perform(topology.get());
                        stateModel.energy() += energy;
                    }
                    for (const auto &torsionPot : topology->getTorsionPotentials()) {
                        auto energy = torsionPot->createForceAndEnergyAction(taf)->perform(topology.get());
                        stateModel.energy() += energy;
                    }
                }
            }
        }
    }
    SCPUKernel *kernel;
};
}
}
}
}
