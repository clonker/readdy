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
            .def(py::init<topology&>())
            .def("change_particle_type", [](reaction_recipe &self, const std::size_t vertex_index, const std::string &to) {
                auto it = self.topology().graph().vertices().begin();
                std::advance(it, vertex_index);
                return self.changeParticleType(it, to);
            }, py::return_value_policy::reference_internal)
            .def("add_edge", [](reaction_recipe &self, std::size_t v_index1, std::size_t v_index2) {
                auto it1 = self.topology().graph().vertices().begin();
                auto it2 = self.topology().graph().vertices().begin();
                std::advance(it1, v_index1);
                std::advance(it2, v_index2);
                return self.addEdge(it1, it2);
            }, py::return_value_policy::reference_internal)
            .def("remove_edge", [](reaction_recipe &self, std::size_t v_index1, std::size_t v_index2) {
                auto it1 = self.topology().graph().vertices().begin();
                auto it2 = self.topology().graph().vertices().begin();
                std::advance(it1, v_index1);
                std::advance(it2, v_index2);
                return self.removeEdge(it1, it2);
            }, py::return_value_policy::reference_internal)
            .def("separate_vertex", [](reaction_recipe &self, const std::size_t index) {
                auto it = self.topology().graph().vertices().begin();
                std::advance(it, index);
                return self.separateVertex(it);
            }, py::return_value_policy::reference_internal)
            .def("change_topology_type", &reaction_recipe::changeTopologyType, py::return_value_policy::reference_internal);

    py::class_<base_topology>(m, "BaseTopology")
            .def("get_n_particles", &base_topology::getNParticles)
            .def("get_particles", [](const base_topology &self) {return self.getParticles();});

    py::class_<topology, base_topology>(m, "Topology")
            .def("add_harmonic_angle_potential", [](topology &self, const harmonic_angle::angle_configurations &angles) {
                self.addAnglePotential<harmonic_angle>(angles);
            }, "angles"_a)
            .def("add_harmonic_bond_potential", [](topology &self, const harmonic_bond::bond_configurations &bonds) {
                self.addBondedPotential<harmonic_bond>(bonds);
            }, "bonds"_a)
            .def("add_cosine_dihedral_potential", [](topology &self, const cosine_dihedral::dihedral_configurations &dihedrals) {
                self.addTorsionPotential<cosine_dihedral>(dihedrals);
            }, "dihedrals"_a)
            .def("get_graph", [](topology &self) -> graph & { return self.graph(); }, rvp::reference_internal)
            .def("configure", &topology::configure)
            .def("validate", &topology::validate);

    py::class_<graph>(m, "Graph")
            .def("get_vertices", [](graph &self) -> graph::vertex_list & { return self.vertices(); },
                 rvp::reference_internal)
            .def("add_edge", [](graph &self, std::size_t v1, std::size_t v2) {
                if (v1 < self.vertices().size() && v2 < self.vertices().size()) {
                    auto it1 = self.vertices().begin();
                    std::advance(it1, v1);
                    auto it2 = self.vertices().begin();
                    std::advance(it2, v2);
                    self.addEdge(it1, it2);
                } else {
                    throw std::invalid_argument("vertices out of bounds!");
                }
            }, "vertex_index_1"_a, "vertex_index_2"_a);

    py::class_<vertex::vertex_ptr>(m, "VertexPointer")
            .def("get", [](const vertex::vertex_ptr &edge) -> const vertex & { return *edge; });

    py::class_<vertex>(m, "Vertex")
            .def_readonly("particle_index", &vertex::particleIndex)
            .def("particle_type", &vertex::particleType)
            .def("neighbors", [](const vertex &self) { return self.neighbors(); })
            .def("__len__", [](const vertex &v) { return v.neighbors().size(); })
            .def("__iter__", [](vertex &v) {
                return py::make_iterator(v.neighbors().begin(), v.neighbors().end());
            }, py::keep_alive<0, 1>())
            .def("__repr__", [](const vertex &v) {
                return readdy::model::_internal::util::to_string(v);
            });

    py::class_<topology_potential>(m, "TopologyPotential");
    {
        py::class_<bonded_potential, topology_potential>(m, "BondedPotential");
        py::class_<harmonic_bond::bond_configuration>(m, "HarmonicBondPotentialBond")
                .def(py::init<std::size_t, std::size_t, readdy::scalar, readdy::scalar>(), "index1"_a, "index2"_a, "force_constant"_a, "length"_a)
                .def_readonly("idx1", &harmonic_bond::bond_configuration::idx1)
                .def_readonly("idx2", &harmonic_bond::bond_configuration::idx2)
                .def_readonly("length", &harmonic_bond::bond_configuration::length)
                .def_readonly("force_constant", &harmonic_bond::bond_configuration::forceConstant);
        py::class_<harmonic_bond, bonded_potential>(m, "HarmonicBondPotential")
                .def("get_bonds", &harmonic_bond::getBonds)
                .def("calculate_energy", &harmonic_bond::calculateEnergy, "x_ij"_a, "bond"_a)
                .def("calculate_force", [](harmonic_bond &self, const vec3 &x_ij, const harmonic_bond::bond_configuration &bond) {
                    vec3 force(0, 0, 0);
                    self.calculateForce(force, x_ij, bond);
                    return force;
                }, "x_ij"_a, "bond"_a);
    }
    {
        py::class_<angle_potential, topology_potential>(m, "AnglePotential");
        py::class_<harmonic_angle::angle>(m, "HarmonicAnglePotentialAngle")
                .def(py::init<std::size_t, std::size_t, std::size_t, readdy::scalar, readdy::scalar>(),
                     "index1"_a, "index2"_a, "index3"_a, "force_constant"_a, "equilibrium_angle"_a)
                .def_readonly("idx1", &harmonic_angle::angle::idx1)
                .def_readonly("idx2", &harmonic_angle::angle::idx2)
                .def_readonly("idx3", &harmonic_angle::angle::idx3)
                .def_readonly("equilibrium_angle", &harmonic_angle::angle::equilibriumAngle)
                .def_readonly("force_constant", &harmonic_angle::angle::forceConstant);
        py::class_<harmonic_angle, angle_potential>(m, "HarmonicAnglePotential")
                .def("get_angles", &harmonic_angle::getAngles)
                .def("calculate_energy", &harmonic_angle::calculateEnergy, "x_ji"_a, "x_jk"_a, "angle"_a)
                .def("calculate_force", [](harmonic_angle &self, const vec3 &x_ij, const vec3 &x_kj,
                                           const harmonic_angle::angle &angle) {
                    vec3 f1(0, 0, 0), f2(0, 0, 0), f3(0, 0, 0);
                    self.calculateForce(f1, f2, f3, x_ij, x_kj, angle);
                    return std::make_tuple(std::move(f1), std::move(f2), std::move(f3));
                }, "x_ij"_a, "x_kj"_a, "angle"_a);
    }
    {
        py::class_<torsion_potential, topology_potential>(m, "TorsionPotential");
        py::class_<cosine_dihedral::dihedral_configuration>(m, "CosineDihedralPotentialDihedral")
                .def(py::init<std::size_t, std::size_t, std::size_t, std::size_t, readdy::scalar, readdy::scalar, readdy::scalar>(),
                     "index1"_a, "index2"_a, "index3"_a, "index4"_a, "force_constant"_a, "multiplicity"_a,
                     "equilibrium_angle"_a)
                .def_readonly("idx1", &cosine_dihedral::dihedral_configuration::idx1)
                .def_readonly("idx2", &cosine_dihedral::dihedral_configuration::idx2)
                .def_readonly("idx3", &cosine_dihedral::dihedral_configuration::idx3)
                .def_readonly("idx4", &cosine_dihedral::dihedral_configuration::idx4)
                .def_readonly("force_constant", &cosine_dihedral::dihedral_configuration::forceConstant)
                .def_readonly("phi_0", &cosine_dihedral::dihedral_configuration::phi_0)
                .def_readonly("multiplicity", &cosine_dihedral::dihedral_configuration::multiplicity);
        py::class_<cosine_dihedral>(m, "CosineDihedralPotential")
                .def("get_dihedrals", &cosine_dihedral::getDihedrals)
                .def("calculate_energy", &cosine_dihedral::calculateEnergy, "x_ji"_a, "x_kj"_a, "x_kl"_a, "dihedral"_a)
                .def("calculate_force", [](cosine_dihedral &self, const vec3 &x_ji, const vec3 &x_kj, const vec3 &x_kl,
                                           const cosine_dihedral::dihedral_configuration &dih) {
                    vec3 f1(0, 0, 0), f2(0, 0, 0), f3(0, 0, 0), f4(0, 0, 0);
                    self.calculateForce(f1, f2, f3, f4, x_ji, x_kj, x_kl, dih);
                    return std::make_tuple(f1, f2, f3, f4);
                }, "x_ji"_a, "x_kj"_a, "x_kl"_a, "dihedral"_a);
    }
}