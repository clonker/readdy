/********************************************************************
 * Copyright © 2018 Computational Molecular Biology Group,          *
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
 * << detailed description >>
 *
 * @file Vertex.h
 * @brief << brief description >>
 * @author clonker
 * @date 16.03.17
 * @copyright BSD-3
 */

#pragma once

#include <ostream>
#include <list>
#include <utility>
#include <vector>
#include <algorithm>
#include <readdy/common/common.h>
#include <readdy/model/Particle.h>

namespace readdy::model::top::graph {

class Graph;

/**
 * Struct representing a vertex in a topology-connectivity-graph
 */
class Vertex {
public:
    /**
     * edge in the graph (i.e., pointer to neighboring vertex)
     */
    using vertex_ptr = std::list<Vertex>::iterator;
    using vertex_cptr = std::list<Vertex>::const_iterator;

    /**
     * default constructor
     */
    Vertex() = default;

    /**
     * constructs a vertex to a graph
     * @param particleIndex the particle index this vertex belongs to
     */
    Vertex(std::size_t particleIndex, ParticleTypeId particleType)
            : particleIndex(particleIndex), particleType_(particleType), visited(false) {}

    Vertex(const Vertex &) = default;

    Vertex &operator=(const Vertex &) = default;

    Vertex(Vertex && other) noexcept = default;

    Vertex &operator=(Vertex && rhs) noexcept = default;

    /**
     * default destructor
     */
    virtual ~Vertex() = default;

    /**
     * particle index in the topology this vertex belongs to
     */
    std::size_t particleIndex{0};

    bool operator==(const Vertex &rhs) const {
        return particleIndex == rhs.particleIndex;
    }

    friend std::ostream &operator<<(std::ostream &os, const Vertex &vertex) {
        os << "Vertex[particleIndex: " << vertex.particleIndex << ", neighbors=[";
        for (const auto neighbor : vertex.neighbors_) {
            os << neighbor->particleIndex << ",";
        }
        os << "]]";
        return os;
    }

    bool operator!=(const Vertex &rhs) const {
        return !(rhs == *this);
    }

    const std::vector<vertex_ptr> &neighbors() const {
        return neighbors_;
    }

    const ParticleTypeId &particleType() const {
        return particleType_;
    }

    ParticleTypeId &particleType() {
        return particleType_;
    }

private:
    friend class readdy::model::top::graph::Graph;

    /**
     * the edges (i.e., pointers to neighboring vertices)
     */
    std::vector<vertex_ptr> neighbors_{};
    mutable bool visited;

    ParticleTypeId particleType_{0};
};

}
