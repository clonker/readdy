# coding=utf-8

# Copyright © 2016 Computational Molecular Biology Group,
#                  Freie Universität Berlin (GER)
#
# This file is part of ReaDDy.
#
# ReaDDy is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General
# Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.

"""
Created on 26.09.17

@author: clonker
"""

import numpy as np

import readdy
from readdy.util.testing_utils import ReaDDyTestCase


class TestTopologies(ReaDDyTestCase):
    """
    A bunch of sanity checks
    """

    def test_kbt(self):
        rdf = readdy.ReactionDiffusionSystem()
        rdf.kbt = 5.
        np.testing.assert_equal(rdf.kbt, 5.)

    def test_box_size(self):
        rdf = readdy.ReactionDiffusionSystem()
        rdf.box_size = [1., 2., 3.]
        np.testing.assert_equal(rdf.box_size, [1., 2., 3.])
        rdf.box_size = np.array([5., 6., 7.])
        np.testing.assert_equal(rdf.box_size, [5., 6., 7.])
        rdf.box_size = (1., 5., 7.)
        np.testing.assert_equal(rdf.box_size, [1., 5., 7.])
        np.testing.assert_equal(rdf.box_volume, 5. * 7.)

    def test_pbc(self):
        rdf = readdy.ReactionDiffusionSystem()
        rdf.periodic_boundary_conditions = True, False, True
        np.testing.assert_equal(rdf.periodic_boundary_conditions, [True, False, True])
        rdf.periodic_boundary_conditions = np.array([False, False, True])
        np.testing.assert_equal(rdf.periodic_boundary_conditions, [False, False, True])

    def test_species(self):
        rdf = readdy.ReactionDiffusionSystem()
        rdf.add_species("A", 1.)
        self.assertTrue("A" in rdf.registered_species())
        rdf.add_topology_species("Top A", 10.)
        self.assertTrue("A" in rdf.registered_species() and "Top A" in rdf.registered_species())

    def test_topology_potentials(self):
        rdf = readdy.ReactionDiffusionSystem()
        rdf.add_topology_species("A", 1.)
        rdf.add_topology_species("B", 1.)
        rdf.add_topology_species("C", 1.)
        rdf.add_topology_species("D", 1.)
        rdf.topologies.configure_harmonic_bond("A", "B", 1., 0.)
        rdf.topologies.configure_harmonic_angle("A", "B", "C", 1., 0.)
        rdf.topologies.configure_cosine_dihedral("A", "B", "C", "D", 1., 1, 0.)

    def test_spatial_topology_reactions(self):
        rdf = readdy.ReactionDiffusionSystem()
        rdf.add_topology_species("A", 0.)
        rdf.topologies.add_type("T1")
        rdf.topologies.add_type("T2")
        rdf.topologies.add_type("T3")
        rdf.topologies.add_spatial_reaction("test_fusion: T1(A)+T2(A) -> T3(A--A)", 1., 1.)
        rdf.topologies.add_spatial_reaction("test_enzymatic: T1(A)+T2(A) -> T3(A)+T2(A)", 1., 1.)

    def test_structural_topology_reactions(self):
        rdf = readdy.ReactionDiffusionSystem()
        rdf.add_topology_species("foo")
        rdf.topologies.add_type("foofoo")

        def reaction_fun(topology):
            recipe = readdy.StructuralReactionRecipe(topology)
            recipe.change_particle_type(0, "foo")
            return recipe

        def rate_fun(topology):
            return len(topology.particles)

        rdf.topologies.add_structural_reaction("foofoo", reaction_fun, rate_fun)

    def test_potentials(self):
        rdf = readdy.ReactionDiffusionSystem()
        rdf.add_species("A")
        rdf.potentials.add_box("A", 1.0, [1.0, 1.0, 1.0], [1.0, 1.0, 1.0])
        rdf.potentials.add_harmonic_repulsion("A", "A", 1.0, 1.0)
        rdf.potentials.add_lennard_jones("A", "A", 12, 6, 10, True, 1, 1)
        rdf.potentials.add_screened_electrostatics("A", "A", 10, 10, 10, 10, 10, 10)
        rdf.potentials.add_sphere_in("A", 10, (10, 10, 10), 1)
        rdf.potentials.add_sphere_out("A", 10, (10, 10, 10), 1)
        rdf.potentials.add_spherical_barrier("A", 1, 1, (0, 0, 0), 1)
        rdf.potentials.add_weak_interaction_piecewise_harmonic("A", "A", 10, 10, 10, 10)
