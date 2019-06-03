/********************************************************************
 * Copyright © 2019 Computational Molecular Biology Group,          *
 *                  Freie Universität Berlin (GER)                  *
 *                                                                  *
 * Redistribution and use in source and binary forms, with or       *
 * without modification, are permitted provided that the            *
 * following conditions are met:                                    *
 *  1. Redistributions of source code must retain the above         *
 *     copyright notice, this list of conditions and the            *
 *     following disclaimer.                                        *
 *  2. Redistributions in binary form must reproduce the above      *
 *     copyright notice, this list of conditions and the following  *
 *     disclaimer in the documentation and/or other materials       *
 *     provided with the distribution.                              *
 *  3. Neither the name of the copyright holder nor the names of    *
 *     its contributors may be used to endorse or promote products  *
 *     derived from this software without specific                  *
 *     prior written permission.                                    *
 *                                                                  *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND           *
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,      *
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF         *
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE         *
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR            *
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,     *
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,         *
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; *
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER *
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,      *
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)    *
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF      *
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                       *
 ********************************************************************/

/**
 * « detailed description »
 *
 * @file MPIActionFactory.cpp
 * @brief « brief description »
 * @author chrisfroe
 * @date 28.05.19
 */

#include <readdy/kernel/mpi/actions/MPIActionFactory.h>
#include <readdy/kernel/mpi/actions/MPIActions.h>

namespace readdy::kernel::mpi::actions {

MPIActionFactory::MPIActionFactory(MPIKernel *kernel) : kernel(kernel) {}

std::unique_ptr<model::actions::EulerBDIntegrator> MPIActionFactory::eulerBDIntegrator(scalar timeStep) const {
    return {std::make_unique<MPIEulerBDIntegrator>(kernel, timeStep)};
}

std::unique_ptr<readdy::model::actions::CalculateForces> MPIActionFactory::calculateForces() const {
    return {std::make_unique<MPICalculateForces>(kernel)};
}

std::unique_ptr<model::actions::AddParticles>
MPIActionFactory::addParticles(const std::vector<model::Particle> &particles) const {
    return {std::make_unique<readdy::model::actions::AddParticles>(kernel, particles)};
}

std::unique_ptr<readdy::model::actions::MdgfrdIntegrator> MPIActionFactory::mdgfrdIntegrator(scalar timeStep) const {
    throw std::invalid_argument("Mdgfrd integrator not implemented for MPI");
}

std::unique_ptr<model::actions::CreateNeighborList>
MPIActionFactory::createNeighborList(scalar interactionDistance) const {
    return {std::make_unique<MPICreateNeighborList>(kernel, interactionDistance)};
}

std::unique_ptr<model::actions::UpdateNeighborList> MPIActionFactory::updateNeighborList() const {
    return {std::make_unique<MPIUpdateNeighborList>(kernel)};
}

std::unique_ptr<model::actions::ClearNeighborList> MPIActionFactory::clearNeighborList() const {
    return {std::make_unique<MPIClearNeighborList>(kernel)};
}

std::unique_ptr<model::actions::EvaluateCompartments> MPIActionFactory::evaluateCompartments() const {
    return {std::make_unique<MPIEvaluateCompartments>(kernel)};
}

std::unique_ptr<model::actions::reactions::UncontrolledApproximation>
MPIActionFactory::uncontrolledApproximation(scalar timeStep) const {
    return {std::make_unique<reactions::MPIUncontrolledApproximation>(kernel, timeStep)};
}

std::unique_ptr<model::actions::reactions::Gillespie>
MPIActionFactory::gillespie(scalar timeStep) const {
    return {std::make_unique<reactions::MPIGillespie>(kernel, timeStep)};
}

std::unique_ptr<model::actions::reactions::DetailedBalance>
MPIActionFactory::detailedBalance(scalar timeStep) const {
    throw std::invalid_argument("DetailedBalance reaction handler not implemented for MPI");
}


}