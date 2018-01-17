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
 * @file PrototypingModule.cpp
 * @brief << brief description >>
 * @author clonker
 * @date 03.08.16
 */

#include <pybind11/pybind11.h>

#include <readdy/kernel/singlecpu/SCPUKernel.h>

namespace py = pybind11;

using rvp = py::return_value_policy;

void exportModelClasses(py::module &);

void exportPotentials(py::module &);

namespace scpu = readdy::kernel::scpu;

using model = scpu::SCPUStateModel;
using scpu_kernel_t = scpu::SCPUKernel;

using core_kernel_t = readdy::model::Kernel;
using core_action_factory = readdy::model::actions::ActionFactory;
using core_action_t = readdy::model::actions::Action;

void exportPrototyping(py::module& proto) {
    exportModelClasses(proto);
    exportPotentials(proto);

    py::class_<scpu_kernel_t>(proto, "SingleCPUKernel")
            .def(py::init<>())
            .def("get_kernel_state_model", [](const scpu_kernel_t &self) -> const scpu::SCPUStateModel& {return self.getSCPUKernelStateModel(); }, rvp::reference_internal)
            .def("get_kernel_context", [](const scpu_kernel_t &self) -> const readdy::model::Context& { return self.context(); }, rvp::reference_internal)
            .def("get_observable_factory", [](const scpu_kernel_t &self) -> const readdy::model::observables::ObservableFactory& {return self.observe();}, rvp::reference_internal)
            .def("get_topology_action_factory", [](const scpu_kernel_t &self) -> const readdy::model::top::TopologyActionFactory*  {return self.getTopologyActionFactory();}, rvp::reference_internal)
            .def("get_action_factory", [](const scpu_kernel_t &self) -> const readdy::model::actions::ActionFactory& {return self.actions();}, rvp::reference_internal);

}
