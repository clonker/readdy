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
 * @file ParticleTypeRegistry.cpp
 * @brief << brief description >>
 * @author clonker
 * @date 29.03.17
 * @copyright GNU Lesser General Public License v3.0
 */

#include <readdy/model/ParticleTypeRegistry.h>
#include <readdy/model/Utils.h>

namespace readdy {
namespace model {


ParticleTypeInfo::ParticleTypeInfo(const std::string &name, const scalar diffusionConstant,
                                   const particle_flavor flavor, const Particle::type_type typeId)
        : name(name), diffusionConstant(diffusionConstant), flavor(flavor), typeId(typeId) {}


void ParticleTypeRegistry::add(const std::string &name, const scalar diffusionConst, const particle_flavor flavor) {
    util::validateTypeName(name);
    {
        if(diffusionConst < 0) {
            throw std::invalid_argument("The diffusion constant must not be negative");
        }
        // check if name already exists
        for(const auto &e : particle_info_) {
            if(e.second.name == name) {
                throw std::invalid_argument(fmt::format("A particle type with name {} already exists.", name));
            }
        }
    }
    particle_type_type t_id = type_counter_++;
    type_mapping_.emplace(name, t_id);
    particle_info_.emplace(std::make_pair(t_id, ParticleTypeInfo{name, diffusionConst, flavor, t_id}));
    n_types_++;
}

std::string ParticleTypeRegistry::describe() const {
    namespace rus = readdy::util::str;
    std::string description;
    description += fmt::format(" - particle types:{}", rus::newline);
    for (const auto &entry : particle_info_) {
        description += fmt::format("     * particle type \"{}\" with D={}, flavor={}, id={}{}", entry.second.name,
                                   entry.second.diffusionConstant,
                                   particleflavor::particle_flavor_to_str(entry.second.flavor), entry.second.typeId,
                                   rus::newline);
    }
    return description;
}

}
}
