# coding=utf-8

# Copyright © 2017 Computational Molecular Biology Group,
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
@author: chrisfroe
"""

import numpy as _np

import readdy._internal.readdybinding.common as _common


class CompartmentRegistry(object):
    def __init__(self, context_compartments):
        self._compartments = context_compartments

    def add_sphere(self, conversions, name, origin, radius, larger_or_less=False):
        """
        Registers a spherical compartment. The sphere is defined by an origin and a radius.
        Depending on the flag `larger_or_less`, the compartment will be the outside (True) of the sphere
        or the inside (False) of the sphere.

        The characteristic function of the sphere compartment for a position x is

            | x - origin | > radius (`larger_or_less` = True)

        or

            | x - origin | < radius (`larger_or_less` = False)

        If the characteristic function evaluates to True for a particle position, the conversions will be applied.

        :param conversions: dictionary of particle types, converting keys to values
        :param name: label for the compartment
        :param origin: origin of the sphere
        :param radius: radius of the sphere
        :param larger_or_less: determines if the compartment is outside/True or inside/False of the sphere
        """
        if not isinstance(conversions, dict) or len(conversions) == 0:
            raise ValueError("conversions must be a dictionary with at least one entry")
        if not isinstance(name, str):
            raise ValueError("name must be a string")
        if not radius > 0.:
            raise ValueError("radius must be positive")
        if isinstance(origin, _np.ndarray):
            if origin.squeeze().ndim != 1:
                raise ValueError("Invalid shape for origin!")
            origin = origin.astype(float).squeeze().tolist()
        origin = list(origin)
        if len(origin) != 3:
            raise ValueError("Invalid length for origin! Length can only be 3 but was {}.".format(len(origin)))
        if not isinstance(larger_or_less, bool):
            raise ValueError("larger_or_less must be a bool")
        self._compartments.add_sphere(conversions, name, _common.Vec(*origin), radius, larger_or_less)

    def add_plane(self, conversions, name, normal_coefficients, distance, larger_or_less=True):
        """
        Registers a planar compartment. The plane is defined in Hesse normal form by a normal vector and a distance.
        Depending on the flag `larger_or_less`, the compartment will be where the normal points or on the other side.

        The characteristic function of the plane compartment for a position x is

            x * normal_coefficients - distance > 0 (`larger_or_less` = True)

        or

            x * normal_coefficients - distance < 0 (`larger_or_less` = False)

        If the characteristic function evaluates to True for a particle position, the conversions will be applied.

        :param conversions: dictionary of particle types, converting keys to values
        :param name: label for the compartment
        :param normal_coefficients: normal coefficients of the plane according to Hesse normal form
        :param distance: shorted distance of the plane from the origin (0,0,0) according to Hesse normal form
        :param larger_or_less: determines if the compartment is where the normal points/True or on the other side/False
        """
        if not isinstance(conversions, dict) or len(conversions) == 0:
            raise ValueError("conversions must be a dictionary with at least one entry")
        if not isinstance(name, str):
            raise ValueError("name must be a string")
        if not distance >= 0.:
            raise ValueError("distance must be non-negative")
        normal_coefficients = _np.array(normal_coefficients, dtype=float)
        if normal_coefficients.squeeze().shape != (3,):
            raise ValueError("normal_coefficients must have 3 elements")
        norm = _np.sqrt(_np.sum(normal_coefficients * normal_coefficients))
        if abs(norm - 1.) > 0.0001:
            raise ValueError("normal_coefficients must be a unit vector")
        if not isinstance(larger_or_less, bool):
            raise ValueError("larger_or_less must be a bool")
        self._compartments.add_plane(conversions, name, _common.Vec(*normal_coefficients), distance, larger_or_less)
