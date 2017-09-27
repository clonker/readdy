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
 * @file ExportTopologies.cpp
 * @brief Impl file for exporting topology related classes and functionality
 * @author clonker
 * @date 04.02.17
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <readdy/model/Particle.h>
#include <readdy/model/topologies/GraphTopology.h>
#include <readdy/model/_internal/Util.h>
#include "PyFunction.h"

namespace py = pybind11;
using rvp = py::return_value_policy;

using particle = readdy::model::Particle;
using topology_particle = readdy::model::TopologyParticle;
using base_topology = readdy::model::top::Topology;
using topology = readdy::model::top::GraphTopology;
using reaction = readdy::model::top::reactions::StructuralTopologyReaction;
using reaction_recipe = readdy::model::top::reactions::Recipe;
using graph = readdy::model::top::graph::Graph;
using vertex = readdy::model::top::graph::Vertex;
using topology_potential = readdy::model::top::pot::TopologyPotential;
using bonded_potential = readdy::model::top::pot::BondedPotential;
using angle_potential = readdy::model::top::pot::AnglePotential;
using torsion_potential = readdy::model::top::pot::TorsionPotential;
using harmonic_bond = readdy::model::top::pot::HarmonicBondPotential;
using harmonic_angle = readdy::model::top::pot::HarmonicAnglePotential;
using cosine_dihedral = readdy::model::top::pot::CosineDihedralPotential;
using vec3 = readdy::Vec3;

struct reaction_function_sink {
    std::shared_ptr<py::function> f;
    reaction_function_sink(py::function f) : f(std::make_shared<py::function>(f)) {};

    inline reaction::reaction_function::result_type operator()(topology& top) {
        py::gil_scoped_acquire gil;
        auto t = py::cast(&top, py::return_value_policy::automatic_reference);
        auto rv = (*f)(*t.cast<topology*>());
        return rv.cast<reaction::reaction_function::result_type>();
    }
};

struct rate_function_sink {
    std::shared_ptr<py::function> f;
    rate_function_sink(py::function f) : f(std::make_shared<py::function>(f)) {};

    inline reaction::rate_function::result_type operator()(const topology& top) {
        py::gil_scoped_acquire gil;
        auto t = py::cast(&top, py::return_value_policy::automatic_reference);
        auto rv = (*f)(*t.cast<topology*>());
        return rv.cast<reaction::rate_function::result_type>();
    }
};

void exportTopologies(py::module &m) {
    using namespace py::literals;

    py::class_<topology_particle>(m, "TopologyParticle")
            .def("get_position", [](topology_particle &self) { return self.getPos(); })
            .def("get_type", [](topology_particle &self) { return self.getType(); })
            .def("get_id", [](topology_particle &self) { return self.getId(); });

    py::class_<reaction_function_sink>(m, "ReactionFunction").def(py::init<py::function>());
    py::class_<rate_function_sink>(m, "RateFunction").def(py::init<py::function>());

    py::class_<reaction>(m, "StructuralTopologyReaction")
            .def(py::init<reaction_function_sink, rate_function_sink>())
            .def("rate", &reaction::rate, "topology"_a)
            .def("raises_if_invalid", &reaction::raises_if_invalid)
            .def("raise_if_invalid", &reaction::raise_if_invalid)
            .def("rolls_back_if_invalid", &reaction::rolls_back_if_invalid)
            .def("roll_back_if_invalid", &reaction::roll_back_if_invalid)
            .def("expects_connected_after_reaction", &reaction::expects_connected_after_reaction)
            .def("expect_connected_after_reaction", &reaction::expect_connected_after_reaction)
            .def("creates_child_topologies_after_reaction", &reaction::creates_child_topologies_after_reaction)
            .def("create_child_topologies_after_reaction", &reaction::create_child_topologies_after_reaction);

    py::class_<reaction_recipe>(m, "Recipe")
            .def(py::init<topology&>(), R"topdoc(
                 Creates a new reaction recipe.

                 :param topology: The topology for which this recipe should be created.
            )topdoc", "topology"_a)
            .def("change_particle_type", [](reaction_recipe &self, const std::size_t vertex_index, const std::string &to) {
                auto it = self.topology().graph().vertices().begin();
                std::advance(it, vertex_index);
                return self.changeParticleType(it, to);
            }, R"topdoc(
                Changes the particle type of the to `vertex_index` associated particle to the given type.

                :param vertex_index: the vertex index as in `topology.get_graph().get_vertices()`
                :param to: the target particle type
                :return: a reference to this recipe to enable a fluent interface
            )topdoc", "vertex_index"_a, "to"_a, py::return_value_policy::reference_internal)
            .def("add_edge", [](reaction_recipe &self, std::size_t v_index1, std::size_t v_index2) -> reaction_recipe& {
                auto it1 = self.topology().graph().vertices().begin();
                auto it2 = self.topology().graph().vertices().begin();
                std::advance(it1, v_index1);
                std::advance(it2, v_index2);
                return self.addEdge(it1, it2);
            }, R"topdoc(
                Adds an edge between the given vertices.

                :param v_index1: index of the first vertex, as in `topology.get_graph().get_vertices()`
                :param v_index2: index of the second vertex, as in `topology.get_graph().get_vertices()`
                :return: a reference to this recipe to enable a fluent interface
            )topdoc", "v_index1"_a, "v_index2"_a, py::return_value_policy::reference_internal)
            .def("remove_edge", [](reaction_recipe &self, std::size_t v_index1, std::size_t v_index2) -> reaction_recipe& {
                auto it1 = self.topology().graph().vertices().begin();
                auto it2 = self.topology().graph().vertices().begin();
                std::advance(it1, v_index1);
                std::advance(it2, v_index2);
                return self.removeEdge(it1, it2);
            }, R"topdoc(
                Removes an edge between given vertices. Depending on the configuration of the topology reaction, this
                can lead to failed states or multiple sub-topologies.

                :param v_index1: index of the first vertex, as in `topology.get_graph().get_vertices()`
                :param v_index2: index of the second vertex, as in `topology.get_graph().get_vertices()`
                :return: a reference to this recipe to enable a fluent interface
            )topdoc", "v_index1"_a, "v_index2"_a, py::return_value_policy::reference_internal)
            .def("separate_vertex", [](reaction_recipe &self, const std::size_t index) -> reaction_recipe& {
                auto it = self.topology().graph().vertices().begin();
                std::advance(it, index);
                return self.separateVertex(it);
            }, R"topdoc(
                Removes all edges from the topology's graph that contain the vertex corresponding to the provided index.

                If no new edge is formed between the given vertex this call, depending on the configuration of the
                reaction, can lead to a failed state or to formation of a topology consisting out of only one particle.
                In the latter case, this call can be followed by a call to `change_particle_type`, where the target
                type is no topology type. Then, no one-particle topology will be formed but the particle will simply
                be emitted and treated as normal particle.

                :param index: The vertex' index with respect to `topology.get_graph().get_vertices()`
                :return: a reference to this recipe to enable a fluent interface
            )topdoc", "index"_a, py::return_value_policy::reference_internal)
            .def("change_topology_type", &reaction_recipe::changeTopologyType, R"topdoc(
                Changes the type of the topology to the given type, potentially changing its structural and spatial
                topology reactions.

                :param type: the target type
                :return: a reference to this recipe to enable a fluent interface
            )topdoc","type"_a, py::return_value_policy::reference_internal);

    py::class_<base_topology>(m, "BaseTopology")
            .def("get_n_particles", &base_topology::getNParticles)
            .def("get_particles", [](const base_topology &self) {return self.getParticles();});

    py::class_<topology, base_topology>(m, "Topology")
            .def("get_graph", [](topology &self) -> graph & { return self.graph(); }, rvp::reference_internal)
            .def_property_readonly("particles", &topology::fetchParticles, R"topdoc(
                Retrieves the particles contained in this topology.

                :return: the particles
            )topdoc")
            .def("particle_type_of_vertex", [](const topology &self, const vertex &v) -> std::string {
                return self.context().particle_types().name_of(self.particleForVertex(v).getType());
            }, R"topdoc(
               Retrieves the particle type corresponding to a vertex.

               :param v: the vertex
               :return: the particle type
            )topdoc")
            .def("position_of_vertex", [](const topology &self, const vertex &v) -> readdy::Vec3 {
                return self.particleForVertex(v).getPos();
            }, R"topdoc(
                Retrieves the position of the particle corresponding to the given vertex.

                :param v: the vertex
                :return: the position
            )topdoc")
            .def("particle_id_of_vertex", [](const topology &self, const vertex &v) -> readdy::model::Particle::id_type {
                return self.particleForVertex(v).getId();
            }, R"topdoc(
                Retrieves the id of the particle corresponding to the given vertex.

                :param v: the vertex
                :return: the id
            )topdoc")
            .def("particle_type_of_vertex", [](const topology &self, const vertex::vertex_ptr &v) -> std::string {
                return self.context().particle_types().name_of(self.particleForVertex(v).getType());
            }, R"topdoc(
               Retrieves the particle type corresponding to a vertex.

               :param v: the vertex
               :return: the particle type
            )topdoc")
            .def("position_of_vertex", [](const topology &self, const vertex::vertex_ptr &v) -> readdy::Vec3 {
                return self.particleForVertex(v).getPos();
            }, R"topdoc(
                Retrieves the position of the particle corresponding to the given vertex.

                :param v: the vertex
                :return: the position
            )topdoc")
            .def("particle_id_of_vertex", [](const topology &self, const vertex::vertex_ptr &v) -> readdy::model::Particle::id_type {
                return self.particleForVertex(v).getId();
            }, R"topdoc(
                Retrieves the id of the particle corresponding to the given vertex.

                :param v: the vertex
                :return: the id
            )topdoc")

            .def("configure", &topology::configure)
            .def("validate", &topology::validate);

    py::class_<graph>(m, "Graph")
            .def("get_vertices", [](graph &self) -> graph::vertex_list & { return self.vertices(); },
            R"topdoc(
                Yields a list of vertices contained in this graph.

                :return: list of vertices
            )topdoc",rvp::reference_internal)
            .def("add_edge", [](graph &self, std::size_t v1, std::size_t v2) {
                if (v1 < self.vertices().size() && v2 < self.vertices().size()) {
                    if(v2 < v1) std::swap(v1, v2);
                    auto it1 = self.vertices().begin();
                    std::advance(it1, v1);
                    auto it2 = it1;
                    std::advance(it2, v2-v1);
                    self.addEdge(it1, it2);
                } else {
                    throw std::invalid_argument("vertices out of bounds!");
                }
            }, "vertex_index_1"_a, "vertex_index_2"_a);

    py::class_<vertex::vertex_ptr>(m, "VertexPointer")
            .def("get", [](const vertex::vertex_ptr &edge) -> const vertex & { return *edge; });

    py::class_<vertex>(m, "Vertex")
            .def("particle_type", &vertex::particleType, R"topdoc(
                Yields this vertex' corresponding particle type.

                :return: the particle type
            )topdoc", rvp::copy)
            .def("neighbors", [](const vertex &self) { return self.neighbors(); }, R"topdoc(
                Yields this vertex' neighbors.

                :return: this vertex' neighbors.
            )topdoc", rvp::reference_internal)
            .def("__len__", [](const vertex &v) { return v.neighbors().size(); }, R"topdoc(
                Yields the number of neighbors of this vertex.

                :return: number of neighbors
            )topdoc", rvp::copy)
            .def("__iter__", [](vertex &self) {
                return py::make_iterator(self.neighbors().begin(), self.neighbors().end());
            }, R"topdoc(
                Yields an iterator over this vertex' neighbors.

                :return: the iterator
            )topdoc", py::keep_alive<0, 1>())
            .def_property_readonly("particle_index", [](const vertex &v) -> std::size_t{
                return v.particleIndex;
            }, R"topdoc(
                Retrieves the particle index for this particle vertex.

                :return: the corresponding particle's index
            )topdoc")
            .def("__repr__", [](const vertex &v) {
                return readdy::model::_internal::util::to_string(v);
            });
}
