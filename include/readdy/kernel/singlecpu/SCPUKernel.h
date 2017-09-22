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


//
// Created by clonker on 07.03.16.
//

#pragma once

#include <readdy/model/RandomProvider.h>
#include <readdy/model/Kernel.h>
#include <readdy/kernel/singlecpu/SCPUStateModel.h>
#include <readdy/kernel/singlecpu/observables/SCPUObservableFactory.h>
#include <readdy/kernel/singlecpu/model/topologies/SCPUTopologyActionFactory.h>
#include <readdy/kernel/singlecpu/actions/SCPUActionFactory.h>

namespace readdy {
namespace kernel {
namespace scpu {

class SCPUKernel : public readdy::model::Kernel {
public:

    static const std::string name;

    SCPUKernel();

    ~SCPUKernel() override;

    // move
    SCPUKernel(SCPUKernel &&rhs) = default;

    SCPUKernel &operator=(SCPUKernel &&rhs) = default;

    // factory method
    static std::unique_ptr<SCPUKernel> create();

    const SCPUStateModel &getSCPUKernelStateModel() const;

    SCPUStateModel &getSCPUKernelStateModel();

    void initialize() override;

protected:
    SCPUStateModel &getKernelStateModelInternal() const override;

    readdy::model::actions::ActionFactory &getActionFactoryInternal() const override;

    readdy::model::observables::ObservableFactory &getObservableFactoryInternal() const override;

    readdy::model::top::TopologyActionFactory *getTopologyActionFactoryInternal() const override;

private:
    std::unique_ptr<SCPUStateModel> _model;
    std::unique_ptr<actions::SCPUActionFactory> _actionFactory;
    std::unique_ptr<observables::SCPUObservableFactory> _observables;
    std::unique_ptr<model::top::SCPUTopologyActionFactory> _topologyActionFactory;
};

}
}
}
