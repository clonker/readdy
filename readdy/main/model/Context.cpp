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

#include <readdy/api/KernelConfiguration.h>

#include <readdy/common/Utils.h>
#include <readdy/model/_internal/Util.h>
#include <readdy/common/boundary_condition_operations.h>
#include <readdy/model/potentials/PotentialsOrder1.h>

namespace readdy {
namespace model {

using particle_t = readdy::model::Particle;

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

void Context::configure() {
    updateFunctions();

    _particleTypeRegistry.configure();
    _potentialRegistry.configure();
    _reactionRegistry.configure();
    _topologyRegistry.configure();

    validate();
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
    _distFun = [this](const Vec3 &v1, const Vec3 &v2) {
        const auto dv = _diffFun(v1, v2);
        return dv * dv;
    };
}

void Context::setKernelConfiguration(const std::string &s) {
    _kernelConfiguration = nlohmann::json::parse(s);
}

namespace {
bool boxPotentialValid(const Context &self, potentials::Box *potential) {
    auto bbox = self.getBoxBoundingVertices();
    auto pbc = self.periodicBoundaryConditions();
    bool valid = true;
    for(std::size_t dim = 0; dim < 3; ++dim) {
        auto periodic = pbc.at(dim);
        auto bbox0Pos = std::get<0>(bbox)[dim];
        auto bbox1Pos = std::get<1>(bbox)[dim];
        auto potential1Pos = potential->getOrigin()[dim];
        auto potential2Pos = potential1Pos + potential->getExtent()[dim];
        if(!periodic) {
            valid &= bbox0Pos < potential1Pos && bbox1Pos > potential2Pos;
        }
    }
    return valid;
}
}

void Context::validate() const {
    auto periodic = std::accumulate(periodicBoundaryConditions().begin(),
                                    periodicBoundaryConditions().end(), true, std::logical_and<bool>());
    if(!periodic) {
        // check if there are box potentials for each particle type and that these box potentials are valid
        for(const auto &entry : particle_types().typeMapping()) {
            auto ptype = entry.second;
            auto potIt = potentials().potentialsOrder1().find(ptype);
            bool valid = true;
            if(potIt != potentials().potentialsOrder1().end()) {
                bool gotValidBoxPotential = false;
                for(const auto potPtr : potIt->second) {
                    if(potPtr->type() == potentials::getPotentialName<potentials::Box>()) {
                        gotValidBoxPotential |= boxPotentialValid(*this, dynamic_cast<potentials::Box*>(potPtr));
                    }
                }
                valid &= gotValidBoxPotential;
            } else {
                valid = false;
            }
            if(!valid) {
                throw std::logic_error(fmt::format("For particle type {} there was no valid box potential in direction "
                                                           "of the non-periodic boundaries configured.", entry.first));
            }
        }
    }
}

std::string Context::describe() {
    namespace rus = readdy::util::str;
    configure();
    std::string description;
    description += fmt::format("Configured kernel context with:{}", rus::newline);
    description += fmt::format("--------------------------------{}", rus::newline);
    description += fmt::format(" - kBT = {}{}", kBT(), rus::newline);
    description += fmt::format(" - periodic b.c. = ({}, {}, {}){}",
                               periodicBoundaryConditions()[0],
                               periodicBoundaryConditions()[1],
                               periodicBoundaryConditions()[2],
                               rus::newline);
    description += fmt::format(" - box size = ({}, {}, {}){}", boxSize()[0], boxSize()[1], boxSize()[2], rus::newline);

    description += _particleTypeRegistry.describe();
    description += _potentialRegistry.describe();
    description += _reactionRegistry.describe();
    description += _topologyRegistry.describe();
    return description;
}

const scalar Context::calculateMaxCutoff() const {
    scalar max_cutoff{0};
    for (const auto &entry : potentials().potentialsOrder2()) {
        for (const auto &potential : entry.second) {
            max_cutoff = std::max(max_cutoff, potential->getCutoffRadius());
        }
    }
    for (const auto &entry : reactions().order2()) {
        for (const auto &reaction : entry.second) {
            max_cutoff = std::max(max_cutoff, reaction->eductDistance());
        }
    }
    for (const auto &entry : _topologyRegistry.spatialReactionRegistry()) {
        for (const auto &reaction : entry.second) {
            max_cutoff = std::max(max_cutoff, reaction.radius());
        }
    }
    return max_cutoff;
}

}
}
