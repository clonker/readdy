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
 * @file Topology.cpp
 * @brief << brief description >>
 * @author clonker
 * @date 26.01.17
 * @copyright GNU Lesser General Public License v3.0
 */

#include <readdy/model/topologies/Topology.h>

namespace readdy {
namespace model {
namespace top {
readdy::model::top::Topology::~Topology() = default;

Topology::Topology(Topology::particle_indices &&p) : particles(std::move(p)) { }
Topology::Topology(const Topology::particle_indices &p) : particles(p) { }

Topology::particle_indices::size_type Topology::getNParticles() const {
    return particles.size();
}

const Topology::particle_indices &Topology::getParticles() const {
    return particles;
}

Topology::particle_indices &Topology::getParticles() {
    return particles;
}

const std::vector<std::unique_ptr<pot::BondedPotential>> &Topology::getBondedPotentials() const {
    return bondedPotentials;
}

const std::vector<std::unique_ptr<pot::AnglePotential>> &Topology::getAnglePotentials() const {
    return anglePotentials;
}

const std::vector<std::unique_ptr<pot::TorsionPotential>> &Topology::getTorsionPotentials() const {
    return torsionPotentials;
}

void Topology::addAnglePotential(std::unique_ptr<pot::AnglePotential> &&pot) {
    anglePotentials.push_back(std::move(pot));
}

void Topology::addTorsionPotential(std::unique_ptr<pot::TorsionPotential> &&pot) {
    torsionPotentials.push_back(std::move(pot));
}

void Topology::addBondedPotential(std::unique_ptr<pot::BondedPotential> &&pot) {
    const auto n = getNParticles();
    for(const auto& bond : pot->getBonds()) {
        if (bond.idx1 >= n) {
            throw std::invalid_argument("the first particle (" + std::to_string(bond.idx1) + ") was out of bounds!");
        }
        if (bond.idx2 >= n) {
            throw std::invalid_argument("the second particle (" + std::to_string(bond.idx2) + ") was out of bounds!");
        }
    }
    bondedPotentials.push_back(std::move(pot));
}

void Topology::permuteIndices(const std::vector<std::size_t> &permutation) {
    std::transform(particles.begin(), particles.end(), particles.begin(), [&permutation](std::size_t index) {
        return permutation[index];
    });
}

}
}
}
