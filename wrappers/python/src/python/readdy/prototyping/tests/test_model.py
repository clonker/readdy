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


from __future__ import print_function
import unittest

import readdy._internal.readdybinding.prototyping as pr
import readdy._internal.readdybinding.common as cmn

import numpy as np

from readdy.util.testing_utils import ReaDDyTestCase


class TestModel(ReaDDyTestCase):
    def setUp(self):
        self.kernel = pr.SingleCPUKernel()
        self.ctx = self.kernel.get_kernel_context()
        self.model = self.kernel.get_kernel_state_model()

    def test_kernel_context_kbt(self):
        self.ctx.kbt = 5.0
        np.testing.assert_equal(self.ctx.kbt, 5.0)

    def test_kernel_context_box_size(self):
        self.ctx.box_size = [5., 5., 5.]
        np.testing.assert_equal(self.ctx.box_size, [5., 5., 5.])

    def test_kernel_context_periodic_boundary(self):
        self.ctx.pbc = [True, False, True]
        np.testing.assert_equal(self.ctx.pbc, [True, False, True])

    def test_kernel_context_register_particle_type(self):
        self.ctx.particle_types.add("A", 13.0, 0)
        np.testing.assert_equal(self.ctx.particle_types.diffusion_constant_of("A"), 13.0)

    def test_kernel_context_fix_position_fun(self):
        self.ctx.box_size = [1, 1, 1]
        self.ctx.pbc = [True, True, True]
        self.ctx.configure()
        fix_pos = self.ctx.fix_position_fun
        v_outside = cmn.Vec(4, 4, 4)
        fix_pos(v_outside)
        v_inside = cmn.Vec(.1, .1, .1)
        fix_pos(v_inside)
        np.testing.assert_equal(v_outside, cmn.Vec(0, 0, 0))
        np.testing.assert_equal(v_inside, cmn.Vec(.1, .1, .1))

    def test_kernel_context_shortest_difference(self):
        self.ctx.box_size = [2, 2, 2]
        self.ctx.pbc = [True, True, True]
        self.ctx.configure()
        diff = self.ctx.shortest_difference_fun
        np.testing.assert_equal(diff(cmn.Vec(0, 0, 0), cmn.Vec(1, 0, 0)), cmn.Vec(1, 0, 0),
                                err_msg="both vectors were already inside the domain")
        np.testing.assert_equal(diff(cmn.Vec(0, 0, 0), cmn.Vec(-1.5, 0, 0)), cmn.Vec(.5, 0, 0),
                                err_msg="The latter vector was outside the domain, thus had to get a "
                                        "position update to (.5, 0, 0)")

    def test_kernel_context_dist_squared_fun(self):
        self.ctx.box_size = [2., 2., 2.]
        self.ctx.pbc = [True, True, True]
        dist = self.ctx.dist_squared_fun
        np.testing.assert_equal(dist(cmn.Vec(0, 0, 0), cmn.Vec(1, 0, 0)), 1.0)
        np.testing.assert_equal(dist(cmn.Vec(-.5, 0, 0), cmn.Vec(-1.5, 0, 0)), 1.0)

    def test_potential_order_1(self):
        self.ctx.box_size = [2., 2., 2.]
        self.ctx.pbc = [True, True, True]
        self.model.get_particle_data().clear()

        class MyPot1(pr.PotentialOrder1):
            def __init__(self, type):
                super(MyPot1, self).__init__(type)

            def calculate_energy(self, pos_vec):
                return 5.0

            def calculate_force(self, pos_vec):
                return cmn.Vec(.1, .1, .1)

            def configure_for_type(self, type):
                pass

            def get_relevant_length_scale(self):
                return 5.0

            def get_maximal_force(self, kbt):
                return kbt

        self.ctx.particle_types.add("A", 1.0, 0)
        pot = MyPot1(self.ctx.particle_types.id_of("A"))
        self.ctx.potentials.add_external_order1(pot)
        particles = [pr.Particle(0, 0, .5, self.ctx.particle_types.id_of("A"))]
        self.model.get_particle_data().add_particles(particles)
        self.ctx.configure()

    def test_potential_order_2(self):
        self.ctx.box_size = [2, 2, 2]
        self.ctx.pbc = [True, True, True]
        self.model.get_particle_data().clear()

        class MyPot2(pr.PotentialOrder2):
            def __init__(self, typea, typeb):
                super(MyPot2, self).__init__(typea, typeb)

            def calculate_energy(self, x_ij):
                return np.sqrt(x_ij * x_ij)

            def calculate_force(self, x_ij):
                return .5 * x_ij

            def configure_for_types(self, type1, type2):
                pass

            def get_cutoff_radius(self):
                return 5.0

            def get_maximal_force(self, kbt):
                return kbt

        self.ctx.particle_types.add("A", 1.0, 0)
        self.ctx.particle_types.add("B", 1.0, 0)
        pot = MyPot2(self.ctx.particle_types.id_of("A"), self.ctx.particle_types.id_of("B"))
        self.ctx.potentials.add_external_order2(pot)
        particles = [pr.Particle(0, 0, 0, self.ctx.particle_types.id_of("A")),
                     pr.Particle(1, 1, 1, self.ctx.particle_types.id_of("B"))]
        self.model.get_particle_data().add_particles(particles)
        self.ctx.configure()

    def test_potential_registry(self):
        self.ctx.particle_types.add("A", 1.0, 0)
        self.ctx.particle_types.add("B", 1.0, 0)
        cube_pot_id = self.ctx.potentials.add_box("A", 1, cmn.Vec(0, 0, 0), cmn.Vec(1, 1, 1))
        repulsion_pot_id = self.ctx.potentials.add_harmonic_repulsion("A", "B", 10, 1.)
        weak_interaction_pot_id = self.ctx.potentials.add_weak_interaction_piecewise_harmonic("A", "B", 0, 0, 0, 0)
        np.testing.assert_(cube_pot_id != repulsion_pot_id != weak_interaction_pot_id)

if __name__ == '__main__':
    unittest.main()
