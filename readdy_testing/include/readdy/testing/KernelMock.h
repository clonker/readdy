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
 * @file KernelMock.h
 * @brief << brief description >>
 * @author clonker
 * @date 13.07.16
 */

#ifndef READDY_MAIN_KERNELMOCK_H
#define READDY_MAIN_KERNELMOCK_H

#include <readdy/model/Kernel.h>
#include <gmock/gmock.h>

namespace readdy {
namespace testing {

class FakeActionFactory : public readdy::model::actions::ActionFactory {
    readdy::model::actions::EulerBDIntegrator *createEulerBDIntegrator(readdy::scalar timeStep) const override {
        return nullptr;
    }

    readdy::model::actions::CalculateForces *createCalculateForces() const override {
        return nullptr;
    }

    readdy::model::actions::UpdateNeighborList *
    createUpdateNeighborList(readdy::model::actions::UpdateNeighborList::Operation operation,
                             readdy::scalar skinSize) const override { return nullptr; }

    readdy::model::actions::EvaluateCompartments *createEvaluateCompartments() const override {
        return nullptr;
    }

    readdy::model::actions::reactions::UncontrolledApproximation *
    createUncontrolledApproximation(readdy::scalar timeStep) const override {
        return nullptr;
    }

    readdy::model::actions::reactions::Gillespie *createGillespie(readdy::scalar timeStep) const override {
        return nullptr;
    }

};

class KernelMock : public readdy::model::Kernel {

public:
    KernelMock(const std::string &name) : Kernel(name) {}

    MOCK_CONST_METHOD0(getActionFactoryInternal, readdy::model::actions::ActionFactory & (void));

    MOCK_CONST_METHOD0(getKernelStateModelInternal, readdy::model::StateModel & (void));

    MOCK_CONST_METHOD0(getTopologyActionFactoryInternal, readdy::model::top::TopologyActionFactory* (void));
};
}
}
#endif //READDY_MAIN_KERNELMOCK_H
