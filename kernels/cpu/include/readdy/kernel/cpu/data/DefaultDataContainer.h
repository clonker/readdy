/********************************************************************
 * Copyright © 2017 Computational Molecular Biology Group,          * 
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
 * @file DefaultDataContainer.h
 * @brief << brief description >>
 * @author clonker
 * @date 14.09.17
 * @copyright GNU Lesser General Public License v3.0
 */

#pragma once

#include <readdy/model/Particle.h>
#include "DataContainer.h"

namespace readdy {
namespace kernel {
namespace cpu {
namespace data {

class NLDataContainer;

class DefaultDataContainer : public EntryDataContainer {
    using super = DataContainer<readdy::kernel::cpu::data::Entry>;
    friend class NLDataContainer;
public:

    explicit DefaultDataContainer(EntryDataContainer *entryDataContainer);

    DefaultDataContainer(const model::Context &context, const util::thread::Config &threadConfig);

    void reserve(std::size_t n) override;

    size_type addEntry(Entry &&entry) override;

    void addParticles(const std::vector<Particle> &particles) override;

    std::vector<size_type>
    addTopologyParticles(const std::vector<TopologyParticle> &topologyParticles) override;

    std::vector<size_type> update(DataUpdate &&update) override;

    void displace(size_type entry, const Particle::pos_type &delta) override;

};

}
}
}
}