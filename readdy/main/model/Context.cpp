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
 * @file KernelContext.cpp
 * @brief Implementation file of the KernelContext.
 * @author clonker
 * @author chrisfroe
 * @date 18.04.16
 * @todo make proper reference to KernelContext.h, is kBT really indepdendent of t?
 */

#include <readdy/model/Context.h>

#include <json.hpp>

#include <readdy/common/Utils.h>
#include <readdy/model/_internal/Util.h>
#include <readdy/common/boundary_condition_operations.h>

namespace readdy {
namespace model {

using particle_t = readdy::model::Particle;

const scalar &Context::kBT() const {
    return _kBT;
}

scalar &Context::kBT() {
    return _kBT;
}

Context::Context()
        : _potentialRegistry(_particleTypeRegistry), _reactionRegistry(_particleTypeRegistry),
          _topologyRegistry(_particleTypeRegistry), _compartmentRegistry(_particleTypeRegistry),
          _kernelConfiguration{} {
    using namespace std::placeholders;
    _pbc = std::bind(&bcs::applyPBC<false, false, false>, _1, c_::one, c_::one, c_::one);
    _fixPositionFun = std::bind(&bcs::fixPosition<false, false, false>, _1, c_::one, c_::one, c_::one);
    _diffFun = std::bind(&bcs::shortestDifference<false, false, false>, _1, _2, c_::one, c_::one, c_::one);
    _distFun = [this](const Vec3 &v1, const Vec3 &v2) {
        const auto dv = _diffFun(v1, v2);
        return dv * dv;
    };
}

const Context::fix_pos_fun &Context::fixPositionFun() const {
    return _fixPositionFun;
}

const Context::dist_squared_fun &Context::distSquaredFun() const {
    return _distFun;
}

const Context::shortest_dist_fun &Context::shortestDifferenceFun() const {
    return _diffFun;
}

void Context::configure(bool debugOutput) {
    updateFunctions();

    _particleTypeRegistry.configure();
    _potentialRegistry.configure();
    _reactionRegistry.configure();
    _topologyRegistry.configure();

    /**
     * Info output
     */
    if (debugOutput) {

        log::debug("Configured kernel context with: ");
        log::debug("--------------------------------");
        log::debug(" - kBT = {}", kBT());
        log::debug(" - periodic b.c. = ({}, {}, {})", periodicBoundaryConditions()[0], periodicBoundaryConditions()[1],
                   periodicBoundaryConditions()[2]);
        log::debug(" - box size = ({}, {}, {})", boxSize()[0], boxSize()[1], boxSize()[2]);

        _particleTypeRegistry.debug_output();
        _potentialRegistry.debug_output();
        _reactionRegistry.debug_output();
        _topologyRegistry.debug_output();
    }

}

std::tuple<Vec3, Vec3> Context::getBoxBoundingVertices() const {
    const auto &boxSize = _box_size;
    Vec3 lowerLeft{static_cast<scalar>(-0.5) * boxSize[0],
                   static_cast<scalar>(-0.5) * boxSize[1],
                   static_cast<scalar>(-0.5) * boxSize[2]};
    auto upperRight = lowerLeft + Vec3(boxSize);
    return std::make_tuple(lowerLeft, upperRight);
}

const bool &Context::recordReactionsWithPositions() const {
    return _recordReactionsWithPositions;
}

bool &Context::recordReactionsWithPositions() {
    return _recordReactionsWithPositions;
}

const bool &Context::recordReactionCounts() const {
    return _recordReactionCounts;
}

bool &Context::recordReactionCounts() {
    return _recordReactionCounts;
}

reactions::ReactionRegistry &Context::reactions() {
    return _reactionRegistry;
}

const reactions::ReactionRegistry &Context::reactions() const {
    return _reactionRegistry;
}

ParticleTypeRegistry &Context::particle_types() {
    return _particleTypeRegistry;
}

const ParticleTypeRegistry &Context::particle_types() const {
    return _particleTypeRegistry;
}

const potentials::PotentialRegistry &Context::potentials() const {
    return _potentialRegistry;
}

potentials::PotentialRegistry &Context::potentials() {
    return _potentialRegistry;
}

const Context::pbc_fun &Context::applyPBCFun() const {
    return _pbc;
}

const scalar Context::calculateMaxCutoff() const {
    scalar max_cutoff{0};
    for (const auto &entry : potentials().potentials_order2()) {
        for (const auto &potential : entry.second) {
            max_cutoff = std::max(max_cutoff, potential->getCutoffRadius());
        }
    }
    for (const auto &entry : reactions().order2()) {
        for (const auto &reaction : entry.second) {
            max_cutoff = std::max(max_cutoff, reaction->getEductDistance());
        }
    }
    for (const auto &entry : _topologyRegistry.spatial_reaction_registry()) {
        for (const auto &reaction : entry.second) {
            max_cutoff = std::max(max_cutoff, reaction.radius());
        }
    }
    return max_cutoff;
}

top::TopologyRegistry &Context::topology_registry() {
    return _topologyRegistry;
}

const top::TopologyRegistry &Context::topology_registry() const {
    return _topologyRegistry;
}

void Context::updateFunctions() {
    using namespace std::placeholders;
    const auto &box = _box_size;
    if (_periodic_boundary[0]) {
        if (_periodic_boundary[1]) {
            if (_periodic_boundary[2]) {
                _pbc = std::bind(&bcs::applyPBC<true, true, true>, _1, box[0], box[1], box[2]);
                _fixPositionFun = std::bind(&bcs::fixPosition<true, true, true>, _1, box[0], box[1], box[2]);
                _diffFun = std::bind(&bcs::shortestDifference<true, true, true>, _1, _2, box[0], box[1], box[2]);
            } else {
                _pbc = std::bind(&bcs::applyPBC<true, true, false>, _1, box[0], box[1], box[2]);
                _fixPositionFun = std::bind(&bcs::fixPosition<true, true, false>, _1, box[0], box[1], box[2]);
                _diffFun = std::bind(&bcs::shortestDifference<true, true, false>, _1, _2, box[0], box[1], box[2]);
            }
        } else {
            if (_periodic_boundary[2]) {
                _pbc = std::bind(&bcs::applyPBC<true, false, true>, _1, box[0], box[1], box[2]);
                _fixPositionFun = std::bind(&bcs::fixPosition<true, false, true>, _1, box[0], box[1], box[2]);
                _diffFun = std::bind(&bcs::shortestDifference<true, false, true>, _1, _2, box[0], box[1], box[2]);
            } else {
                _pbc = std::bind(&bcs::applyPBC<true, false, false>, _1, box[0], box[1], box[2]);
                _fixPositionFun = std::bind(&bcs::fixPosition<true, false, false>, _1, box[0], box[1], box[2]);
                _diffFun = std::bind(&bcs::shortestDifference<true, false, false>, _1, _2, box[0], box[1], box[2]);
            }
        }
    } else {
        if (_periodic_boundary[1]) {
            if (_periodic_boundary[2]) {
                _pbc = std::bind(&bcs::applyPBC<false, true, true>, _1, box[0], box[1], box[2]);
                _fixPositionFun = std::bind(&bcs::fixPosition<false, true, true>, _1, box[0], box[1], box[2]);
                _diffFun = std::bind(&bcs::shortestDifference<false, true, true>, _1, _2, box[0], box[1], box[2]);
            } else {
                _pbc = std::bind(&bcs::applyPBC<false, true, false>, _1, box[0], box[1], box[2]);
                _fixPositionFun = std::bind(&bcs::fixPosition<false, true, false>, _1, box[0], box[1], box[2]);
                _diffFun = std::bind(&bcs::shortestDifference<false, true, false>, _1, _2, box[0], box[1], box[2]);
            }
        } else {
            if (_periodic_boundary[2]) {
                _pbc = std::bind(&bcs::applyPBC<false, false, true>, _1, box[0], box[1], box[2]);
                _fixPositionFun = std::bind(&bcs::fixPosition<false, false, true>, _1, box[0], box[1], box[2]);
                _diffFun = std::bind(&bcs::shortestDifference<false, false, true>, _1, _2, box[0], box[1], box[2]);
            } else {
                _pbc = std::bind(&bcs::applyPBC<false, false, false>, _1, box[0], box[1], box[2]);
                _fixPositionFun = std::bind(&bcs::fixPosition<false, false, false>, _1, box[0], box[1], box[2]);
                _diffFun = std::bind(&bcs::shortestDifference<false, false, false>, _1, _2, box[0], box[1], box[2]);
            }
        }
    }
}

const Context::BoxSize &Context::boxSize() const {
    return _box_size;
}

Context::BoxSize &Context::boxSize() {
    return _box_size;
}

const Context::PeriodicBoundaryConditions &Context::periodicBoundaryConditions() const {
    return _periodic_boundary;
}

Context::PeriodicBoundaryConditions &Context::periodicBoundaryConditions() {
    return _periodic_boundary;
}

scalar Context::boxVolume() const {
    return _box_size.at(0) * _box_size.at(1) * _box_size.at(2);
}

Context::KernelConfiguration &Context::kernelConfiguration() {
    return _kernelConfiguration;
}

const Context::KernelConfiguration &Context::kernelConfiguration() const {
    return _kernelConfiguration;
}

void Context::setKernelConfiguration(const std::string &s) {
    _kernelConfiguration = nlohmann::json::parse(s);
}

const compartments::CompartmentRegistry &Context::compartments() const {
    return _compartmentRegistry;
}

compartments::CompartmentRegistry &Context::compartments() {
    return _compartmentRegistry;
}

Context::~Context() = default;


}
}






