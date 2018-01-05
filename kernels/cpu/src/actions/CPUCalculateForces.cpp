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
 * @file CPUCalculateForces.cpp
 * @author clonker
 * @date 1/3/18
 */

#include "readdy/kernel/cpu/actions/CPUCalculateForces.h"

namespace readdy {
namespace kernel {
namespace cpu {
namespace actions {

void CPUCalculateForces::perform(const util::PerformanceNode &node) {
    auto t = node.timeit();

    const auto &ctx = kernel->context();

    auto &stateModel = kernel->getCPUKernelStateModel();
    auto neighborList = stateModel.getNeighborList();
    auto data = stateModel.getParticleData();
    auto taf = kernel->getTopologyActionFactory();
    auto &topologies = stateModel.topologies();

    stateModel.energy() = 0;

    const auto &potOrder1 = ctx.potentials().potentialsOrder1();
    const auto &potOrder2 = ctx.potentials().potentialsOrder2();
    if (!potOrder1.empty() || !potOrder2.empty() || !stateModel.topologies().empty()) {
        {
            // todo maybe optimize this by transposing data structure
            auto tClear = node.subnode("clear forces").timeit();
            std::for_each(data->begin(), data->end(), [](auto &entry) {
                entry.force = {0, 0, 0};
            });
        }
        {
            auto &pool = data->pool();
            std::vector<std::promise<scalar>> promises;
            // 1st order pot + topologies = 2*pool size
            // 2nd order pot <= nl.nCells
            size_t nThreads = pool.size();
            auto numberTasks = (!potOrder1.empty() ? nThreads : 0)
                               + (!potOrder2.empty() ? nThreads : 0)
                               + (!topologies.empty() ? nThreads : 0);
            {
                const auto &nTasks = node.subnode("create tasks");
                auto tTasks = nTasks.timeit();
                size_t nCells = neighborList->nCells();
                promises.reserve(numberTasks);
                if (!potOrder1.empty()) {
                    // 1st order pot
                    auto tO1 = nTasks.subnode("order1").timeit();
                    std::vector<std::function<void(std::size_t)>> tasks;
                    tasks.reserve(nThreads);

                    const std::size_t grainSize = data->size() / nThreads;
                    auto it = data->begin();
                    for (auto i = 0_z; i < nThreads - 1; ++i) {
                        auto itNext = std::min(it + grainSize, data->end());
                        if (it != itNext) {
                            promises.emplace_back();
                            auto dataBounds = std::make_tuple(it, itNext);
                            tasks.push_back(pool.pack(calculate_order1, dataBounds, std::ref(promises.back()), data,
                                                      ctx.potentials().potentialsOrder1(), ctx.shortestDifferenceFun()));
                        }
                        it = itNext;
                    }
                    if (it != data->end()) {
                        promises.emplace_back();
                        auto dataBounds = std::make_tuple(it, data->end());
                        tasks.push_back(pool.pack(calculate_order1, dataBounds, std::ref(promises.back()), data,
                                                  ctx.potentials().potentialsOrder1(), ctx.shortestDifferenceFun()));
                    }
                    {
                        auto tPush = nTasks.subnode("execute order 1 tasks and wait").timeit();
                        auto futures = pool.pushAll(std::move(tasks));
                        std::vector<util::thread::joining_future<void>> joiningFutures;
                        std::transform(futures.begin(), futures.end(), std::back_inserter(joiningFutures),
                                       [](auto &&future) {
                                           return util::thread::joining_future<void>{std::move(future)};
                                       });
                    }
                }
                if (!topologies.empty()) {
                    auto tTops = nTasks.subnode("topologies").timeit();
                    std::vector<std::function<void(std::size_t)>> tasks;
                    tasks.reserve(nThreads);
                    const std::size_t grainSize = topologies.size() / nThreads;
                    auto it = topologies.cbegin();
                    for (auto i = 0_z; i < nThreads - 1; ++i) {
                        auto itNext = std::min(it + grainSize, topologies.cend());
                        if (it != itNext) {
                            promises.emplace_back();
                            auto bounds = std::make_tuple(it, itNext);
                            tasks.push_back(
                                    pool.pack(calculate_topologies, bounds, taf, std::ref(promises.back())));
                        }
                        it = itNext;
                    }
                    if (it != topologies.cend()) {
                        promises.emplace_back();
                        auto bounds = std::make_tuple(it, topologies.cend());
                        tasks.push_back(pool.pack(calculate_topologies, bounds, taf, std::ref(promises.back())));
                    }
                    {
                        auto tPush = nTasks.subnode("execute topology tasks and wait").timeit();
                        auto futures = pool.pushAll(std::move(tasks));
                        std::vector<util::thread::joining_future<void>> joiningFutures;
                        std::transform(futures.begin(), futures.end(), std::back_inserter(joiningFutures),
                                       [](auto &&future) {
                                           return util::thread::joining_future<void>{std::move(future)};
                                       });
                    }
                }
                if (!potOrder2.empty()) {
                    auto tO2 = nTasks.subnode("order2").timeit();
                    std::vector<std::function<void(std::size_t)>> tasks;
                    tasks.reserve(nThreads);
                    auto granularity = nThreads;
                    const std::size_t grainSize = nCells / granularity;
                    auto it = 0_z;
                    for (auto i = 0_z; i < granularity - 1; ++i) {
                        auto itNext = std::min(it + grainSize, nCells);
                        if (it != itNext) {
                            promises.emplace_back();
                            tasks.push_back(pool.pack(
                                    calculate_order2, std::make_tuple(it, itNext), data, std::cref(*neighborList),
                                    std::ref(promises.back()), ctx.potentials().potentialsOrder2(),
                                    ctx.shortestDifferenceFun()
                            ));
                        }
                        it = itNext;
                    }
                    if (it != nCells) {
                        promises.emplace_back();
                        tasks.push_back(pool.pack(
                                calculate_order2, std::make_tuple(it, nCells), data, std::cref(*neighborList),
                                std::ref(promises.back()), ctx.potentials().potentialsOrder2(),
                                ctx.shortestDifferenceFun()
                        ));
                    }
                    {
                        auto tPush = nTasks.subnode("execute order 2 tasks and wait").timeit();
                        auto futures = pool.pushAll(std::move(tasks));
                        std::vector<util::thread::joining_future<void>> joiningFutures;
                        std::transform(futures.begin(), futures.end(), std::back_inserter(joiningFutures),
                                       [](auto &&future) {
                                           return util::thread::joining_future<void>{std::move(future)};
                                       });
                    }
                }
            }

            {
                auto tFutures = node.subnode("get energy futures").timeit();
                for (auto &f : promises) {
                    stateModel.energy() += f.get_future().get();
                }
            }
        }
    }
}

void CPUCalculateForces::calculate_order2(std::size_t, nl_bounds nlBounds,
                                          CPUStateModel::data_type *data, const CPUStateModel::neighbor_list &nl,
                                          std::promise<scalar> &energyPromise,
                                          model::potentials::PotentialRegistry::potential_o2_registry pot2,
                                          model::Context::shortest_dist_fun d) {
    scalar energyUpdate = 0.0;

    //
    // 2nd order potentials
    //
    for (auto cell = std::get<0>(nlBounds); cell < std::get<1>(nlBounds); ++cell) {
        for (auto particleIt = nl.particlesBegin(cell); particleIt != nl.particlesEnd(cell); ++particleIt) {
            auto &entry = data->entry_at(*particleIt);
            if (entry.deactivated) {
                log::critical("deactivated particle in neighbor list!");
                continue;
            }

            nl.forEachNeighbor(*particleIt, cell, [&](auto neighborIndex) {
                auto &neighbor = data->entry_at(neighborIndex);
                if (!neighbor.deactivated) {
                    auto &force = entry.force;
                    const auto &myPos = entry.pos;

                    //
                    // 2nd order potentials
                    //
                    scalar mySecondOrderEnergy = 0.;
                    auto potit = pot2.find(std::tie(entry.type, neighbor.type));
                    if (potit != pot2.end()) {
                        auto x_ij = d(myPos, neighbor.pos);
                        auto distSquared = x_ij * x_ij;
                        for (const auto &potential : potit->second) {
                            if (distSquared < potential->getCutoffRadiusSquared()) {
                                Vec3 forceUpdate{0, 0, 0};
                                potential->calculateForceAndEnergy(forceUpdate, mySecondOrderEnergy, x_ij);
                                force += forceUpdate;
                            }
                        }
                    }
                    // The contribution of second order potentials must be halved since we parallelize over particles.
                    // Thus every particle pair potential is seen twice
                    energyUpdate += 0.5 * mySecondOrderEnergy;
                } else {
                    log::critical("disabled neighbour");
                }
            });
        }

    }
    energyPromise.set_value(energyUpdate);

}

void CPUCalculateForces::calculate_topologies(std::size_t, top_bounds topBounds,
                                              model::top::TopologyActionFactory *taf,
                                              std::promise<scalar> &energyPromise) {
    scalar energyUpdate = 0.0;
    for (auto it = std::get<0>(topBounds); it != std::get<1>(topBounds); ++it) {
        const auto &top = *it;
        if (!top->isDeactivated()) {
            for (const auto &bondedPot : top->getBondedPotentials()) {
                auto energy = bondedPot->createForceAndEnergyAction(taf)->perform(top.get());
                energyUpdate += energy;
            }
            for (const auto &anglePot : top->getAnglePotentials()) {
                auto energy = anglePot->createForceAndEnergyAction(taf)->perform(top.get());
                energyUpdate += energy;
            }
            for (const auto &torsionPot : top->getTorsionPotentials()) {
                auto energy = torsionPot->createForceAndEnergyAction(taf)->perform(top.get());
                energyUpdate += energy;
            }
        }
    }

    energyPromise.set_value(energyUpdate);
}

void CPUCalculateForces::calculate_order1(std::size_t, data_bounds dataBounds,
                                          std::promise<scalar> &energyPromise, CPUStateModel::data_type *data,
                                          model::potentials::PotentialRegistry::potential_o1_registry pot1,
                                          model::Context::shortest_dist_fun d) {
    scalar energyUpdate = 0.0;

    //
    // 1st order potentials
    //
    for (auto it = std::get<0>(dataBounds); it != std::get<1>(dataBounds); ++it) {
        auto &entry = *it;
        if (!entry.deactivated) {
            auto &force = entry.force;
            force = {c_::zero, c_::zero, c_::zero};
            const auto &myPos = entry.pos;
            auto find_it = pot1.find(entry.type);
            if (find_it != pot1.end()) {
                for (const auto &potential : find_it->second) {
                    potential->calculateForceAndEnergy(force, energyUpdate, myPos);
                }
            }
        }
    }
    energyPromise.set_value(energyUpdate);
}
}
}
}
}