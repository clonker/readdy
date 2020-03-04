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
 * @file MPIStateModel.cpp
 * @brief « brief description »
 * @author chrisfroe
 * @date 28.05.19
 */

#include <readdy/kernel/mpi/MPIStateModel.h>
#include <readdy/kernel/mpi/Timer.h>

namespace readdy::kernel::mpi {

MPIStateModel::MPIStateModel(Data &data, const readdy::model::Context &context) : _data(data), _context(context) {}

const std::vector<readdy::Vec3> MPIStateModel::getParticlePositions() const {
    const auto data = getParticleData();
    std::vector<Vec3> target{};
    target.reserve(data->size());
    for (const auto &entry : *data) {
        if (!entry.deactivated) target.push_back(entry.pos);
    }
    return target;
}

// todo procedure to send variable type objects?
// -> templated wrappers around commonly used messages,
// e.g. the following combination of Gather and Gatherv
// to fill a vector<T> in targetRank that is a concatenation of all vector<T> on source ranks.
const std::vector<MPIStateModel::Particle>
MPIStateModel::gatherParticles() const {
    if (_domain->isIdleRank()) {
        return {};
    }
    util::Timer timer("MPIStateModel::getParticles");

    // find out how many particles (and bytes) each worker sends
    int nParticles = 0;
    if (_domain->isWorkerRank()) {
        nParticles = _data.get().size();
    }
    std::vector<int> numberParticles(_domain->nUsedRanks(), 0);

    MPI_Gather(&nParticles, 1, MPI_INT, numberParticles.data(), 1, MPI_INT, 0, _commUsedRanks);

    std::vector<int> numberBytes(_domain->nUsedRanks(), 0);
    // convert number of particles to number of bytes from each rank
    for (std::size_t i = 0; i<numberParticles.size(); ++i) {
        numberBytes[i] = static_cast<int>(numberParticles[i] * sizeof(util::ParticlePOD));
    }

    // now send and receive thin particles
    if (_domain->isMasterRank()) {
        std::size_t totalNumberParticles = std::accumulate(numberParticles.begin(), numberParticles.end(), 0);
        std::vector<util::ParticlePOD> thinParticles(totalNumberParticles, {{0., 0., 0.}, 0}); // receive buffer
        std::vector<int> displacements(_domain->nUsedRanks(), 0);
        displacements[0] = 0;
        for (int i = 1; i<displacements.size(); ++i) {
            displacements[i] = displacements[i-1] + numberBytes[i-1];
        }
        MPI_Gatherv(nullptr, 0, MPI_BYTE, thinParticles.data(), numberBytes.data(), displacements.data(), MPI_BYTE, 0, _commUsedRanks);
        // convert to particles
        std::vector<Particle> particles;
        std::for_each(thinParticles.begin(), thinParticles.end(),
                      [&particles](const util::ParticlePOD &tp) {
                          particles.emplace_back(tp.position, tp.typeId);
                      });
        return particles;
    } else {
        // prepare send data
        std::vector<util::ParticlePOD> thinParticles;
        for (const auto &entry : _data.get()) {
            thinParticles.push_back({entry.position(), entry.type});
        }
        MPI_Gatherv((void *) thinParticles.data(), static_cast<int>(thinParticles.size() * sizeof(util::ParticlePOD)), MPI_BYTE, nullptr, nullptr, nullptr, nullptr, 0, _commUsedRanks);
        return {};
    }
}

void MPIStateModel::resetReactionCounts() {
    if (!reactionCounts().empty()) {
        for (auto &e : reactionCounts()) {
            e.second = 0;
        }
    } else {
        const auto &reactions = _context.get().reactions();
        for (const auto &entry : reactions.order1()) {
            for (auto reaction : entry.second) {
                reactionCounts()[reaction->id()] = 0;
            }
        }
        for (const auto &entry : reactions.order2()) {
            for (auto reaction : entry.second) {
                reactionCounts()[reaction->id()] = 0;
            }
        }
    }
}

void MPIStateModel::toDenseParticleIndices(
        std::vector<std::size_t>::iterator begin,
        std::vector<std::size_t>::iterator end) const {
    const auto &blanks = _data.get().blanks();
    std::transform(begin, end, begin, [&blanks](const std::size_t &ix) {
        auto result = ix;
        for (auto blankIx : blanks) {
            if (blankIx < ix) {
                --result;
            }
        }
        return result;
    });
}

void MPIStateModel::clear() {
    getParticleData()->clear();
    //topologies().clear();
    reactionRecords().clear();
    resetReactionCounts();
    virial() = {};
    energy() = 0;
}

void MPIStateModel::distributeParticle(const Particle &p) {
    distributeParticles({p});
}

void MPIStateModel::addParticles(const std::vector<Particle> &particles) {
    getParticleData()->addParticles(particles);
}

void MPIStateModel::distributeParticles(const std::vector<Particle> &ps) {
    if (_domain->isIdleRank()) {
        return;
    }
    util::Timer timer("MPIStateModel::addParticles");
    // todo use MPI_Scatter
    if (_domain->rank() == 0) {
        std::unordered_map<int, std::vector<util::ParticlePOD>> targetParticleMap;
        for (const auto &particle : ps) {
            int target = _domain->rankOfPosition(particle.pos());
            const auto &find = targetParticleMap.find(target);
            if (find != targetParticleMap.end()) {
                find->second.push_back({particle.pos(), particle.type()});
            } else {
                targetParticleMap.emplace(std::make_pair(
                        target, std::vector<util::ParticlePOD>{{particle.pos(), particle.type()}}
                ));
            }
        }
        std::vector<char> isTarget(_domain->nUsedRanks(), false);
        for (auto&&[target, vec] : targetParticleMap) {
            isTarget[target] = true;
        }
        // broadcast target
        MPI_Bcast(isTarget.data(), isTarget.size(), MPI_CHAR, 0, _commUsedRanks);
        // send
        for (auto&&[target, thinParticles] : targetParticleMap) {
            MPI_Request req;
            MPI_Isend((void *) thinParticles.data(),
                      static_cast<int>(thinParticles.size() * sizeof(util::ParticlePOD)), MPI_BYTE,
                      target, util::tags::sendParticles, _commUsedRanks, &req);
            MPI_Request_free(&req);
        }
        MPI_Barrier(_commUsedRanks);
    } else {
        std::vector<char> isTarget(_domain->nUsedRanks(), false);
        // am i one of the targets?
        MPI_Bcast(isTarget.data(), isTarget.size(), MPI_CHAR, 0, _commUsedRanks);
        if (isTarget[_domain->rank()]) {
            // receive, ignore argument p here
            const auto thinParticles = util::receiveParticlesFrom(0, _commUsedRanks);
            std::vector<Particle> particles;
            std::for_each(thinParticles.begin(), thinParticles.end(),
                          [&particles](const util::ParticlePOD &tp) {
                              particles.emplace_back(tp.position, tp.typeId);
                          });
            addParticles(particles);
        }
        MPI_Barrier(_commUsedRanks);
    }
}

const std::vector<readdy::model::Particle> MPIStateModel::getParticles() const {
    const auto *data = getParticleData();
    std::vector<readdy::model::Particle> result;
    result.reserve(data->size());
    for (const auto &entry : *data) {
        if (!entry.is_deactivated()) {
            result.push_back(data->toParticle(entry));
        }
    }
    return result;
}

void MPIStateModel::addParticle(const MPIStateModel::Particle &p) {
    addParticles({p});
}

}
