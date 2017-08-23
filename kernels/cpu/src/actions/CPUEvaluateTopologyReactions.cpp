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
 * @file CPUEvaluateTopologyReactions.cpp
 * @brief << brief description >>
 * @author clonker
 * @date 15.06.17
 * @copyright GNU Lesser General Public License v3.0
 */

#include <readdy/kernel/cpu/actions/CPUEvaluateTopologyReactions.h>
#include <readdy/kernel/cpu/nl/NeighborList.h>

namespace readdy {
namespace kernel {
namespace cpu {
namespace actions {
namespace top {

CPUEvaluateTopologyReactions::CPUEvaluateTopologyReactions(CPUKernel *const kernel, scalar timeStep)
        : EvaluateTopologyReactions(timeStep), kernel(kernel) {}

/**
 * Struct holding information about a topology reaction event.
 * If:
 *   - reaction_idx == -1: external topology reaction
 *   - reaction_idx >= 0: internal (fission/conversion-type) topology reaction
 */
struct CPUEvaluateTopologyReactions::TREvent {
    using index_type = model::CPUParticleData::index_t;

    rate_t cumulative_rate{0};
    rate_t own_rate{0};
    std::size_t topology_idx{0};
    std::size_t reaction_idx{0};
    particle_type_type t1{0}, t2{0};
    // idx1 is always the particle that belongs to a topology
    index_type idx1{0}, idx2{0};
    bool external {false};

};

template<bool approximated>
bool performReactionEvent(scalar rate, scalar timeStep);

template<>
bool performReactionEvent<true>(const scalar rate, const scalar timeStep) {
    return readdy::model::rnd::uniform_real() < rate * timeStep;
}

template<>
bool performReactionEvent<false>(const scalar rate, const scalar timeStep) {
    return readdy::model::rnd::uniform_real() < 1 - std::exp(-rate * timeStep);
}

void CPUEvaluateTopologyReactions::perform() {
    auto &model = kernel->getCPUKernelStateModel();
    auto &topologies = model.topologies();

    if (!topologies.empty()) {

        auto events = gatherEvents();

        if (!events.empty()) {
            auto end = events.end();

            std::vector<readdy::model::top::GraphTopology> new_topologies;
            while (end != events.begin()) {
                const auto cumulative_rate = (end - 1)->cumulative_rate;

                const auto x = readdy::model::rnd::uniform_real(c_::zero, cumulative_rate);

                const auto eventIt = std::lower_bound(
                        events.begin(), end, x, [](const TREvent &elem1, const rate_t elem2) {
                            return elem1.cumulative_rate < elem2;
                        }
                );

                if (eventIt != events.end()) {
                    const auto &event = *eventIt;

                    if (performReactionEvent<true>(event.own_rate, timeStep)) {
                        log::trace("picked event {} / {} with rate {}", std::distance(events.begin(), eventIt) + 1,
                                   events.size(), eventIt->own_rate);
                        // perform the event!
                        auto &topology = topologies.at(event.topology_idx);
                        if (topology->isDeactivated()) {
                            log::critical("deactivated topology with idx {}", event.topology_idx);
                            for (auto it = events.begin(); it != end; ++it) {
                                log::warn(" -> event {}, {}, {}, {}", it->topology_idx, it->reaction_idx, it->own_rate,
                                          it->cumulative_rate);
                            }
                            for (auto it = end; it != events.end(); ++it) {
                                log::warn(" -> deactivated event {}, {}, {}, {}", it->topology_idx, it->reaction_idx,
                                          it->own_rate, it->cumulative_rate);
                            }
                        }
                        assert(!topology->isDeactivated());
                        if(!event.external) {
                            handleInternalReaction(topologies, new_topologies, event, topology);
                        } else {
                            handleExternalReaction(topology, event);
                        }

                        // loop over all other events, swap events considering this particular topology to the end
                        rate_t cumsum = 0;
                        //log::warn("------");
                        for (auto it = events.begin();;) {
                            if (it->topology_idx == event.topology_idx) {
                                --end;
                                //log::warn("swapping event with topology idx {}", it->topology_idx);
                                std::iter_swap(it, end);
                            }
                            cumsum += it->own_rate;
                            it->cumulative_rate = cumsum;
                            if (it >= end) {
                                break;
                            }
                            if (it->topology_idx != event.topology_idx) {
                                ++it;
                            }
                        }

                    } else {

                        // remove event from the list (ie shift it to the end)
                        --end;

                        // swap with last element that is not yet deactivated
                        std::iter_swap(eventIt, end);

                        // this element's cumulative rate gets initialized with its own rate
                        eventIt->cumulative_rate = eventIt->own_rate;
                        if (eventIt > events.begin()) {
                            // and then increased by the cumulative rate of its predecessor
                            eventIt->cumulative_rate += (eventIt - 1)->cumulative_rate;
                        }
                        // now update the cumulative rates of all following elements...
                        auto cumsum = (*eventIt).cumulative_rate;
                        for (auto _it = eventIt + 1; _it < end; ++_it) {
                            cumsum += (*_it).own_rate;
                            (*_it).cumulative_rate = cumsum;
                        }

                    }

                } else {
                    log::critical("this should not happen (event not found by drawn rate)");
                    throw std::logic_error("this should not happen (event not found by drawn rate)");
                }
            }

            if (!new_topologies.empty()) {
                for (auto &&top : new_topologies) {
                    if (!top.isNormalParticle(*kernel)) {
                        // we have a new topology here, update data accordingly.
                        top.updateReactionRates();
                        top.configure();
                        model.insert_topology(std::move(top));
                    } else {
                        // if we have a single particle that is not of flavor topology, remove from topology structure!
                        model.getParticleData()->entry_at(top.getParticles().front()).topology_index = -1;
                    }
                }
            }
        }
    }
}

void CPUEvaluateTopologyReactions::handleInternalReaction(CPUStateModel::topologies_vec &topologies,
                                                          std::vector<CPUStateModel::topology> &new_topologies,
                                                          const CPUEvaluateTopologyReactions::TREvent &event,
                                                          CPUStateModel::topology_ref &topology) const {
    auto &reaction = topology->registeredReactions().at(static_cast<std::size_t>(event.reaction_idx));
    auto result = std::get<0>(reaction).execute(*topology, kernel);
    if (!result.empty()) {
        // we had a topology fission, so we need to actually remove the current topology from the
        // data structure
        topologies.erase(topologies.begin() + event.topology_idx);
        //log::error("erased topology with index {}", event.topology_idx);
        assert(topology->isDeactivated());
        move(result.begin(), result.end(), back_inserter(new_topologies));
    } else {
        if (topology->isNormalParticle(*kernel)) {
            kernel->getCPUKernelStateModel().getParticleData()->entry_at(topology->getParticles().front()).topology_index = -1;
            topologies.erase(topologies.begin() + event.topology_idx);
            //log::error("erased topology with index {}", event.topology_idx);
            assert(topology->isDeactivated());
        }
    }
}

CPUEvaluateTopologyReactions::topology_reaction_events CPUEvaluateTopologyReactions::gatherEvents() {
    topology_reaction_events events;
    {
        rate_t current_cumulative_rate = 0;
        std::size_t topology_idx = 0;
        for (auto &top : kernel->getCPUKernelStateModel().topologies()) {
            if (!top->isDeactivated()) {
                std::size_t reaction_idx = 0;
                for (const auto &reaction : top->registeredReactions()) {
                    TREvent event{};
                    event.own_rate = std::get<1>(reaction);
                    event.cumulative_rate = event.own_rate + current_cumulative_rate;
                    current_cumulative_rate = event.cumulative_rate;
                    event.topology_idx = topology_idx;
                    event.reaction_idx = reaction_idx;

                    events.push_back(event);
                    ++reaction_idx;
                }
            }
            ++topology_idx;
        }

        const auto &context = kernel->getKernelContext();
        if (!context.reactions().external_topology_reactions().empty()) {
            const auto &reaction_registry = context.reactions();
            const auto &d2 = context.getDistSquaredFun();
            const auto &data = *kernel->getCPUKernelStateModel().getParticleData();
            const auto &nl = *kernel->getCPUKernelStateModel().getNeighborList();

            std::size_t index{0};
            // todo parallelize this
            for (auto it = data.begin(); it != data.end(); ++it, ++index) {
                const auto &entry = *it;
                if (!entry.deactivated && reaction_registry.is_topology_reaction_type(entry.type)) {
                    for (auto neighbor_index : nl.neighbors_of(index)) {
                        if (index > neighbor_index) {
                            continue;
                        }
                        const auto &neighbor = data.entry_at(neighbor_index);

                        if (!neighbor.deactivated) {
                            if (!reaction_registry.is_topology_reaction_type(neighbor.type)) {
                                continue;
                            }

                            const auto &reactions = reaction_registry.external_top_reactions_by_type(entry.type,
                                                                                                     neighbor.type);
                            const auto distSquared = d2(entry.pos, neighbor.pos);
                            std::size_t reaction_index = 0;
                            for (const auto &reaction : reactions) {
                                // todo this only covers the particle<->topology case, not the topology<->topology case
                                if (distSquared < reaction.radius() * reaction.radius()) {
                                    TREvent event{};
                                    event.own_rate = reaction.rate();
                                    event.cumulative_rate = event.own_rate + current_cumulative_rate;
                                    current_cumulative_rate = event.cumulative_rate;
                                    if (entry.topology_index >= 0 && neighbor.topology_index < 0) {
                                        event.topology_idx = static_cast<std::size_t>(entry.topology_index);
                                        event.t1 = entry.type;
                                        event.t2 = neighbor.type;
                                        event.idx1 = index;
                                        event.idx2 = neighbor_index;
                                    } else if (entry.topology_index < 0 && neighbor.topology_index >= 0) {
                                        event.topology_idx = static_cast<std::size_t>(neighbor.topology_index);
                                        event.t1 = neighbor.type;
                                        event.t2 = entry.type;
                                        event.idx1 = neighbor_index;
                                        event.idx2 = index;
                                    } else if (entry.topology_index >= 0 && neighbor.topology_index >= 0) {
                                        // todo this is a topology-topology fusion
                                        log::critical(
                                                "topology <-> topology fusion encountered, this should currently not happen");
                                    } else {
                                        log::critical("got no topology for topology-fusion");
                                    }
                                    event.reaction_idx = reaction_index;
                                    event.external = true;

                                    events.push_back(event);
                                }
                                ++reaction_index;
                            }
                        } else {
                            log::critical("Handling topology reactions: Deactivated entry in neighbor list!");
                        }
                    }
                }
            }
        }
    }
    return events;
}

void CPUEvaluateTopologyReactions::handleExternalReaction(CPUStateModel::topology_ref &topology,
                                                          const CPUEvaluateTopologyReactions::TREvent &event) {
    const auto& context = kernel->getKernelContext();
    const auto& reaction_registry = context.reactions();
    const auto& reaction = reaction_registry.external_top_reactions_by_type(event.t1, event.t2).at(event.reaction_idx);

    auto& model = kernel->getCPUKernelStateModel();
    auto& data = *model.getParticleData();

    auto& entry1 = data.entry_at(event.idx1);
    auto& entry2 = data.entry_at(event.idx2);
    auto& entry1Type = entry1.type;
    auto& entry2Type = entry2.type;
    if(entry1Type == reaction.type1()) {
        entry1Type = reaction.type_to1();
        entry2Type = reaction.type_to2();
    } else {
        entry1Type = reaction.type_to2();
        entry2Type = reaction.type_to1();
    }
    entry1.topology_index = event.topology_idx;
    entry2.topology_index = event.topology_idx;

    topology->appendParticle(event.idx2, entry2Type, event.idx1, entry1Type);
    topology->updateReactionRates();
    topology->configure();
}

}
}
}
}
}