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
Created on 27.09.17

@author: chrisfroe
"""

from readdy.api.reaction_diffusion_system import ReactionDiffusionSystem
from readdy.util.testing_utils import ReaDDyTestCase


class TestToplevelRegistryReactions(ReaDDyTestCase):
    def setUp(self):
        self.rds = ReactionDiffusionSystem()
        self.rds.add_species("A", 1.)
        self.rds.add_species("B", 1.)
        self.rds.add_species("C", 1.)

    def test_add_decay(self):
        self.rds.reactions.add_decay("label", "A", 1.0)

    def test_add_conversion(self):
        self.rds.reactions.add_conversion("label", "A", "B", 2.0)

    def test_add_fusion(self):
        self.rds.reactions.add_fusion("label", "A", "B", "C", 2.0, 3.0)
        self.rds.reactions.add_fusion("label", "A", "B", "C", 2.0, 3.0, weight1=0., weight2=1.)

    def test_add_fission(self):
        self.rds.reactions.add_fission("label", "C", "A", "B", 2.0, 3.0, weight1=0., weight2=1.)

    def test_add_enzymatic(self):
        self.rds.reactions.add_enzymatic("label", "C", "A", "B", 2.0, 4.0)
