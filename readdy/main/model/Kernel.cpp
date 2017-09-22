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
 * @file Kernel.cpp
 * @brief Core library Implementation of the kernel.
 * @author clonker
 * @date 02.05.16
 */

#include <readdy/common/make_unique.h>
#include <readdy/model/Kernel.h>

namespace readdy {
namespace model {

struct Kernel::Impl {
    /**
     * The name of the kernel.
     */
    std::string name;
    /**
     * todo
     */
    std::unique_ptr<observables::signal_type> signal;
    /**
     * todo
     */
    std::unique_ptr<observables::ObservableFactory> observableFactory;

};

const std::string &Kernel::getName() const {
    return pimpl->name;
}

Kernel::Kernel(const std::string &name) : pimpl(std::make_unique<Kernel::Impl>()), _context{} {
    pimpl->name = name;
    pimpl->observableFactory = std::make_unique<observables::ObservableFactory>(this);
    pimpl->signal = std::make_unique<observables::signal_type>();
}

Kernel::~Kernel() {
    log::trace("destroying kernel instance");
}

readdy::signals::scoped_connection Kernel::connectObservable(observables::ObservableBase *const observable) {
    observable->initialize(this);
    return pimpl->signal->connect_scoped([observable](const time_step_type t) {
        observable->callback(t);
    });
}

std::tuple<std::unique_ptr<readdy::model::observables::ObservableWrapper>, readdy::signals::scoped_connection>
Kernel::registerObservable(const observables::observable_type &observable, unsigned int stride) {
    auto &&wrap = std::make_unique<observables::ObservableWrapper>(this, observable, stride);
    auto &&connection = connectObservable(wrap.get());
    return std::make_tuple(std::move(wrap), std::move(connection));
}

void Kernel::evaluateObservables(time_step_type t) {
    (*pimpl->signal)(t);
}

readdy::model::Particle::id_type Kernel::addParticle(const std::string &type, const Vec3 &pos) {
    readdy::model::Particle particle {pos[0], pos[1], pos[2], context().particle_types().id_of(type)};
    stateModel().addParticle(particle);
    return particle.getId();
}

particle_type_type Kernel::getTypeId(const std::string &name) const {
    auto findIt = context().particle_types().type_mapping().find(name);
    if (findIt != context().particle_types().type_mapping().end()) {
        return findIt->second;
    }
    log::critical("did not find type id for {}", name);
    throw std::invalid_argument("did not find type id for " + name);
}

particle_type_type Kernel::getTypeIdRequireNormalFlavor(const std::string &name) const {
    auto findIt = context().particle_types().type_mapping().find(name);
    if (findIt != context().particle_types().type_mapping().end()) {
        const auto &info = context().particle_types().info_of(findIt->second);
        if (info.flavor == particleflavor::NORMAL) {
            return findIt->second;
        }
        log::critical("particle type {} had no \"normal\" flavor", name);
        throw std::invalid_argument("particle type " + name + " had no \"normal\" flavor");
    }
    log::critical("did not find type id for {}", name);
    throw std::invalid_argument("did not find type id for " + name);
}

observables::ObservableFactory &Kernel::getObservableFactoryInternal() const {
    return *pimpl->observableFactory;
}

const readdy::model::Context &Kernel::context() const {
    return _context;
}

readdy::model::Context &Kernel::context() {
    return _context;
}

const readdy::model::KernelStateModel &Kernel::stateModel() const {
    return getKernelStateModelInternal();
}

readdy::model::KernelStateModel &Kernel::stateModel() {
    return getKernelStateModelInternal();
}

const readdy::model::actions::ActionFactory &Kernel::getActionFactory() const {
    return getActionFactoryInternal();
}

readdy::model::actions::ActionFactory &Kernel::getActionFactory() {
    return getActionFactoryInternal();
}

const readdy::model::observables::ObservableFactory &Kernel::getObservableFactory() const {
    return getObservableFactoryInternal();
}

readdy::model::observables::ObservableFactory &Kernel::getObservableFactory() {
    return getObservableFactoryInternal();
}

const readdy::model::top::TopologyActionFactory *const Kernel::getTopologyActionFactory() const {
    return getTopologyActionFactoryInternal();
}

readdy::model::top::TopologyActionFactory *const Kernel::getTopologyActionFactory() {
    return getTopologyActionFactoryInternal();
}

TopologyParticle Kernel::createTopologyParticle(const std::string &type, const Vec3 &pos) const {
    const auto& info = context().particle_types().info_of(type);
    if(info.flavor != particleflavor::TOPOLOGY) {
        throw std::invalid_argument("You can only create topology particles of a type that is topology flavored.");
    }
    return TopologyParticle(pos, info.typeId);
}

bool Kernel::supportsTopologies() const {
    return getTopologyActionFactory() != nullptr;
}

void Kernel::initialize() {
    context().configure(true);
}

void Kernel::finalize() {
}

}
}
